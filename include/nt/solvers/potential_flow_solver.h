#pragma once
#include <string>
#include <vector>
#include "mesh_generation/mesh_generation.h"


namespace nt::solvers {

    class PotentialFlowSolver {
    public:
        virtual ~PotentialFlowSolver() = default;

        virtual std::vector<double> solve(meshgeneration::Mesh& mesh, double U_inf, double alpha) const = 0;

        virtual std::string name() const = 0;
    };

}

