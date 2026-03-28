#include "nt/forward_euler.h"
#include "Eigen/Sparse"
#include <vector>

namespace nt::fe
{

    Eigen::SparseMatrix<double> setup_forward_coefficients(int N)
    {
        Eigen::SparseMatrix<double> A(N, N);
        std::vector<Eigen::Triplet<double>> triplets;
    
        // Reserve space for roughly 3n non-zero elements
        triplets.reserve(3 * N - 2);

        for (int i = 0; i < N; i++) {
            // Main diagonal
            triplets.push_back(Eigen::Triplet<double>(i, i, 2.0));
            // Super-diagonal
            if (i < N - 1) triplets.push_back(Eigen::Triplet<double>(i, i + 1, -1.0));
            // Sub-diagonal
            if (i > 0) triplets.push_back(Eigen::Triplet<double>(i, i - 1, -1.0));
        }

        A.setFromTriplets(triplets.begin(), triplets.end());
        return A;

    }

    Eigen::VectorXd forward_euler_next_vector(const Eigen::SparseMatrix<double>& L, const Eigen::VectorXd& u_l)
    {

        return L * u_l;
    
    }
    
    Eigen::VectorXd forward_euler_step(const Eigen::SparseMatrix<double>& L, const Eigen::VectorXd& u_l, double p)
    {
        return u_l + p * (L * u_l);
    }

}