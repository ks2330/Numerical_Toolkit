#pragma once
#include "mesh_generation/mesh_geometry.h"
#include <map>
#include <vector>
#include <utility>
#include <algorithm>
#include <cmath>

namespace nt::fvm
{
    struct Vec2 {
        double x, y;
    };

    struct Face {
        int n1_id, n2_id;
        int leftElement_id;
        int rightElement_id;
        double length;  
        Vec2 normal;  
        int bcType;
    };

    // --- Cell geometry ---

    inline double cellArea(const meshgeneration::Node& n1,
                           const meshgeneration::Node& n2,
                           const meshgeneration::Node& n3) {
        return orient2d(n1, n2, n3) / 2.0;
    }

    inline meshgeneration::Node cellCentroid(const meshgeneration::Node& n1,
                                             const meshgeneration::Node& n2,
                                             const meshgeneration::Node& n3) {
        double cx = (n1.x + n2.x + n3.x) / 3.0;
        double cy = (n1.y + n2.y + n3.y) / 3.0;
        return {cx, cy, -1};
    }

    // --- Face geometry ---

    inline double faceLength(const meshgeneration::Node& n1, const meshgeneration::Node& n2) {
        double dx = n2.x - n1.x, dy = n2.y - n1.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    // Unit normal to edge n1->n2, oriented to point AWAY from the left cell's centroid.
    inline Vec2 faceNormal(const meshgeneration::Node& n1,
                           const meshgeneration::Node& n2,
                           const meshgeneration::Node& leftCentroid) {
        double dx = n2.x - n1.x, dy = n2.y - n1.y;
        double len = std::sqrt(dx * dx + dy * dy);
        Vec2 n = { dy / len, -dx / len };          // perpendicular to (dx,dy), unit length

        // flip if it points toward the left centroid (we want outward from the left cell)
        double mx = (n1.x + n2.x) / 2.0, my = (n1.y + n2.y) / 2.0;
        double toCx = leftCentroid.x - mx, toCy = leftCentroid.y - my;
        if (n.x * toCx + n.y * toCy > 0.0) { n.x = -n.x; n.y = -n.y; }
        return n;
    }

    // --- Face construction ---

    // Direction-independent edge key: (min, max) node ids so a->b and b->a collide.
    inline std::pair<int,int> canonicalEdge(int a, int b) {
        return { std::min(a, b), std::max(a, b) };
    }

    // Map each undirected edge -> the cell indices that own it (1 = boundary, 2 = interior).
    inline std::map<std::pair<int,int>, std::vector<int>>
    buildEdgeMap(const std::vector<meshgeneration::Element>& elements) {
        std::map<std::pair<int,int>, std::vector<int>> edgeMap;
        for (size_t i = 0; i < elements.size(); ++i) {
            const auto& e = elements[i];
            const std::pair<int,int> edges[3] = {
                canonicalEdge(e.n0_id, e.n1_id),
                canonicalEdge(e.n1_id, e.n2_id),
                canonicalEdge(e.n2_id, e.n0_id)
            };
            for (const auto& key : edges)
                edgeMap[key].push_back(static_cast<int>(i));
        }
        return edgeMap;
    }

    // Assemble one Face (connectivity + geometry) from an edge-map entry.
    inline Face makeFace(const std::pair<int,int>& edgeKey,
                         const std::vector<int>& cells,
                         const std::vector<meshgeneration::Element>& elements,
                         const std::vector<meshgeneration::Node>& nodes) {
        Face f;
        f.n1_id = edgeKey.first;
        f.n2_id = edgeKey.second;
        f.leftElement_id  = cells[0];
        f.rightElement_id = (cells.size() == 1) ? -1 : cells[1];

        const auto& n1 = nodes[f.n1_id];
        const auto& n2 = nodes[f.n2_id];
        f.length = faceLength(n1, n2);

        const auto& left = elements[f.leftElement_id];
        meshgeneration::Node c = cellCentroid(nodes[left.n0_id], nodes[left.n1_id], nodes[left.n2_id]);
        f.normal = faceNormal(n1, n2, c);
        f.bcType = -1;   // interior by default; buildFaces tags boundary faces
        return f;
    }

    // --- Face list ---

    inline std::vector<Face> buildFaces(const std::vector<meshgeneration::Element>& elements,
                                        const std::vector<meshgeneration::Node>& nodes,
                                        const std::vector<meshgeneration::Edge>& boundaryEdges = {}) {
        // Look up each boundary edge's group id by its canonical (min,max) node pair.
        std::map<std::pair<int,int>, int> edgeGroups;
        for (const auto& e : boundaryEdges)
            edgeGroups[canonicalEdge(e.n0_id, e.n1_id)] = e.group_id;

        std::vector<Face> faces;
        for (const auto& [edgeKey, cells] : buildEdgeMap(elements)) {
            Face f = makeFace(edgeKey, cells, elements, nodes);
            if (f.rightElement_id == -1) {                 // boundary face -> inherit the edge's group
                auto it = edgeGroups.find(edgeKey);
                if (it != edgeGroups.end()) f.bcType = it->second;
            }
            faces.push_back(f);
        }
        return faces;
    }
}