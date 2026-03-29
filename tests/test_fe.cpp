#include <gtest/gtest.h>
#include <eigen/Sparse>
#include "nt/finite_methods/forward_euler.h"

TEST(Forward_Euler_Test, SetupCoeff) {
    int N = 5;
    Eigen::SparseMatrix<double> expected(N, N);
    std::vector<Eigen::Triplet<double>> triplets;
    triplets.emplace_back(1, 0, 1.0);    // Sub-diagonal
    triplets.emplace_back(1, 1, -2.0);   // Main diagonal
    triplets.emplace_back(1, 2, 1.0);    // Super-diagonal
    triplets.emplace_back(2, 1, 1.0);    // Sub-diagonal
    triplets.emplace_back(2, 2, -2.0);   // Main diagonal
    triplets.emplace_back(2, 3, 1.0);    // Super-diagonal
    triplets.emplace_back(3, 2, 1.0);    // Sub-diagonal
    triplets.emplace_back(3, 3, -2.0);   // Main diagonal
    triplets.emplace_back(3, 4, 1.0);    // Super-diagonal
    expected.setFromTriplets(triplets.begin(), triplets.end());
    Eigen::SparseMatrix<double> actual = nt::fe::setup_forward_coefficients(N);

    EXPECT_TRUE(actual.isApprox(expected)) << "The setup_forward_coefficients function did not produce the expected sparse matrix.";
}

TEST(Forward_Euler_Test_1_Step, SolveEuler) {
    double p = (1.0/3.0); // Example value for p (dt * alpha / dx^2)
    int max_steps = 1;
    int N = 5;
    Eigen::SparseMatrix<double> expected(N, N);
    std::vector<Eigen::Triplet<double>> triplets;
    triplets.emplace_back(1, 0, 1.0);    // Sub-diagonal
    triplets.emplace_back(1, 1, -2.0);   // Main diagonal
    triplets.emplace_back(1, 2, 1.0);    // Super-diagonal
    triplets.emplace_back(2, 1, 1.0);    // Sub-diagonal
    triplets.emplace_back(2, 2, -2.0);   // Main diagonal
    triplets.emplace_back(2, 3, 1.0);    // Super-diagonal
    triplets.emplace_back(3, 2, 1.0);    // Sub-diagonal
    triplets.emplace_back(3, 3, -2.0);   // Main diagonal
    triplets.emplace_back(3, 4, 1.0);    // Super-diagonal
    expected.setFromTriplets(triplets.begin(), triplets.end());

    Eigen::VectorXd u_current(N);
    u_current << 0, 100, 100, 100, 0; // Initial condition: u(0) = 0, u(1) = u(2) = u(3) = 100, u(4) = 0
    Eigen::VectorXd expected_u_next(N);
    expected_u_next << 0, 200/3.0, 100.0, 200/3.0, 0; // Expected result after one step
    Eigen::VectorXd actual_u_next = nt::fe::solve_forward_euler(expected, u_current, p, max_steps);

    EXPECT_TRUE(actual_u_next.isApprox(expected_u_next)) << "The solve_forward_euler function did not produce the expected result after one step.";

}

TEST(Forward_Euler_Test_all_Step, SolveEuler) {
    double p = (1.0/3.0); // Example value for p (dt * alpha / dx^2)
    int max_steps = 6;
    int N = 5;
    Eigen::SparseMatrix<double> expected(N, N);
    std::vector<Eigen::Triplet<double>> triplets;
    triplets.emplace_back(1, 0, 1.0);    // Sub-diagonal
    triplets.emplace_back(1, 1, -2.0);   // Main diagonal
    triplets.emplace_back(1, 2, 1.0);    // Super-diagonal
    triplets.emplace_back(2, 1, 1.0);    // Sub-diagonal
    triplets.emplace_back(2, 2, -2.0);   // Main diagonal
    triplets.emplace_back(2, 3, 1.0);    // Super-diagonal
    triplets.emplace_back(3, 2, 1.0);    // Sub-diagonal
    triplets.emplace_back(3, 3, -2.0);   // Main diagonal
    triplets.emplace_back(3, 4, 1.0);    // Super-diagonal
    expected.setFromTriplets(triplets.begin(), triplets.end());

    Eigen::VectorXd u_current(N);
    u_current << 50, 0, 0, 0, 0; // Initial condition: u(0) = 50, u(1) = u(2) = u(3) = 0, u(4) = 0
    Eigen::VectorXd expected_u_final(N);
    expected_u_final << 50, 31.70, 16.80, 6.72, 0; // Expected result after all steps
    Eigen::VectorXd actual_u_final = nt::fe::solve_forward_euler(expected, u_current, p, max_steps);

    EXPECT_TRUE(actual_u_final.isApprox(expected_u_final)) << "The solve_forward_euler function did not produce the expected result after all steps.";

}