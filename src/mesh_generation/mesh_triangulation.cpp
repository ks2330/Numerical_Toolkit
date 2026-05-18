#include "mesh_generation/mesh_generation.h"
#include <iostream>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <map>

namespace meshgeneration {

void Mesh::triangulate(std::string method) {
    if (method == "advancing_front") {
        if (nodes.empty()) return;
        AdvancingFront();
        deleteHoles();
        enforceOutsideConstraints();
        buildNeighbours();
        LaplacianSmoothing();
        std::cout << "Size of Elements" << elements.size() << "\n";
    } else {
        if (nodes.empty()) return;
        elements = bowyerWatson();
        enforceConstraint();
        deleteHoles();
        enforceOutsideConstraints();
        MetricAngles("results/metrics/angle_distribution.csv");
        MetricAspectRatios("results/metrics/aspect_ratio_distribution.csv");
        ImproveMesh();
        buildNeighbours();
        LaplacianSmoothing();
        MetricAngles("results/metrics/angle_distribution_improved.csv");
        MetricAspectRatios("results/metrics/aspect_ratio_distribution_improved.csv");
    }
}

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
                if (e.Element_id == ie.Element_id) return true;
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
        for (const auto& b : bad) if (t.Element_id == b.Element_id) return true;
        return false;
    }), elements.end());

    fillCavity(polygon, n, elements, element_id_counter, nodes, idMap, &boundaryNodes);
}

std::vector<Edge> Mesh::findCavityEdges(const std::vector<Element>& badTriangles) {
    std::vector<Edge> polygon;
    for (const auto& tri : badTriangles) {
        Edge tedges[3] = {{tri.n0_id, tri.n1_id, -1},
                           {tri.n1_id, tri.n2_id, -1},
                           {tri.n2_id, tri.n0_id, -1}};
        for (const auto& e : tedges) {
            int count = 0;
            for (const auto& other : badTriangles)
                if (isSameEdge(other, e)) ++count;
            if (count == 1) {
                bool dup = false;
                for (const auto& pe : polygon)
                    if ((pe.n0_id == e.n0_id && pe.n1_id == e.n1_id) ||
                        (pe.n0_id == e.n1_id && pe.n1_id == e.n0_id)) { dup = true; break; }
                if (!dup) polygon.push_back(e);
            }
        }
    }
    return polygon;
}

std::vector<Element> Mesh::findBadTriangles(
    const std::vector<Element>& triangles,
    const Node& point,
    const std::vector<Node>& nodeList,
    const std::map<int, size_t>& indexMap)
{
    std::vector<Element> bad;
    for (const auto& t : triangles) {
        const Node& n0 = nodeList[indexMap.at(t.n0_id)];
        const Node& n1 = nodeList[indexMap.at(t.n1_id)];
        const Node& n2 = nodeList[indexMap.at(t.n2_id)];
        if (isInCircle(n0, n1, n2, point)) bad.push_back(t);
    }
    return bad;
}

void Mesh::fillCavity(
    const std::vector<Edge>& polygon,
    const Node& apexNode,
    std::vector<Element>& outElements,
    int& idCounter,
    const std::vector<Node>& nodeList,
    const std::map<int, size_t>& indexMap,
    const std::vector<Node>* boundary)
{
    for (const auto& edge : polygon) {
        const Node& na = nodeList[indexMap.at(edge.n0_id)];
        const Node& nb = nodeList[indexMap.at(edge.n1_id)];
        if (boundary) {
            Node mid = {(na.x + nb.x + apexNode.x) / 3.0,
                        (na.y + nb.y + apexNode.y) / 3.0, -1};
            if (!isPointInPolygon(mid, *boundary)) continue;
        }
        double cross = orient2d(na, nb, apexNode);
        if (cross < 0)
            outElements.push_back({edge.n1_id, edge.n0_id, apexNode.Node_id, idCounter++});
        else
            outElements.push_back({edge.n0_id, edge.n1_id, apexNode.Node_id, idCounter++});
    }
}

std::vector<Element> Mesh::bowyerWatson() {
    auto bbox = GetBoundingBox(nodes);
    double minX = bbox[0].x, maxX = bbox[1].x;
    double minY = bbox[0].y, maxY = bbox[2].y;
    double M = std::max(maxX-minX, maxY-minY) * 2.0;
    double midX = (minX+maxX)/2.0, midY = (minY+maxY)/2.0;

    std::vector<Node> superNodes = {
        {midX-M, midY-M, -1},
        {midX+M, midY-M, -2},
        {midX,   midY+2*M, -3}
    };

    std::vector<Node> all = nodes;
    all.insert(all.end(), superNodes.begin(), superNodes.end());

    std::map<int, size_t> localMap;
    for (size_t i = 0; i < all.size(); ++i) localMap[all[i].Node_id] = i;

    element_id_counter = 0;
    std::vector<Element> tris = {{-1, -2, -3, element_id_counter++}};

    for (const auto& point : nodes) {
        std::vector<Element> bad     = findBadTriangles(tris, point, all, localMap);
        std::vector<Edge>    polygon = findCavityEdges(bad);

        tris.erase(std::remove_if(tris.begin(), tris.end(), [&](const Element& t) {
            for (const auto& b : bad) if (t.Element_id == b.Element_id) return true;
            return false;
        }), tris.end());

        fillCavity(polygon, point, tris, element_id_counter, all, localMap);
    }

    tris.erase(std::remove_if(tris.begin(), tris.end(), [](const Element& t) {
        return t.n0_id < 0 || t.n1_id < 0 || t.n2_id < 0;
    }), tris.end());

    return tris;
}

} // namespace meshgeneration