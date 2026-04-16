#include <iostream>
#include <fstream>
#include <vector>
#include "nt/setup/finite_element_methods/FEM_Global_Stiffness_Matrix.h"

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

    return 0;
}
