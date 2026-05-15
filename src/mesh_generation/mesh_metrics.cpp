#include "mesh_generation/mesh_generation.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cmath>

namespace meshgeneration {

void Mesh::MetricAngles(std::string outputFile) {
    std::vector<int> bins(18, 0);
    for (const auto& e : elements) {
        double a = minAngle(nodes[e.n0_id], nodes[e.n1_id], nodes[e.n2_id]);
        bins[std::min(static_cast<int>(a * 180.0 / M_PI) / 10, 17)]++;
    }
    std::ofstream f(outputFile);
    if (!f.is_open()) {
        std::cerr << "Could not write to " << outputFile << "\n";
        return;
    }
    f << "Angle (degrees),Count\n";
    for (size_t i = 0; i < bins.size(); ++i) f << i * 10 << "," << bins[i] << "\n";
    std::cout << "Angle distribution written to " << outputFile << "\n";
}

void Mesh::MetricAspectRatios(std::string outputFile) {
    std::vector<int> bins(10, 0);
    for (const auto& e : elements) {
        double r = aspectRatio(nodes[e.n0_id], nodes[e.n1_id], nodes[e.n2_id]);
        bins[std::min(static_cast<int>(r / 10), 9)]++;
    }
    std::ofstream f(outputFile);
    if (!f.is_open()) {
        std::cerr << "Could not write to " << outputFile << "\n";
        return;
    }
    f << "Aspect Ratio,Count\n";
    for (size_t i = 0; i < bins.size(); ++i) f << i * 10 << "," << bins[i] << "\n";
    std::cout << "Aspect ratio distribution written to " << outputFile << "\n";
}

Node Mesh::computeCentroid(const Element& e) {
    const Node& n1 = nodes[e.n0_id];
    const Node& n2 = nodes[e.n1_id];
    const Node& n3 = nodes[e.n2_id];
    return {(n1.x+n2.x+n3.x)/3.0, (n1.y+n2.y+n3.y)/3.0, -1};
}

std::tuple<Node,Node,Node> Mesh::computeEdgeMidpoint(const Element& e) {
    const Node& n1 = nodes[e.n0_id];
    const Node& n2 = nodes[e.n1_id];
    const Node& n3 = nodes[e.n2_id];
    return { {(n1.x+n2.x)/2.0, (n1.y+n2.y)/2.0, -1},
             {(n2.x+n3.x)/2.0, (n2.y+n3.y)/2.0, -1},
             {(n3.x+n1.x)/2.0, (n3.y+n1.y)/2.0, -1} };
}

} // namespace meshgeneration