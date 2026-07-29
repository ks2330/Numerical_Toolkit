#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// solver_api — the in-memory, CSV-free entry point the pybind11 module wraps.
//
// Goal: ONE library function per solver that returns everything the GUI needs as
// plain data (mesh arrays, per-cell fields, forces, residual history), so the
// Python front-end never shells out or parses CSV. The existing CLIs
// (apps/FVM_solver, apps/UI) can be refactored onto these too, keeping their CSV
// output as a regression anchor.
//
// SCAFFOLD: the structs/signatures are the contract; the bodies in solver_api.cpp
// are stubs for you to fill (they throw until implemented).
// ─────────────────────────────────────────────────────────────────────────────
#include <string>
#include <vector>
#include <functional>

#include "mesh_generation/mesh_generation.h"
#include "nt/finite_volume_methods/FVM_gas_model.h"   // nt::fvm::ConservativeState
#include "nt/finite_volume_methods/FVM_forces.h"       // nt::fvm::Forces

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
        // Geometry: aerofoil generated in memory from a NACA 4-digit code (no file I/O).
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

    using ProgressCallback = std::function<bool(int iter, double residual)>;


    meshgeneration::Mesh buildAerofoilMesh(const FvmConfig& cfg);

    FvmResult runFvm(const FvmConfig& cfg, ProgressCallback cb = {}, meshgeneration::Mesh mesh = {});

    // ── FEM potential-flow airfoil — scaffold later  ─────────────────────
    // struct PotentialResult { meshgeneration::Mesh mesh; std::vector<double> cp; };
    // PotentialResult runPotential(const FvmConfig& cfg);

    // ── FEM 2-D steady heat — scaffold later  ────────────────────────────
    // struct HeatConfig { /* geometry + T_inlet/T_outlet + density */ };
    // struct HeatResult { meshgeneration::Mesh mesh; std::vector<double> temperature; };
    // HeatResult runHeat(const HeatConfig& cfg);

}
