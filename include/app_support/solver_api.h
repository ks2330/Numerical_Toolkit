#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// solver_api — the in-memory, CSV-free entry point the pybind11 module wraps.

// ─────────────────────────────────────────────────────────────────────────────
#include <string>
#include <vector>
#include <functional>

#include "mesh_generation/mesh_generation.h"
#include "nt/finite_volume_methods/FVM_gas_model.h"  
#include "nt/finite_volume_methods/FVM_forces.h"      

namespace app_support {

    // ── FVM (compressible Euler airfoil) ────────────────────────────────────
    struct NacaSpec {
        int digits4  = 12;    
        int nPoints  = 160;   
    };

    struct meshData {
        std::vector<meshgeneration::Node> nodes;
        std::vector<meshgeneration::Element> elements;
    };

    struct FvmConfig {
        // Geometry: 
        NacaSpec naca;                                        
        double   density = 300.0;                             

        // Free stream / gas
        double mach = 0.3, alphaDeg = 2.0, rhoInf = 1.0, pInf = 1.0;
        double gamma = 1.4, R = 287.0;

        // Numerics
        double cfl = 0.5, tolerance = 1e-6;
        int    maxIters = 100000;

        // Output CSVs (for regression / debugging; GUI uses in-memory data)
        std::string pressureFieldCSV = "results/csv/fvm_pressure_field.csv";
        std::string forcesCSV        = "results/csv/fvm_forces.csv";
        double Pi = 3.14159265358979323846;
    };

    struct FvmResult {
        meshgeneration::Mesh                    mesh;             
        std::vector<nt::fvm::ConservativeState> state;            
        std::vector<double>                     pressure;         
        std::vector<double>                     mach;             
        std::vector<float>                      residualHistory;  
        nt::fvm::Forces                         forces{};        
        bool                                    watertight = true;
    };

    struct FemResult {
        meshgeneration::Mesh mesh;
        std::vector<double>  field; 
    };


    using ProgressCallback = std::function<bool(int iter, double residual)>;


    meshgeneration::Mesh buildAerofoilMesh(const FvmConfig& cfg);

    FvmResult runFvm(const FvmConfig& cfg, ProgressCallback cb = {}, meshgeneration::Mesh mesh = {});

    // ── FEM potential-flow airfoil ──────────────────────────────────────────
    FemResult runPotential(const FvmConfig& cfg, meshgeneration::Mesh mesh = {});


    struct HeatConfig {
        double width    = 4.0;
        double height   = 2.0;
        double T_inlet  = 100.0;  
        double density  = 10.0;    
    };

    FemResult runHeat(const HeatConfig& cfg);

    // ── Solver-efficiency benchmark (dense Gaussian vs Eigen sparse LDLT) ─────
    struct BenchmarkResult {
        std::vector<int>    numNodes;    
        std::vector<double> denseTimes;   
        std::vector<double> sparseTimes;   
        std::vector<double> speedups;      
        std::vector<double> maxDiffs;      
    };

    BenchmarkResult runBenchmark(std::vector<double> densities = {5.0, 10.0, 20.0},
                                 int reps = 3, int warmup = 1);   

}
