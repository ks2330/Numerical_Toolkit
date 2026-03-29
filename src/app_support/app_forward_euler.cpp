#include <iostream>
#include <eigen/Dense>
#include <fstream>
#include "nt/setup/grid.h"
#include "nt/setup/init_matrix.h"
#include "nt/setup/build_matrix.h"
#include "nt/setup/p_cal.h"
#include "nt/finite_methods/forward_euler.h" 
#include "app_support/app_forward_euler.h"

namespace app_support::forward_euler
{
    void run_forward_euler(const int N, const double L, const double U0, const double U1, const double UL, 
                          const double k, const double rho, const double cp, const double dt, int max_steps)
    {
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
        nt::fe::solve_forward_euler(forwardEulerCoeffs, A.row(0), p, max_steps);
    }
}