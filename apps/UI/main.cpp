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
int numRandomNodes = 40;

int main() {
    app_support::FEM::run::run_FEM("rectangle", nx, ny, segsPerUnit, numRandomNodes);
    meshgeneration::Mesh mesh;
    mesh.initialize("rectangle", nx, ny, segsPerUnit);
    mesh.generateRandomNodes(numRandomNodes, nx, ny);
    app_support::FEM::run::run_Triangulation(mesh, nx, ny);
    app_support::FEM::UI::write_boundry_nodes_to_csv(mesh, mesh.nodes, "boundary_nodes_rectangular.csv");
    app_support::FEM::UI::write_triangulation_to_csv(mesh, mesh.elements, mesh.nodes, "triangulation.csv");


    return 0;
}

/*
int main() {
    // Solve steady-state Laplace (heat) equation on a 2x6 FEM mesh.
    // Dirichlet BCs: T=100 at x=0 (left wall), T=0 at x=6 (right wall).
    // Neumann (zero flux) BCs on top and bottom — satisfied naturally.
        // Exact solution: T(x) = 100*(6-x)/6   (linear in x)

    nt::fem::Mesh mesh;
    mesh.initialize(6, 2); // nx=6, ny=2 → 21 nodes, 24 triangular elements

    const int N      = static_cast<int>(mesh.nodes.size());
    const int nx     = 6;
    const int ny     = 2;
    const int stride = nx + 1; // 7 nodes per row

    auto K = nt::fem::assembleGlobalStiffnessMatrix(mesh);
    std::vector<double> rhs(N, 0.0);

    // Enforce Dirichlet BCs on left (x=0) and right (x=6) columns.
    for (int row = 0; row <= ny; ++row) {
        nt::fem::applyDirichletBC(K, rhs, row * stride,          100.0);
        nt::fem::applyDirichletBC(K, rhs, row * stride + nx,       0.0);
    }

    std::vector<double> T = nt::fem::gaussianElimination(K, rhs);

    // ── Write node data ──────────────────────────────────────────────────
    std::ofstream nodesFile("steady_state_nodes.csv");
    nodesFile << "id,x,y,temperature\n";
    for (int i = 0; i < N; ++i) {
        nodesFile << i << ","
                  << mesh.nodes[i].x << ","
                  << mesh.nodes[i].y << ","
                  << T[i] << "\n";
    }
    nodesFile.close();

    // ── Write element connectivity ───────────────────────────────────────
    std::ofstream elemsFile("steady_state_elements.csv");
    elemsFile << "id,n0,n1,n2\n";
    for (int e = 0; e < static_cast<int>(mesh.elements.size()); ++e) {
        elemsFile << e << ","
                  << mesh.elements[e].nodeIDs[0] << ","
                  << mesh.elements[e].nodeIDs[1] << ","
                  << mesh.elements[e].nodeIDs[2] << "\n";
    }
    elemsFile.close();

    std::cout << "Steady-state solution written to:\n"
              << "  steady_state_nodes.csv\n"
              << "  steady_state_elements.csv\n"
              << "Run plot_steady_state.py to visualise.\n";

    // -- Write nodes to csv

    return 0;
}

*/