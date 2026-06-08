#pragma once
#include <string>
#include "mesh_generation/mesh_generation.h"

namespace app_support::FEM::run
{
    meshgeneration::Mesh run_FEM();
    void run_Triangulation(meshgeneration::Mesh& mesh, std::string method);
    void run_Advancing_Front();
    std::vector<double> run_FEM_Heat_Equation(meshgeneration::Mesh& mesh,
                                               int inletGroup, double T_inlet,
                                               int outletGroup, double T_outlet);
    std::vector<double> run_Potential_Flow(meshgeneration::Mesh& mesh,
                                           double U_inf, double alpha = 0.0);
    meshgeneration::Mesh initialise_from_CSV(std::string filename, double density = 1.0);
}
