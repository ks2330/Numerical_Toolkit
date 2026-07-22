#pragma once

#include "potential_flow_solver.h"
#include "nt/solvers/FEM_Global_Stiffness_Matrix.h"
#include "nt/solvers/FEM_Potential_Flow.h"

namespace nt::solvers {

    class DefaultPotentialFlowSolver : public PotentialFlowSolver {
        public:
            std::vector<double> solve(meshgeneration::Mesh& mesh, double U_inf, double alpha) const override {
                const int N = static_cast<int>(mesh.nodes.size());
                std::vector<std::vector<double>> K_dense;
                nt::fem::solvers::assembleGlobalStiffnessMatrix(mesh, K_dense);
                std::vector<double> rhs(N, 0.0);
                nt::fem::solvers::applyPotentialFlowBCs(mesh, K_dense, rhs, U_inf, alpha);
                return nt::fem::solvers::gaussianElimination(K_dense, rhs);
            }

            std::string name() const override {
                return "Potential Flow (Default Dense Solver)";
            }
    };

    class FlatPotentialFlowSolver : public PotentialFlowSolver {
        public:
            std::vector<double> solve(meshgeneration::Mesh& mesh, double U_inf, double alpha) const override {
                const int N = static_cast<int>(mesh.nodes.size());
                std::vector<double> K;
                nt::fem::solvers::assembleGlobalStiffnessMatrix(mesh, K);
                std::vector<double> rhs(N, 0.0);
                nt::fem::solvers::applyPotentialFlowBCs(mesh, K, rhs, U_inf, alpha);
                return nt::fem::solvers::gaussianEliminationFlat(K, rhs);
            }

            std::string name() const override {
                return "Potential Flow (Flat Dense Solver)";
            }
    };



}