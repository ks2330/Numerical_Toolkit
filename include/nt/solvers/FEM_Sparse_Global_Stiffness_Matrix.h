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
    
    struct SparseMatrixCSR {
        std::vector<double> values;
        std::vector<int> col_indices;
        std::vector<int> row_ptr; 
    };

    inline std::vector<double> computeElementStiffnessMatrix(const meshgeneration::Mesh& mesh, const meshgeneration::Element& element) {
        meshgeneration::Node n1 = mesh.getNodeByID(element.n0_id);
        meshgeneration::Node n2 = mesh.getNodeByID(element.n1_id);
        meshgeneration::Node n3 = mesh.getNodeByID(element.n2_id);

        std::vector<double> stiffnessMatrix = {9, 0.0};

        double area = 0.5 * std::abs(n1.x * (n2.y - n3.y) + n2.x * (n3.y - n1.y) + n3.x * (n1.y - n2.y));
        
        if (area < 1e-14)
            throw std::runtime_error("FEM assembly: degenerate element " +
                std::to_string(element.Element_id) + " (area ≈ 0) — run validateMesh() before assembly");

        double b[3] = { n2.y - n3.y, n3.y - n1.y, n1.y - n2.y };
        double c[3] = { n3.x - n2.x, n1.x - n3.x, n2.x - n1.x };

        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                stiffnessMatrix[i * 3 + j] = (b[i] * b[j] + c[i] * c[j]) / (4.0 * area);
            }
        }
        return stiffnessMatrix;
    }


    inline void assembleGlobalStiffnessMatrix(const meshgeneration::Mesh& mesh) {
        int N = static_cast<int>(mesh.nodes.size());
        
        std::vector<std::map<int, double>> K(N);
        const int num_nodes = 3;
        for (const auto& element : mesh.elements) {
            std::vector<double> Ke = computeElementStiffnessMatrix(mesh, element);
            int globalNodeIndices[num_nodes] = {
                (int)mesh.getNodeIndex(element.n0_id),
                (int)mesh.getNodeIndex(element.n1_id),
                (int)mesh.getNodeIndex(element.n2_id)
            };
            
            for (int i = 0; i < num_nodes; ++i) {
                int row = globalNodeIndices[i];
                for (int j = 0; j < num_nodes; ++j) {
                    int col = globalNodeIndices[j];
                    K[row][col] += Ke[i * num_nodes + j];

                }
            }
        }
    }
    inline SparseMatrixCSR convertToCSR(const std::vector<std::map<int, double>>& K) {
        SparseMatrixCSR csr;
        int N = static_cast<int>(K.size());
    }
}