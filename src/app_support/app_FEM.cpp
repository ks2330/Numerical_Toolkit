#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "mesh_generation/mesh_generation.h"
#include "nt/finite_element_methods/FEM_Global_Stiffness_Matrix.h"
#include "nt/finite_element_methods/FEM_Heat_Equation.h"
#include "nt/finite_element_methods/FEM_Potential_Flow.h"

namespace app_support::FEM::run
{
    meshgeneration::Mesh run_FEM() {
        std::cout << "This is the UI application for the Numerical Toolkit.\n";
        std::cout << "Please run the individual test applications to see specific functionalities in action.\n";
        meshgeneration::Mesh mesh;
        //mesh.initialize(shape, static_cast<int>(dim1), static_cast<int>(dim2), segsPerUnit);
        mesh.init("results/csv/ushape_nodes.csv");
        //mesh.generateRandomNodes();

        std::cout << "Generated rectangular mesh with " << mesh.nodes.size() << " nodes.\n";
        
        return mesh;
    }

    void run_Advancing_Front() {
        std::cout << "Advancing Front method is not yet implemented.\n";
    }

    std::vector<double> run_FEM_Heat_Equation(meshgeneration::Mesh& mesh,
                                               int inletGroup, double T_inlet,
                                               int outletGroup, double T_outlet) {
        const int N = static_cast<int>(mesh.nodes.size());
        auto K = nt::fem::assembleGlobalStiffnessMatrix(mesh);
        std::vector<double> rhs(N, 0.0);
        nt::fem::applyHeatEquationBCs(mesh, K, rhs, inletGroup, T_inlet, outletGroup, T_outlet);
        return nt::fem::gaussianElimination(K, rhs);
    }

    std::vector<double> run_Potential_Flow(meshgeneration::Mesh& mesh,
                                           double U_inf, double alpha = 0.0) {
        const int N = static_cast<int>(mesh.nodes.size());
        auto K = nt::fem::assembleGlobalStiffnessMatrix(mesh);
        std::vector<double> rhs(N, 0.0);
        nt::fem::applyPotentialFlowBCs(mesh, K, rhs, U_inf, alpha);
        return nt::fem::gaussianElimination(K, rhs);
    }

    meshgeneration::Mesh initialise_from_CSV(std::string filename, double density = 1.0) {
        meshgeneration::Mesh mesh;
        mesh.init(filename, density);
        mesh.generateRandomNodes();
        return mesh;
    }

}