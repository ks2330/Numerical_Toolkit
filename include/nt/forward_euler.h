#pragma once // Prevents the header from being included multiple times
#include <Eigen/Sparse>


namespace nt::fe
{
    Eigen::SparseMatrix<double> setup_forward_coefficients(int N);

}