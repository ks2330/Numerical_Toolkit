#include "app_support/solver_api.h"

#include <cmath>
#include <fstream>
#include <utility>

#include "mesh_generation/algorithm_delaunay_triangulation.h"
#include "nt/finite_volume_methods/FVM_mesh.h" 
#include "nt/finite_volume_methods/FVM_solver.h"  

#include "nt/finite_element_methods/FEM_Potential_Flow.h"
#include "app_support/app_FEM.h"

#include <algorithm>  
#include <cstdlib>     
#include "nt/solvers/potential_flow_solvers.h"       
#include "nt/solvers/eigen_sparse_potential_flow.h"  
#include "nt/Bench/bench.h"                            

namespace app_support {

    meshgeneration::Mesh buildAerofoilMesh(const FvmConfig& cfg) {
        meshgeneration::Mesh mesh;
        mesh.generateNACA4(cfg.naca.digits4, cfg.naca.nPoints);  
        mesh.buildAerofoilDomain(cfg.density);                    
        mesh.generateRandomNodes();                         

        meshgeneration::DelaunayTriangulation algo;
        mesh.triangulate(algo);
        mesh.deleteHoles();                
        mesh.enforceOutsideConstraints();   

        nt::fvm::repairMissingBoundaryEdges(mesh.elements, mesh.boundaryEdges);
        return mesh;
    }
    
    meshData getMeshData(const FvmConfig& cfg) {
        meshgeneration::Mesh mesh = buildAerofoilMesh(cfg);
        meshData data;
        data.nodes = std::move(mesh.nodes);
        data.elements = std::move(mesh.elements);
        return data;
    }


    FemResult runPotential(const FvmConfig& cfg, meshgeneration::Mesh mesh)
    {
        FemResult result;
        if (mesh.elements.empty())
            mesh = buildAerofoilMesh(cfg);
        const double alpha = cfg.alphaDeg * cfg.Pi / 180.0; 
        const double U_inf = 1.0;
        std::vector<double> phi = app_support::FEM::run::run_Potential_Flow(mesh, U_inf, alpha);
        auto velocity = nt::fem::computeVelocityField(mesh, phi);
        result.field  = nt::fem::computePressureCoefficients(velocity, U_inf);

        result.mesh = std::move(mesh);  
        return result;
    }


    FemResult runHeat(const HeatConfig& cfg) {
        FemResult result;

        meshgeneration::Mesh mesh;
        mesh.buildRectangleDomain(cfg.width, cfg.height, cfg.density);
        mesh.generateRandomNodes();
        meshgeneration::DelaunayTriangulation algo;
        mesh.triangulate(algo);

        std::vector<double> T = app_support::FEM::run::run_FEM_Heat_Equation(
            mesh, mesh.groupId("inlet"), cfg.T_inlet, mesh.groupId("outlet"), 0.0);

        result.field = std::move(T);
        result.mesh  = std::move(mesh);
        return result;
    }

    BenchmarkResult runBenchmark(std::vector<double> densities, int reps, int warmup) {
        BenchmarkResult result;
        const nt::solvers::DefaultPotentialFlowSolver     dense;   
        const nt::solvers::EigenSparsePotentialFlowSolver sparse;  
        const double U_inf = 1.0, alpha = 0.0;

        for (double d : densities) {
            auto makeMesh = [&]() {
                std::srand(42);
                FvmConfig cfg;
                cfg.density = d;
                return buildAerofoilMesh(cfg);
            };
            meshgeneration::Mesh refMesh = makeMesh();
            const int N = static_cast<int>(refMesh.nodes.size());
            const std::vector<double> phiRef = dense.solve(refMesh, U_inf, alpha);  

            std::vector<double> phiDense, phiSparse;
            nt::bench::BenchStats ds = nt::bench::timed_runs(
                makeMesh, [&](meshgeneration::Mesh& m){ return dense.solve(m, U_inf, alpha); },
                reps, warmup, &phiDense);
            nt::bench::BenchStats ss = nt::bench::timed_runs(
                makeMesh, [&](meshgeneration::Mesh& m){ return sparse.solve(m, U_inf, alpha); },
                reps, warmup, &phiSparse);

            double diff = 0.0;
            for (size_t i = 0; i < std::min(phiSparse.size(), phiRef.size()); ++i)
                diff = std::max(diff, std::abs(phiSparse[i] - phiRef[i]));

            result.numNodes.push_back(N);
            result.denseTimes.push_back(ds.median_s);
            result.sparseTimes.push_back(ss.median_s);
            result.speedups.push_back(ss.median_s > 0.0 ? ds.median_s / ss.median_s : 0.0);
            result.maxDiffs.push_back(diff);
        }
        return result;
    }

