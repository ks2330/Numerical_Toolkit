#pragma once
#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>
#include <string_view>
#include <vector>
#include "mesh_generation/mesh_generation.h"
#include "app_support/app_FEM.h"


namespace nt::bench {

    using PotentialFlowSolver = std::function<std::vector<double>(meshgeneration::Mesh&, double, double)>;

    inline void run_benchmark(std::string_view benchmark_name, meshgeneration::Mesh& mesh,
                              PotentialFlowSolver solver, double U_inf, double alpha) {
        // This benchmark function will mutate the mesh passed to it.
        // To compare multiple solvers, create a fresh mesh for each call.
        auto start = std::chrono::high_resolution_clock::now();
        std::vector<double> phi = solver(mesh, U_inf, alpha);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "Benchmark '" << benchmark_name << "': Computation took " << elapsed.count() << " seconds.\n";
    }
}