#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include <cmath>

#include "potential_flow_solver.h"
#include "nt/solvers/FEM_Global_Stiffness_Matrix.h" 
#include "mesh_generation/mesh_generation.h"

namespace nt::solvers {

    // TODO after implementing Python Front End.

    // THIS IS A SKELETON. Fill in the TODOs to complete the CSR assembly, Dirichlet elimination,
    // ────────────────────────────────────────────────────────────────────────────
    class SparsePotentialFlowSolver : public PotentialFlowSolver {
    public:
        std::vector<double> solve(meshgeneration::Mesh& mesh, double U_inf, double alpha) const override {
            const int N = static_cast<int>(mesh.nodes.size());

            // 1. Assemble the global stiffness matrix in CSR (+ zero rhs).
            nt::fem::solvers::SparseMatrixCSR K;
            std::vector<double> rhs(N, 0.0);
            assembleCSR(mesh, K, rhs);

            // 2. Far-field Dirichlet BCs, applied symmetrically so K stays SPD.
            applyDirichletBCs(mesh, K, rhs, U_inf, alpha);

            // 3. Solve with Conjugate Gradient.
            std::vector<double> phi(N, 0.0);
            conjugateGradient(K, rhs, phi, /*tol=*/1e-10, /*maxIters=*/10 * N + 100);
            return phi;
        }

        std::string name() const override { return "Potential Flow (Hand-rolled CSR + CG)"; }

    private:
        // TODO: assemble the global stiffness matrix in CSR format, and zero the rhs.
        static void assembleCSR(const meshgeneration::Mesh& mesh,
                                nt::fem::solvers::SparseMatrixCSR& K,
                                std::vector<double>& rhs) {
            (void)mesh; (void)K; (void)rhs;
            throw std::runtime_error("SparsePotentialFlowSolver::assembleCSR not implemented yet");
        }

        // TODO: symmetric Dirichlet elimination on the CSR system
        static void applyDirichletBCs(const meshgeneration::Mesh& mesh,
                                      nt::fem::solvers::SparseMatrixCSR& K,
                                      std::vector<double>& rhs,
                                      double U_inf, double alpha) {
            (void)mesh; (void)K; (void)rhs; (void)U_inf; (void)alpha;
            throw std::runtime_error("SparsePotentialFlowSolver::applyDirichletBCs not implemented yet");
        }

        // TODO: sparse matrix-vector product  y = A * x  (CSR).
        static void spmv(const nt::fem::solvers::SparseMatrixCSR& A,
                         const std::vector<double>& x, std::vector<double>& y) {
            (void)A; (void)x; (void)y;
            throw std::runtime_error("SparsePotentialFlowSolver::spmv not implemented yet");
        }

        // TODO: Conjugate Gradient for the SPD system A·x = b.
        static void conjugateGradient(const nt::fem::solvers::SparseMatrixCSR& A,
                                      const std::vector<double>& b,
                                      std::vector<double>& x,
                                      double tol, int maxIters) {
            (void)A; (void)b; (void)x; (void)tol; (void)maxIters;
            throw std::runtime_error("SparsePotentialFlowSolver::conjugateGradient not implemented yet");
        }
    };

}
