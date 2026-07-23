#include <gtest/gtest.h>
#include "nt/finite_volume_methods/FVM_forces.h"
#include "nt/finite_volume_methods/FVM_mesh.h"
#include "nt/finite_volume_methods/FVM_gas_model.h"

using namespace nt::fvm;

static meshgeneration::Node FN(double x, double y, int id) { return {x, y, id}; }
static meshgeneration::Element FE(int a, int b, int c) { return {a, b, c}; }

TEST(ForcesTest, ClosedBodyUniformPressureZeroForce) {
    GasModel gm{1.4, 287.0};
    std::vector<meshgeneration::Node> nodes = { FN(0,0,0), FN(4,0,1), FN(0,3,2) };
    std::vector<meshgeneration::Element> elements = { FE(0,1,2) };
    std::vector<meshgeneration::Edge> wallEdges = { {0,1,-1,1}, {1,2,-1,1}, {2,0,-1,1} };
    auto faces = buildFaces(elements, nodes, wallEdges);

    ConservativeState U = gm.toConservative({1.0, 0.0, 0.0, 1.0});
    std::vector<ConservativeState> state(1, U);

    Forces F = computeForces(state, faces, gm, 1, 0.0, 0.5, 1.0);
    EXPECT_NEAR(F.fx,   0.0, 1e-10);
    EXPECT_NEAR(F.fy,   0.0, 1e-10);
    EXPECT_NEAR(F.lift, 0.0, 1e-10);
    EXPECT_NEAR(F.drag, 0.0, 1e-10);
}

TEST(ForcesTest, WallFaceForceAndGroupFilter) {
    GasModel gm{1.4, 287.0};
    ConservativeState s = gm.toConservative({1.0, 0.0, 0.0, 2.0});
    std::vector<ConservativeState> state(1, s);

    Face wall;
    wall.n1_id = 0; wall.n2_id = 1;
    wall.leftElement_id = 0; wall.rightElement_id = -1;
    wall.length = 3.0; wall.normal = {1.0, 0.0}; wall.bcType = 1;

    Face farField;
    farField.n1_id = 1; farField.n2_id = 2;
    farField.leftElement_id = 0; farField.rightElement_id = -1;
    farField.length = 5.0; farField.normal = {0.0, 1.0}; farField.bcType = 0;

    std::vector<Face> faces = { wall, farField };

    Forces F = computeForces(state, faces, gm, 1, 0.0, 0.5, 1.0);
    EXPECT_NEAR(F.fx,   6.0, 1e-10);
    EXPECT_NEAR(F.fy,   0.0, 1e-10);
    EXPECT_NEAR(F.drag, 6.0, 1e-10);
    EXPECT_NEAR(F.lift, 0.0, 1e-10);
}

TEST(ForcesTest, LiftDragResolveWithAngle) {
    GasModel gm{1.4, 287.0};
    ConservativeState s = gm.toConservative({1.0, 0.0, 0.0, 2.0});
    std::vector<ConservativeState> state(1, s);

    Face wall;
    wall.n1_id = 0; wall.n2_id = 1;
    wall.leftElement_id = 0; wall.rightElement_id = -1;
    wall.length = 1.0; wall.normal = {0.0, 1.0}; wall.bcType = 1;
    std::vector<Face> faces = { wall };

    double alpha = M_PI / 2.0;
    Forces F = computeForces(state, faces, gm, 1, alpha, 0.5, 1.0);
    EXPECT_NEAR(F.fx,    0.0, 1e-10);
    EXPECT_NEAR(F.fy,    2.0, 1e-10);
    EXPECT_NEAR(F.drag,  2.0, 1e-10);
    EXPECT_NEAR(F.lift,  0.0, 1e-10);
}

// Regression for the missing-surface-edge bug: when the wall loop is NOT closed, integrating
// absolute pressure leaks the (large) free-stream pressure into the force. Integrating gauge
// pressure (p - pInf) cancels that leak, so an open loop in uniform free-stream gives ~0 force.
TEST(ForcesTest, GaugePressureCancelsFreeStreamLeakOnOpenLoop) {
    GasModel gm{1.4, 287.0};
    // Two wall faces that do NOT close: sum(n*len) = (1,1) != 0  -> mimics a hole in the surface.
    Face a; a.n1_id = 0; a.n2_id = 1; a.leftElement_id = 0; a.rightElement_id = -1;
    a.length = 1.0; a.normal = {1.0, 0.0}; a.bcType = 1;
    Face b; b.n1_id = 1; b.n2_id = 2; b.leftElement_id = 0; b.rightElement_id = -1;
    b.length = 1.0; b.normal = {0.0, 1.0}; b.bcType = 1;
    std::vector<Face> faces = { a, b };

    ConservativeState U = gm.toConservative({1.0, 0.0, 0.0, 1.0});   // uniform field, p = 1.0
    std::vector<ConservativeState> state(1, U);

    // Absolute pressure (pInf = 0): the open loop leaks p_inf*sum(n*len) = (1,1) into the force.
    Forces absP = computeForces(state, faces, gm, 1, 0.0, 0.5, 1.0, 0.0);
    EXPECT_GT(std::abs(absP.fx) + std::abs(absP.fy), 0.5);

    // Gauge pressure (pInf = 1.0 = the uniform field): the leak cancels, force is ~0.
    Forces gaugeP = computeForces(state, faces, gm, 1, 0.0, 0.5, 1.0, 1.0);
    EXPECT_NEAR(gaugeP.fx, 0.0, 1e-12);
    EXPECT_NEAR(gaugeP.fy, 0.0, 1e-12);
}
