#include <Eigen/Sparse>
#include "nt/build_matrix.h"
#include "nt/grid.h" // We need the full Grid definition to use its methods


namespace nt::matrix
{
    // This function would build a sparse matrix representing a spatial
    // operator, like the 1D Laplacian, based on the grid properties.
    Eigen::SparseMatrix<double> build_matrix(const nt::matrix::Grid& grid, const std::vector<Eigen::Triplet<double>>& triplet)
    {
        // Implementation would go here.
        Eigen::SparseMatrix<double> A(grid.size(), grid.size());
        A.setFromTriplets(triplet.begin(), triplet.end());
        return A;
    }

}
