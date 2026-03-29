#include "nt/finite_methods/crank_nicholson.h"
#include <Eigen/SparseLU>
#include <vector>
#include <iostream>

namespace nt::fe
{
    // Sets up the implicit part of the Crank-Nicolson scheme, which is the matrix 'A'
    // in the linear system A * u_next = B * u_current.
    // For the 1D heat equation, A = I - (p/2) * Laplacian.
    Eigen::SparseMatrix<double> setup_crank_nicholson_lhs_coefficients(int N, double p)
    {
        Eigen::SparseMatrix<double> A_lhs(N, N);
        std::vector<Eigen::Triplet<double>> triplets;
        // Reserve space for the triplets: 3 for each of the N-2 interior points, plus 2 for boundaries.
        if (N > 0) {
            triplets.reserve(3 * (N - 2) + 2);
        }

        // For Dirichlet boundary conditions, the boundary rows are identity.
        triplets.emplace_back(0, 0, 1.0);
        if (N > 1) {
            triplets.emplace_back(N - 1, N - 1, 1.0);
        }

        // Interior rows for the implicit part: stencil [-p/2, 1+p, -p/2]
        for (int i = 1; i < N - 1; ++i) {
            triplets.emplace_back(i, i - 1, -p / 2.0);    // Sub-diagonal
            triplets.emplace_back(i, i, 1.0 + p);       // Main diagonal
            triplets.emplace_back(i, i + 1, -p / 2.0);    // Super-diagonal
        }

        A_lhs.setFromTriplets(triplets.begin(), triplets.end());
        return A_lhs;
    }

    // Sets up the implicit part of the Crank-Nicolson scheme, which is the matrix 'A'
    // in the linear system A * u_next = B * u_current.
    // For the 1D heat equation, A = I - (p/2) * Laplacian.
    Eigen::SparseMatrix<double> setup_crank_nicholson_rhs_coefficients(int N, double p)
    {
        Eigen::SparseMatrix<double> A_rhs(N, N);
        std::vector<Eigen::Triplet<double>> triplets;
        // Reserve space for the triplets: 3 for each of the N-2 interior points, plus 2 for boundaries.
        if (N > 0) {
            triplets.reserve(3 * (N - 2) + 2);
        }

        // For Dirichlet boundary conditions, the boundary rows are identity.
        triplets.emplace_back(0, 0, 1.0);
        if (N > 1) {
            triplets.emplace_back(N - 1, N - 1, 1.0);
        }

        // Interior rows for the implicit part: stencil [-p/2, 1+p, -p/2]
        for (int i = 1; i < N - 1; ++i) {
            triplets.emplace_back(i, i - 1, p / 2.0);    // Sub-diagonal
            triplets.emplace_back(i, i, 1.0 - p);       // Main diagonal
            triplets.emplace_back(i, i + 1, p / 2.0);    // Super-diagonal
        }

        A_rhs.setFromTriplets(triplets.begin(), triplets.end());
        return A_rhs;
    }

    // Sets up the explicit part of the Crank-Nicolson scheme, which is the matrix 'B'
    // in the linear system A * u_next = A * u_current.
    Eigen::VectorXd setup_crank_nicholson_rhs_matrix(const Eigen::SparseMatrix<double> A, const Eigen::VectorXd u_current)
    {
        Eigen::VectorXd rhs = A * u_current;
        return rhs;
    }


    // This function simulates the system for multiple time steps.
    // A name like 'solve_crank_nicholson' might be more descriptive.
    
    Eigen::VectorXd solve_crank_nicholson(Eigen::VectorXd u_current, double p, int max_steps)
    {
        // u_current is passed by value, so we can modify it freely inside the function.
        Eigen::VectorXd u_next(u_current.size());
        Eigen::SparseLU<Eigen::SparseMatrix<double>> solver;
        solver.compute(setup_crank_nicholson_lhs_coefficients(u_current.size(), p));
        if (solver.info() != Eigen::Success) {
            std::cerr << "Decomposition failed!" << std::endl;
            return u_current; // Return the current state if decomposition fails.
        }
        std::cout << "Initial state: " << u_current.transpose() << std::endl;
        for (int step = 0; step < max_steps; ++step) {
            u_next = solver.solve(setup_crank_nicholson_rhs_matrix(setup_crank_nicholson_rhs_coefficients(u_current.size(), p), u_current));
            u_current = u_next;
            std::cout << "After step " << step + 1 << ": " << u_current.transpose() << std::endl;
        }
        return u_current;
    }
    
}