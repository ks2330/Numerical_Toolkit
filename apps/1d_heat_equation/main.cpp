#include <iostream>
#include <eigen/Dense>
#include <fkYAML/node.hpp>
#include <fstream>

int L = 100; // Length of the rod
int N = 101; // Number of points

int main() {
    // std::cout << "--- Numerical Toolkit: 1D Heat Equation Solver ---" << std::endl;
    
    // std::ifstream ifs("config.yaml");
    // fkyaml::node config = fkyaml::node::deserialize(ifs);

    // if (config.contains("Rod_Length")) {
    //     L = config["Rod_Length"].get_value<int>();
    // }
    // if (config.contains("Num_Points")) {
    //     N = L / (config["Num_Points"].get_value<int>());
    // }
    // Spatial discretization
    double dx = L / (N - 1);
    Eigen::VectorXd x  = Eigen::VectorXd::Zero(N);

    std::cout << "Rod length (L): " << L << std::endl;
    std::cout << "Number of points (N): " << N << std::endl;

    std::cout << "Spatial grid (x):" << std::endl;
    std::cout << x.transpose() << std::endl;

    return 0;
}