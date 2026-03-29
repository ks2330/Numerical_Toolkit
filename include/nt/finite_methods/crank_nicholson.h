#pragma once // Prevents the header from being included multiple times
#include <Eigen/Sparse>


namespace nt::fe
{
    Eigen::SparseMatrix<double> setup_crank_nicholson_lhs_coefficients(int N, double p);
    Eigen::SparseMatrix<double> setup_crank_nicholson_rhs_coefficients(int N, double p);
    Eigen::VectorXd setup_crank_nicholson_rhs_matrix(const Eigen::SparseMatrix<double> A, const Eigen::VectorXd u_current);

    Eigen::VectorXd solve_crank_nicholson(Eigen::VectorXd u_current, double p, int max_steps);
    
}