#pragma once
#include "mesh_generation/mesh_geometry.h"
#include <map>
#include <set>
#include <array>
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

    inline std::vector<double> computeAllCellArea(const std::vector<meshgeneration::Element>& elements,
                                              const std::vector<meshgeneration::Node>& nodes) {
        std::vector<double> areas;
        areas.reserve(elements.size());
        for (const auto& e : elements) {
            areas.push_back(std::abs(cellArea(nodes[e.n0_id], nodes[e.n1_id], nodes[e.n2_id])));
        }
        return areas;
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


    inline std::pair<int,int> canonicalEdge(int a, int b) {
        return { std::min(a, b), std::max(a, b) };
    }

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
        f.bcType = -1;
        return f;
    }

    // --- Face list ---

    inline std::vector<Face> buildFaces(const std::vector<meshgeneration::Element>& elements,
                                        const std::vector<meshgeneration::Node>& nodes,
                                        const std::vector<meshgeneration::Edge>& boundaryEdges = {}) {
        std::map<std::pair<int,int>, int> edgeGroups;
        for (const auto& e : boundaryEdges)
            edgeGroups[canonicalEdge(e.n0_id, e.n1_id)] = e.group_id;

        std::vector<Face> faces;
        for (const auto& [edgeKey, cells] : buildEdgeMap(elements)) {
            Face f = makeFace(edgeKey, cells, elements, nodes);
            if (f.rightElement_id == -1) {                 
                auto it = edgeGroups.find(edgeKey);
                if (it != edgeGroups.end()) f.bcType = it->second;
            }
            faces.push_back(f);
        }
        return faces;
    }
    inline std::vector<std::pair<int,int>>
    uncoveredBoundaryEdges(const std::vector<Face>& faces,
                           const std::vector<meshgeneration::Edge>& boundaryEdges) {
        std::map<std::pair<int,int>, bool> coveredAsBoundary;  
        for (const auto& f : faces) {
            auto key = canonicalEdge(f.n1_id, f.n2_id);
            if (f.rightElement_id == -1)
                coveredAsBoundary[key] = true;
            else
                coveredAsBoundary.emplace(key, false);          
        }
        std::vector<std::pair<int,int>> missing;
        for (const auto& e : boundaryEdges) {
            auto key = canonicalEdge(e.n0_id, e.n1_id);
            auto it = coveredAsBoundary.find(key);
            if (it == coveredAsBoundary.end() || !it->second)
                missing.push_back(key);
        }
        return missing;
    }

    // --- Boundary-edge repair ---

    inline std::array<int,3> sortedTri(int a, int b, int c) {
        std::array<int,3> t{a, b, c};
        std::sort(t.begin(), t.end());
        return t;
    }

    // Close one-triangle "notches": the unconstrained Delaunay sometimes picks the wrong diagonal
    // and never creates a tagged boundary edge, leaving a single empty triangle against the surface
    // (this is what dropped aerofoil edge 158-159 and broke the wall loop). For each tagged boundary
    // edge absent from the triangulation, add the triangle whose apex is the unique common neighbour
    // of its two endpoints. The FVM solver only reads node ids (areas use |orient2d|, normals use the
    // centroid-flip), so the added triangle's winding and Element_id are irrelevant. Returns #added.
    inline int repairMissingBoundaryEdges(std::vector<meshgeneration::Element>& elements,
                                          const std::vector<meshgeneration::Edge>& boundaryEdges) {
        std::set<std::pair<int,int>> edgeSet;
        std::set<std::array<int,3>>  triSet;
        std::map<int, std::set<int>> adj;
        int maxId = -1;
        for (const auto& e : elements) {
            const int v[3] = { e.n0_id, e.n1_id, e.n2_id };
            for (int i = 0; i < 3; ++i) {
                int u = v[i], w = v[(i + 1) % 3];
                edgeSet.insert(canonicalEdge(u, w));
                adj[u].insert(w);
                adj[w].insert(u);
            }
            triSet.insert(sortedTri(v[0], v[1], v[2]));
            maxId = std::max(maxId, e.Element_id);
        }

        int added = 0;
        for (const auto& be : boundaryEdges) {
            auto key = canonicalEdge(be.n0_id, be.n1_id);
            if (edgeSet.count(key)) continue;                  // edge already meshed
            int a = key.first, b = key.second;
            auto ia = adj.find(a), ib = adj.find(b);
            if (ia == adj.end() || ib == adj.end()) continue;  // nothing to stitch to

            int apex = -1;
            for (int c : ia->second) {
                if (!ib->second.count(c)) continue;            // must be a common neighbour
                if (triSet.count(sortedTri(a, b, c))) continue;// that triangle must be absent
                apex = c;
                break;
            }
            if (apex < 0) continue;                            // not a fillable single-triangle notch

            elements.push_back({ a, b, apex, ++maxId });
            edgeSet.insert(key);
            edgeSet.insert(canonicalEdge(a, apex));
            edgeSet.insert(canonicalEdge(b, apex));
            triSet.insert(sortedTri(a, b, apex));
            adj[a].insert(b);
            adj[b].insert(a);
            ++added;
        }
        return added;
    }
}