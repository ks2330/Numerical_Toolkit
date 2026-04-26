#pragma once
#include <vector>
#include "mesh_generation/mesh_generation.h"

namespace app_support::FEM::UI
{
    void write_boundry_nodes_to_csv(const meshgeneration::Mesh& mesh, const std::vector<meshgeneration::Node>& nodes, const std::string& outputPath);
    void write_triangulation_to_csv(const meshgeneration::Mesh& mesh, const std::vector<meshgeneration::Element>& elements, const std::vector<meshgeneration::Node>& nodes, const std::string& outputPath);
}


//#include "include/nt/finite_element_methods/FEM_Global_Stiffness_Matrix.h"
//#include "nt/setup_FEM/setup.h"