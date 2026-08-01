#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include <cmath>
#include <Eigen/Sparse>

#include "potential_flow_solver.h"
#include "nt/solvers/FEM_Global_Stiffness_Matrix.h"   
#include "mesh_generation/mesh_generation.h"

namespace nt::solvers {

    class EigenSparsePotentialFlowSolver : public PotentialFlowSolver {
    public:
        std::vector<double> solve(meshgeneration::Mesh& mesh, double U_inf, double alpha) const override {
            using nt::fem::solvers::computeElementStiffnessMatrix;
            const int N = static_cast<int>(mesh.nodes.size());

            std::vector<char>   isDirichlet(N, 0);
            std::vector<double> dirichletValue(N, 0.0);
            for (int i = 0; i < N; ++i) {
                if (mesh.nodes[i].type == meshgeneration::NodeType::Boundary) {
                    isDirichlet[i] = 1;
                    dirichletValue[i] = U_inf * (mesh.nodes[i].x * std::cos(alpha)
                                               + mesh.nodes[i].y * std::sin(alpha));
                }
            }

            std::vector<Eigen::Triplet<double>> triplets;
            triplets.reserve(mesh.elements.size() * 9);
            Eigen::VectorXd rhs = Eigen::VectorXd::Zero(N);

            for (const auto& element : mesh.elements) {
                auto Ke = computeElementStiffnessMatrix(mesh, element);
                const int g[3] = {
                    static_cast<int>(mesh.getNodeIndex(element.n0_id)),
                    static_cast<int>(mesh.getNodeIndex(element.n1_id)),
                    static_cast<int>(mesh.getNodeIndex(element.n2_id))
                };
                for (int i = 0; i < 3; ++i) {
                    const int I = g[i];
                    if (isDirichlet[I]) continue;               
                    for (int j = 0; j < 3; ++j) {
                        const int J = g[j];
                        const double v = Ke.data[i][j];
                        if (isDirichlet[J]) rhs[I] -= v * dirichletValue[J];
                        else                triplets.emplace_back(I, J, v);
                    }
                }
            }
            for (int i = 0; i < N; ++i) {
                if (isDirichlet[i]) {
                    triplets.emplace_back(i, i, 1.0);
                    rhs[i] = dirichletValue[i];
                }
            }

            Eigen::SparseMatrix<double> K(N, N);
            K.setFromTriplets(triplets.begin(), triplets.end());  
            K.makeCompressed();

            Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> chol;
            chol.compute(K);
            if (chol.info() != Eigen::Success)
                throw std::runtime_error("EigenSparsePotentialFlowSolver: factorisation failed (matrix not SPD?)");

            Eigen::VectorXd x = chol.solve(rhs);
            if (chol.info() != Eigen::Success)
                throw std::runtime_error("EigenSparsePotentialFlowSolver: solve failed");

            return std::vector<double>(x.data(), x.data() + N);
        }

        std::string name() const override {
            return "Potential Flow (Eigen Sparse LDLT)";
        }
    };

}
