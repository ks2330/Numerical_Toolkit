#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "mesh_generation/mesh_generation.h"
#include "nt/finite_element_methods/FEM_Global_Stiffness_Matrix.h"
//#include "nt/setup_FEM/setup.h"

namespace app_support::FEM::run
{
    meshgeneration::Mesh run_FEM(std::string shape, double dim1, double dim2, int segsPerUnit, int numRandomNodes) {
        std::cout << "This is the UI application for the Numerical Toolkit.\n";
        std::cout << "Please run the individual test applications to see specific functionalities in action.\n";
        meshgeneration::Mesh mesh;
        mesh.initialize(shape, static_cast<int>(dim1), static_cast<int>(dim2), segsPerUnit);
        mesh.generateRandomNodes(numRandomNodes, static_cast<int>(dim1), static_cast<int>(dim2), "poisson");

        std::cout << "Generated rectangular mesh with " << mesh.nodes.size() << " nodes.\n";
        
        return mesh;
    }


    void run_Triangulation(meshgeneration::Mesh& mesh, int nx, int ny) {
        mesh.triangulate();   
    }

    std::vector<double> run_FEM_Heat_Equation(meshgeneration::Mesh& mesh) {
        // Solve steady-state Laplace (heat) equation on a 2x6 FEM mesh.
        // Dirichlet BCs: T=100 at x=0 (left wall), T=0 at x=6 (right wall).
        // Neumann (zero flux) BCs on top and bottom — satisfied naturally.
        // Exact solution: T(x) = 100*(6-x)/6   (linear in x)
        const int N = static_cast<int>(mesh.nodes.size());
        const int MaxX = mesh.getMaxNodeCol();

        auto K = nt::fem::assembleGlobalStiffnessMatrix(mesh);
        std::vector<double> rhs(N, 0.0);

        for (size_t i = 0; i < mesh.nodes.size(); ++i) {
            if (std::abs(mesh.nodes[i].x) < 1e-9)
                nt::fem::applyDirichletBC(K, rhs, i, 100.0);  // left wall
            if (std::abs(mesh.nodes[i].x - MaxX) < 1e-9)
                nt::fem::applyDirichletBC(K, rhs, i, 0.0);    // right wall
        }
        std::vector<double> T = nt::fem::gaussianElimination(K, rhs);
    return T;
    } 

    meshgeneration::Mesh initialise_from_CSV(std::string filename, double dim1, double dim2, int numRandomNodes) {
        meshgeneration::Mesh mesh;
        mesh.init(filename);
        mesh.generateRandomNodes(numRandomNodes, static_cast<int>(dim1), static_cast<int>(dim2), "poisson");
        return mesh;
    }

}