#include "mesh_generation/mesh_generation.h"
#include "mesh_generation/quadtree.h"

#include <iostream>
#include <cmath>
#include <algorithm>
#include <limits>
#include <cassert>
#include <memory>

namespace meshgeneration {

void Mesh::generateRandomNodes() {
    std::vector<Node> boundingBoxNodes = GetBoundingBox(boundaryNodes);
    double minX = boundingBoxNodes[0].x, maxX = boundingBoxNodes[1].x;
    double minY = boundingBoxNodes[0].y, maxY = boundingBoxNodes[3].y;

    // 1. Define the AABB for the entire domain
    AABB domain_boundary = {
        (minX + maxX) / 2.0,      // center x
        (minY + maxY) / 2.0,      // center y
        (maxX - minX) / 2.0,      // half_width
        (maxY - minY) / 2.0       // half_height
    };

    node_quadtree = std::make_unique<Quadtree>(domain_boundary);

    int k = 30;
    double s_min = std::min((maxX - minX), (maxY - minY)) / std::sqrt(numRandomNodes) * 0.05;
    std::vector<Node> activeNodes = initPoisson();

    while (!activeNodes.empty() && internalNodes.size() < static_cast<size_t>(numRandomNodes)) {
        int idx = rand() % activeNodes.size();
        Node activeNode = activeNodes[idx];
        double d = GetClosestHoleDistance(activeNode);
        double growth_rate = 10.0;
        double s_max = s_min * 5.0;
        double s_varied = std::clamp(s_min + growth_rate * d, s_min, s_max);

        bool found = false;
        for (int tries = 0; tries < k; ++tries) {
            double angle = static_cast<double>(rand()) / RAND_MAX * 2 * M_PI;
            double radius = s_varied * (1 + static_cast<double>(rand()) / RAND_MAX);
            int nextId = static_cast<int>(nodes.size());
            Node newNode = {activeNode.x + radius * cos(angle),
                            activeNode.y + radius * sin(angle),
                            nextId, NodeType::Internal, -1};
            bool inOuter = isPointInPolygon(newNode, boundaryNodes);
            bool inHole  = !holeNodes.empty() && isPointInPolygon(newNode, holeNodes);
            if (inOuter && !inHole && !isSdistanceTooClose(newNode, s_varied, s_min)) {
                internalNodes.push_back(newNode);
                nodes.push_back(newNode);
                activeNodes.push_back(newNode);
                node_quadtree->insert(newNode);
                found = true;
                break;
            }
        }
        if (!found) activeNodes.erase(activeNodes.begin() + idx);
    }
}

std::vector<Node> Mesh::initPoisson() {
    int nextId = static_cast<int>(nodes.size());

    double minX, maxX, minY, maxY;
    if (!boundaryNodes.empty()) {
        auto bbox = GetBoundingBox(boundaryNodes);
        minX = bbox[0].x; maxX = bbox[1].x; minY = bbox[0].y; maxY = bbox[3].y;
    } else if (!holeNodes.empty()) {
        auto bbox = GetBoundingBox(holeNodes);
        minX = bbox[0].x; maxX = bbox[1].x; minY = bbox[0].y; maxY = bbox[3].y;
    } else {
        return {};
    }

    std::vector<Node> activeNodes;

    bool placed = false;
    int attempts = 0;
    while (!placed && attempts < 10000) {
        ++attempts;
        double x = static_cast<double>(rand()) / RAND_MAX * (maxX - minX) + minX;
        double y = static_cast<double>(rand()) / RAND_MAX * (maxY - minY) + minY;
        Node seed = {x, y, nextId, NodeType::Internal, -1};
        bool inOuter = isPointInPolygon(seed, boundaryNodes);
        bool inHole  = !holeNodes.empty() && isPointInPolygon(seed, holeNodes);
        if (inOuter && !inHole) {
            internalNodes.push_back(seed);
            nodes.push_back(seed);
            activeNodes.push_back(seed);
            placed = true;
        }
    }
    if (!placed)
        std::cerr << "Failed to place initial Poisson node after 10000 attempts.\n";
    return activeNodes;
}

void Mesh::BoundaryLayerSeeding() {
    if (boundaryEdges.empty()) return;
    if (holeNodes.empty()) return;

    int numLayers = 5;
    bool isCCW_BOOL = isCCW(holeNodes);

    double ratio = 1.1;
    double h0 = chord * 0.01;

    for (int j = 0; j < static_cast<int>(holeNodes.size()); ++j) {
        std::pair<double, double> perpendicular, tangent, unitVec;
        double layerDistance = 0;

        if (isCCW_BOOL) {
            perpendicular = edgeDirection(holeNodes[j], holeNodes[(j + 1) % holeNodes.size()]);
            tangent = {perpendicular.second, -perpendicular.first};
            double len = distance(holeNodes[j], holeNodes[(j + 1) % holeNodes.size()]);
            unitVec = {tangent.first / len, tangent.second / len};
        } else {
            perpendicular = edgeDirection(holeNodes[(j + 1) % holeNodes.size()], holeNodes[j]);
            tangent = {-perpendicular.second, perpendicular.first};
            double len = distance(holeNodes[(j + 1) % holeNodes.size()], holeNodes[j]);
            unitVec = {tangent.first / len, tangent.second / len};
        }

        for (int i = 0; i < numLayers; ++i) {
            layerDistance += h0 * std::pow(ratio, i);
            Node newNode = {holeNodes[j].x + unitVec.first * layerDistance,
                            holeNodes[j].y + unitVec.second * layerDistance,
                            static_cast<int>(nodes.size()), NodeType::Internal, -1};
            if (isPointInPolygon(newNode, boundaryNodes) && !isPointInPolygon(newNode, holeNodes)) {
                internalNodes.push_back(newNode);
                nodes.push_back(newNode);
            } else {
                break;
            }
        }
    }
}

bool Mesh::isSdistanceTooClose(const Node& node, double s, double s_boundary) {
    for (const auto& b : boundaryNodes) {
        double dx = b.x - node.x, dy = b.y - node.y;
        if (dx*dx + dy*dy < s_boundary*s_boundary) return true;
    }
    for (const auto& n : internalNodes) {
        double dx = n.x - node.x, dy = n.y - node.y;
        if (dx*dx + dy*dy < s*s) return true;
    }
    return false;
}

double Mesh::GetClosestHoleDistance(const Node& node) {
    double minDist = std::numeric_limits<double>::max();
    for (const auto& n : holeNodes) {
        double dx = n.x - node.x, dy = n.y - node.y;
        double distSq = dx*dx + dy*dy;
        if (distSq < minDist) minDist = distSq;
    }
    return std::sqrt(minDist);
}

std::vector<Node> Mesh::NodesWithinDistanceAdvancingFront(const Node& node, double s) {
    AABB query_box = {node.x, node.y, s, s};
    std::vector<Node> nearbyNodes = node_quadtree->query(query_box);
    std::vector<Node> result;
    for (const auto& n : nearbyNodes) {
        double dx = n.x - node.x, dy = n.y - node.y;
        if (dx*dx + dy*dy < s*s) result.push_back(n);
    }
    return result;

}

} // namespace meshgeneration