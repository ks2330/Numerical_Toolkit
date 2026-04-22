#include <iostream>
#include <fstream>
#include <vector>
//#include "include/nt/finite_element_methods/FEM_Global_Stiffness_Matrix.h"
//#include "nt/setup_FEM/setup.h"
#include "mesh_generation/mesh_generation.h"

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



int main() {
    std::cout << "This is the UI application for the Numerical Toolkit.\n";
    std::cout << "Please run the individual test applications to see specific functionalities in action.\n";
    const std::string outputPath;

    std::vector<nt::fem::meshgen::Node> nodes;

    bool isRectangular = false; // Change to false to generate circular mesh
    if (isRectangular) {
    
        nt::fem::meshgen::ShapeGenerator::generateRectangularMesh(6, 2, nodes);
        
        const std::string outputPath = "boundary_nodes_rectangular.csv";
    } else {
        nt::fem::meshgen::ShapeGenerator::generateCircularMesh(1.0, 10, 36, nodes);
        const std::string outputPath = "boundary_nodes_circular.csv";
    }
    std::ofstream nodesFile(outputPath);
    nodesFile << "id,x,y\n";
    for (size_t i = 0; i < nodes.size(); ++i) {
        nodesFile << i << ","
                  << nodes[i].x << ","
                  << nodes[i].y << "\n";
    }
    nodesFile.close();

    std::cout << "Boundary nodes written to " << outputPath << "\n";

    return 0;
}

*/
int main() {
    std::cout << "This is the UI application for the Numerical Toolkit.\n";
    std::cout << "Please run the individual test applications to see specific functionalities in action.\n";

    meshgeneration::Mesh mesh;
    mesh.initialize("rectangle", 6, 2, 8); // nx=6, ny=2 → 21 nodes, 24 triangular elements
    mesh.generateRandomNodes(30, 6, 2); // Generate 10 random nodes within the rectangle

    const std::string outputPath = "boundary_nodes_rectangular.csv";
    std::cout << "Generated rectangular mesh with " << mesh.nodes.size() << " nodes.\n";
    std::cout << mesh.nodes[0].x << ", " << mesh.nodes[0].y << "\n";
    std::cout << mesh.nodes[1].x << ", " << mesh.nodes[1].y << "\n";

    std::ofstream nodesFile(outputPath);
    nodesFile << "id,x,y\n";
    for (size_t i = 0; i < mesh.nodes.size(); ++i) {
        nodesFile << i << ","
                  << mesh.nodes[i].x << ","
                  << mesh.nodes[i].y << "\n";
    }
    nodesFile.close();

    std::cout << "Boundary nodes written to " << outputPath << "\n";

    return 0;
}
