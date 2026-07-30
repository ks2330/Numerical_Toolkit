#pragma once
#include <algorithm>
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

    struct BenchStats {
        double median_s = 0.0;
        double min_s    = 0.0;
        int    reps     = 0;
    };

    // Rigorous timing: run `warmup` untimed passes then `reps` timed passes, each on a
    // FRESH object from `setup()` (the potential-flow solvers mutate the mesh in place,
    // so every run must start from an identical, un-mutated mesh). Reports median and
    // min wall-clock; a single std::chrono sample is far too noisy to trust. The last
    // solution vector is returned via `out_phi` for a downstream correctness check.
    //   setup: () -> meshgeneration::Mesh   (must be deterministic to be comparable)
    //   solve: (meshgeneration::Mesh&) -> std::vector<double>
    template <typename SetupFn, typename SolveFn>
    inline BenchStats timed_runs(SetupFn setup, SolveFn solve, int reps, int warmup,
                                 std::vector<double>* out_phi = nullptr) {
        for (int w = 0; w < warmup; ++w) {
            auto mesh = setup();
            auto phi = solve(mesh);
            (void)phi;   // side effects (allocation + mesh mutation) prevent elision
        }

        std::vector<double> times;
        times.reserve(reps > 0 ? reps : 1);
        std::vector<double> phi;
        for (int r = 0; r < reps; ++r) {
            auto mesh = setup();
            auto t0 = std::chrono::high_resolution_clock::now();
            phi = solve(mesh);
            auto t1 = std::chrono::high_resolution_clock::now();
            times.push_back(std::chrono::duration<double>(t1 - t0).count());
        }
        if (out_phi) *out_phi = phi;

        BenchStats stats;
        stats.reps = reps;
        if (!times.empty()) {
            std::sort(times.begin(), times.end());
            stats.min_s    = times.front();
            stats.median_s = times[times.size() / 2];
        }
        return stats;
    }
}