#include <iostream>
#include <fstream>
#include <vector>
//#include "include/nt/finite_element_methods/FEM_Global_Stiffness_Matrix.h"
//#include "nt/setup_FEM/setup.h"
//#include "mesh_generation/mesh_generation.h"

#include "app_support/app_FEM.h"
#include "app_support/app_FEM_UI.h"

int nx = 6;
int ny = 2;
int segsPerUnit = 1;
int numRandomNodes = 30;

int main() {
    app_support::FEM::run::run_FEM("rectangle", nx, ny, segsPerUnit, numRandomNodes);
    meshgeneration::Mesh mesh;
    mesh.initialize("rectangle", nx, ny, segsPerUnit);
    mesh.generateRandomNodes(numRandomNodes, nx, ny);
    app_support::FEM::run::run_Triangulation(mesh, nx, ny);
    app_support::FEM::UI::write_boundry_nodes_to_csv(mesh, mesh.nodes, "boundary_nodes_rectangular.csv");
    app_support::FEM::UI::write_triangulation_to_csv(mesh, mesh.elements, mesh.nodes, "triangulation.csv");

    std::vector<double> T = app_support::FEM::run::run_FEM_Heat_Equation(mesh);
    app_support::FEM::UI::write_Solution_to_csv(T, "steady_state_nodes.csv", mesh.nodes.size(), mesh);

    return 0;
}

