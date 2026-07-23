#include <gtest/gtest.h>
#include "nt/finite_volume_methods/FVM_mesh.h"
#include "mesh_generation/mesh_geometry.h"
#include "mesh_generation/mesh_types.h"


// --- Helpers ---

static meshgeneration::Node N(double x, double y, int id = -1) {
    return {x, y, id};
}

static meshgeneration::Node EN(double x, double y, int id) {
    return {x, y, id};
}

static meshgeneration::Edge E(int n1, int n2) {
    return {n1, n2};
}

static meshgeneration::Element E(int n1, int n2, int n3) {
    return {n1, n2, n3};
}

// --- Cell Geometry Tests ---

TEST(Cellgeometrytest, KnownArea) {
    // Triangle (0,0),(4,0),(0,3): area = 0.5*4*3 = 6
    EXPECT_DOUBLE_EQ(nt::fvm::cellArea(N(0,0), N(4,0), N(0,3)), 6.0);
}

TEST(Cellgeometrytest, Centroid) {
    meshgeneration::Node centroid = nt::fvm::cellCentroid(N(0,0), N(4,0), N(0,3));
    EXPECT_DOUBLE_EQ(centroid.x, 4.0/3.0);
    EXPECT_DOUBLE_EQ(centroid.y, 1.0);
}

TEST(Cellgeometrytest, AreaPositiveforCCWElement){
    EXPECT_GT(nt::fvm::cellArea(N(0,0), N(4,0), N(0,3)), 0.0);
}

// --- Face Connectivity Tests ---

std::vector<meshgeneration::Node> TestNodes = {
    EN(0,0,0), EN(1,0,1), EN(1,1,2), EN(0,1,3)
};

std::vector<meshgeneration::Element> TestElements = {
    E(0,1,2), E(0,2,3)
};

TEST(Facegconnectivitytest, NumFaces) {
    std::vector<nt::fvm::Face> faces = nt::fvm::buildFaces(TestElements, TestNodes);
    EXPECT_EQ(faces.size(), 5); // 4 boundary + 1 internal
}
TEST(Faceconnectivitytest, NumInternalFaces) {
    std::vector<nt::fvm::Face> faces = nt::fvm::buildFaces(TestElements, TestNodes);
    int internalFaceCount = 0;
    for (const auto& face : faces) {
        if (face.rightElement_id != -1) {
            internalFaceCount++;
        }
    }
    EXPECT_EQ(internalFaceCount, 1); // Only the face between the two triangles is internal
}

TEST(Faceconnectivitytest, NumBoundaryFaces) {
    std::vector<nt::fvm::Face> faces = nt::fvm::buildFaces(TestElements, TestNodes);
    int boundaryFaceCount = 0;
    for (const auto& face : faces) {
        if (face.rightElement_id == -1) {
            boundaryFaceCount++;
        }
    }
    EXPECT_EQ(boundaryFaceCount, 4); // 4 boundary faces
}

TEST(Faceconnectivitytest, InternalFaceElementIDs) {
    std::vector<nt::fvm::Face> faces = nt::fvm::buildFaces(TestElements, TestNodes);
    for (const auto& face : faces) {
        if (face.rightElement_id != -1) {
            EXPECT_TRUE((face.leftElement_id == 0 && face.rightElement_id == 1) ||
                        (face.leftElement_id == 1 && face.rightElement_id == 0));
        }
    }
}

// --- Face Geometry Tests ---

TEST(FaceGeometryTest, KnownLength) {
    double length = nt::fvm::faceLength(N(0,0), N(3,4));
    EXPECT_DOUBLE_EQ(length, 5.0);
}

TEST(FaceNormalTest, IsUnitLength) {
    nt::fvm::Vec2 n = nt::fvm::faceNormal(N(0,0), N(3,4), N(1,0));   // edge (3,4), length 5
    EXPECT_NEAR(std::sqrt(n.x*n.x + n.y*n.y), 1.0, 1e-12);
}

TEST(FaceNormalTest, IsPerpendicularToEdge) {
    nt::fvm::Vec2 n = nt::fvm::faceNormal(N(0,0), N(3,4), N(1,0));
    EXPECT_NEAR(n.x*3.0 + n.y*4.0, 0.0, 1e-12);    // edge vector = (3,4)
}

TEST(FaceNormalTest, PointsAwayFromLeftCentroid) {
    nt::fvm::Vec2 n = nt::fvm::faceNormal(N(0,0), N(0,1), N(-0.5, 0.5));
    EXPECT_NEAR(n.x,  1.0, 1e-12);
    EXPECT_NEAR(n.y,  0.0, 1e-12);
}

TEST(FaceNormalTest, FlipsWhenLeftCentroidOnOtherSide) {
    nt::fvm::Vec2 n = nt::fvm::faceNormal(N(0,0), N(0,1), N(0.5, 0.5));
    EXPECT_NEAR(n.x, -1.0, 1e-12);
    EXPECT_NEAR(n.y,  0.0, 1e-12);
}



