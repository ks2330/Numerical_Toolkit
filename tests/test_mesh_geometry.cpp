#include <gtest/gtest.h>
#include <cmath>
#include <vector>
#include "mesh_generation/mesh_geometry.h"
#include "mesh_generation/mesh_types.h"
#include "mesh_generation/quadtree.h"

using namespace meshgeneration;

// ─── Helpers ─────────────────────────────────────────────────────────────────

static Node N(double x, double y, int id = -1) {
    return {x, y, id};
}

// ─── orient2d ────────────────────────────────────────────────────────────────

TEST(Orient2dTest, CCWTrianglePositive) {
    // (0,0) → (1,0) → (0,1) is CCW
    EXPECT_GT(orient2d(N(0,0), N(1,0), N(0,1)), 0.0);
}

TEST(Orient2dTest, CWTriangleNegative) {
    // Reversing last two vertices flips sign
    EXPECT_LT(orient2d(N(0,0), N(0,1), N(1,0)), 0.0);
}

TEST(Orient2dTest, CollinearPointsZero) {
    EXPECT_DOUBLE_EQ(orient2d(N(0,0), N(1,0), N(2,0)), 0.0);
}

TEST(Orient2dTest, KnownArea) {
    // Triangle (0,0),(4,0),(0,3): area = 0.5*4*3 = 6, orient2d = 2*area = 12
    EXPECT_DOUBLE_EQ(orient2d(N(0,0), N(4,0), N(0,3)), 12.0);
}

// ─── distance & distanceSquared ──────────────────────────────────────────────

TEST(DistanceTest, PythagoreanTriple) {
    EXPECT_DOUBLE_EQ(distance(N(0,0), N(3,4)), 5.0);
}

TEST(DistanceTest, Symmetry) {
    Node a = N(1,2), b = N(4,6);
    EXPECT_DOUBLE_EQ(distance(a, b), distance(b, a));
}

TEST(DistanceTest, SamePoint) {
    EXPECT_DOUBLE_EQ(distance(N(3,7), N(3,7)), 0.0);
}

TEST(DistanceSquaredTest, MatchesDistanceSquared) {
    Node a = N(0,0), b = N(3,4);
    double d = distance(a, b);
    EXPECT_NEAR(distanceSquared(a, b), d * d, 1e-10);
}

TEST(DistanceSquaredTest, NoPythagoras) {
    EXPECT_DOUBLE_EQ(distanceSquared(N(0,0), N(3,4)), 25.0);
}

// ─── isCCW ───────────────────────────────────────────────────────────────────

TEST(IsCCWTest, CCWSquare) {
    // (0,0)→(1,0)→(1,1)→(0,1) — standard CCW
    std::vector<Node> poly = {N(0,0), N(1,0), N(1,1), N(0,1)};
    EXPECT_TRUE(isCCW(poly));
}

TEST(IsCCWTest, CWSquare) {
    // Reversed traversal
    std::vector<Node> poly = {N(0,0), N(0,1), N(1,1), N(1,0)};
    EXPECT_FALSE(isCCW(poly));
}

// ─── edgeDirection ───────────────────────────────────────────────────────────

TEST(EdgeDirectionTest, Horizontal) {
    auto [dx, dy] = edgeDirection(N(0,0), N(3,0));
    EXPECT_DOUBLE_EQ(dx, 3.0);
    EXPECT_DOUBLE_EQ(dy, 0.0);
}

TEST(EdgeDirectionTest, Diagonal) {
    auto [dx, dy] = edgeDirection(N(1,1), N(4,5));
    EXPECT_DOUBLE_EQ(dx, 3.0);
    EXPECT_DOUBLE_EQ(dy, 4.0);
}

// ─── isInCircle ──────────────────────────────────────────────────────────────
// For CCW triangle A,B,C: det>0 means D is inside the circumcircle.
// Triangle (0,1),(-1,0),(1,0) is CCW and its circumcircle is the unit circle.

TEST(IsInCircleTest, PointInsideCircumcircle) {
    Node A = N(0,1), B = N(-1,0), C = N(1,0);
    EXPECT_TRUE(isInCircle(A, B, C, N(0, 0)));    // origin — clearly inside
}

TEST(IsInCircleTest, PointOutsideCircumcircle) {
    Node A = N(0,1), B = N(-1,0), C = N(1,0);
    EXPECT_FALSE(isInCircle(A, B, C, N(0, 1.5))); // above top vertex
}

