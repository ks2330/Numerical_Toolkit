#include <iostream>
#include <eigen/Dense>
#include <fstream>
#include "nt/build_matrix.h"
#include "nt/grid.h"
#include "nt/init_matrix.h"

// Use constexpr for true compile-time constants.
constexpr int N = 5;      // Number of points
constexpr double L = 40.0; // Length of the rod. Use double for physical quantities.
constexpr double U0 = 0.0; // Initial temperature at the first point (boundary condition)
constexpr double U1 = 100.0; // Initial temperature at interior points
constexpr double UL = 0.0; // Initial temperature at the last point (boundary condition)


int main() {

    std::cout << "--- New Spacing Function ---" << std::endl;
    
    nt::matrix::Grid grid(N, L);
    std::cout << "Grid spacing (dx): " << grid.dx() << std::endl;
    std::cout << "Grid size (number of points): " << grid.size() << std::endl;

    std::cout << "\n--- Build Matrix ---" << std::endl;
    std::vector<Eigen::Triplet<double>> triplets = nt::setup::init_matrix(N, U1, U0, UL);

    Eigen::SparseMatrix<double> A = nt::matrix::build_matrix(grid, triplets);
    std::cout << "Sparse Matrix A:" << std::endl;
    std::cout << A << std::endl;

    
    return 0;
}