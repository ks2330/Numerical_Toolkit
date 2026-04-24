#include <iostream>
#include <fstream>
#include <vector>
//#include "include/nt/finite_element_methods/FEM_Global_Stiffness_Matrix.h"
//#include "nt/setup_FEM/setup.h"
#include "mesh_generation/mesh_generation.h"
#include "mesh_generation/mesh_triangulation_algorithm.h"

int nx = 6;
int ny = 2;
int segsPerUnit = 1;
int numRandomNodes = 10;

int main() {
    std::cout << "This is the UI application for the Numerical Toolkit.\n";
    std::cout << "Please run the individual test applications to see specific functionalities in action.\n";
    meshgeneration::Mesh mesh;
    mesh.initialize("both", nx, ny, segsPerUnit); // nx=6, ny=2 → 21 nodes, 24 triangular elements
    mesh.generateRandomNodes(numRandomNodes, nx, ny); // Generate 40 random nodes within the rectangle

    const std::string outputPath_NODES = "boundary_nodes_rectangular.csv";
    std::cout << "Generated rectangular mesh with " << mesh.nodes.size() << " nodes.\n";

    std::ofstream nodesFile(outputPath_NODES);
    nodesFile << "id,x,y\n";
    for (size_t i = 0; i < mesh.nodes.size(); ++i) {
        nodesFile << i << ","
                  << mesh.nodes[i].x << ","
                  << mesh.nodes[i].y << "\n";
    }
    for (size_t i = 0; i < mesh.randomNodes.size(); ++i) {
        nodesFile << (mesh.nodes.size() + i) << ","
                  << mesh.randomNodes[i].x << ","
                  << mesh.randomNodes[i].y << "\n";
    }
    nodesFile.close();

    // Run Bowyer-Watson
    std::vector<meshgen::triangulation::Vec2> bwPoints;
    for (const auto& node : mesh.nodes)
        bwPoints.push_back({node.x, node.y});
    for (const auto& node : mesh.randomNodes)
        bwPoints.push_back({node.x, node.y});

    std::vector<meshgen::triangulation::Triangle> triangles =
        meshgen::triangulation::bowyerWatson(bwPoints, static_cast<double>(nx), static_cast<double>(ny));

    std::ofstream triFile("triangulation.csv");
    triFile << "ax,ay,bx,by,cx,cy\n";
    for (const auto& T : triangles)
        triFile << T.a.x << "," << T.a.y << ","
                << T.b.x << "," << T.b.y << ","
                << T.c.x << "," << T.c.y << "\n";
    triFile.close();
    std::cout << "Initial triangulation: " << triangles.size() << " triangles written to triangulation.csv\n";

/*
    
    std::ofstream circleFile("circumcircles.csv");
    circleFile << "cx,cy,radius\n";
    for (const auto& T : triangles) {
        auto cc = meshgen::triangulation::drawCircle(T.a, T.b, T.c);
        circleFile << cc.center.x << "," << cc.center.y << "," << cc.radius << "\n";
    }
    circleFile.close();
*/

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