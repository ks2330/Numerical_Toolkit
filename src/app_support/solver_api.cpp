#include "app_support/solver_api.h"

#include <cmath>
#include <fstream>
#include <utility>

#include "mesh_generation/algorithm_delaunay_triangulation.h"
#include "nt/finite_volume_methods/FVM_mesh.h" 
#include "nt/finite_volume_methods/FVM_solver.h"  

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