    FvmResult runFvm(const FvmConfig& cfg, ProgressCallback cb, meshgeneration::Mesh mesh) {
        FvmResult result;

        if (mesh.elements.empty())
            mesh = buildAerofoilMesh(cfg);

        nt::fvm::GasModel gm{cfg.gamma, cfg.R};
        const double alpha = cfg.alphaDeg * cfg.Pi / 180.0;
        const double a     = std::sqrt(gm.gamma * cfg.pInf / cfg.rhoInf);  
        const double V     = cfg.mach * a;
        const double qInf  = 0.5 * cfg.rhoInf * V * V;
        nt::fvm::ConservativeState Uinf =
            gm.toConservative({cfg.rhoInf, V * std::cos(alpha), V * std::sin(alpha), cfg.pInf});

        const int farGroup  = mesh.groupId("outer");
        const int wallGroup = mesh.groupId("aerofoil");

        nt::fvm::EulerSolver solver(mesh.elements, mesh.nodes, mesh.boundaryEdges,
                                    gm, Uinf, cfg.cfl, farGroup);
        for (int it = 0; it < cfg.maxIters; ++it) {
            double rn = solver.residualNorm();
            result.residualHistory.push_back(static_cast<float>(rn));
            if (rn < cfg.tolerance) break;
            if (cb && !cb(it, rn)) break;         
            solver.step();
        }


        const std::vector<nt::fvm::ConservativeState>& state = solver.getState();
        result.state = state;
        result.pressure.reserve(state.size());
        result.mach.reserve(state.size());
        for (const auto& s : state) {
            nt::fvm::PrimitiveState prim = gm.toPrimitive(s);
            double speed = std::sqrt(prim.u * prim.u + prim.v * prim.v);
            result.pressure.push_back(prim.p);
            result.mach.push_back(speed / gm.soundSpeed(prim));
        }

        result.forces = nt::fvm::computeForces(state, solver.getFaces(),
                                               gm, wallGroup, alpha, qInf, mesh.chord, cfg.pInf);
        result.watertight =
            nt::fvm::uncoveredBoundaryEdges(solver.getFaces(), mesh.boundaryEdges).empty();


        if (!cfg.pressureFieldCSV.empty()) {
            std::ofstream field(cfg.pressureFieldCSV);
            field << "x,y,pressure,mach\n";
            for (size_t i = 0; i < mesh.elements.size(); ++i) {
                const auto& e = mesh.elements[i];
                meshgeneration::Node ctr = nt::fvm::cellCentroid(
                    mesh.nodes[e.n0_id], mesh.nodes[e.n1_id], mesh.nodes[e.n2_id]);
                field << ctr.x << "," << ctr.y << ","
                      << result.pressure[i] << "," << result.mach[i] << "\n";
            }
        }
        if (!cfg.forcesCSV.empty()) {
            std::ofstream fcsv(cfg.forcesCSV);
            const nt::fvm::Forces& F = result.forces;
            fcsv << "fx,fy,lift,drag,cl,cd\n";
            fcsv << F.fx << "," << F.fy << "," << F.lift << ","
                 << F.drag << "," << F.cl << "," << F.cd << "\n";
        }

        result.mesh = std::move(mesh);  
        return result;
    }

}
