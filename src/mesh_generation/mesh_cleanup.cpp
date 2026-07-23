#include "mesh_generation/mesh_generation.h"
#include <iostream>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <map>
#include <set>

namespace meshgeneration {

void Mesh::improveMesh() {
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

void Mesh::laplacianSmoothing(int iterations) {
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

namespace {

// Circumcircle test that tolerates either orientation of triangle (a, b, c).
bool inCircumcircle(const Node& a, const Node& b, const Node& c, const Node& d) {
    return orient2d(a, b, c) > 0 ? isInCircle(a, b, c, d) : isInCircle(a, c, b, d);
}

// Delaunay triangulation of a pseudo-polygon against base edge a-b
// (Anglada 1997): pick the vertex c whose circumcircle with the base is
// empty of the other vertices, emit (a, b, c), recurse on both sub-chains.
// `verts` must hold the cavity vertices in boundary order from a to b.
void triangulatePseudoPolygon(int a, int b, const std::vector<int>& verts,
                              const std::vector<Node>& nodes,
                              std::vector<Element>& outElements, int& idCounter) {
    if (verts.empty()) return;
    size_t ci = 0;
    for (size_t i = 1; i < verts.size(); ++i)
        if (inCircumcircle(nodes[a], nodes[b], nodes[verts[ci]], nodes[verts[i]]))
            ci = i;
    int c = verts[ci];
    triangulatePseudoPolygon(a, c, {verts.begin(), verts.begin() + ci},
                             nodes, outElements, idCounter);
    triangulatePseudoPolygon(c, b, {verts.begin() + ci + 1, verts.end()},
                             nodes, outElements, idCounter);
    if (orient2d(nodes[a], nodes[b], nodes[c]) > 0)
        outElements.push_back({a, b, c, idCounter++});
    else
        outElements.push_back({a, c, b, idCounter++});
}

// Bowyer-Watson cavity that respects constrained edges: start from the
// triangle containing p and flood outward through circumcircle-violating
// neighbours, never stepping across a constraint. A plain circumcircle sweep
// is wrong here — the mesh is no longer Delaunay near enforced edges, and a
// sliver's huge circumcircle can capture triangles on the far side of the
// aerofoil, producing a fill that pierces the surface.
std::vector<Element> constrainedCavity(const Node& p,
                                       const std::vector<Element>& elements,
                                       const std::vector<Node>& nodes,
                                       const std::map<int, size_t>& idMap,
                                       const std::vector<Edge>& constraints) {
    int start = -1;
    for (size_t i = 0; i < elements.size(); ++i) {
        const Node& a = nodes[idMap.at(elements[i].n0_id)];
        const Node& b = nodes[idMap.at(elements[i].n1_id)];
        const Node& c = nodes[idMap.at(elements[i].n2_id)];
        double d0 = orient2d(a, b, p), d1 = orient2d(b, c, p), d2 = orient2d(c, a, p);
        bool hasNeg = d0 < 0 || d1 < 0 || d2 < 0;
        bool hasPos = d0 > 0 || d1 > 0 || d2 > 0;
        if (!(hasNeg && hasPos)) { start = static_cast<int>(i); break; }
    }
    if (start < 0) return {};

    auto key = [](int u, int v) { return std::make_pair(std::min(u, v), std::max(u, v)); };
    std::map<std::pair<int,int>, std::vector<int>> edgeToElements;
    for (size_t i = 0; i < elements.size(); ++i) {
        const Element& t = elements[i];
        edgeToElements[key(t.n0_id, t.n1_id)].push_back(static_cast<int>(i));
        edgeToElements[key(t.n1_id, t.n2_id)].push_back(static_cast<int>(i));
        edgeToElements[key(t.n2_id, t.n0_id)].push_back(static_cast<int>(i));
    }
    std::set<std::pair<int,int>> constrained;
    for (const auto& e : constraints) constrained.insert(key(e.n0_id, e.n1_id));

    std::vector<bool> visited(elements.size(), false);
    std::vector<int> stack = {start};
    visited[start] = true;
    std::vector<Element> cavity;
    while (!stack.empty()) {
        int i = stack.back(); stack.pop_back();
        cavity.push_back(elements[i]);
        const Element& t = elements[i];
        for (auto& k : {key(t.n0_id, t.n1_id), key(t.n1_id, t.n2_id), key(t.n2_id, t.n0_id)}) {
            if (constrained.count(k)) continue;
            for (int j : edgeToElements[k]) {
                if (visited[j]) continue;
                const Node& a = nodes[idMap.at(elements[j].n0_id)];
                const Node& b = nodes[idMap.at(elements[j].n1_id)];
                const Node& c = nodes[idMap.at(elements[j].n2_id)];
                if (inCircumcircle(a, b, c, p)) {
                    visited[j] = true;
                    stack.push_back(j);
                }
            }
        }
    }
    return cavity;
}

} // namespace

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
        if (intersected.empty()) continue;

        // Walk the cavity boundary as an ordered loop starting at edge.n0.
        std::vector<Edge> cavity = findCavityEdges(intersected);
        std::map<int, std::vector<int>> adj;
        for (const auto& cEdge : cavity) {
            adj[cEdge.n0_id].push_back(cEdge.n1_id);
            adj[cEdge.n1_id].push_back(cEdge.n0_id);
        }

        std::vector<int> loop = {edge.n0_id};
        bool simpleLoop = adj.count(edge.n0_id) && adj[edge.n0_id].size() == 2;
        int prev = -1, cur = edge.n0_id;
        while (simpleLoop) {
            int nxt = (adj[cur][0] == prev) ? adj[cur][1] : adj[cur][0];
            if (nxt == edge.n0_id) break;                       // loop closed
            if (adj[nxt].size() != 2 ||
                loop.size() >= adj.size()) { simpleLoop = false; break; }
            loop.push_back(nxt);
            prev = cur; cur = nxt;
        }
        auto n1It = std::find(loop.begin(), loop.end(), edge.n1_id);
        if (!simpleLoop || n1It == loop.end()) {
            std::cerr << "enforceConstraint: cavity of edge " << edge.n0_id << "-"
                      << edge.n1_id << " is not a simple loop; constraint skipped\n";
            continue;
        }

        elements.erase(std::remove_if(elements.begin(), elements.end(), [&](const Element& e) {
            for (const auto& ie : intersected)
                if (e == ie) return true;
            return false;
        }), elements.end());

        // The constraint splits the loop into the two pseudo-polygons on
        // either side of the edge; retriangulate each independently.
        std::vector<int> sideA(loop.begin() + 1, n1It);
        std::vector<int> sideB(n1It + 1, loop.end());
        triangulatePseudoPolygon(edge.n0_id, edge.n1_id, sideA,
                                 nodes, elements, element_id_counter);
        triangulatePseudoPolygon(edge.n1_id, edge.n0_id, sideB,
                                 nodes, elements, element_id_counter);
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

    std::vector<Element> bad = constrainedCavity(n, elements, nodes, idMap, edges);
    if (bad.empty()) {          // point lies in no triangle — nothing to refine
        nodes.pop_back();
        internalNodes.pop_back();
        return;
    }
    std::vector<Edge> polygon = findCavityEdges(bad);

    elements.erase(std::remove_if(elements.begin(), elements.end(), [&](const Element& t) {
        for (const auto& b : bad) if (t == b) return true;
        return false;
    }), elements.end());

    fillCavity(polygon, n, elements, element_id_counter, nodes, idMap, &boundaryNodes);
}

} // namespace meshgeneration