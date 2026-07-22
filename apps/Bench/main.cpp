#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <limits>
#include <string>


#include "app_support/app_FEM.h"
#include "app_support/app_FEM_UI.h"
#include "nt/finite_element_methods/FEM_Potential_Flow.h"
#include "mesh_generation/algorithm_delaunay_triangulation.h"
#include "mesh_generation/algorithm_advancing_front_triangulation.h"
#include "nt/Bench/bench.h"
#include "nt/solvers/potential_flow_solver.h"
#include "nt/solvers/potential_flow_solvers.h"
#include "nt/solvers/eigen_sparse_potential_flow.h"
#include "nt/solvers/sparse_potential_flow.h"       // hand-rolled CSR + CG (skeleton — fill in the TODOs)

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
    meshgeneration::Mesh mesh = app_support::FEM::run::initialise_from_CSV(config.aerfoilDAT, density_factor);
    meshgeneration::DelaunayTriangulation algo;
    mesh.triangulate(algo);

    return mesh;
}

// Largest absolute difference between two solution vectors (correctness metric).
static double max_abs_diff(const std::vector<double>& a, const std::vector<double>& b) {
    double d = 0.0;
    const size_t n = std::min(a.size(), b.size());
    for (size_t i = 0; i < n; ++i) d = std::max(d, std::abs(a[i] - b[i]));
    if (a.size() != b.size()) d = std::numeric_limits<double>::infinity();
    return d;
}

int main() {
    try {
        // ── Guard: the whole benchmark hinges on every solver seeing the SAME mesh.
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

        // Contenders. The FIRST one is the reference every other solver is checked against.
        std::vector<std::unique_ptr<nt::solvers::PotentialFlowSolver>> solvers;
        solvers.push_back(std::make_unique<nt::solvers::DefaultPotentialFlowSolver>());      // dense vector<vector>, Gaussian
        solvers.push_back(std::make_unique<nt::solvers::FlatPotentialFlowSolver>());         // flat contiguous, Gaussian
        solvers.push_back(std::make_unique<nt::solvers::EigenSparsePotentialFlowSolver>());  // Eigen sparse LDLT
        // solvers.push_back(std::make_unique<nt::solvers::SparsePotentialFlowSolver>());  // uncomment once the CSR + CG TODOs are filled in

        const std::vector<double> mesh_sizes = {100.0, 200.0, 400.0, 800.0};
        const int    reps    = 5;      // timed passes per (solver, size)
        const int    warmup  = 1;      // untimed pass to prime caches/allocator
        const double CORRECTNESS_TOL = 1e-6;   // max |phi - phi_reference|

        std::ofstream csv("results/csv/bench.csv");
        csv << "density,num_nodes,solver,median_s,min_s,max_abs_diff_vs_ref\n";

        std::cout << "--- Benchmarking potential-flow solvers (median of " << reps << " runs) ---\n";
        for (double density : mesh_sizes) {
            // Reference solution from the first solver, on an identical deterministic mesh.
            auto ref_mesh = create_deterministic_mesh_for_benchmark(density);
            const int N = static_cast<int>(ref_mesh.nodes.size());
            const std::vector<double> phi_ref = solvers.front()->solve(ref_mesh, U_inf, alpha);

            std::cout << "\n=== density " << density << "  (" << N << " nodes) ===\n";
            for (const auto& solver : solvers) {
                std::vector<double> phi;
                auto stats = nt::bench::timed_runs(
                    [&] { return create_deterministic_mesh_for_benchmark(density); },
                    [&](meshgeneration::Mesh& m) { return solver->solve(m, U_inf, alpha); },
                    reps, warmup, &phi);

                const double diff = max_abs_diff(phi, phi_ref);
                const char*  ok   = (diff <= CORRECTNESS_TOL) ? "OK" : "MISMATCH";

                std::cout << "  " << solver->name()
                          << "  |  median " << stats.median_s << " s"
                          << "  min " << stats.min_s << " s"
                          << "  |  max|dphi| " << diff << " [" << ok << "]\n";

                csv << density << "," << N << ",\"" << solver->name() << "\","
                    << stats.median_s << "," << stats.min_s << "," << diff << "\n";
            }
        }
        csv.close();
        std::cout << "\nWrote results/csv/bench.csv  (plot with: python apps/Bench/plot_bench.py)\n";

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
