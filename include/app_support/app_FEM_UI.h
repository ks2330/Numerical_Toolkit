#pragma once
#include <vector>
#include "mesh_generation/mesh_generation.h"

namespace app_support::FEM::UI
{
    void write_boundry_nodes_to_csv(const meshgeneration::Mesh& mesh, const std::vector<meshgeneration::Node>& nodes, const std::string& outputPath);
    void write_triangulation_to_csv(const meshgeneration::Mesh& mesh, const std::vector<meshgeneration::Element>& elements, const std::vector<meshgeneration::Node>& nodes, const std::string& outputPath);
    void write_Solution_to_csv(std::vector<double>& T, const std::string& outputPath, int N, meshgeneration::Mesh& mesh);
}


