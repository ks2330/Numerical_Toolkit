#include <gtest/gtest.h>
#include <vector>
#include "nt/setup/finite_element_methods/FEM_Global_Stiffness_Matrix.h"

TEST(MeshTest, Initialization) {
    nt::fem::Mesh mesh;
    mesh.initialize();

    // Check the number of nodes
    // A 2x6 grid (3 rows × 7 columns) should have 21 nodes
    EXPECT_EQ(mesh.nodes.size(), 21) << "Mesh should have 21 nodes for a 3x7 grid.";

    // Print out the nodes for visual verification
    std::cout << "Nodes:" << std::endl;
    for (size_t i = 0; i < mesh.nodes.size(); ++i) {
        std::cout << "Node " << i << ": (" << mesh.nodes[i].x << ", " << mesh.nodes[i].y << ")" << std::endl;
    }
}