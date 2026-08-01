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
            mesh.improveMesh();
            mesh.buildNeighbours();
            mesh.laplacianSmoothing();
        }
    };

}
