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

}
