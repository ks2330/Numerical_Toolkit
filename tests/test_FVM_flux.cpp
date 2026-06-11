#include <gtest/gtest.h>
#include "nt/finite_volume_methods/FVM_flux.h"
#include "nt/finite_volume_methods/FVM_gas_model.h"
#include "nt/finite_volume_methods/FVM_mesh.h"

// re tests for my sanity but for Gas at rest
TEST(GasModelStateTestforFlux, PrimToConsMatchesAnalytic_rest) {
    nt::fvm::GasModel gm{1.4, 287.0};
    nt::fvm::PrimitiveState ps{1.0, 0.0, 0.0, 1.0};
    nt::fvm::ConservativeState s = gm.toConservative(ps);
    EXPECT_NEAR(s.rho,   1.0,   1e-12);
    EXPECT_NEAR(s.rho_u, 0.0,   1e-12);
    EXPECT_NEAR(s.rho_v, 0.0,   1e-12);
    EXPECT_NEAR(s.rho_e, 2.5,   1e-12);
}

// re tests for my sanity but for Gas at Horizontal flow
TEST(GasModelStateTestforFlux, PrimToConsMatchesAnalytic_horizontal) {
    nt::fvm::GasModel gm{1.4, 287.0};
    nt::fvm::PrimitiveState ps{1.0, 1.0, 0.0, 1.0};
    nt::fvm::ConservativeState s = gm.toConservative(ps);
    EXPECT_NEAR(s.rho,   1.0,   1e-12);
    EXPECT_NEAR(s.rho_u, 1.0,   1e-12);
    EXPECT_NEAR(s.rho_v, 0.0,   1e-12);
    EXPECT_NEAR(s.rho_e, 3.0,   1e-12);
}

TEST(FluxVectorTest, FluxConstruction_Rest) {
    nt::fvm::GasModel gm{1.4, 287.0};
    nt::fvm::PrimitiveState ps{1.0, 0.0, 0.0, 1.0};
    nt::fvm::ConservativeState s = gm.toConservative(ps);
    nt::fvm::Vec2 normal{1.0, 0.0};
    nt::fvm::FluxVector flux = computeFlux(s, normal, gm);
    EXPECT_NEAR(flux.mass,   0.0,   1e-12);
    EXPECT_NEAR(flux.x_mom, 1.0,   1e-12);
    EXPECT_NEAR(flux.y_mom, 0.0,   1e-12);
    EXPECT_NEAR(flux.energy, 0,   1e-12);
}

const nt::fvm::GasModel gm{1.4, 287.0}; //air

TEST(FluxVectorTest, FluxConstruction_Horizontal) {
    nt::fvm::GasModel gm{1.4, 287.0};
    nt::fvm::PrimitiveState ps{1.0, 1.0, 0.0, 1.0};
    nt::fvm::ConservativeState s = gm.toConservative(ps);
    nt::fvm::Vec2 normal{1.0, 0.0};
    nt::fvm::FluxVector flux = computeFlux(s, normal, gm);
    EXPECT_NEAR(flux.mass,   1.0,   1e-12);
    EXPECT_NEAR(flux.x_mom, 2.0,   1e-12);
    EXPECT_NEAR(flux.y_mom, 0.0,   1e-12);
    EXPECT_NEAR(flux.energy, 4.0,   1e-12);
}

TEST(FluxVectorTest, FluxConstruction_Angled) {
    nt::fvm::GasModel gm{1.4, 287.0};
    nt::fvm::PrimitiveState ps{1.0, 1.0, 0.0, 1.0};
    nt::fvm::ConservativeState s = gm.toConservative(ps);
    nt::fvm::Vec2 normal{0.0, 1.0};
    nt::fvm::FluxVector flux = computeFlux(s, normal, gm);
    EXPECT_NEAR(flux.mass,   0.0,   1e-12);
    EXPECT_NEAR(flux.x_mom, 0.0,   1e-12);
    EXPECT_NEAR(flux.y_mom, 1.0,   1e-12);
    EXPECT_NEAR(flux.energy, 0,   1e-12);
}



