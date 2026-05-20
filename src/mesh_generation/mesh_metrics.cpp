#include "mesh_generation/mesh_generation.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cmath>

namespace meshgeneration {


void Mesh::metricAngles(const std::string& outputFile) {
    writeMetric(18, outputFile,
        [](const Node& n0, const Node& n1, const Node& n2) {
            return minAngle(n0, n1, n2);
        },
        [](double a) { return std::min(static_cast<int>(a * 180.0 / M_PI) / 10, 17); });
}

void Mesh::metricAspectRatios(const std::string& outputFile) {
    writeMetric(10, outputFile,
        [](const Node& n0, const Node& n1, const Node& n2) {
            return aspectRatio(n0, n1, n2);
        },
        [](double ar) { return std::min(static_cast<int>(ar / 10), 9); });
}

Node Mesh::computeCentroid(const Element& e) {
    return (nodes[e.n0_id] + nodes[e.n1_id] + nodes[e.n2_id]) * (1.0/3.0);
}

std::tuple<Node,Node,Node> Mesh::computeEdgeMidpoint(const Element& e) {
    const Node& n1 = nodes[e.n0_id];
    const Node& n2 = nodes[e.n1_id];
    const Node& n3 = nodes[e.n2_id];
    return { (n1+n2)*0.5, (n2+n3)*0.5, (n3+n1)*0.5 };
}

} // namespace meshgeneration