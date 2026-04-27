#include <gtest/gtest.h>
#include <vector>
#include "mesh_generation/mesh_generation.h"

TEST(ShapeGenerationTest, RectangleBoundary) {
    using namespace meshgeneration;
    const double width = 6.0;
    const double height = 2.0;
    const int segsPerUnit = 8;
    const int nx = static_cast<int>(width * segsPerUnit);
    const int ny = static_cast<int>(height * segsPerUnit);
    int expectedNumNodes = 2 * nx + 2 * ny;
    int NumRandomNodes = 30;
    meshgeneration::Mesh mesh;
    mesh.initialize("rectangle", width, height, segsPerUnit);

    // Test this is working before we add random nodes
    EXPECT_EQ(mesh.nodes.size(), expectedNumNodes);

    // Check first few nodes
    EXPECT_DOUBLE_EQ(mesh.nodes[0].x, 0.0);
    EXPECT_DOUBLE_EQ(mesh.nodes[0].y, 0.0);

    EXPECT_DOUBLE_EQ(mesh.nodes[1].x, width / nx);
    EXPECT_DOUBLE_EQ(mesh.nodes[1].y, 0.0);

    // All Corner nodes should be present
    EXPECT_DOUBLE_EQ(mesh.nodes[0].x, 0.0);           // Bottom-left
    EXPECT_DOUBLE_EQ(mesh.nodes[0].y, 0.0);
    EXPECT_DOUBLE_EQ(mesh.nodes[nx].x, width);        // Bottom-right
    EXPECT_DOUBLE_EQ(mesh.nodes[nx].y, 0.0);
    EXPECT_DOUBLE_EQ(mesh.nodes[nx + ny].x, width);   // Top-right
    EXPECT_DOUBLE_EQ(mesh.nodes[nx + ny].y, height);
    EXPECT_DOUBLE_EQ(mesh.nodes[nx + ny + nx].x, 0.0); // Top-left
    EXPECT_DOUBLE_EQ(mesh.nodes[nx + ny + nx].y, height);
    
    mesh.generateRandomNodes(NumRandomNodes, width, height); // Generate 30 random nodes within the rectangle
    EXPECT_EQ(mesh.nodes.size(), expectedNumNodes + NumRandomNodes); // 128 initial + 30 random

}