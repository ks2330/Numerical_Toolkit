#include "mesh_generation/mesh_generation.h"
#include <iostream>

namespace meshgeneration {

void Mesh::triangulate(std::string method) {
    if (nodes.empty()) return;
    if (method == "advancing_front") {
        AdvancingFront();
        deleteHoles();
        enforceOutsideConstraints();
        buildNeighbours();
        LaplacianSmoothing();
    } else {
        elements = bowyerWatson();
        enforceConstraint();
        deleteHoles();
        enforceOutsideConstraints();
        MetricAngles("results/metrics/angle_distribution.csv");
        MetricAspectRatios("results/metrics/aspect_ratio_distribution.csv");
        ImproveMesh();
        buildNeighbours();
        LaplacianSmoothing();
        MetricAngles("results/metrics/angle_distribution_improved.csv");
        MetricAspectRatios("results/metrics/aspect_ratio_distribution_improved.csv");
    }
}

} // namespace meshgeneration