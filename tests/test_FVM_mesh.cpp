#include <gtest/gtest.h>
#include "nt/finite_volume_methods/FVM_mesh.h"
#include "mesh_generation/mesh_geometry.h"
#include "mesh_generation/mesh_types.h"


// --- Helpers ---

static meshgeneration::Node N(double x, double y, int id = -1) {
    return {x, y, id};
}

TEST(Cellgeometrytest, KnownArea) {
    // Triangle (0,0),(4,0),(0,3): area = 0.5*4*3 = 6
    EXPECT_DOUBLE_EQ(nt::fvm::cellArea(N(0,0), N(4,0), N(0,3)), 6.0);
}