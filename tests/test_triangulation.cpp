#include <gtest/gtest.h>
#include <cmath>
#include "mesh_generation/mesh_generation.h"
#include "mesh_generation/mesh_geometry.h"

using namespace meshgeneration;

// --- Bowyer-Watson ---

TEST(BowyerWatsonTest, FourPointsProducesTwoTriangles) {
    // 4 convex-position points → exactly 2 triangles (2N - h - 2 = 2*4 - 4 - 2 = 2)
    Mesh mesh;
    mesh.nodes = {
        {0.0, 0.0, 0}, {1.0, 0.0, 1}, {1.0, 1.0, 2}, {0.0, 1.0, 3},
    };
    auto elems = mesh.bowyerWatson();
    EXPECT_EQ(elems.size(), 2u);
}

TEST(BowyerWatsonTest, AllTrianglesHavePositiveArea) {
    Mesh mesh;
    mesh.nodes = {
        {0.0, 0.0, 0}, {1.0, 0.0, 1}, {0.5, 1.0, 2},
        {0.25, 0.4, 3}, {0.75, 0.4, 4},
    };
    auto elems = mesh.bowyerWatson();
    ASSERT_FALSE(elems.empty());
    for (const auto& e : elems) {
        const Node& n0 = mesh.nodes[e.n0_id];
        const Node& n1 = mesh.nodes[e.n1_id];
        const Node& n2 = mesh.nodes[e.n2_id];
        double area = 0.5 * std::abs(
            n0.x * (n1.y - n2.y) +
            n1.x * (n2.y - n0.y) +
            n2.x * (n0.y - n1.y));
        EXPECT_GT(area, 1e-10)
            << "Degenerate triangle: nodes " << e.n0_id << " " << e.n1_id << " " << e.n2_id;
    }
}

TEST(BowyerWatsonTest, NoNegativeNodeIDs) {
    // Super-triangle nodes use negative IDs (-1,-2,-3) — they must be filtered out
    Mesh mesh;
    mesh.nodes = {
        {0.0, 0.0, 0}, {1.0, 0.0, 1}, {1.0, 1.0, 2}, {0.0, 1.0, 3},
    };
    auto elems = mesh.bowyerWatson();
    for (const auto& e : elems) {
        EXPECT_GE(e.n0_id, 0) << "Super-triangle node leaked into output";
        EXPECT_GE(e.n1_id, 0) << "Super-triangle node leaked into output";
        EXPECT_GE(e.n2_id, 0) << "Super-triangle node leaked into output";
    }
}

TEST(BowyerWatsonTest, DelaunayProperty) {
    // No node should lie strictly inside the circumcircle of any output triangle
    Mesh mesh;
    mesh.nodes = {
        {0.0, 0.0, 0}, {1.0, 0.0, 1}, {0.5, 1.0, 2},
        {0.3, 0.35, 3}, {0.7, 0.35, 4},
    };
    auto elems = mesh.bowyerWatson();
    ASSERT_FALSE(elems.empty());
    for (const auto& e : elems) {
        const Node& a = mesh.nodes[e.n0_id];
        const Node& b = mesh.nodes[e.n1_id];
        const Node& c = mesh.nodes[e.n2_id];
        for (const auto& n : mesh.nodes) {
            if (n.Node_id == e.n0_id || n.Node_id == e.n1_id || n.Node_id == e.n2_id)
                continue;
            EXPECT_FALSE(isInCircle(a, b, c, n))
                << "Node " << n.Node_id << " inside circumcircle of triangle ("
                << e.n0_id << ", " << e.n1_id << ", " << e.n2_id << ")";
        }
    }
}

TEST(BowyerWatsonTest, NodeCountScaling) {
    // For N points in general position: expect ~2N triangles (Euler formula)
    Mesh mesh;
    const int N = 10;
    for (int i = 0; i < N; ++i)
        mesh.nodes.push_back({0.1 * i, 0.05 * (i % 3), i});
    auto elems = mesh.bowyerWatson();
    // 2N - h - 2 for convex hull size h; at minimum 2*N - N - 2 = N - 2
    EXPECT_GE(static_cast<int>(elems.size()), N - 2);
}
