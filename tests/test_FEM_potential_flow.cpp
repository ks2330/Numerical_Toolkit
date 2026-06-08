#include <gtest/gtest.h>
#include <cmath>
#include "mesh_generation/mesh_generation.h"
#include "nt/finite_element_methods/FEM_Global_Stiffness_Matrix.h"
#include "nt/finite_element_methods/FEM_Potential_Flow.h"

using namespace meshgeneration;
using namespace nt::fem;

// Single right-angled triangle: nodes at (0,0), (1,0), (0,1)
// All three nodes are Boundary type so all get Dirichlet BCs
static Mesh make_triangle_mesh() {
    Mesh mesh;
    mesh.nodes = {
        {0.0, 0.0, 0, NodeType::Boundary, -1},
        {1.0, 0.0, 1, NodeType::Boundary, -1},
        {0.0, 1.0, 2, NodeType::Boundary, -1},
    };
    mesh.elements = {{0, 1, 2, 0}};
    return mesh;
}

// --- applyPotentialFlowBCs ---

TEST(PotentialFlowBCTest, DirichletRowsAreSet) {
    // With U_inf=2, alpha=0: phi = 2x
    // Node 0 (x=0) → phi=0,  Node 1 (x=1) → phi=2,  Node 2 (x=0) → phi=0
    auto mesh = make_triangle_mesh();
    int N = 3;
    auto K = assembleGlobalStiffnessMatrix(mesh);
    std::vector<double> rhs(N, 0.0);

    applyPotentialFlowBCs(mesh, K, rhs, 2.0, 0.0);

    // Each BC row: K[i][i]=1, rhs[i]=phi
    EXPECT_DOUBLE_EQ(K[0][0], 1.0);  EXPECT_DOUBLE_EQ(rhs[0], 0.0);
    EXPECT_DOUBLE_EQ(K[1][1], 1.0);  EXPECT_DOUBLE_EQ(rhs[1], 2.0);
    EXPECT_DOUBLE_EQ(K[2][2], 1.0);  EXPECT_DOUBLE_EQ(rhs[2], 0.0);
}

TEST(PotentialFlowBCTest, OnlyBoundaryNodesGetDirichletBC) {
    // Node 1 is Internal — should NOT get Dirichlet BC applied
    auto mesh = make_triangle_mesh();
    mesh.nodes[1].type = NodeType::Internal;

    int N = 3;
    auto K = assembleGlobalStiffnessMatrix(mesh);
    std::vector<double> rhs(N, 0.0);
    double K11_before = K[1][1];

    applyPotentialFlowBCs(mesh, K, rhs, 1.0, 0.0);

    // Node 1 (Internal) should not have its row set to identity
    EXPECT_NE(K[1][1], 1.0) << "Internal node had Dirichlet BC applied";
}

// --- Potential flow solve: linear solution reproduced exactly ---

TEST(PotentialFlowSolveTest, UniformHorizontalFlow) {
    // phi = U_inf * x is the exact solution; linear FEM reproduces it exactly
    auto mesh = make_triangle_mesh();
    int N = 3;
    auto K = assembleGlobalStiffnessMatrix(mesh);
    std::vector<double> rhs(N, 0.0);

    applyPotentialFlowBCs(mesh, K, rhs, 1.0, 0.0);
    auto phi = gaussianElimination(K, rhs);

    EXPECT_NEAR(phi[0], 0.0, 1e-10);  // x=0
    EXPECT_NEAR(phi[1], 1.0, 1e-10);  // x=1
    EXPECT_NEAR(phi[2], 0.0, 1e-10);  // x=0
}

TEST(PotentialFlowSolveTest, UniformVerticalFlow) {
    // alpha = pi/2: phi = U_inf * y
    auto mesh = make_triangle_mesh();
    int N = 3;
    auto K = assembleGlobalStiffnessMatrix(mesh);
    std::vector<double> rhs(N, 0.0);

    applyPotentialFlowBCs(mesh, K, rhs, 1.0, M_PI / 2.0);
    auto phi = gaussianElimination(K, rhs);

    EXPECT_NEAR(phi[0], 0.0, 1e-8);  // y=0
    EXPECT_NEAR(phi[1], 0.0, 1e-8);  // y=0
    EXPECT_NEAR(phi[2], 1.0, 1e-8);  // y=1
}

// --- Velocity field ---

TEST(VelocityFieldTest, UniformHorizontalFlowVelocity) {
    // phi = x → grad(phi) = (1, 0) everywhere
    auto mesh = make_triangle_mesh();
    int N = 3;
    auto K = assembleGlobalStiffnessMatrix(mesh);
    std::vector<double> rhs(N, 0.0);
    applyPotentialFlowBCs(mesh, K, rhs, 1.0, 0.0);
    auto phi = gaussianElimination(K, rhs);

    auto vel = computeVelocityField(mesh, phi);
    ASSERT_EQ(static_cast<int>(vel.size()), N);
    for (int i = 0; i < N; ++i) {
        EXPECT_NEAR(vel[i].u, 1.0, 1e-8) << "u wrong at node " << i;
        EXPECT_NEAR(vel[i].v, 0.0, 1e-8) << "v wrong at node " << i;
    }
}

// --- Pressure coefficient ---

TEST(PressureCoefficientTest, UniformFlowCpIsZero) {
    // Uniform flow: |v| = U_inf everywhere → Cp = 1 - 1 = 0
    auto mesh = make_triangle_mesh();
    int N = 3;
    auto K = assembleGlobalStiffnessMatrix(mesh);
    std::vector<double> rhs(N, 0.0);
    applyPotentialFlowBCs(mesh, K, rhs, 1.0, 0.0);
    auto phi = gaussianElimination(K, rhs);
    auto vel = computeVelocityField(mesh, phi);
    auto Cp  = computePressureCoefficients(vel, 1.0);

    ASSERT_EQ(static_cast<int>(Cp.size()), N);
    for (int i = 0; i < N; ++i)
        EXPECT_NEAR(Cp[i], 0.0, 1e-8) << "Cp wrong at node " << i;
}

TEST(PressureCoefficientTest, StagnationPointCpIsOne) {
    // A node with zero velocity → Cp = 1
    std::vector<Velocity> vel = {{0.0, 0.0}};
    auto Cp = computePressureCoefficients(vel, 2.0);
    EXPECT_NEAR(Cp[0], 1.0, 1e-10);
}

TEST(PressureCoefficientTest, DoubleSpeedCpIsMinusThree) {
    // |v| = 2 * U_inf → Cp = 1 - 4 = -3
    std::vector<Velocity> vel = {{2.0, 0.0}};
    auto Cp = computePressureCoefficients(vel, 1.0);
    EXPECT_NEAR(Cp[0], -3.0, 1e-10);
}
