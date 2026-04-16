#pragma once
#include <vector>
#include <iostream>
#include <cmath>
#include <algorithm>

namespace nt::fem
{
    struct Node {
        double x, y; // The physical location (e.g., 0.0, 1.0)
    };

    struct Element {
        // The indices (IDs) of the 3 nodes that form this triangle.
        // In our 2x6 grid, these will be numbers between 0 and 20.
        int nodeIDs[3]; 
    };

    class Mesh {
    public:

        std::vector<Node> nodes;
        // For a 2x6 grid, we have 2 rows and 6 columns of squares.
        // mesh.nodes will look like this as a vector of Node structs:
        // 0: (0,0), 1: (1,0) ect.
        // it will be a 1 D vector of length 21, but we can think of it as a 2D grid of nodes.
        std::vector<Element> elements;
        // For a 2x6 grid, we have 12 rectangles, and each rectangle is split into 2 triangles, so we have 24 elements.
        // mesh.elements will look like this as a vector of Element structs:
        // 0: (0,1,7), 1: (1,7,8) for the first rectangle, then 2: (1,2,8), 3: (2,8,9) for the second rectangle, etc.
        // it will be a 1 D vector of length 24, but we can think of it as a 2D grid of elements.
        // element.nodeIDs will contain the indices of the nodes that form each triangle element.
        // For example, the first element (triangle) in the first rectangle will be formed by nodes 0, 1, and 7 (the bottom left, bottom right, and top left nodes of the rectangle).
        // e.g element.n

        // A simple function to initialize the nodes in a 2x6 grid.
        void initialize(int nx = 6, int ny = 2) {
            std::cout << "Initializing mesh nodes..." << std::endl;
            for (int i = 0; i <= ny; ++i) {
                for (int j = 0; j <= nx; ++j) {
                    nodes.push_back({ static_cast<double>(j), static_cast<double>(i)});
                }
            }
            std::cout << "Mesh nodes initialized." << std::endl;
            std::cout << "Initializing mesh elements..." << std::endl;
            // Create elements (triangles) for the nx x ny grid.
            for (int i = 0; i < ny; ++i) {
                for (int j = 0; j < nx; ++j) {
                    int bottomLeft  = i * (nx + 1) + j;
                    int bottomRight = i * (nx + 1) + (j + 1);
                    int topLeft     = (i + 1) * (nx + 1) + j;
                    int topRight    = (i + 1) * (nx + 1) + (j + 1);

                    // We can split the rectangle into two triangles:
                    // Triangle 1: (bottom left, bottom right, top left)
                    elements.push_back({ bottomLeft, bottomRight, topLeft });
                    // Triangle 2: (bottom right, top left, top right)
                    elements.push_back({ bottomRight, topLeft, topRight });
                }
            
            }
            std::cout << "Mesh elements initialized." << std::endl;
        
        };
    };

    struct Matrix3x3 {
        double data[3][3];
    };


    Matrix3x3 computeElementStiffnessMatrix(const Element& element, const std::vector<Node>& nodes) {
        // Placeholder for the actual stiffness matrix computation.
        // In a real implementation, this would involve calculating the area of the triangle,
        // the gradients of the shape functions, and then assembling the stiffness matrix.
        Matrix3x3 stiffnessMatrix = {0};
        
        Node n1 = nodes[element.nodeIDs[0]];
        Node n2 = nodes[element.nodeIDs[1]];
        Node n3 = nodes[element.nodeIDs[2]];


        double area = 0.5 * std::abs(n1.x * (n2.y - n3.y) + n2.x * (n3.y - n1.y) + n3.x * (n1.y - n2.y));
        // 3. Slopes (b is x-dir, c is y-dir)
        double b[3] = { n2.y - n3.y, n3.y - n1.y, n1.y - n2.y };
        double c[3] = { n3.x - n2.x, n1.x - n3.x, n2.x - n1.x };

        // 4. Fill the 3x3 matrix
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                // This is the (Slope_i dot Slope_j) / (4 * Area)
                // The 4*Area comes from the calculus derivation of the gradients
                stiffnessMatrix.data[i][j] = (b[i] * b[j] + c[i] * c[j]) / (4.0 * area);
            }
        }

        return stiffnessMatrix;
        // For our simple 2x6 grid, the stiffness matrix for the first element (nodes 0,1,7) should be:
        // [ 1.0, -0.5, -0.5]
    }


    inline std::vector<std::vector<double>> assembleGlobalStiffnessMatrix(const Mesh& mesh) {
        int N = static_cast<int>(mesh.nodes.size());
        std::vector<std::vector<double>> K(N, std::vector<double>(N, 0.0));
        for (const auto& element : mesh.elements) {
            Matrix3x3 Ke = computeElementStiffnessMatrix(element, mesh.nodes);
            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j)
                    K[element.nodeIDs[i]][element.nodeIDs[j]] += Ke.data[i][j];
        }
        return K;
    }

    // Applies a Dirichlet BC by eliminating the row/column for nodeID.
    inline void applyDirichletBC(std::vector<std::vector<double>>& K, std::vector<double>& rhs, int nodeID, double value) {
        int N = static_cast<int>(K.size());
        for (int i = 0; i < N; ++i) {
            if (i == nodeID) continue;
            rhs[i] -= K[i][nodeID] * value;
            K[i][nodeID] = 0.0;
            K[nodeID][i] = 0.0;
        }
        K[nodeID][nodeID] = 1.0;
        rhs[nodeID] = value;
    }

    // Solves Ax = b via Gaussian elimination with partial pivoting. Takes A and b by value.
    inline std::vector<double> gaussianElimination(std::vector<std::vector<double>> A, std::vector<double> b) {
        int N = static_cast<int>(A.size());
        for (int col = 0; col < N; ++col) {
            // Partial pivoting
            int maxRow = col;
            for (int row = col + 1; row < N; ++row)
                if (std::abs(A[row][col]) > std::abs(A[maxRow][col]))
                    maxRow = row;
            std::swap(A[col], A[maxRow]);
            std::swap(b[col], b[maxRow]);

            for (int row = col + 1; row < N; ++row) {
                if (std::abs(A[col][col]) < 1e-15) continue;
                double factor = A[row][col] / A[col][col];
                for (int k = col; k < N; ++k)
                    A[row][k] -= factor * A[col][k];
                b[row] -= factor * b[col];
            }
        }
        // Back substitution
        std::vector<double> x(N, 0.0);
        for (int i = N - 1; i >= 0; --i) {
            x[i] = b[i];
            for (int j = i + 1; j < N; ++j)
                x[i] -= A[i][j] * x[j];
            x[i] /= A[i][i];
        }
        return x;
    }
}