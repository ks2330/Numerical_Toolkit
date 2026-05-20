#pragma once
#include "triangulation_algorithm.h"
#include "mesh_generation/mesh_generation.h"

namespace meshgeneration {

    class DelaunayTriangulation : public TriangulationAlgorithm {
    public:
        void run(Mesh& mesh) override {
            mesh.elements = mesh.bowyerWatson();
            mesh.enforceConstraint();
            mesh.deleteHoles();
            mesh.enforceOutsideConstraints();
            mesh.metricAngles("results/metrics/angle_distribution.csv");
            mesh.metricAspectRatios("results/metrics/aspect_ratio_distribution.csv");
            mesh.improveMesh();
            mesh.buildNeighbours();
            mesh.laplacianSmoothing();
            mesh.metricAngles("results/metrics/angle_distribution_improved.csv");
            mesh.metricAspectRatios("results/metrics/aspect_ratio_distribution_improved.csv");
        }
    };

}
