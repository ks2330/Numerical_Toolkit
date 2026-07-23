#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>

#include "mesh_generation/mesh_generation.h"
#include "mesh_generation/algorithm_delaunay_triangulation.h"
#include "app_support/app_FEM.h"
#include "nt/finite_volume_methods/FVM_solver.h"
#include "nt/finite_volume_methods/FVM_forces.h"

struct Config {
    std::string aerofoilDAT;
    std::string pressureFieldCSV;
    std::string forcesCSV;
    double density;
    double mach;
    double alphaDeg;
    double rhoInf;
    double pInf;
    double cfl;
    double tolerance;
    int    maxIters;
};

Config config = {
    .aerofoilDAT      = "results/dat/aerofoil.dat",
    .pressureFieldCSV = "results/csv/fvm_pressure_field.csv",
    .forcesCSV        = "results/csv/fvm_forces.csv",
    .density          = 300.0,
    .mach             = 0.3,
    .alphaDeg         = 2.0,
    .rhoInf           = 1.0,
    .pInf             = 1.0,
    .cfl              = 0.5,
    .tolerance        = 1e-6,
    .maxIters         = 100000,
};

int main() {
    try {
        meshgeneration::Mesh mesh = app_support::FEM::run::initialise_from_CSV(config.aerofoilDAT, config.density);
        meshgeneration::DelaunayTriangulation algo;
        mesh.triangulate(algo);
        mesh.deleteHoles();
        mesh.enforceOutsideConstraints();
        // Close any tagged boundary edge the unconstrained Delaunay failed to create (1-triangle
        // notches), so the wall loop is watertight before the solver builds faces.
        int patched = nt::fvm::repairMissingBoundaryEdges(mesh.elements, mesh.boundaryEdges);
        if (patched > 0)
            std::cout << "Patched " << patched << " missing boundary edge(s) in the mesh.\n";

        std::ofstream meshNodes("results/csv/fvm_nodes.csv");
        meshNodes << "x,y\n";
        for (const auto& n : mesh.nodes) meshNodes << n.x << "," << n.y << "\n";
        meshNodes.close();

        std::ofstream meshElems("results/csv/fvm_elements.csv");
        meshElems << "n0,n1,n2\n";
        for (const auto& e : mesh.elements)
            meshElems << e.n0_id << "," << e.n1_id << "," << e.n2_id << "\n";
        meshElems.close();

        nt::fvm::GasModel gm{1.4, 287.0};

        constexpr double PI = 3.14159265358979323846;
        const double alpha = config.alphaDeg * PI / 180.0;
        const double c     = std::sqrt(gm.gamma * config.pInf / config.rhoInf);
        const double V     = config.mach * c;
        const double qInf  = 0.5 * config.rhoInf * V * V;

        nt::fvm::ConservativeState Uinf =
            gm.toConservative({config.rhoInf, V * std::cos(alpha), V * std::sin(alpha), config.pInf});

        const int farGroup  = mesh.groupId("outer");
        const int wallGroup = mesh.groupId("aerofoil");

        nt::fvm::EulerSolver solver(mesh.elements, mesh.nodes, mesh.boundaryEdges,
                                    gm, Uinf, config.cfl, farGroup);
        solver.solve(config.tolerance, config.maxIters);

        nt::fvm::Forces F = nt::fvm::computeForces(solver.getState(), solver.getFaces(),
                                                   gm, wallGroup, alpha, qInf, mesh.chord, config.pInf);

        std::cout << "Final residual norm: " << solver.residualNorm() << "\n";
        std::cout << "C_L = " << F.cl << "   C_D = " << F.cd << "\n";

        // Safety net: repair closes 1-triangle notches; warn if any tagged wall edge is still open
        // (e.g. a larger hole repair can't fill), since that silently corrupts the force integral.
        auto uncovered = nt::fvm::uncoveredBoundaryEdges(solver.getFaces(), mesh.boundaryEdges);
        if (!uncovered.empty())
            std::cerr << "WARNING: surface NOT watertight -- " << uncovered.size()
                      << " tagged boundary edge(s) missing from the wall loop\n";

        const auto& state = solver.getState();

        std::ofstream field(config.pressureFieldCSV);
        field << "x,y,pressure,mach\n";
        for (size_t i = 0; i < mesh.elements.size(); ++i) {
            const auto& e = mesh.elements[i];
            meshgeneration::Node ctr = nt::fvm::cellCentroid(mesh.nodes[e.n0_id],
                                                             mesh.nodes[e.n1_id],
                                                             mesh.nodes[e.n2_id]);
            nt::fvm::PrimitiveState prim = gm.toPrimitive(state[i]);
            double speed = std::sqrt(prim.u * prim.u + prim.v * prim.v);
            double localMach = speed / gm.soundSpeed(prim);
            field << ctr.x << "," << ctr.y << "," << prim.p << "," << localMach << "\n";
        }
        field.close();

        std::ofstream fcsv(config.forcesCSV);
        fcsv << "fx,fy,lift,drag,cl,cd\n";
        fcsv << F.fx << "," << F.fy << "," << F.lift << "," << F.drag << "," << F.cl << "," << F.cd << "\n";
        fcsv.close();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
