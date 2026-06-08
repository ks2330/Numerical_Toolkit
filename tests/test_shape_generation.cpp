#include <gtest/gtest.h>
#include <cmath>
#include "mesh_generation/shape_generators.h"

using namespace shapegeneration::shapes;

// --- Rectangle ---

TEST(RectangleTest, NodeCount) {
    // segsPerUnit=4, so nx = 6*4/4 = 6, ny = 2*4/4 = 2
    // bottom: nx+1, right: ny, top: nx, left: ny-1  → 2*nx + 2*ny = 16
    auto nodes = rectangle(6.0, 2.0, 4);
    EXPECT_EQ(nodes.size(), 16u);
}

TEST(RectangleTest, FirstNodeAtOrigin) {
    auto nodes = rectangle(6.0, 2.0, 4);
    EXPECT_DOUBLE_EQ(nodes[0].x, 0.0);
    EXPECT_DOUBLE_EQ(nodes[0].y, 0.0);
}

TEST(RectangleTest, CornerNodesPresent) {
    const double W = 6.0, H = 2.0;
    const int segs = 4;
    const int nx = static_cast<int>(W * segs / 4); // 6
    const int ny = static_cast<int>(H * segs / 4); // 2

    auto nodes = rectangle(W, H, segs);

    // Bottom-right corner
    EXPECT_NEAR(nodes[nx].x, W, 1e-10);
    EXPECT_NEAR(nodes[nx].y, 0.0, 1e-10);

    // Top-right corner
    EXPECT_NEAR(nodes[nx + ny].x, W, 1e-10);
    EXPECT_NEAR(nodes[nx + ny].y, H, 1e-10);

    // Top-left corner
    EXPECT_NEAR(nodes[nx + ny + nx].x, 0.0, 1e-10);
    EXPECT_NEAR(nodes[nx + ny + nx].y, H, 1e-10);
}

TEST(RectangleTest, NodeIDsAreSequential) {
    auto nodes = rectangle(4.0, 2.0, 4, 10); // id_offset = 10
    for (int i = 0; i < static_cast<int>(nodes.size()); ++i)
        EXPECT_EQ(nodes[i].Node_id, 10 + i);
}

// --- Circle ---

TEST(CircleTest, NodeCount) {
    const int segs = 16;
    auto nodes = circle(1.0, 0.0, 0.0, segs, 0);
    EXPECT_EQ(nodes.size(), static_cast<size_t>(segs));
}

TEST(CircleTest, AllNodesAtCorrectRadius) {
    const double R = 2.5;
    auto nodes = circle(R, 0.0, 0.0, 24, 0);
    for (const auto& n : nodes) {
        double r = std::sqrt(n.x * n.x + n.y * n.y);
        EXPECT_NEAR(r, R, 1e-10) << "Node at (" << n.x << ", " << n.y << ") has wrong radius";
    }
}

TEST(CircleTest, FirstNodeAtAngleZero) {
    const double R = 1.0;
    auto nodes = circle(R, 0.0, 0.0, 12, 0);
    EXPECT_NEAR(nodes[0].x, R, 1e-10);
    EXPECT_NEAR(nodes[0].y, 0.0, 1e-10);
}

TEST(CircleTest, CentreOffset) {
    auto nodes = circle(1.0, 3.0, 4.0, 8, 0);
    for (const auto& n : nodes) {
        double r = std::sqrt((n.x - 3.0) * (n.x - 3.0) + (n.y - 4.0) * (n.y - 4.0));
        EXPECT_NEAR(r, 1.0, 1e-10);
    }
}

// --- U-Shape ---

TEST(UShapeTest, NodeCountNonZero) {
    auto nodes = uShape(3.0, 3.0, 4);
    EXPECT_GT(nodes.size(), 0u);
}

TEST(UShapeTest, AllNodesWithinBounds) {
    const double W = 3.0, H = 3.0;
    auto nodes = uShape(W, H, 4);
    for (const auto& n : nodes) {
        EXPECT_GE(n.x, -1e-10) << "Node x below 0";
        EXPECT_LE(n.x, W + 1e-10) << "Node x above width";
        EXPECT_GE(n.y, -1e-10) << "Node y below 0";
        EXPECT_LE(n.y, H + 1e-10) << "Node y above height";
    }
}

TEST(UShapeTest, NodeIDsAreSequential) {
    auto nodes = uShape(3.0, 3.0, 4, 5); // id_offset = 5
    for (int i = 0; i < static_cast<int>(nodes.size()); ++i)
        EXPECT_EQ(nodes[i].Node_id, 5 + i);
}
