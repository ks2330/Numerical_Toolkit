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


TEST(StiffnessMatrixTest, ComputeElementStiffnessMatrix) {
    nt::fem::Mesh mesh;
    mesh.initialize();

    // Test the stiffness matrix computation for the first element
    nt::fem::Matrix3x3 stiffnessMatrix = nt::fem::computeElementStiffnessMatrix(mesh.elements[0], mesh.nodes);

    // Since the computeElementStiffnessMatrix function is a placeholder,
    // we will just check if it returns a Matrix3x3 object without crashing.
    EXPECT_NO_THROW({
        nt::fem::Matrix3x3 testMatrix = nt::fem::computeElementStiffnessMatrix(mesh.elements[1], mesh.nodes);
    });

    // Test the values of the stiffness matrix for the first element
    // Element 0: nodes (0,1,7) -> coords (0,0), (1,0), (0,1)
    // area = 0.5, b = {-1, 1, 0}, c = {-1, 0, 1}
    // K[i][j] = (b[i]*b[j] + c[i]*c[j]) / (4 * area) = (b[i]*b[j] + c[i]*c[j]) / 2

    EXPECT_DOUBLE_EQ(stiffnessMatrix.data[0][0],  1.0);
    EXPECT_DOUBLE_EQ(stiffnessMatrix.data[0][1], -0.5);
    EXPECT_DOUBLE_EQ(stiffnessMatrix.data[0][2], -0.5);
    EXPECT_DOUBLE_EQ(stiffnessMatrix.data[1][0], -0.5);
    EXPECT_DOUBLE_EQ(stiffnessMatrix.data[1][1],  0.5);
    EXPECT_DOUBLE_EQ(stiffnessMatrix.data[1][2],  0.0);
    EXPECT_DOUBLE_EQ(stiffnessMatrix.data[2][0], -0.5);
    EXPECT_DOUBLE_EQ(stiffnessMatrix.data[2][1],  0.0);
    EXPECT_DOUBLE_EQ(stiffnessMatrix.data[2][2],  0.5);

    // Symmetry: K[i][j] == K[j][i] for all i, j
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            EXPECT_DOUBLE_EQ(stiffnessMatrix.data[i][j], stiffnessMatrix.data[j][i])
                << "Symmetry failed at [" << i << "][" << j << "]";

    // Row sum: each row must sum to zero (partition of unity)
    for (int i = 0; i < 3; ++i) {
        double rowSum = stiffnessMatrix.data[i][0] + stiffnessMatrix.data[i][1] + stiffnessMatrix.data[i][2];
        EXPECT_NEAR(rowSum, 0.0, 1e-10) << "Row sum failed for row " << i;
    }

}

// --- assembleGlobalStiffnessMatrix ---

TEST(GlobalStiffnessMatrixTest, Assembly) {
    nt::fem::Mesh mesh;
    mesh.initialize();
    int N = static_cast<int>(mesh.nodes.size()); // 21

    auto K = nt::fem::assembleGlobalStiffnessMatrix(mesh);

    // Size: N x N
    ASSERT_EQ(static_cast<int>(K.size()), N);
    for (int i = 0; i < N; ++i)
        ASSERT_EQ(static_cast<int>(K[i].size()), N);

    // Symmetry
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            EXPECT_NEAR(K[i][j], K[j][i], 1e-10)
                << "Symmetry failed at [" << i << "][" << j << "]";

    // Row sums = 0 (before any BCs, no external loads)
    for (int i = 0; i < N; ++i) {
        double rowSum = 0.0;
        for (int j = 0; j < N; ++j) rowSum += K[i][j];
        EXPECT_NEAR(rowSum, 0.0, 1e-10) << "Row sum failed for row " << i;
    }

    // Corner node 0 only belongs to element 0 (nodes 0,1,7).
    // From that element: K[0][0]=1.0, K[0][1]=-0.5, K[0][7]=-0.5, all others 0.
    EXPECT_DOUBLE_EQ(K[0][0],  1.0);
    EXPECT_DOUBLE_EQ(K[0][1], -0.5);
    EXPECT_DOUBLE_EQ(K[0][7], -0.5);
    for (int j = 0; j < N; ++j)
        if (j != 0 && j != 1 && j != 7)
            EXPECT_DOUBLE_EQ(K[0][j], 0.0) << "Expected zero at K[0][" << j << "]";

    std::cout << "Global stiffness matrix K:" << std::endl;
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            std::cout << K[i][j] << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;

}


// --- gaussianElimination ---

TEST(GaussianEliminationTest, SimpleSystem) {
    // 2x + y = 5
    // x + 3y = 10
    // Solution: x=1, y=3
    std::vector<std::vector<double>> A = {{2.0, 1.0}, {1.0, 3.0}};
    std::vector<double> b = {5.0, 10.0};

    auto x = nt::fem::gaussianElimination(A, b);

    EXPECT_NEAR(x[0], 1.0, 1e-10);
    EXPECT_NEAR(x[1], 3.0, 1e-10);
}

// --- Full solver: 2x6 grid, left boundary = 100 degrees ---

TEST(HeatEquationTest, LeftBoundary100) {
    // Solve steady-state heat equation (Laplace) on a 2x6 grid.
    // Dirichlet BCs: T=100 on left (x=0), T=0 on right (x=6).
    // Neumann (zero flux) on top and bottom — natural BC, nothing to add.
    //
    // Exact solution: T(x) = 100*(6-x)/6  (linear, reproduced exactly by linear FEM)

    nt::fem::Mesh mesh;
    mesh.initialize(); // nx=6, ny=2 -> 21 nodes, stride 7

    int N = static_cast<int>(mesh.nodes.size());
    auto K = nt::fem::assembleGlobalStiffnessMatrix(mesh);
    std::vector<double> rhs(N, 0.0);

    // Left boundary (x=0): nodes 0, 7, 14  (index = i*7 + 0)
    // Right boundary (x=6): nodes 6, 13, 20 (index = i*7 + 6)
    for (int i = 0; i <= 2; ++i) {
        nt::fem::applyDirichletBC(K, rhs, i * 7,     100.0);
        nt::fem::applyDirichletBC(K, rhs, i * 7 + 6,   0.0);
    }

    auto T = nt::fem::gaussianElimination(K, rhs);

    for (int node = 0; node < N; ++node) {
        double x = mesh.nodes[node].x;
        double expected = 100.0 * (6.0 - x) / 6.0;
        EXPECT_NEAR(T[node], expected, 1e-8)
            << "Node " << node << " at x=" << x << " y=" << mesh.nodes[node].y;
    }
    std::cout << "Computed temperatures T:" << std::endl;
    for (int node = 0; node < N; ++node) {
        std::cout << "Node " << node << " (x=" << mesh.nodes[node].x << ", y=" << mesh.nodes[node].y << "): T=" << T[node] << std::endl;
    }  
}