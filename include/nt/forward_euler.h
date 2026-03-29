#pragma once // Prevents the header from being included multiple times
#include <Eigen/Sparse>


namespace nt::fe
{
    Eigen::SparseMatrix<double> setup_forward_coefficients(int N);

    Eigen::VectorXd solve_forward_euler(const Eigen::SparseMatrix<double>& Lv, Eigen::VectorXd u_l, double p, int max_steps);
    
}

