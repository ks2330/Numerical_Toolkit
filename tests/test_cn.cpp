#include <gtest/gtest.h>
#include <eigen/Sparse>
#include "nt/finite_methods/crank_nicholson.h"

TEST(Crank_Nicholson_Test, SetupCoeff_lhs) {
    int N = 5;
    double p = (1.0/3.0);
    Eigen::SparseMatrix<double> expected(N, N);
    std::vector<Eigen::Triplet<double>> triplets;
    triplets.emplace_back(0, 0, 1.0);
    triplets.emplace_back(1, 0, -p / 2.0);    // Sub-diagonal
    triplets.emplace_back(1, 1, 1.0 + p);     // Main diagonal
    triplets.emplace_back(1, 2, -p / 2.0);    // Super-diagonal
    triplets.emplace_back(2, 1, -p / 2.0);    // Sub-diagonal
    triplets.emplace_back(2, 2, 1.0 + p);     // Main diagonal
    triplets.emplace_back(2, 3, -p / 2.0);    // Super-diagonal
    triplets.emplace_back(3, 2, -p / 2.0);    // Sub-diagonal
    triplets.emplace_back(3, 3, 1.0 + p);     // Main diagonal
    triplets.emplace_back(3, 4, -p / 2.0);    // Super-diagonal
    triplets.emplace_back(4, 4, 1.0);
    expected.setFromTriplets(triplets.begin(), triplets.end());
    Eigen::SparseMatrix<double> actual = nt::fe::setup_crank_nicholson_lhs_coefficients(N, p);
    EXPECT_TRUE(actual.isApprox(expected)) << "The setup_crank_nicholson_lhs_coefficients function did not produce the expected sparse matrix.";
}

TEST(Crank_Nicholson_Test, SetupCoeff_rhs) {
    int N = 5;
    double p = (1.0/3.0);
    Eigen::SparseMatrix<double> expected(N, N);
    std::vector<Eigen::Triplet<double>> triplets;
    triplets.emplace_back(0, 0, 1.0);
    triplets.emplace_back(1, 0, p / 2.0);    // Sub-diagonal
    triplets.emplace_back(1, 1, 1.0 - p);     // Main diagonal
    triplets.emplace_back(1, 2, p / 2.0);    // Super-diagonal
    triplets.emplace_back(2, 1, p / 2.0);    // Sub-diagonal
    triplets.emplace_back(2, 2, 1.0 - p);     // Main diagonal
    triplets.emplace_back(2, 3, p / 2.0);    // Super-diagonal
    triplets.emplace_back(3, 2, p / 2.0);    // Sub-diagonal
    triplets.emplace_back(3, 3, 1.0 - p);     // Main diagonal
    triplets.emplace_back(3, 4, p / 2.0);    // Super-diagonal
    triplets.emplace_back(4, 4, 1.0);
    expected.setFromTriplets(triplets.begin(), triplets.end());
    Eigen::SparseMatrix<double> actual = nt::fe::setup_crank_nicholson_rhs_coefficients(N, p);
    EXPECT_TRUE(actual.isApprox(expected)) << "The setup_crank_nicholson_rhs_coefficients function did not produce the expected sparse matrix.";
}



TEST(Crank_Nicholson_Test_1_Step, Crank_Nicholson) {
    int N = 5;
    double p = (1.0/3.0);
    Eigen::SparseMatrix<double> A = nt::fe::setup_crank_nicholson_rhs_coefficients(N, p);
    Eigen::VectorXd u_current(N);
    u_current << 0, 100, 100, 100, 0; // Initial condition
    Eigen::VectorXd expected(N);
    expected << 0, (p/2.0)*0 + (1.0 - p)*100.0 + (p/2.0)*100.0,
    (p/2.0)*100.0 + (1.0 - p)*100.0 + (p/2.0)*100.0,
    (p/2.0)*100.0 + (1.0 - p)*100.0 + (p/2.0)*0,
    0; // Expected result
    Eigen::VectorXd actual = nt::fe::setup_crank_nicholson_rhs_matrix(A, u_current);
    EXPECT_TRUE(actual.isApprox(expected)) << "The setup_crank_nicholson_rhs_matrix function did not produce the expected RHS vector.";
}

TEST(Crank_Nicholson_Test_2nd_Step, Crank_Nicholson) {
    int N = 5;
    double p = (1.0/3.0);
    int max_steps = 1;
    Eigen::VectorXd u_current(N);
    u_current << 0, 100, 100, 100, 0; // Initial condition
    Eigen::VectorXd expected(N);
    expected << 0, 
    2300/31.0,
    2900/31.0,
    2300/31.0,
    0; // Expected result
    Eigen::VectorXd actual = nt::fe::solve_crank_nicholson(u_current, p, max_steps);
    EXPECT_TRUE(actual.isApprox(expected)) << "The solve_crank_nicholson function did not produce the expected result after " << max_steps << " steps.";
}