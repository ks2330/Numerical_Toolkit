#pragma once
#include <string>
#include "mesh_generation/mesh_generation.h"

namespace app_support::FEM::run
{
    void run_FEM(std::string shape, double dim1, double dim2, int segsPerUnit, int numRandomNodes);
    void run_Triangulation(meshgeneration::Mesh& mesh, int nx, int ny);
    void run_FEM_Heat_Equation(meshgeneration::Mesh& mesh);
}


//#include "include/nt/finite_element_methods/FEM_Global_Stiffness_Matrix.h"
//#include "nt/setup_FEM/setup.h"