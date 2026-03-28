#include <Eigen/Dense>
#include <vector>
#include "nt/init_matrix.h"

namespace nt::setup
{   
    // This function initializes a vector of Eigen triplets to set up the initial conditions
    // @param N The number of grid points
    // @param U1 The value to set for interior points
    // @param U0 The value to set for the first point (boundary condition)
    // @param UL The value to set for the last point (boundary condition)
    std::vector<Eigen::Triplet<double>> init_matrix(int N, double U1, double U0, double UL) {
        std::vector<Eigen::Triplet<double>> triplets;
        for (int i = 0; i < N; ++i) {
            if (i != 0 && i != N - 1) {
                triplets.emplace_back(0, i, U1); // Set boundary conditions
            } else if (i == N - 1) {
                triplets.emplace_back(0, i, UL); // Set boundary conditions
            } else {
                triplets.emplace_back(0, i, U0); // Interior points start at U0
            }
        }
        return triplets;
    }
}