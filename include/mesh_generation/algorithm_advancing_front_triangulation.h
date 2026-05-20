#pragma once
#include "triangulation_algorithm.h"
#include "mesh_generation/mesh_generation.h"

namespace meshgeneration {

    class AdvancingFrontTriangulation : public TriangulationAlgorithm {
    public:
        void run(Mesh& mesh) override {
            mesh.advancingFront();
            mesh.deleteHoles();
            mesh.enforceOutsideConstraints();
            mesh.buildNeighbours();
            mesh.laplacianSmoothing();
        }
    };

}