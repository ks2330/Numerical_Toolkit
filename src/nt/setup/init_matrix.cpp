#include <Eigen/Dense>
#include <vector>
#include "nt/setup/init_matrix.h"

namespace nt::setup
{   
    // This function initializes a vector of Eigen triplets to set up the initial conditions for a 1D system.
    // These triplets are intended to form a single-row sparse matrix or a sparse vector.
    // @param N The number of grid points.
    // @param U1 The value for interior points.
    // @param U0 The value for the first point (boundary condition).
    // @param UL The value for the last point (boundary condition).
    std::vector<Eigen::Triplet<double>> init_matrix(int N, double U1, double U0, double UL) {
        if (N <= 0) {
            return {};
        }

        std::vector<Eigen::Triplet<double>> triplets;
        triplets.reserve(N);

        // First point (boundary at i=0)
        triplets.emplace_back(0, 0, U0);

        // Interior points
        for (int i = 1; i < N - 1; ++i) {
            triplets.emplace_back(0, i, U1);
        }

        // Last point (boundary at i=N-1)
        if (N > 1) {
            triplets.emplace_back(0, N - 1, UL);
        }

        return triplets;
    }
}