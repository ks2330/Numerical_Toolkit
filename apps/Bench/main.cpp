#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <cstdlib> 
#include <cmath> 


#include "app_support/app_FEM.h"
#include "app_support/app_FEM_UI.h"
#include "nt/finite_element_methods/FEM_Potential_Flow.h"
#include "mesh_generation/algorithm_delaunay_triangulation.h"
#include "mesh_generation/algorithm_advancing_front_triangulation.h"
#include "nt/Bench/bench.h"
#include "nt/solvers/potential_flow_solver.h"
#include "nt/solvers/potential_flow_solvers.h"

struct Config {
    int nx;
    int ny;
    int segsPerUnit;
    int numRandomNodes;
    std::string shape;
    std::string shapeCSV;
    std::string boundaryCSV;
    std::string outputCSV;
    std::string triangulationCSV;
    std::string solutionCSV;
    std::string aerfoilDAT;
    std::string pressureFieldCSV;
};

Config config = {
    .nx = 600,
    .ny = 200,
    .segsPerUnit = 12,
    .numRandomNodes = 100,
    .shape = "ushape", // "rectangle", "circle", "both", "triangle" (not implemented yet), "ushape"
    .shapeCSV = "results/csv/Nodes.csv",
    .boundaryCSV = "results/csv/boundary_nodes_rectangular.csv",
    .outputCSV = "results/csv/ushape_nodes.csv",
    .triangulationCSV = "results/csv/triangulation.csv",
    .solutionCSV = "results/csv/steady_state_nodes.csv",
    .aerfoilDAT = "results/dat/aerfoil.dat",
    .pressureFieldCSV = "results/csv/pressure_field.csv",
};


bool are_meshes_identical(const meshgeneration::Mesh& mesh1, const meshgeneration::Mesh& mesh2) {
    if (mesh1.nodes.size() != mesh2.nodes.size()) {
        std::cerr << "Verification failed: Node counts differ ("
                  << mesh1.nodes.size() << " vs " << mesh2.nodes.size() << ").\n";
        return false;
    }
    if (mesh1.elements.size() != mesh2.elements.size()) {
        std::cerr << "Verification failed: Element counts differ ("
                  << mesh1.elements.size() << " vs " << mesh2.elements.size() << ").\n";
        return false;
    }


    for (size_t i = 0; i < mesh1.nodes.size(); ++i) {
        const auto& n1 = mesh1.nodes[i];
        const auto& n2 = mesh2.nodes[i];

        if (n1.Node_id != n2.Node_id || std::abs(n1.x - n2.x) > 1e-12 || std::abs(n1.y - n2.y) > 1e-12 || n1.type != n2.type || n1.group_id != n2.group_id) {
            std::cerr << "Verification failed: Node " << i << " data differs.\n";
            return false;
        }
    }

    for (size_t i = 0; i < mesh1.elements.size(); ++i) {
        const auto& e1 = mesh1.elements[i];
        const auto& e2 = mesh2.elements[i];
        if (e1.n0_id != e2.n0_id || e1.n1_id != e2.n1_id || e1.n2_id != e2.n2_id) {
            std::cerr << "Verification failed: Element " << i << " connectivity differs.\n";
            return false;
        }
    }

    return true;
}

auto create_deterministic_mesh_for_benchmark(double density_factor) -> meshgeneration::Mesh {

    srand(42);
    std::cout << "\n--- Creating and triangulating a deterministic mesh for benchmark ---\n";
    meshgeneration::Mesh mesh = app_support::FEM::run::initialise_from_CSV(config.aerfoilDAT, density_factor);
    meshgeneration::DelaunayTriangulation algo;
    mesh.triangulate(algo);
    
    return mesh;
}

int main() {
    try {

        std::cout << "--- Verifying mesh generation determinism ---\n";
        auto mesh1 = create_deterministic_mesh_for_benchmark(100.0);
        
        app_support::FEM::UI::write_boundry_nodes_to_csv(mesh1, mesh1.nodes, config.boundaryCSV);
        app_support::FEM::UI::write_triangulation_to_csv(mesh1, mesh1.elements, mesh1.nodes, config.triangulationCSV);
        
        auto mesh2 = create_deterministic_mesh_for_benchmark(100.0);
        if (are_meshes_identical(mesh1, mesh2)) {
            std::cout << "Verification successful: Meshes are identical.\n";
        } else {
            std::cerr << "Verification FAILED: Meshes are NOT identical. Benchmark results will be invalid.\n";
            return 1;
        }
        std::cout << "---------------------------------------------\n\n";

        const double U_inf = 1.0;
        const double alpha = 0.0;

        std::vector<std::unique_ptr<nt::solvers::PotentialFlowSolver>> solvers;
        solvers.push_back(std::make_unique<nt::solvers::DefaultPotentialFlowSolver>());
        solvers.push_back(std::make_unique<nt::solvers::FlatPotentialFlowSolver>());

        std::cout << "--- Running benchmarks for potential flow solvers ---\n";

        std::vector<double> mesh_sizes = {100.0, 200.0, 400.0, 800.0};

        for (double density : mesh_sizes) {
            std::cout << "\nBenchmarking with mesh density factor: " << density << "\n";
            for (const auto& solver_ptr : solvers) {
                auto mesh = create_deterministic_mesh_for_benchmark(density);

                auto solver_function = [&](meshgeneration::Mesh& m, double u, double a) {
                    return solver_ptr->solve(m, u, a);
                };
                nt::bench::run_benchmark(solver_ptr->name(), mesh, solver_function, U_inf, alpha);
            }
        }

    } catch (const std::runtime_error& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
