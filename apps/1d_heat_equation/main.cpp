#include <iostream>
#include <eigen/Dense>
#include <fstream>

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

    return 0;
}