TEST(IsInCircleTest, VertexNotInsideItsOwnCircumcircle) {
    // A vertex sits exactly on the circumcircle, det == 0 → not strictly inside
    Node A = N(0,1), B = N(-1,0), C = N(1,0);
    EXPECT_FALSE(isInCircle(A, B, C, A));          // det == 0 → false
}

// ─── isSameEdge ──────────────────────────────────────────────────────────────

TEST(IsSameEdgeTest, ForwardEdge) {
    Element t{0, 1, 2, 0};
    EXPECT_TRUE(isSameEdge(t, {0, 1, -1}));
    EXPECT_TRUE(isSameEdge(t, {1, 2, -1}));
    EXPECT_TRUE(isSameEdge(t, {2, 0, -1}));
}

TEST(IsSameEdgeTest, ReversedEdge) {
    Element t{0, 1, 2, 0};
    EXPECT_TRUE(isSameEdge(t, {1, 0, -1}));
    EXPECT_TRUE(isSameEdge(t, {2, 1, -1}));
    EXPECT_TRUE(isSameEdge(t, {0, 2, -1}));
}

TEST(IsSameEdgeTest, NonExistentEdge) {
    Element t{0, 1, 2, 0};
    EXPECT_FALSE(isSameEdge(t, {0, 3, -1}));
    EXPECT_FALSE(isSameEdge(t, {3, 4, -1}));
}

// ─── Edge::operator== (replaces edgesMatch) ──────────────────────────────────

TEST(EdgeEqualityTest, SameDirection) {
    Edge a{0, 1, -1}, b{0, 1, -1};
    EXPECT_TRUE(a == b);
}

TEST(EdgeEqualityTest, Reversed) {
    Edge a{0, 1, -1}, b{1, 0, -1};
    EXPECT_TRUE(a == b);
}

TEST(EdgeEqualityTest, DifferentNodes) {
    Edge a{0, 1, -1};
    Edge b{0, 2, -1};
    Edge c{2, 3, -1};
    EXPECT_FALSE(a == b);
    EXPECT_FALSE(a == c);
}

// ─── isPointInPolygon ────────────────────────────────────────────────────────

TEST(IsPointInPolygonTest, InsideSquare) {
    std::vector<Node> sq = {N(0,0), N(1,0), N(1,1), N(0,1)};
    EXPECT_TRUE(isPointInPolygon(N(0.5, 0.5), sq));
}

TEST(IsPointInPolygonTest, OutsideSquare) {
    std::vector<Node> sq = {N(0,0), N(1,0), N(1,1), N(0,1)};
    EXPECT_FALSE(isPointInPolygon(N(1.5, 0.5), sq));
    EXPECT_FALSE(isPointInPolygon(N(-0.5, 0.5), sq));
    EXPECT_FALSE(isPointInPolygon(N(0.5, 1.5), sq));
}

TEST(IsPointInPolygonTest, EmptyBoundaryReturnsFalse) {
    EXPECT_FALSE(isPointInPolygon(N(0,0), {}));
}

// ─── GetBoundingBox ──────────────────────────────────────────────────────────

TEST(GetBoundingBoxTest, CorrectCorners) {
    std::vector<Node> pts = {N(1,2), N(3,0), N(2,4), N(0,1)};
    auto bb = GetBoundingBox(pts);
    ASSERT_EQ(bb.size(), 4u);
    // [minX,minY], [maxX,minY], [maxX,maxY], [minX,maxY]
    EXPECT_DOUBLE_EQ(bb[0].x, 0.0);  EXPECT_DOUBLE_EQ(bb[0].y, 0.0);
    EXPECT_DOUBLE_EQ(bb[1].x, 3.0);  EXPECT_DOUBLE_EQ(bb[1].y, 0.0);
    EXPECT_DOUBLE_EQ(bb[2].x, 3.0);  EXPECT_DOUBLE_EQ(bb[2].y, 4.0);
    EXPECT_DOUBLE_EQ(bb[3].x, 0.0);  EXPECT_DOUBLE_EQ(bb[3].y, 4.0);
}

TEST(GetBoundingBoxTest, EmptyReturnsEmpty) {
    EXPECT_TRUE(GetBoundingBox({}).empty());
}

TEST(GetBoundingBoxTest, SinglePoint) {
    auto bb = GetBoundingBox({N(5,3)});
    EXPECT_DOUBLE_EQ(bb[0].x, 5.0);
    EXPECT_DOUBLE_EQ(bb[2].y, 3.0);
}

// ─── minAngle ────────────────────────────────────────────────────────────────

TEST(MinAngleTest, EquilateralTriangle) {
    // All angles exactly 60 degrees
    Node a = N(0, 0), b = N(1, 0), c = N(0.5, std::sqrt(3.0)/2.0);
    EXPECT_NEAR(minAngle(a, b, c), M_PI / 3.0, 1e-10);
}

