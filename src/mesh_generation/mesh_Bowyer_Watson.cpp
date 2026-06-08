#include "mesh_generation/mesh_generation.h"
#include <algorithm>
#include <map>

namespace meshgeneration {

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
            for (const auto& b : bad) if (t == b) return true;
            return false;
        }), tris.end());

        fillCavity(polygon, point, tris, element_id_counter, all, localMap);
    }

    tris.erase(std::remove_if(tris.begin(), tris.end(), [](const Element& t) {
        return t.n0_id < 0 || t.n1_id < 0 || t.n2_id < 0;
    }), tris.end());

    return tris;
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
                    if (pe == e) { dup = true; break; }
                if (!dup) polygon.push_back(e);
            }
        }
    }
    return polygon;
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
            Node mid = (na + nb + apexNode) * (1.0/3.0);
            if (!isPointInPolygon(mid, *boundary)) continue;
        }
        double cross = orient2d(na, nb, apexNode);
        if (cross < 0)
            outElements.push_back({edge.n1_id, edge.n0_id, apexNode.Node_id, idCounter++});
        else
            outElements.push_back({edge.n0_id, edge.n1_id, apexNode.Node_id, idCounter++});
    }
}

} // namespace meshgeneration