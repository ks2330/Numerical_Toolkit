#include "nt/forward_euler.h"
#include "Eigen/Sparse"
#include <vector>
#include <iostream>

namespace nt::fe
{

    Eigen::SparseMatrix<double> setup_forward_coefficients(int N)
    {
        Eigen::SparseMatrix<double> laplacian(N, N);
        std::vector<Eigen::Triplet<double>> triplets;
        // Reserve space for the triplets: 3 for each of the N-2 interior points.
        if (N > 2) {
            triplets.reserve(3 * (N - 2));
        }

        // The 1D Laplacian stencil [1, -2, 1] is applied to interior nodes only.
        // For Dirichlet boundary conditions, the first and last rows of the
        // Laplacian matrix are zero, which keeps the boundary values constant during time-stepping.
        for (int i = 1; i < N - 1; ++i) {
            triplets.emplace_back(i, i - 1, 1.0);    // Sub-diagonal
            triplets.emplace_back(i, i, -2.0);       // Main diagonal
            triplets.emplace_back(i, i + 1, 1.0);    // Super-diagonal
        }

        laplacian.setFromTriplets(triplets.begin(), triplets.end());
        return laplacian;
    }


    // This function simulates the system for multiple time steps.
    // A name like 'solve_forward_euler' might be more descriptive.
    Eigen::VectorXd solve_forward_euler(const Eigen::SparseMatrix<double>& Lv, Eigen::VectorXd u_current, double p, int max_steps)
    {
        // u_current is passed by value, so we can modify it freely inside the function.
        Eigen::VectorXd u_next(u_current.size());
        for (int step = 0; step < max_steps; ++step) {
            // The Forward Euler update equation for u_t = alpha * u_xx is u_{n+1} = u_n + dt * alpha / dx^2 * (D_xx * u_n)
            // Here, p = dt * alpha / dx^2 and Lv is the discrete Laplacian D_xx.
            u_next = u_current + p * (Lv * u_current);
            u_current = u_next;
        }
        return u_current;
    }

}