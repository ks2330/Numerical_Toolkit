#include <iostream>
#include <eigen/Dense>

int main() {
    std::cout << "--- Numerical Toolkit: Navier-Stokes Solver ---" << std::endl;
    
    // Test Eigen Matrix
    Eigen::Matrix2d m;
    m << 1, 2, 3, 4;
    std::cout << "Eigen Test Matrix:\n" << m << std::endl;

    std::cout << "\nStatus: System Ready for Sprint 1." << std::endl;
    return 0;
}