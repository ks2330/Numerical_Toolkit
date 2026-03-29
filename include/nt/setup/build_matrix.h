#pragma once // Prevents the header from being included multiple times
#include <Eigen/Sparse>
#include "nt/setup/grid.h"

namespace nt::matrix
{
    // This function would build a sparse matrix representing a spatial
    // operator, like the 1D Laplacian, based on the grid properties.
    Eigen::SparseMatrix<double> build_matrix(const nt::matrix::Grid& grid, const std::vector<Eigen::Triplet<double>>& triplets);
}
