#include <iostream>
#include <eigen/Dense>
#include <fstream>
<<<<<<< HEAD
#include "app_support/app_forward_euler.h"

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
    
    app_support::forward_euler::run_forward_euler(N, L, U0, U1, UL, k, rho, cp, dt, max_steps); 
=======

int L = 100; // Length of the rod
int N = 101; // Number of points

int main() {
    std::cout << "--- Numerical Toolkit: 1D Heat Equation Solver ---" << std::endl;

    double dx = L / (N - 1);
    Eigen::VectorXd x  = Eigen::VectorXd::Zero(N);

    std::cout << "Rod length (L): " << L << std::endl;
    std::cout << "Number of points (N): " << N << std::endl;

    std::cout << "Spatial grid (x):" << std::endl;
    std::cout << x.transpose() << std::endl;

>>>>>>> origin/main
    return 0;
}