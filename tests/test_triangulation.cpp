#include <gtest/gtest.h>
#include <vector>
#include "mesh_generation/mesh_triangulation_algorithm.h"

TEST(MeshTest, Triangulation) {
    using namespace meshgen::triangulation;

    meshgen::triangulation::Vec2 a = {0, 5}, b = {-5, -5}, c = {5, -5}; // A big triangle
    meshgen::triangulation::Vec2 p_inside = {0, 0};
    meshgen::triangulation::Vec2 p_outside = {0, 10};

    EXPECT_TRUE(isInCircle(a, b, c, p_inside));
    EXPECT_FALSE(isInCircle(a, b, c, p_outside));


}