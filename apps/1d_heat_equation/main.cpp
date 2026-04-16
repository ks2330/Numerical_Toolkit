#include <iostream>
#include <eigen/Dense>
#include <fstream>
#include "app_support/app_forward_euler.h"
#include "nt/setup/finite_element_methods/FEM_Global_Stiffness_Matrix.h"

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
    std::cout << "Running 1D Heat Equation Simulation with Forward Euler Method..." << std::endl;
    app_support::forward_euler::run_forward_euler(N, L, U0, U1, UL, k, rho, cp, dt, max_steps); 

    return 0;
}