// --- Rusanov flux test ---

TEST(RusanovFluxTest, CentralAverage_Same) {
    nt::fvm::GasModel gm{1.4, 287.0};
    nt::fvm::PrimitiveState ps{1.0, 1.0, 0.0, 1.0};
    nt::fvm::ConservativeState s = gm.toConservative(ps);
    nt::fvm::Vec2 normal{1.0, 0.0};

    // Rusanov flux with identical states should reduce to the physical flux.
    nt::fvm::FluxVector flux_Left = computeFlux(s, normal, gm);
    nt::fvm::FluxVector flux_Right = computeFlux(s, normal, gm);
    nt::fvm::FluxVector rusanov_flux = CentralAverage(flux_Left, flux_Right);
    EXPECT_NEAR(rusanov_flux.mass,   1.0,   1e-12);
    EXPECT_NEAR(rusanov_flux.x_mom, 2.0,   1e-12);
    EXPECT_NEAR(rusanov_flux.y_mom, 0.0,   1e-12);
    EXPECT_NEAR(rusanov_flux.energy, 4.0,   1e-12);
}

TEST(RusanovFluxTest, CentralAverage_Different) {
    nt::fvm::GasModel gm{1.4, 287.0};

    nt::fvm::PrimitiveState ps_left{1.0, 1.0, 0.0, 1.0};
    nt::fvm::ConservativeState s_left = gm.toConservative(ps_left);
    nt::fvm::Vec2 normal_left{1.0, 0.0};

    nt::fvm::PrimitiveState ps_right{1.0, 1.0, 0.0, 1.0};
    nt::fvm::ConservativeState s_right = gm.toConservative(ps_right);
    nt::fvm::Vec2 normal_right{0.0, 1.0};

    // Rusanov flux with identical states should reduce to the physical flux.
    nt::fvm::FluxVector flux_Left = computeFlux(s_left, normal_left, gm);
    nt::fvm::FluxVector flux_Right = computeFlux(s_right, normal_right, gm);
    nt::fvm::FluxVector rusanov_flux = CentralAverage(flux_Left, flux_Right);
    EXPECT_NEAR(rusanov_flux.mass,   0.5,   1e-12);
    EXPECT_NEAR(rusanov_flux.x_mom, 1.0,   1e-12);
    EXPECT_NEAR(rusanov_flux.y_mom, 0.5,   1e-12);
    EXPECT_NEAR(rusanov_flux.energy, 2.0,   1e-12);
}

TEST(RusanovFluxTest, RusanovDissipation_MaxWaveSpeed_Left) {
    nt::fvm::GasModel gm{1.4, 287.0};

    nt::fvm::PrimitiveState psLeft{1.0, 2.0, 0.0, 1.4};
    nt::fvm::ConservativeState sLeft = gm.toConservative(psLeft);

    nt::fvm::PrimitiveState psRight{1.0, 1.0, 0.0, 1.4};
    nt::fvm::ConservativeState sRight = gm.toConservative(psRight);

    double s_max = maxWaveSpeed(sLeft, sRight, {1.0, 0.0}, gm);
    EXPECT_NEAR(s_max, 3.4, 1e-10);
}

TEST(RusanovFluxTest, RusanovDissipation_MaxWaveSpeed_Right) {
    nt::fvm::GasModel gm{1.4, 287.0};

    nt::fvm::PrimitiveState psLeft{1.0, 2.0, 0.0, 1.4};
    nt::fvm::ConservativeState sLeft = gm.toConservative(psLeft);

    nt::fvm::PrimitiveState psRight{1.0, -3.0, 0.0, 1.4};
    nt::fvm::ConservativeState sRight = gm.toConservative(psRight);

    double s_max = maxWaveSpeed(sLeft, sRight, {1.0, 0.0}, gm);
    EXPECT_NEAR(s_max, 4.4, 1e-10);
}

