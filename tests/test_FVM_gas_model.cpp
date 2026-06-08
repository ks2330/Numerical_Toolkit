#include <gtest/gtest.h>
#include "nt/finite_volume_methods/FVM_gas_model.h"

using namespace nt::fvm;

// --- State struct ---

TEST(GasModelStateTest, DefaultConstruction) {
    State s{1.2, 0.3, 0.0, 2.5};
    EXPECT_DOUBLE_EQ(s.rho,   1.2);
    EXPECT_DOUBLE_EQ(s.rho_u, 0.3);
    EXPECT_DOUBLE_EQ(s.rho_v, 0.0);
    EXPECT_DOUBLE_EQ(s.rho_e, 2.5);
}

TEST(GasModelStateTest, ZeroState) {
    State s{0.0, 0.0, 0.0, 0.0};
    EXPECT_DOUBLE_EQ(s.rho,   0.0);
    EXPECT_DOUBLE_EQ(s.rho_u, 0.0);
    EXPECT_DOUBLE_EQ(s.rho_v, 0.0);
    EXPECT_DOUBLE_EQ(s.rho_e, 0.0);
}
