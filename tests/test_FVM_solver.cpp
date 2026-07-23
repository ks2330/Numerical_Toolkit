#include <gtest/gtest.h>
#include "nt/finite_volume_methods/FVM_solver.h"
#include "nt/finite_volume_methods/FVM_flux.h"
#include "nt/finite_volume_methods/FVM_gas_model.h"
#include "nt/finite_volume_methods/FVM_mesh.h"

const nt::fvm::GasModel gm{1.4, 287.0}; //air

static meshgeneration::Node EN(double x, double y, int id) {
    return {x, y, id};
}


static meshgeneration::Element E(int n1, int n2, int n3) {
    return {n1, n2, n3};
}

const std::vector<meshgeneration::Node> nodes =
{
    EN(0.0, 0.0, 0), EN(1.0, 0.0, 1), EN(0.5, 0.866, 2), EN(0.5, -0.866, 3) , EN(1.5, 0.866, 4), EN(-0.5, 0.866, 5)
};

const std::vector<meshgeneration::Edge> edges = {
    {0,3,-1,9}, {1,3,-1,9}, {1,4,-1,9}, {2,4,-1,9}, {2,5,-1,9}, {0,5,-1,9}
};

const std::vector<meshgeneration::Element> elements = {
    E(0, 1, 2), E(0, 1, 3), E(1, 2, 4), E(2, 0, 5)
};

nt::fvm::ConservativeState Uinf = gm.toConservative({1.0, 0.5, 0.2, 1.5});
nt::fvm::EulerSolver solver(elements, nodes, edges, gm, Uinf, 0.5, 9);

// --- Solver Tests ---


TEST(EulerSolverTest, ConstructorInitialisesStateToFreeStream) {
    const auto& state = solver.getState();
    ASSERT_EQ(state.size(), elements.size());          
    for (size_t i = 0; i < state.size(); ++i) {
        EXPECT_NEAR(state[i].rho,   Uinf.rho,   1e-12) << "cell " << i;
        EXPECT_NEAR(state[i].rho_u, Uinf.rho_u, 1e-12) << "cell " << i;
        EXPECT_NEAR(state[i].rho_v, Uinf.rho_v, 1e-12) << "cell " << i;
        EXPECT_NEAR(state[i].rho_e, Uinf.rho_e, 1e-12) << "cell " << i;
    }
}

TEST(EulerSolverTest, ConstructorComputesCellVolumes) {
    const auto& vols = solver.getVolumes();
    ASSERT_EQ(vols.size(), elements.size());
    EXPECT_NEAR(vols[0], 0.433, 1e-12);
    for (double v : vols) EXPECT_GT(v, 0.0);
}

TEST(EulerSolverTest, AssertUniformFreeStreamZeroResidual){
    solver.step();
    auto& state = solver.getState();

    ASSERT_EQ(state.size(), elements.size());    
    for (size_t i = 0; i < state.size(); ++i) {
        EXPECT_NEAR(state[i].rho,   Uinf.rho,   1e-12) << "cell " << i;
        EXPECT_NEAR(state[i].rho_u, Uinf.rho_u, 1e-12) << "cell " << i;
        EXPECT_NEAR(state[i].rho_v, Uinf.rho_v, 1e-12) << "cell " << i;
        EXPECT_NEAR(state[i].rho_e, Uinf.rho_e, 1e-12) << "cell " << i;
    }
}

TEST(EulerSolverTest, ResidualNormZeroForFreeStream){
    double res_sum = solver.residualNorm();
    EXPECT_NEAR(res_sum,   0,   1e-12);
    
}

TEST(EulerSolverTest, PerturbationDecays) {
    nt::fvm::ConservativeState Uinf = gm.toConservative({1.0, 0.5, 0.2, 1.5});
    nt::fvm::EulerSolver solver(elements, nodes, edges, gm, Uinf, 0.5, 9);


    std::vector<nt::fvm::ConservativeState> s0 = solver.getState();   // COPY (by value, no &)
    s0[0].rho   *= 1.2;
    s0[0].rho_e *= 1.2;
    solver.setState(s0);

    double res_0 = solver.residualNorm();
    solver.solve(1e-8, 100000);
    double res_n = solver.residualNorm();

    EXPECT_LT(res_n, res_0);
}