TEST(RusanovFluxTest, RusanovDissipation_SameState) {

    nt::fvm::GasModel gm{1.4, 287.0};

    nt::fvm::PrimitiveState psLeft{1.0, 1.0, 0.0, 1.0};
    nt::fvm::ConservativeState sLeft = gm.toConservative(psLeft);

    nt::fvm::PrimitiveState psRight{1.0, 1.0, 0.0, 1.0};
    nt::fvm::ConservativeState sRight = gm.toConservative(psRight);

    double s_max = maxWaveSpeed(sLeft, sRight, {1.0, 0.0}, gm);

    nt::fvm::ConservativeState Dissipation = calculateDissipation(s_max, sRight, sLeft);

    EXPECT_NEAR(Dissipation.rho, 0.0, 1e-10);
    EXPECT_NEAR(Dissipation.rho_u, 0.0, 1e-10);
    EXPECT_NEAR(Dissipation.rho_v, 0.0, 1e-10);
    EXPECT_NEAR(Dissipation.rho_e, 0.0, 1e-10);

}

TEST(RusanovFluxTest, RusanovDissipation_DifferentPressure) {

    nt::fvm::GasModel gm{1.4, 287.0};

    nt::fvm::PrimitiveState psLeft{1.0, 2.0, 0.0, 1.4};
    nt::fvm::ConservativeState sLeft = gm.toConservative(psLeft);

    nt::fvm::PrimitiveState psRight{1.0, 1.0, 0.0, 1.4};
    nt::fvm::ConservativeState sRight = gm.toConservative(psRight);

    double s_max = maxWaveSpeed(sLeft, sRight, {1.0, 0.0}, gm);

    nt::fvm::ConservativeState Dissipation = calculateDissipation(s_max, sRight, sLeft);

    EXPECT_NEAR(Dissipation.rho, 0.0, 1e-10);
    EXPECT_NEAR(Dissipation.rho_u, -1.7, 1e-10);
    EXPECT_NEAR(Dissipation.rho_v, 0.0, 1e-10);
    EXPECT_NEAR(Dissipation.rho_e, -2.55, 1e-10);

}

TEST(RusanovFluxTest, RusanovFinalCalc_Different) {

    nt::fvm::GasModel gm{1.4, 287.0};

    nt::fvm::PrimitiveState psLeft{1.0, 2.0, 0.0, 1.4};
    nt::fvm::ConservativeState sLeft = gm.toConservative(psLeft);

    nt::fvm::PrimitiveState psRight{1.0, 1.0, 0.0, 1.4};
    nt::fvm::ConservativeState sRight = gm.toConservative(psRight);

    nt::fvm::FluxVector Solved_Flux = RusanovFlux(sLeft, sRight, {1.0, 0.0}, gm);

    EXPECT_NEAR(Solved_Flux.mass,   1.5,   1e-10);
    EXPECT_NEAR(Solved_Flux.x_mom,  5.6,   1e-10);
    EXPECT_NEAR(Solved_Flux.y_mom,  0.0,   1e-10);
    EXPECT_NEAR(Solved_Flux.energy, 12.15, 1e-10);

}

TEST(RusanovFluxTest, Consistency_EqualStatesEqualsPhysicalFlux) {
    nt::fvm::GasModel gm{1.4, 287.0};
    nt::fvm::ConservativeState s = gm.toConservative({1.0, 2.0, 1.0, 1.4});
    nt::fvm::Vec2 n{0.6, 0.8};

    nt::fvm::FluxVector rus  = RusanovFlux(s, s, n, gm);
    nt::fvm::FluxVector phys = computeFlux(s, n, gm);

    EXPECT_NEAR(rus.mass,   phys.mass,   1e-10);
    EXPECT_NEAR(rus.x_mom,  phys.x_mom,  1e-10);
    EXPECT_NEAR(rus.y_mom,  phys.y_mom,  1e-10);
    EXPECT_NEAR(rus.energy, phys.energy, 1e-10);
}

