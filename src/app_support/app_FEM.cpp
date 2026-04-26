#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "mesh_generation/mesh_generation.h"

namespace app_support::FEM::run
{
    void run_FEM(std::string shape, double dim1, double dim2, int segsPerUnit, int numRandomNodes) {
        std::cout << "This is the UI application for the Numerical Toolkit.\n";
        std::cout << "Please run the individual test applications to see specific functionalities in action.\n";
        meshgeneration::Mesh mesh;
        mesh.initialize(shape, static_cast<int>(dim1), static_cast<int>(dim2), segsPerUnit);
        mesh.generateRandomNodes(numRandomNodes, static_cast<int>(dim1), static_cast<int>(dim2));

        std::cout << "Generated rectangular mesh with " << mesh.nodes.size() << " nodes.\n";
    }


    void run_Triangulation(meshgeneration::Mesh& mesh, int nx, int ny) {
        mesh.triangulate(static_cast<double>(nx), static_cast<double>(ny));   
    }
}





//#include "include/nt/finite_element_methods/FEM_Global_Stiffness_Matrix.h"
//#include "nt/setup_FEM/setup.h"