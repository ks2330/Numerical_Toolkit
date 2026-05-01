#pragma once
#include <vector>
#include <cmath>
#include "mesh_generation/mesh_types.h"

// Boundary node generators for standard 2D shapes.
// Each function returns an ordered list of boundary nodes (CCW for outer boundaries).
// Node IDs are assigned sequentially starting from `id_offset`.
// These are intended to be passed to Mesh::setOuterBoundary() or Mesh::addHole().

namespace shapegeneration::shapes {
    using meshgeneration::Node;

    // Generates boundary nodes for a rectangle, traversing CCW:
    // bottom (L→R), right (B→T), top (R→L), left (T→B).
    // segsPerUnit controls node spacing (e.g. 1 = one node per unit length).
    inline std::vector<Node> rectangle(double width, double height, int segsPerUnit, int id_offset = 0) {
        std::vector<Node> boundary;
        const int nx = static_cast<int>(width  * segsPerUnit/4);
        const int ny = static_cast<int>(height * segsPerUnit/4);
        int id = id_offset;

        for (int i = 0; i <= nx; ++i)
            boundary.push_back({ i * width / nx, 0.0, id++ });
        for (int j = 1; j <= ny; ++j)
            boundary.push_back({ width, j * height / ny, id++ });
        for (int i = nx - 1; i >= 0; --i)
            boundary.push_back({ i * width / nx, height, id++ });
        for (int j = ny - 1; j >= 1; --j)
            boundary.push_back({ 0.0, j * height / ny, id++ });

        return boundary;
    }

    // Generates boundary nodes for a circle, sampled at numSegments equally spaced angles.
    // cx, cy define the centre. Nodes are ordered CCW starting from angle 0.
    inline std::vector<Node> circle(double radius, double cx, double cy, int numSegments, int id_offset) {
    //inline std::vector<Node> circle(double radius = 6, double cx = 0, double cy = 0, int numSegments = 12, int id_offset = 0) {
        std::vector<Node> boundary;
        const double angleStep = 2.0 * M_PI / numSegments;
        for (int i = 0; i < numSegments; ++i) {
            double angle = i * angleStep;
            boundary.push_back({ cx + radius * cos(angle), cy + radius * sin(angle), id_offset + i });
        }
        return boundary;
    }

    
    // Generates boundary nodes for an upside-down U (Pi shape), traversing CCW.
    // thickness = width/3, notchDepth = 60% of height.
    inline std::vector<Node> uShape(double width, double height, int segsPerUnit, int id_offset = 0) {
        std::vector<Node> boundary;
        double t = width / 3.0;
        double d = height * 0.6;
        int id = id_offset;

        // 8 corners in CCW order
        struct Corner { double x, y; };
        Corner corners[8] = {
            {0,       0},
            {t,       0},
            {t,       d},
            {width-t, d},
            {width-t, 0},
            {width,   0},
            {width,   height},
            {0,       height}
        };

        for (int c = 0; c < 8; ++c) {
            Corner from = corners[c];
            Corner to   = corners[(c + 1) % 8];
            double len  = std::sqrt((to.x-from.x)*(to.x-from.x) + (to.y-from.y)*(to.y-from.y));
            int n = std::max(1, static_cast<int>(len * segsPerUnit / 4));
            for (int i = 0; i < n; ++i) {  // exclude endpoint to avoid duplicates
                double fx = from.x + i * (to.x - from.x) / n;
                double fy = from.y + i * (to.y - from.y) / n;
                boundary.push_back({fx, fy, id++});
            }
        }

        return boundary;
    }

}
