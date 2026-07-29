#include <iostream>
#include <fstream>
#include <utility>

#include "mesh_generation/mesh_generation.h"
#include "app_support/solver_api.h"

struct Config {
    int    naca;
    int    nacaPoints;
    std::string pressureFieldCSV;
    std::string forcesCSV;
    double density;
    double mach;
    double alphaDeg;
    double rhoInf;
    double pInf;
    double cfl;
    double gamma;
    double R;
    double tolerance;
    int    maxIters;
    double Pi;
};

Config config = {
    .naca             = 2412,
    .nacaPoints       = 160,
    .pressureFieldCSV = "results/csv/fvm_pressure_field.csv",
    .forcesCSV        = "results/csv/fvm_forces.csv",
    .density          = 300.0,
    .mach             = 0.3,
    .alphaDeg         = 2.0,
    .rhoInf           = 1.0,
    .pInf             = 1.0,
    .cfl              = 0.5,
    .gamma            = 1.4,
    .R                = 287.0,
    .tolerance        = 1e-6,
    .maxIters         = 100000,
    .Pi               = 3.14159265358979323846
};

int main() {
    try {
        app_support::FvmConfig cfg;

        meshgeneration::Mesh mesh = app_support::buildAerofoilMesh(cfg);

        app_support::FvmResult result = app_support::runFvm(cfg,
            [](int iter, double residual) {
                if (iter % 1000 == 0)
                    std::cout << "Iteration " << iter << ", residual = " << residual << "\n";
                return true;  
            },
            std::move(mesh));

        if (!result.residualHistory.empty())
            std::cout << "Final residual norm: " << result.residualHistory.back() << "\n";
        std::cout << "C_L = " << result.forces.cl << "   C_D = " << result.forces.cd << "\n";
        if (!result.watertight)
            std::cerr << "WARNING: aerofoil surface NOT watertight\n";

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