TEST(FaceGeometryTest, ClosureTest) {
    std::vector<meshgeneration::Node>    nodes = { N(0,0,0), N(4,0,1), N(0,3,2) };
    std::vector<meshgeneration::Element> elems = { E(0,1,2) };   // single cell, index 0
    auto faces = nt::fvm::buildFaces(elems, nodes);
    double sx = 0.0, sy = 0.0;
    for (const auto& face : faces) {
        sx += face.length * face.normal.x;
        sy += face.length * face.normal.y;
    }
    EXPECT_NEAR(sx, 0.0, 1e-12);
    EXPECT_NEAR(sy, 0.0, 1e-12);
}

// --- Surface watertightness tests (regression for the missing aerofoil edge bug) ---

TEST(WatertightTest, ClosedLoopHasNoUncoveredEdges) {
    std::vector<meshgeneration::Node>    nodes = { N(0,0,0), N(1,0,1), N(0,1,2) };
    std::vector<meshgeneration::Element> elems = { E(0,1,2) };
    std::vector<meshgeneration::Edge>    wall  = { {0,1,-1,1}, {1,2,-1,1}, {2,0,-1,1} };
    auto faces = nt::fvm::buildFaces(elems, nodes, wall);

    EXPECT_TRUE(nt::fvm::uncoveredBoundaryEdges(faces, wall).empty());
}

TEST(WatertightTest, MissingBoundaryEdgeIsReported) {
    // node 3 exists, but NO triangle uses edge (1,3): it is a tagged wall edge with no face,
    // exactly the defect that broke the aerofoil loop (edge 158-159).
    std::vector<meshgeneration::Node>    nodes = { N(0,0,0), N(1,0,1), N(0,1,2), N(2,0,3) };
    std::vector<meshgeneration::Element> elems = { E(0,1,2) };
    std::vector<meshgeneration::Edge>    wall  = { {0,1,-1,1}, {1,2,-1,1}, {2,0,-1,1}, {1,3,-1,1} };
    auto faces = nt::fvm::buildFaces(elems, nodes, wall);

    auto missing = nt::fvm::uncoveredBoundaryEdges(faces, wall);
    ASSERT_EQ(missing.size(), 1u);
    EXPECT_EQ(missing[0], std::make_pair(1, 3));
}

TEST(WatertightTest, SwallowedInteriorEdgeIsReported) {
    // Two triangles share edge (1,2): if that edge is tagged as a wall it is interior (2 cells),
    // not a single-sided boundary face, so it must be reported as uncovered.
    std::vector<meshgeneration::Node>    nodes = { N(0,0,0), N(1,0,1), N(0,1,2), N(1,1,3) };
    std::vector<meshgeneration::Element> elems = { E(0,1,2), E(1,3,2) };
    std::vector<meshgeneration::Edge>    wall  = { {1,2,-1,1} };
    auto faces = nt::fvm::buildFaces(elems, nodes, wall);

    auto missing = nt::fvm::uncoveredBoundaryEdges(faces, wall);
    ASSERT_EQ(missing.size(), 1u);
    EXPECT_EQ(missing[0], std::make_pair(1, 2));
}

// The unconstrained Delaunay can pick the wrong diagonal and leave a tagged boundary edge with no
// triangle (the aerofoil 158-159 notch). repairMissingBoundaryEdges fills it via the common-neighbour
// apex so the surface becomes watertight.
TEST(WatertightTest, RepairFillsSingleTriangleNotch) {
    // edge (0,1) is tagged wall but unmeshed; node 2 is the common neighbour of 0 and 1.
    std::vector<meshgeneration::Node>    nodes = { N(0,0,0), N(1,0,1), N(0.5,1,2), N(-1,1,3), N(2,1,4) };
    std::vector<meshgeneration::Element> elems = { E(0,3,2), E(1,2,4) };
    std::vector<meshgeneration::Edge>    wall  = { {0,1,-1,1} };

    auto before = nt::fvm::buildFaces(elems, nodes, wall);
    EXPECT_EQ(nt::fvm::uncoveredBoundaryEdges(before, wall).size(), 1u);

    int added = nt::fvm::repairMissingBoundaryEdges(elems, wall);
    EXPECT_EQ(added, 1);
    ASSERT_EQ(elems.size(), 3u);

    auto after = nt::fvm::buildFaces(elems, nodes, wall);
    EXPECT_TRUE(nt::fvm::uncoveredBoundaryEdges(after, wall).empty());
}

TEST(WatertightTest, RepairIsNoOpWhenAlreadyWatertight) {
    std::vector<meshgeneration::Node>    nodes = { N(0,0,0), N(1,0,1), N(0,1,2) };
    std::vector<meshgeneration::Element> elems = { E(0,1,2) };
    std::vector<meshgeneration::Edge>    wall  = { {0,1,-1,1}, {1,2,-1,1}, {2,0,-1,1} };

    EXPECT_EQ(nt::fvm::repairMissingBoundaryEdges(elems, wall), 0);
    EXPECT_EQ(elems.size(), 1u);
}