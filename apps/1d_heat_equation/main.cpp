#include <iostream>
#include <eigen/Dense>
#include <fstream>
#include "nt/build_matrix.h"
#include "nt/grid.h"
#include "nt/init_matrix.h"
#include "nt/p_cal.h"
#include "nt/forward_euler.h" 

// Use constexpr for true compile-time constants.
constexpr int N = 5;      // Number of points
constexpr double L = 0.04; // Length of the rod. Use double for physical quantities.
constexpr double U0 = 0.0; // Initial temperature at the first point (boundary condition)
constexpr double U1 = 100.0; // Initial temperature at interior points
constexpr double UL = 0.0; // Initial temperature at the last point (boundary condition)

constexpr double k = 0.1;   // Thermal conductivity
constexpr double rho = 1000; // Density of the material
constexpr double cp = 10.0; // Specific heat capacity
constexpr double dt = (10/3.0); // Time step size

constexpr int max_steps = 6; // Maximum number of time steps for the forward Euler method

int main() {
    
    std::cout << "--- New Spacing Function ---" << std::endl;
    
    nt::matrix::Grid grid(N, L);
    std::cout << "Grid spacing (dx): " << grid.dx() << std::endl;
    std::cout << "Grid size (number of points): " << grid.size() << std::endl;

    std::cout << "\n--- Build Matrix ---" << std::endl;
    std::vector<Eigen::Triplet<double>> triplets = nt::setup::init_matrix(N, U1, U0, UL);

    std::cout << "Triplets for initial conditions:" << std::endl;
    Eigen::SparseMatrix<double> A = nt::matrix::build_matrix(grid, triplets);
    std::cout << "Sparse Matrix A:" << std::endl;
    std::cout << A.toDense() << std::endl;

    std::cout << "\n--- Setting Up Coefficients. ---" << std::endl;
    double alpha = nt::nm::alpha_value(k, rho, cp);
    double p = nt::nm::p_value(alpha, dt, grid.dx());
    std::cout << "Alpha (thermal diffusivity): " << alpha << std::endl;
    std::cout << "P value (stability parameter): " << p << std::endl;

    std::cout << "\n--- Forward Euler Coefficients ---" << std::endl;
    Eigen::SparseMatrix<double> forwardEulerCoeffs = nt::fe::setup_forward_coefficients(N);
    std::cout << "Forward Euler Coefficients:" << std::endl;
    std::cout << forwardEulerCoeffs.toDense() << std::endl;

    std::cout << "\n--- Forward Euler Step ---" << std::endl;
    nt::fe::forward_euler_step(forwardEulerCoeffs, A.row(0), p, max_steps);







    return 0;
}