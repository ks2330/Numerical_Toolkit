#pragma once
#include "FEM_Global_Stiffness_Matrix.h"
#include "mesh_generation/mesh_generation.h"

namespace nt::fem
{
    // Applies Dirichlet temperature BCs for the steady-state heat equation.
    // inletGroup/outletGroup match mesh.groupId("name") — avoids hardcoded coordinates.
    inline void applyHeatEquationBCs(
        const meshgeneration::Mesh& mesh,
        std::vector<std::vector<double>>& K,
        std::vector<double>& rhs,
        int inletGroup, double T_inlet,
        int outletGroup, double T_outlet)
    {
        if (inletGroup == -1 || outletGroup == -1) {
            std::cerr << "applyHeatEquationBCs: group not found — register 'inlet' and 'outlet' groups on the mesh\n";
            return;
        }
        for (int i = 0; i < static_cast<int>(mesh.nodes.size()); ++i) {
            if (mesh.nodes[i].group_id == inletGroup)
                applyDirichletBC(K, rhs, i, T_inlet);
            else if (mesh.nodes[i].group_id == outletGroup)
                applyDirichletBC(K, rhs, i, T_outlet);
        }
    }
}
