#pragma once
#include <vector>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <algorithm>
#include <tuple>
#include "mesh_generation/mesh_generation.h"

namespace nt::fem::solvers
{
    template <typename T, int Rows, int Cols>
    struct Matrix {
        T data[Rows][Cols];
        const T& operator()(int i, int j) const { return data[i][j]; }
    };
    
    struct SparseMatrixCSR {
        std::vector<double> values;
        std::vector<int> col_indices;
        std::vector<int> row_ptr; 
    };

    using Matrix3x3 = Matrix<double, 3, 3>;

    inline Matrix<double, 3, 3> computeElementStiffnessMatrix(const meshgeneration::Mesh& mesh, const meshgeneration::Element& element) {
        meshgeneration::Node n1 = mesh.getNodeByID(element.n0_id);
        meshgeneration::Node n2 = mesh.getNodeByID(element.n1_id);
        meshgeneration::Node n3 = mesh.getNodeByID(element.n2_id);

        Matrix3x3 stiffnessMatrix = {0};

        double area = 0.5 * std::abs(n1.x * (n2.y - n3.y) + n2.x * (n3.y - n1.y) + n3.x * (n1.y - n2.y));
        
        if (area < 1e-14)
            throw std::runtime_error("FEM assembly: degenerate element " +
                std::to_string(element.Element_id) + " (area ≈ 0) — run validateMesh() before assembly");

        double b[3] = { n2.y - n3.y, n3.y - n1.y, n1.y - n2.y };
        double c[3] = { n3.x - n2.x, n1.x - n3.x, n2.x - n1.x };

        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                stiffnessMatrix.data[i][j] = (b[i] * b[j] + c[i] * c[j]) / (4.0 * area);
            }
        }
        return stiffnessMatrix;
    }

    template <typename MatrixContainer>
    inline void assembleGlobalStiffnessMatrix(const meshgeneration::Mesh& mesh, MatrixContainer& K) {
        int N = static_cast<int>(mesh.nodes.size());

        constexpr bool is_flat = std::is_same_v<MatrixContainer, std::vector<double>>;
        
        if constexpr (is_flat) {
            K.assign(N * N, 0.0);
        } else {
            K.assign(N, std::vector<double>(N, 0.0));
        }
        
        for (const auto& element : mesh.elements) {
            Matrix3x3 Ke = computeElementStiffnessMatrix(mesh, element);
            int globalNodeIndices[3] = {
                (int)mesh.getNodeIndex(element.n0_id),
                (int)mesh.getNodeIndex(element.n1_id),
                (int)mesh.getNodeIndex(element.n2_id)
            };
            
            for (int i = 0; i < 3; ++i) {
                int row = globalNodeIndices[i];
                for (int j = 0; j < 3; ++j) {
                    int col = globalNodeIndices[j];
                    
                    if constexpr (is_flat) {
                        K[row * N + col] += Ke.data[i][j];
                    } else {
                        K[row][col] += Ke.data[i][j];
                    }
                }
            }
        }
    }


    template <typename MatrixContainer>
    inline void applyDirichletBC(MatrixContainer& K, std::vector<double>& rhs, int nodeID, double value) {
        
        int N = static_cast<int>(rhs.size());
        if (nodeID < 0 || nodeID >= N) return;
        
        constexpr bool is_flat = std::is_same_v<MatrixContainer, std::vector<double>>;

        for (int i = 0; i < N; ++i) {
            if (i == nodeID) continue;
            
            if constexpr (is_flat) {
                rhs[i] -= K[i * N + nodeID] * value;
                K[i * N + nodeID] = 0.0;
                K[nodeID * N + i] = 0.0;
            } else {
                rhs[i] -= K[i][nodeID] * value;
                K[i][nodeID] = 0.0;
                K[nodeID][i] = 0.0;
            }
        }
        
        if constexpr (is_flat) {
            K[nodeID * N + nodeID] = 1.0;
        } else {
            K[nodeID][nodeID] = 1.0;
        }
        rhs[nodeID] = value;
    }

    inline std::vector<double> gaussianElimination(std::vector<std::vector<double>>& K, std::vector<double>& rhs) {
        int N = static_cast<int>(rhs.size());
        if (K.size() != static_cast<size_t>(N)) {
            std::cerr << "gaussianElimination: K has incorrect size for rhs.\n";
            return {}; 
        }
        for (int col = 0; col < N; ++col) {

            int maxRow = col;
            for (int row = col + 1; row < N; ++row)
                if (std::abs(K[row][col]) > std::abs(K[maxRow][col]))
                    maxRow = row;
            std::swap(K[col], K[maxRow]);
            std::swap(rhs[col], rhs[maxRow]);


            for (int row = col + 1; row < N; ++row) {
                if (std::abs(K[col][col]) < 1e-15) continue;
                double factor = K[row][col] / K[col][col];
                for (int k = col; k < N; ++k)
                    K[row][k] -= factor * K[col][k];
                rhs[row] -= factor * rhs[col];
            }
        }
        // Back substitution
        std::vector<double> x(N, 0.0);
        for (int i = N - 1; i >= 0; --i) {
            x[i] = rhs[i];
            for (int j = i + 1; j < N; ++j)
                x[i] -= K[i][j] * x[j];
            if (std::abs(K[i][i]) < 1e-15) return std::vector<double>(N, 0.0);
            x[i] /= K[i][i];
        }
        return x;
    }


    inline std::vector<double> gaussianEliminationFlat(std::vector<double>& K, std::vector<double>& rhs) {
        int N = static_cast<int>(rhs.size());
        if (K.size() != static_cast<size_t>(N) * N) {
            std::cerr << "gaussianEliminationFlat: K has incorrect size for rhs.\n";
            return {}; 
        }

        for (int col = 0; col < N; ++col) {

            int maxRow = col;
            for (int row = col + 1; row < N; ++row) {
                if (std::abs(K[row * N + col]) > std::abs(K[maxRow * N + col]))
                    maxRow = row;
            }
            
            if (maxRow != col) {
                for (int k = 0; k < N; ++k) std::swap(K[col * N + k], K[maxRow * N + k]);
                std::swap(rhs[col], rhs[maxRow]);
            }

            // Elimination loop (Uses FAST contiguous memory strides)
            for (int row = col + 1; row < N; ++row) {
                if (std::abs(K[col * N + col]) < 1e-15) continue;
                double factor = K[row * N + col] / K[col * N + col];
                
                for (int k = col; k < N; ++k) {
                    K[row * N + k] -= factor * K[col * N + k];
                }
                rhs[row] -= factor * rhs[col];
            }
        }
        
        // Back substitution
        std::vector<double> x(N, 0.0);
        for (int i = N - 1; i >= 0; --i) {
            x[i] = rhs[i];
            for (int j = i + 1; j < N; ++j) {
                x[i] -= K[i * N + j] * x[j];
            }
            if (std::abs(K[i * N + i]) < 1e-15) return std::vector<double>(N, 0.0);
            x[i] /= K[i * N + i];
        }
        return x;
    }
}