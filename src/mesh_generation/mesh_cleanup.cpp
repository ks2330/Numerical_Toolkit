#include "mesh_generation/mesh_generation.h"
#include <iostream>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <map>

namespace meshgeneration {

void Mesh::ImproveMesh() {
    std::cout << "Improving mesh quality...\n";
    bool foundBadElement = true;
    int iteration = 0;
    while (foundBadElement && iteration < 100) {
        foundBadElement = false; ++iteration;
        for (auto& element : elements) {
            if (nodes[element.n0_id].type == NodeType::Hole ||
                nodes[element.n1_id].type == NodeType::Hole ||
                nodes[element.n2_id].type == NodeType::Hole) continue;
            double angle0 = minAngle(nodes[element.n0_id], nodes[element.n1_id], nodes[element.n2_id]);
            double ratio  = aspectRatio(nodes[element.n0_id], nodes[element.n1_id], nodes[element.n2_id]);
            if (angle0 < 20 * M_PI / 180 || ratio > 10) {
                insertNode(computeCentroid(element));
                foundBadElement = true;
                break;
            }
        }
    }
    deleteHoles();
    enforceConstraint();
    enforceOutsideConstraints();
}

void Mesh::LaplacianSmoothing(int iterations) {
    if (nodes.empty()) return;
    std::cout << "Laplacian smoothing...\n";
    for (int i = 0; i < iterations; ++i) {
        std::vector<std::pair<double,double>> newPos(nodes.size());
        for (int n = 0; n < static_cast<int>(neighbours.size()); ++n) {
            if (nodes[n].type != NodeType::Internal) continue;
            bool adjacentToHole = false;
            for (int nb : neighbours[n])
                if (nodes[nb].type == NodeType::Hole) { adjacentToHole = true; break; }

            if (adjacentToHole) { newPos[n] = {nodes[n].x, nodes[n].y}; continue; }
            double tx = 0, ty = 0;
            int count = 0;
            for (int neighbourID : neighbours[n]) {
                tx += nodes[neighbourID].x;
                ty += nodes[neighbourID].y;
                count++;
            }
            newPos[n] = {tx/count, ty/count};
        }
        for (int n = 0; n < static_cast<int>(nodes.size()); ++n) {
            if (nodes[n].type != NodeType::Internal) continue;
            nodes[n].x = newPos[n].first;
            nodes[n].y = newPos[n].second;
        }
    }
    deleteHoles();
    enforceConstraint();
    enforceOutsideConstraints();
}

bool Mesh::edgesIntersect(const Edge& e1, const Edge& e2) {
    assert(e1.n0_id >= 0 && e1.n0_id < (int)nodes.size());
    assert(e1.n1_id >= 0 && e1.n1_id < (int)nodes.size());
    const Node& p1 = nodes[e1.n0_id]; const Node& p2 = nodes[e1.n1_id];
    const Node& q1 = nodes[e2.n0_id]; const Node& q2 = nodes[e2.n1_id];
    double rx = p2.x - p1.x, ry = p2.y - p1.y;
    double qx = q2.x - q1.x, qy = q2.y - q1.y;
    double det = rx * qy - ry * qx;
    if (std::abs(det) < 1e-10) return false;
    double t = ((q1.x - p1.x) * qy - (q1.y - p1.y) * qx) / det;
    double u = ((q1.x - p1.x) * ry - (q1.y - p1.y) * rx) / det;
    return t > 1e-10 && t < 1 - 1e-10 && u > 1e-10 && u < 1 - 1e-10;
}

void Mesh::enforceConstraint() {
    for (const auto& edge : edges) {
        bool exists = false;
        for (const auto& element : elements) {
            if (isSameEdge(element, edge)) { exists = true; break; }
        }
        if (exists) continue;

        std::vector<Element> intersected;
        for (const auto& element : elements) {
            Edge e1 = {element.n0_id, element.n1_id, -1};
            Edge e2 = {element.n1_id, element.n2_id, -1};
            Edge e3 = {element.n2_id, element.n0_id, -1};
            if (edgesIntersect(edge, e1) || edgesIntersect(edge, e2) || edgesIntersect(edge, e3))
                intersected.push_back(element);
        }
        elements.erase(std::remove_if(elements.begin(), elements.end(), [&](const Element& e) {
            for (const auto& ie : intersected)
                if (e == ie) return true;
            return false;
        }), elements.end());

        std::vector<Edge> cavity = findCavityEdges(intersected);
        for (const auto& cEdge : cavity) {
            if (cEdge.n0_id == edge.n0_id || cEdge.n1_id == edge.n0_id) continue;
            double cross = orient2d(nodes[edge.n0_id], nodes[cEdge.n0_id], nodes[cEdge.n1_id]);
            if (cross < 0)
                elements.push_back({edge.n0_id, cEdge.n1_id, cEdge.n0_id, element_id_counter++});
            else
                elements.push_back({edge.n0_id, cEdge.n0_id, cEdge.n1_id, element_id_counter++});
        }
    }
}

void Mesh::enforceOutsideConstraints() {
    if (boundaryNodes.empty()) return;
    elements.erase(std::remove_if(elements.begin(), elements.end(), [&](const Element& e) {
        return !isPointInPolygon(computeCentroid(e), boundaryNodes);
    }), elements.end());
}

void Mesh::deleteHoles() {
    if (holeNodes.empty()) return;
    elements.erase(std::remove_if(elements.begin(), elements.end(), [&](const Element& e) {
        return isPointInPolygon(computeCentroid(e), holeNodes);
    }), elements.end());
}

void Mesh::insertNode(const Node& newNode) {
    if (!holeNodes.empty() && isPointInPolygon(newNode, holeNodes)) return;
    Node n  = newNode;
    n.Node_id = static_cast<int>(nodes.size());
    n.type    = NodeType::Internal;
    nodes.push_back(n);
    internalNodes.push_back(n);

    std::map<int, size_t> idMap;
    for (size_t i = 0; i < nodes.size(); ++i) idMap[nodes[i].Node_id] = i;

    std::vector<Element> bad     = findBadTriangles(elements, n, nodes, idMap);
    std::vector<Edge>    polygon = findCavityEdges(bad);

    elements.erase(std::remove_if(elements.begin(), elements.end(), [&](const Element& t) {
        for (const auto& b : bad) if (t == b) return true;
        return false;
    }), elements.end());

    fillCavity(polygon, n, elements, element_id_counter, nodes, idMap, &boundaryNodes);
}

} // namespace meshgeneration