TEST(MinAngleTest, RightTriangle) {
    // (0,0),(1,0),(0,1): angles 45, 45, 90 → min = 45° = π/4
    EXPECT_NEAR(minAngle(N(0,0), N(1,0), N(0,1)), M_PI / 4.0, 1e-10);
}

TEST(MinAngleTest, NonNegative) {
    EXPECT_GE(minAngle(N(0,0), N(3,0), N(1,2)), 0.0);
}

// ─── aspectRatio ─────────────────────────────────────────────────────────────

TEST(AspectRatioTest, EquilateralIsNearOne) {
    Node a = N(0, 0), b = N(1, 0), c = N(0.5, std::sqrt(3.0)/2.0);
    EXPECT_NEAR(aspectRatio(a, b, c), 1.0, 1e-3);
}

TEST(AspectRatioTest, ElongatedTriangleHighRatio) {
    // Very flat triangle: (0,0),(10,0),(5,0.01)
    Node a = N(0,0), b = N(10,0), c = N(5, 0.01);
    EXPECT_GT(aspectRatio(a, b, c), 100.0);
}

TEST(AspectRatioTest, RightTriangle) {
    // (0,0),(4,0),(0,3): sides 3,4,5; longest=5; area=6
    // ratio = 0.433*25/6 ≈ 1.804
    EXPECT_NEAR(aspectRatio(N(0,0), N(4,0), N(0,3)), 0.433 * 25.0 / 6.0, 1e-6);
}

// ─── RotateVector ────────────────────────────────────────────────────────────

TEST(RotateVectorTest, ZeroAngle) {
    Node result = RotateVector(N(0,0), N(1,0), 0.0);
    EXPECT_NEAR(result.x, 1.0, 1e-10);
    EXPECT_NEAR(result.y, 0.0, 1e-10);
}

TEST(RotateVectorTest, NinetyDegrees) {
    // Rotate (1,0) 90° CCW around origin → (0,1)
    Node result = RotateVector(N(0,0), N(1,0), M_PI / 2.0);
    EXPECT_NEAR(result.x, 0.0, 1e-10);
    EXPECT_NEAR(result.y, 1.0, 1e-10);
}

TEST(RotateVectorTest, MinusSixtyDegrees) {
    // Equilateral triangle ideal point: rotate edge (0→1) by -60° → apex below
    Node result = RotateVector(N(0,0), N(1,0), -M_PI / 3.0);
    EXPECT_NEAR(result.x, 0.5, 1e-10);
    EXPECT_NEAR(result.y, -std::sqrt(3.0)/2.0, 1e-10);
}

TEST(RotateVectorTest, WithOffset) {
    // a=(1,1), b=(2,1): rotate vector (1,0) by 90° → adds (0,1) to a = (1,2)
    Node result = RotateVector(N(1,1), N(2,1), M_PI / 2.0);
    EXPECT_NEAR(result.x, 1.0, 1e-10);
    EXPECT_NEAR(result.y, 2.0, 1e-10);
}

// ─── Node operators ──────────────────────────────────────────────────────────

TEST(NodeOperatorTest, Addition) {
    Node a = N(1,2), b = N(3,4);
    Node c = a + b;
    EXPECT_DOUBLE_EQ(c.x, 4.0);
    EXPECT_DOUBLE_EQ(c.y, 6.0);
}

TEST(NodeOperatorTest, Subtraction) {
    Node a = N(5,7), b = N(2,3);
    Node c = a - b;
    EXPECT_DOUBLE_EQ(c.x, 3.0);
    EXPECT_DOUBLE_EQ(c.y, 4.0);
}

TEST(NodeOperatorTest, ScalarMultiply) {
    Node a = N(1,2);
    Node c = a * 3.0;
    EXPECT_DOUBLE_EQ(c.x, 3.0);
    EXPECT_DOUBLE_EQ(c.y, 6.0);
}

TEST(NodeOperatorTest, EqualityByID) {
    Node a = {1.0, 2.0, 5};
    Node b = {9.0, 9.0, 5}; // same id, different coords
    Node c = {1.0, 2.0, 6}; // same coords, different id
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
}

// ─── Edge operator== ─────────────────────────────────────────────────────────

TEST(EdgeOperatorTest, SameDirection) {
    Edge a{0, 1, -1}, b{0, 1, -1};
    EXPECT_TRUE(a == b);
}

