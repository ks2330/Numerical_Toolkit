#include <gtest/gtest.h>
#include "nt/finite_volume_methods/FVM_gas_model.h"

using namespace nt::fvm;

// --- ConservativeState struct ---

TEST(GasModelStateTest, DefaultConstruction) {
    ConservativeState s{1.2, 0.3, 0.0, 2.5};
    EXPECT_DOUBLE_EQ(s.rho,   1.2);
    EXPECT_DOUBLE_EQ(s.rho_u, 0.3);
    EXPECT_DOUBLE_EQ(s.rho_v, 0.0);
    EXPECT_DOUBLE_EQ(s.rho_e, 2.5);
}

TEST(GasModelStateTest, ZeroState) {
    ConservativeState s{0.0, 0.0, 0.0, 0.0};
    EXPECT_DOUBLE_EQ(s.rho,   0.0);
    EXPECT_DOUBLE_EQ(s.rho_u, 0.0);
    EXPECT_DOUBLE_EQ(s.rho_v, 0.0);
    EXPECT_DOUBLE_EQ(s.rho_e, 0.0);
}

TEST(GasModelStateTest, PrimConsRoundtripIsIdentity) {
    GasModel gm{1.4, 287.0};
    PrimitiveState in{1.2, 0.25, 0.0, 0.985};
    PrimitiveState out = gm.toPrimitive(gm.toConservative(in));
    EXPECT_NEAR(out.rho, in.rho, 1e-12);
    EXPECT_NEAR(out.u,   in.u,   1e-12);
    EXPECT_NEAR(out.v,   in.v,   1e-12);
    EXPECT_NEAR(out.p,   in.p,   1e-12);
}

TEST(GasModelStateTest, PrimToConsMatchesAnalytic) {
    GasModel gm{1.4, 287.0};
    PrimitiveState ps{1.2, 0.25, 0.0, 0.985};
    ConservativeState s = gm.toConservative(ps);
    EXPECT_NEAR(s.rho,   1.2,   1e-12);
    EXPECT_NEAR(s.rho_u, 0.3,   1e-12);
    EXPECT_NEAR(s.rho_v, 0.0,   1e-12);
    EXPECT_NEAR(s.rho_e, 2.5,   1e-12);
}


// --- PrimitiveState struct ---

TEST(PrimitiveStateTest, DefaultConstruction) {
    PrimitiveState ps{1.2, 0.3, 0.0, 101325};
    EXPECT_DOUBLE_EQ(ps.rho, 1.2);
    EXPECT_DOUBLE_EQ(ps.u,   0.3);
    EXPECT_DOUBLE_EQ(ps.v,   0.0);
    EXPECT_DOUBLE_EQ(ps.p,   101325);
}

TEST(PrimitiveStateTest, ZeroState) {
    PrimitiveState ps{0.0, 0.0, 0.0, 0.0};
    EXPECT_DOUBLE_EQ(ps.rho, 0.0);
    EXPECT_DOUBLE_EQ(ps.u,   0.0);
    EXPECT_DOUBLE_EQ(ps.v,   0.0);
    EXPECT_DOUBLE_EQ(ps.p,   0.0);
}

TEST(PrimitiveStateTest, ConsToPrimMatchesAnalytic) {
    GasModel gm{1.4, 287.0};
    ConservativeState s{1.2, 0.3, 0.0, 2.5};
    PrimitiveState ps = gm.toPrimitive(s);
    EXPECT_NEAR(ps.rho, 1.2,   1e-12);
    EXPECT_NEAR(ps.u,   0.25,  1e-12);
    EXPECT_NEAR(ps.v,   0.0,   1e-12);
    EXPECT_NEAR(ps.p,   0.985, 1e-12);
}


// --- GasModel struct ---

TEST(GasModelTest, DefaultConstruction) {
    GasModel gm{1.4, 287.0};
    EXPECT_DOUBLE_EQ(gm.gamma, 1.4);
    EXPECT_DOUBLE_EQ(gm.R, 287.0);
}

// --- Gas Pressure calculation ---

TEST(GasModelTest, PressureCalculation) {
    GasModel gm{1.4, 287.0};
    ConservativeState s{2, 4, 0.0, 10}; 
    EXPECT_DOUBLE_EQ(gm.pressure(s), 2.4);
}


// -- Speed of Sound calculation ---

TEST(GasModelTest, SpeedOfSoundCalculation) {
    GasModel gm{1.4, 287.0};
    ConservativeState s{1.2, 0.3, 0.0, 2.5};
    PrimitiveState ps = gm.toPrimitive(s);
    EXPECT_NEAR(gm.soundSpeed(ps), 1.07199, 1e-5);

}


// --- Positivity guards (throw on unphysical states) ---

TEST(GasModelGuardTest, NonPositiveDensityThrows) {
    GasModel gm{1.4, 287.0};
    ConservativeState bad{0.0, 0.0, 0.0, 1.0};   // rho = 0
    EXPECT_THROW(gm.pressure(bad),    std::runtime_error);
    EXPECT_THROW(gm.toPrimitive(bad), std::runtime_error);
}

TEST(GasModelGuardTest, NonPositivePressureThrows) {
    GasModel gm{1.4, 287.0};
    // KE = 0.5*(2^2)/1 = 2, but rho_e = 1 < 2  ->  negative internal energy  ->  p <= 0
    ConservativeState bad{1.0, 2.0, 0.0, 1.0};
    EXPECT_THROW(gm.pressure(bad), std::runtime_error);
}

TEST(GasModelGuardTest, ValidStateHasPositivePressure) {
    GasModel gm{1.4, 287.0};
    ConservativeState s{1.0, 2.0, 0.0, 5.0};   // KE=2, internal=3, p=1.2
    EXPECT_GT(gm.pressure(s), 0.0);
}