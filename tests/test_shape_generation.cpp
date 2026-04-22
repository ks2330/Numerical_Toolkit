#include <gtest/gtest.h>
#include <vector>
#include "mesh_generation/mesh_generation.h"

TEST(MeshTest, Triangulation) {
    using namespace meshgeneration;
    int expectedNumNodes = 128; // For a 6x2 rectangle with segsPerUnit=8, we expect 128 nodes along the edges
    int NumRandomNodes = 30;
    meshgeneration::Mesh mesh;
    mesh.initialize("rectangle", 6, 2, 8); // nx=6, ny=2 → 21 nodes, 24 triangular elements

    // Test this is working before we add random nodes
    // Need to also account for the fine mesh generation from segsPerUnit = 8, which adds more nodes along the edges
    EXPECT_EQ(mesh.nodes.size(), expectedNumNodes);

    
    EXPECT_EQ(mesh.nodes[0].x, 0);
    EXPECT_EQ(mesh.nodes[0].y, 0);

    EXPECT_EQ(mesh.nodes[1].x, 0.125);
    EXPECT_EQ(mesh.nodes[1].y, 0);

    // All Corner nodes should be present
    EXPECT_EQ(mesh.nodes[0].x, 0);
    EXPECT_EQ(mesh.nodes[0].y, 0);
    EXPECT_EQ(mesh.nodes[7].x, 6);
    EXPECT_EQ(mesh.nodes[7].y, 0);
    EXPECT_EQ(mesh.nodes[28].x, 6);
    EXPECT_EQ(mesh.nodes[28].y, 2);
    EXPECT_EQ(mesh.nodes[35].x, 0);
    EXPECT_EQ(mesh.nodes[35].y, 2);

    // Need to also check cicle is working



    mesh.generateRandomNodes(NumRandomNodes, 6, 2); // Generate 30 random nodes within the rectangle
    EXPECT_EQ(mesh.nodes.size(), expectedNumNodes + NumRandomNodes); // 128 initial + 30 random

}