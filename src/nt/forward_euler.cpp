#include "nt/forward_euler.h"
#include "Eigen/Sparse"
#include <vector>
#include <iostream>

namespace nt::fe
{

    Eigen::SparseMatrix<double> setup_forward_coefficients(int N)
    {
        Eigen::SparseMatrix<double> A(N, N);
        std::vector<Eigen::Triplet<double>> triplets;
        triplets.reserve(3 * N - 2);

        for (int i = 0; i < N; i++) {
            // Only apply the Laplacian stencil to INTERIOR nodes
            if (i > 0 && i < N - 1) {
                // Main diagonal
                triplets.push_back(Eigen::Triplet<double>(i, i, -2.0));
                // Super-diagonal
                triplets.push_back(Eigen::Triplet<double>(i, i + 1, 1.0));
                // Sub-diagonal
                triplets.push_back(Eigen::Triplet<double>(i, i - 1, 1.0));
            }
            // Rows 0 and N-1 are left empty (implicitly all zeros)
        }

        A.setFromTriplets(triplets.begin(), triplets.end());
        return A;
    }


    Eigen::VectorXd forward_euler_step(const Eigen::SparseMatrix<double>& Lv,  Eigen::VectorXd u_l, double p, int max_steps)
    {
        Eigen::VectorXd u_next(u_l.size());
        for (int step = 0; step < max_steps; ++step) {
            u_next = u_l + p * (Lv * u_l);
            u_l = u_next;
            std::cout << "Step " << step + 1 << ": " << u_next.transpose() << std::endl;
        }
        return u_l;
    }

}