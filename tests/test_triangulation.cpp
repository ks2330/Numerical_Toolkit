#include <gtest/gtest.h>
#include <vector>
#include "mesh_generation/mesh_triangulation_algorithm.h"

TEST(TriangulationTest, IsInCircle) {
    using namespace meshgen::triangulation;
    using meshgeneration::Node;

    Node a = {0, 5, 0}, b = {-5, -5, 1}, c = {5, -5, 2}; // A big triangle
    Node p_inside = {0, 0, 3};
    Node p_outside = {0, 10, 4};

    EXPECT_TRUE(isInCircle(a, b, c, p_inside));
    EXPECT_FALSE(isInCircle(a, b, c, p_outside));

}