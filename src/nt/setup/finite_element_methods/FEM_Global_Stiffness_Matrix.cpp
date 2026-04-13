#include <Eigen/Sparse>
#include <vector>
#include "FEM_Global_Stiffness_Matrix.h"

namespace nt::fem
{

    Eigen::SparseMatrix<double> setup_global_stiffness_matrix(int N, int num_elements)
    {
        Eigen::SparseMatrix<double> global_stiffness_matrix(N, N);
        for (int i = 0; i < num_elements; ++i) {
            // Local stiffness matrix for a linear element
            Eigen::Matrix2d local_stiffness;
            local_stiffness << 1, -1,
                              -1, 1;

            // Map local stiffness to global stiffness matrix
            int node1 = i;     // First node of the element
            int node2 = i + 1; // Second node of the element

            global_stiffness_matrix.coeffRef(node1, node1) += local_stiffness(0, 0);
            global_stiffness_matrix.coeffRef(node1, node2) += local_stiffness(0, 1);
            global_stiffness_matrix.coeffRef(node2, node1) += local_stiffness(1, 0);
            global_stiffness_matrix.coeffRef(node2, node2) += local_stiffness(1, 1);
        }
        return global_stiffness_matrix;
    }
}


    {
        

}