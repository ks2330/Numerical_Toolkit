#pragma once
#include <vector>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <tuple>
#include "mesh_generation/mesh_generation.h"


namespace nt::fem
{

    struct Matrix3x3 {
        double data[3][3];
    };
   

    Matrix3x3 computeElementStiffnessMatrix(const meshgeneration::Mesh& mesh, const meshgeneration::Element& element) {
        meshgeneration::Node n1 = mesh.getNodeByID(element.n0_id);
        meshgeneration::Node n2 = mesh.getNodeByID(element.n1_id);
        meshgeneration::Node n3 = mesh.getNodeByID(element.n2_id);

        Matrix3x3 stiffnessMatrix = {0};

        double area = 0.5 * std::abs(n1.x * (n2.y - n3.y) + n2.x * (n3.y - n1.y) + n3.x * (n1.y - n2.y));

        double b[3] = { n2.y - n3.y, n3.y - n1.y, n1.y - n2.y };
        double c[3] = { n3.x - n2.x, n1.x - n3.x, n2.x - n1.x };


        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {

                stiffnessMatrix.data[i][j] = (b[i] * b[j] + c[i] * c[j]) / (4.0 * area);
            }
        }

        return stiffnessMatrix;

    }


    inline std::vector<std::vector<double>> assembleGlobalStiffnessMatrix(const meshgeneration::Mesh& mesh) {
        int N = static_cast<int>(mesh.nodes.size());
        std::vector<std::vector<double>> K(N, std::vector<double>(N, 0.0));
        for (const auto& element : mesh.elements) {
            Matrix3x3 Ke = computeElementStiffnessMatrix(mesh, element);
            int globalNodeIndices[3] = {
                (int)mesh.getNodeIndex(element.n0_id),
                (int)mesh.getNodeIndex(element.n1_id),
                (int)mesh.getNodeIndex(element.n2_id)
            };
            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j)
                    K[globalNodeIndices[i]][globalNodeIndices[j]] += Ke.data[i][j];

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