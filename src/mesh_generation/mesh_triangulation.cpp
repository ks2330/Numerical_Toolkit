#include "mesh_generation/mesh_generation.h"
#include <iostream>
#include <stdexcept>
#include <cmath>

namespace meshgeneration {

void Mesh::triangulate(TriangulationAlgorithm& algorithm) {
    if (nodes.empty())
        throw std::runtime_error("triangulate: mesh has no nodes");
    algorithm.run(*this);
    validateMesh();
}

void Mesh::validateMesh() const {
    if (elements.empty())
        throw std::runtime_error("validateMesh: triangulation produced no elements");

    int nodeCount = static_cast<int>(nodes.size());
    int degenerate = 0;

    for (const auto& e : elements) {
        if (e.n0_id < 0 || e.n0_id >= nodeCount ||
            e.n1_id < 0 || e.n1_id >= nodeCount ||
            e.n2_id < 0 || e.n2_id >= nodeCount)
            throw std::runtime_error("validateMesh: element " +
                std::to_string(e.Element_id) + " references out-of-bounds node");

        const Node& n0 = nodes[e.n0_id];
        const Node& n1 = nodes[e.n1_id];
        const Node& n2 = nodes[e.n2_id];
        double area = 0.5 * std::abs(n0.x * (n1.y - n2.y) +
                                     n1.x * (n2.y - n0.y) +
                                     n2.x * (n0.y - n1.y));
        if (area < 1e-14) ++degenerate;
    }

    if (degenerate > 0)
        std::cerr << "validateMesh: " << degenerate
                  << " degenerate element(s) detected (area \xe2\x89\x88 0)\n";
}

} // namespace meshgeneration