#include <gtest/gtest.h>
#include <vector>
#include "nt/setup/finite_element_methods/FEM_Global_Stiffness_Matrix.h"

TEST(MeshTest, Initialization) {
    nt::fem::Mesh mesh;
    mesh.initialize();

    // Check the number of nodes
    // A 2x6 grid (3 rows × 7 columns) should have 21 nodes
    EXPECT_EQ(mesh.nodes.size(), 21) << "Mesh should have 21 nodes for a 3x7 grid.";

    // Check the coordinates of the first few nodes
    EXPECT_DOUBLE_EQ(mesh.nodes[0].x, 0.0);
    EXPECT_DOUBLE_EQ(mesh.nodes[0].y, 0.0);
    EXPECT_DOUBLE_EQ(mesh.nodes[1].x, 1.0);
    EXPECT_DOUBLE_EQ(mesh.nodes[1].y, 0.0);
    EXPECT_DOUBLE_EQ(mesh.nodes[7].x, 0.0);
    EXPECT_DOUBLE_EQ(mesh.nodes[7].y, 1.0);
    EXPECT_DOUBLE_EQ(mesh.nodes[20].x, 6.0);
    EXPECT_DOUBLE_EQ(mesh.nodes[20].y, 2.0);

    EXPECT_EQ(mesh.elements.size(), 24) << "Mesh should have 24 elements (2 triangles per rectangle in a 2x6 grid).";

    // Check the node IDs of the first few elements
    EXPECT_EQ(mesh.elements[0].nodeIDs[0], 0);
    EXPECT_EQ(mesh.elements[0].nodeIDs[1], 1);
    EXPECT_EQ(mesh.elements[0].nodeIDs[2], 7);


    EXPECT_EQ(mesh.elements[19].nodeIDs[0], 11);
    EXPECT_EQ(mesh.elements[19].nodeIDs[1], 17);
    EXPECT_EQ(mesh.elements[19].nodeIDs[2], 18);

}