TEST(EdgeOperatorTest, Reversed) {
    Edge a{0, 1, -1}, b{1, 0, -1};
    EXPECT_TRUE(a == b);
}

TEST(EdgeOperatorTest, Different) {
    Edge a{0, 1, -1}, b{0, 2, -1};
    EXPECT_FALSE(a == b);
}

// ─── AABB::intersects ────────────────────────────────────────────────────────

using meshgeneration::AABB;

TEST(AABBTest, OverlappingBoxes) {
    AABB a{0.5, 0.5, 0.5, 0.5}; // covers (0,0)→(1,1)
    AABB b{1.0, 1.0, 0.5, 0.5}; // covers (0.5,0.5)→(1.5,1.5)
    EXPECT_TRUE(a.intersects(b));
    EXPECT_TRUE(b.intersects(a));
}

TEST(AABBTest, SeparateBoxes) {
    AABB a{0.5, 0.5, 0.4, 0.4}; // covers (0.1,0.1)→(0.9,0.9)
    AABB b{2.0, 2.0, 0.4, 0.4}; // covers (1.6,1.6)→(2.4,2.4)
    EXPECT_FALSE(a.intersects(b));
    EXPECT_FALSE(b.intersects(a));
}

TEST(AABBTest, ContainedBox) {
    AABB outer{5.0, 5.0, 5.0, 5.0}; // covers (0,0)→(10,10)
    AABB inner{5.0, 5.0, 1.0, 1.0}; // covers (4,4)→(6,6)
    EXPECT_TRUE(outer.intersects(inner));
}

// ─── Quadtree ────────────────────────────────────────────────────────────────

TEST(QuadtreeTest, InsertAndQuerySingleNode) {
    AABB bounds{0.5, 0.5, 0.5, 0.5}; // covers (0,0)→(1,1)
    meshgeneration::Quadtree qt(bounds);

    Node n = {0.3, 0.3, 0};
    qt.insert(n);

    AABB range{0.3, 0.3, 0.3, 0.3}; // covers (0,0)→(0.6,0.6)
    auto result = qt.query(range);
    EXPECT_EQ(result.size(), 1u);
    EXPECT_NEAR(result[0].x, 0.3, 1e-10);
}

TEST(QuadtreeTest, NodeOutsideRangeNotReturned) {
    AABB bounds{0.5, 0.5, 0.5, 0.5};
    meshgeneration::Quadtree qt(bounds);

    qt.insert({0.1, 0.1, 0}); // inside small range below
    qt.insert({0.9, 0.9, 1}); // outside small range

    AABB range{0.1, 0.1, 0.15, 0.15}; // only catches (0.1,0.1)
    auto result = qt.query(range);
    EXPECT_EQ(result.size(), 1u);
}

TEST(QuadtreeTest, EmptyQueryReturnsNothing) {
    AABB bounds{0.5, 0.5, 0.5, 0.5};
    meshgeneration::Quadtree qt(bounds);
    qt.insert({0.5, 0.5, 0});

    AABB range{5.0, 5.0, 0.1, 0.1}; // far away
    auto result = qt.query(range);
    EXPECT_TRUE(result.empty());
}

TEST(QuadtreeTest, TriggersSubdivisionAllNodesStillFound) {
    // Insert more nodes than the leaf capacity (4) to force subdivision.
    // Positions deliberately avoid exact quadrant boundaries (x=0.5, y=0.5).
    AABB bounds{0.5, 0.5, 0.5, 0.5}; // covers (0,0)→(1,1)
    meshgeneration::Quadtree qt(bounds);

    std::vector<Node> inserted = {
        {0.1, 0.1, 0}, {0.2, 0.1, 1}, {0.1, 0.2, 2},
        {0.2, 0.2, 3}, {0.3, 0.1, 4}, {0.6, 0.6, 5},
    };
    for (const auto& n : inserted) qt.insert(n);

    AABB fullRange{0.5, 0.5, 0.49, 0.49}; // slightly inside bounds to avoid edge
    auto result = qt.query(fullRange);
    EXPECT_EQ(result.size(), inserted.size());
}

TEST(QuadtreeTest, MultipleNodesInRange) {
    AABB bounds{0.5, 0.5, 0.5, 0.5};
    meshgeneration::Quadtree qt(bounds);

    qt.insert({0.2, 0.2, 0});
    qt.insert({0.3, 0.3, 1});
    qt.insert({0.8, 0.8, 2}); // outside query range

    AABB range{0.25, 0.25, 0.2, 0.2}; // covers (0.05,0.05)→(0.45,0.45)
    auto result = qt.query(range);
    EXPECT_EQ(result.size(), 2u);
}