TEST(RusanovFluxTest, Antisymmetry_FaceFlip) {
    nt::fvm::GasModel gm{1.4, 287.0};
    nt::fvm::ConservativeState L = gm.toConservative({1.0, 2.0,  1.0, 1.4});
    nt::fvm::ConservativeState R = gm.toConservative({1.2, 0.5, -0.5, 2.0});
    nt::fvm::Vec2 n{0.6, 0.8};
    nt::fvm::Vec2 nFlip{-0.6, -0.8};

    nt::fvm::FluxVector f    = RusanovFlux(L, R, n, gm);
    nt::fvm::FluxVector fRev = RusanovFlux(R, L, nFlip, gm);

    EXPECT_NEAR(f.mass,   -fRev.mass,   1e-10);
    EXPECT_NEAR(f.x_mom,  -fRev.x_mom,  1e-10);
    EXPECT_NEAR(f.y_mom,  -fRev.y_mom,  1e-10);
    EXPECT_NEAR(f.energy, -fRev.energy, 1e-10);
}


// --- Computing Residual test ---

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
const std::vector<meshgeneration::Element> elements = {
    E(0, 1, 2), E(0, 1, 3), E(1, 2, 4), E(2, 0, 5)
};

const std::vector<nt::fvm::Face> faces = nt::fvm::buildFaces(elements, nodes);

TEST(ResidualTest, FreeStreamTest) {

    nt::fvm::GasModel gm{1.4, 287.0};
    nt::fvm::ConservativeState U = gm.toConservative({1.0, 1.0, 0.0, 1.0});
    std::vector<nt::fvm::ConservativeState> state(4, U);

    auto residual = computeResidual(state, faces, gm);
    EXPECT_EQ(residual.size(), state.size());
    EXPECT_NEAR(residual[0].rho,   0.0, 1e-10);
    EXPECT_NEAR(residual[0].rho_u, 0.0, 1e-10);
    EXPECT_NEAR(residual[0].rho_v, 0.0, 1e-10);
    EXPECT_NEAR(residual[0].rho_e, 0.0, 1e-10);

}

TEST(ResidualTest, NonUniformStateTest) {

    nt::fvm::GasModel gm{1.4, 287.0};
    std::vector<nt::fvm::ConservativeState> state;
    state.push_back(gm.toConservative({1.0,  1.0,  0.0, 1.0}));
    state.push_back(gm.toConservative({1.2,  0.5,  0.2, 1.5}));
    state.push_back(gm.toConservative({0.8, -0.3,  0.1, 1.2}));
    state.push_back(gm.toConservative({1.1,  0.0, -0.4, 2.0}));


    auto residual = computeResidual(state, faces, gm);

    nt::fvm::ConservativeState total{0.0, 0.0, 0.0, 0.0};
    for (int i = 0; i < residual.size(); ++i){
        total = total + residual[i];
    }

    EXPECT_EQ(residual.size(), state.size());
    EXPECT_NEAR(total.rho,   0.0, 1e-10);
    EXPECT_NEAR(total.rho_u, 0.0, 1e-10);
    EXPECT_NEAR(total.rho_v, 0.0, 1e-10);
    EXPECT_NEAR(total.rho_e, 0.0, 1e-10);

}

// Boundary faces inherit the group_id of the mesh edge they coincide with; interior faces stay -1.
TEST(FaceTaggingTest, BoundaryFacesInheritGroupId) {
    // the 6 outer edges of the fixture, all tagged with group 7
    const std::vector<meshgeneration::Edge> boundaryEdges = {
        {0,3,-1,7}, {1,3,-1,7}, {1,4,-1,7}, {2,4,-1,7}, {2,5,-1,7}, {0,5,-1,7}
    };
    const auto taggedFaces = nt::fvm::buildFaces(elements, nodes, boundaryEdges);

    int boundaryCount = 0, interiorCount = 0;
    for (const auto& f : taggedFaces) {
        if (f.rightElement_id == -1) {
            EXPECT_EQ(f.bcType, 7) << "boundary face not tagged with its group";
            ++boundaryCount;
        } else {
            EXPECT_EQ(f.bcType, -1) << "interior face should stay untagged";
            ++interiorCount;
        }
    }
    EXPECT_EQ(boundaryCount, 6);
    EXPECT_EQ(interiorCount, 3);
}