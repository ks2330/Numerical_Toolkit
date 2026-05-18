#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include <limits>
#include "mesh_generation/mesh_types.h"

namespace meshgeneration {

// Geometry utility functions for mesh generation algorithms

// Compute the signed area of the triangle formed by points a, b, c
inline double orient2d(const Node& a, const Node& b, const Node& c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

// Computes Length of edge between two nodes
inline double distance(const Node& a, const Node& b) {
    return std::sqrt((b.x-a.x)*(b.x-a.x) + (b.y-a.y)*(b.y-a.y));
}

// Compute the squared distance between two nodes (avoids sqrt for efficiency)
inline double distanceSquared(const Node& a, const Node& b) {
    return (b.x-a.x)*(b.x-a.x) + (b.y-a.y)*(b.y-a.y);
}
// Check if the points in 'nodes' are ordered counter-clockwise
inline bool isCCW(const std::vector<Node>& nodes) {
    double sum = 0;
    for (int i = 0; i < (int)nodes.size(); ++i) {
        const Node& a = nodes[i];
        const Node& b = nodes[(i + 1) % nodes.size()];
        sum += (b.x - a.x) * (b.y + a.y);
    }
    return sum < 0;
}

// edge direction vector from node a to node b
inline std::pair<double, double> edgeDirection(const Node& a, const Node& b) {
    return {b.x - a.x, b.y - a.y};
}

inline bool isInCircle(const Node& A, const Node& B, const Node& C, const Node& D) {
    double adx = A.x-D.x, ady = A.y-D.y;
    double bdx = B.x-D.x, bdy = B.y-D.y;
    double cdx = C.x-D.x, cdy = C.y-D.y;
    double det = (adx*adx+ady*ady)*(bdx*cdy-cdx*bdy)
                -(bdx*bdx+bdy*bdy)*(adx*cdy-cdx*ady)
                +(cdx*cdx+cdy*cdy)*(adx*bdy-bdx*ady);
    return det > 0;
}

inline bool isSameEdge(const Element& t, const Edge& e) {
    auto match = [](int a, int b, const Edge& edge) {
        return (a == edge.n0_id && b == edge.n1_id) || (a == edge.n1_id && b == edge.n0_id);
    };
    return match(t.n0_id, t.n1_id, e) ||
           match(t.n1_id, t.n2_id, e) ||
           match(t.n2_id, t.n0_id, e);
}

inline bool isPointInPolygon(const Node& point, const std::vector<Node>& boundary) {
    if (boundary.empty()) return false;
    int hits = 0;
    size_t n = boundary.size();
    for (size_t i = 0; i < n; ++i) {
        const Node& p1 = boundary[i];
        const Node& p2 = boundary[(i + 1) % n];
        if (point.y > std::min(p1.y, p2.y) && point.y <= std::max(p1.y, p2.y)) {
            double xInt = p1.x + (point.y - p1.y) * (p2.x - p1.x) / (p2.y - p1.y);
            if (xInt > point.x) ++hits;
        }
    }
    return hits % 2 == 1;
}

inline std::vector<Node> GetBoundingBox(const std::vector<Node>& b) {
    if (b.empty()) return {};
    double minX = b[0].x, maxX = b[0].x, minY = b[0].y, maxY = b[0].y;
    for (const auto& n : b) {
        minX = std::min(minX, n.x); maxX = std::max(maxX, n.x);
        minY = std::min(minY, n.y); maxY = std::max(maxY, n.y);
    }
    return {{minX,minY,-1},{maxX,minY,-1},{maxX,maxY,-1},{minX,maxY,-1}};
}

inline double minAngle(const Node& a, const Node& b, const Node& c) {
    double ab = distance(a, b), bc = distance(b, c), ca = distance(c, a);
    double A = std::acos(std::clamp((ab*ab + ca*ca - bc*bc) / (2*ab*ca), -1.0, 1.0));
    double B = std::acos(std::clamp((ab*ab + bc*bc - ca*ca) / (2*ab*bc), -1.0, 1.0));
    double C = std::acos(std::clamp((bc*bc + ca*ca - ab*ab) / (2*bc*ca), -1.0, 1.0));
    return std::min({A, B, C});
}

inline double aspectRatio(const Node& a, const Node& b, const Node& c) {
    double ab = distance(a, b), bc = distance(b, c), ca = distance(c, a);
    double longest = std::max({ab, bc, ca});
    double area = 0.5 * std::abs(orient2d(a, b, c));
    return (0.433 * longest * longest) / area;
}

inline Node RotateVector(Node a, Node b, double angle) {
    double cosA = std::cos(angle), sinA = std::sin(angle);
    return {cosA *(b.x - a.x) - sinA * (b.y - a.y) + a.x,
            sinA *(b.x - a.x) + cosA * (b.y - a.y) + a.y, -1};
}

inline bool edgesMatch(const Edge& a, const Edge& b) {
    return (a.n0_id == b.n0_id && a.n1_id == b.n1_id) ||
           (a.n0_id == b.n1_id && a.n1_id == b.n0_id);
}

} // namespace meshgeneration