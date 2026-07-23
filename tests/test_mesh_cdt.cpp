#include <gtest/gtest.h>
#include <cmath>
#include <cstdlib>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "mesh_generation/mesh_generation.h"
#include "mesh_generation/mesh_geometry.h"
#include "mesh_generation/algorithm_delaunay_triangulation.h"
#include "app_support/app_FEM.h"

using namespace meshgeneration;

// ─── Helpers ─────────────────────────────────────────────────────────────────

static Node N(double x, double y, int id, NodeType type = NodeType::Internal) {
    return {x, y, id, type};
}

static int countElementsWithEdge(const Mesh& m, const Edge& e) {
    int count = 0;
    for (const auto& el : m.elements)
        if (isSameEdge(el, e)) ++count;
    return count;
}

static double totalMeshArea(const Mesh& m) {
    double area = 0.0;
    for (const auto& el : m.elements)
        area += 0.5 * std::abs(orient2d(m.nodes[el.n0_id], m.nodes[el.n1_id], m.nodes[el.n2_id]));
    return area;
}

// Unit square split along the 0-2 diagonal.
// Fixture element IDs start at 100 so they never collide with IDs handed out
// by the private element_id_counter (which starts at 0) during recovery.
static Mesh makeSquareMesh() {
    Mesh m;
    m.nodes = {N(0,0,0), N(1,0,1), N(1,1,2), N(0,1,3)};
    m.elements = {{0, 1, 2, 100}, {0, 2, 3, 101}};
    return m;
}

// Hexagon A(0,0) D(2,-1.5) B(4,0) u3(3,2) u2(2,0.5) u1(1,2), triangulated as a
// fan from D. The constraint A-B crosses all four triangles, so the cavity is
// the whole hexagon (area 7.5). The cavity is NOT star-shaped from A: the
// segment A-u3 exits the hexagon through edge u1-u2, so a naive fan from A
// emits overlapping triangles.
static Mesh makeNonConvexCavityMesh() {
    Mesh m;
    m.nodes = {N(0,0,0),        // A
               N(2,-1.5,1),     // D
               N(4,0,2),        // B
               N(3,2,3),        // u3
               N(2,0.5,4),      // u2
               N(1,2,5)};       // u1
    m.elements = {{1, 2, 3, 100}, {1, 3, 4, 101}, {1, 4, 5, 102}, {1, 5, 0, 103}};
    m.edges = {{0, 2, -1}};     // constraint A-B
    return m;
}

// ─── enforceConstraint: unit fixtures ────────────────────────────────────────

// #1: a constraint edge already present in the triangulation must be left alone.
TEST(EnforceConstraintTest, ExistingEdgeIsNoOp) {
    Mesh m = makeSquareMesh();
    m.edges = {{0, 2, -1}};

    m.enforceConstraint();

    ASSERT_EQ(m.elements.size(), 2u);
    EXPECT_EQ(m.elements[0].Element_id, 100);
    EXPECT_EQ(m.elements[1].Element_id, 101);
    EXPECT_EQ(countElementsWithEdge(m, {0, 2, -1}), 2);
}

// #2: recovering the crossing diagonal of a convex quad swaps the diagonal.
TEST(EnforceConstraintTest, RecoversEdgeInConvexQuad) {
    Mesh m = makeSquareMesh();
    m.edges = {{1, 3, -1}};

    m.enforceConstraint();

    EXPECT_EQ(countElementsWithEdge(m, {1, 3, -1}), 2);
    EXPECT_EQ(m.elements.size(), 2u);
    EXPECT_NEAR(totalMeshArea(m), 1.0, 1e-12);
}

// #3 (red driver): retriangulating a non-convex cavity must preserve area.
// The naive fan from the constraint's first node creates overlapping
// triangles here (sums to 10.0 instead of 7.5).
TEST(EnforceConstraintTest, NonConvexCavityPreservesArea) {
    Mesh m = makeNonConvexCavityMesh();
    ASSERT_NEAR(totalMeshArea(m), 7.5, 1e-12);  // fixture sanity

    m.enforceConstraint();

    EXPECT_NEAR(totalMeshArea(m), 7.5, 1e-9);
    EXPECT_EQ(countElementsWithEdge(m, {0, 2, -1}), 2);
}

// #4: recovered triangles must be valid — CCW, non-degenerate, and no cavity
// vertex strictly inside any recovered triangle.
TEST(EnforceConstraintTest, RecoveredTrianglesAreValid) {
    Mesh m = makeSquareMesh();
    m.edges = {{1, 3, -1}};

    m.enforceConstraint();

    for (const auto& el : m.elements) {
        double o = orient2d(m.nodes[el.n0_id], m.nodes[el.n1_id], m.nodes[el.n2_id]);
        EXPECT_GT(o, 1e-12) << "element " << el.Element_id << " is not CCW / degenerate";
        for (const auto& n : m.nodes) {
            if (n.Node_id == el.n0_id || n.Node_id == el.n1_id || n.Node_id == el.n2_id)
                continue;
            bool inside = orient2d(m.nodes[el.n0_id], m.nodes[el.n1_id], n) > 1e-12 &&
                          orient2d(m.nodes[el.n1_id], m.nodes[el.n2_id], n) > 1e-12 &&
                          orient2d(m.nodes[el.n2_id], m.nodes[el.n0_id], n) > 1e-12;
            EXPECT_FALSE(inside) << "node " << n.Node_id
                                 << " lies inside element " << el.Element_id;
        }
    }
}

// ─── Full pipeline: watertightness ───────────────────────────────────────────

// #5 (red driver): after the full aerofoil pipeline every constrained edge
// must bound exactly one triangle, every other edge exactly two — i.e. the
// mesh is watertight with no notches on the aerofoil surface.
TEST(CDTAerofoilTest, PipelineProducesWatertightMesh) {
    // Seeds 9 and 11 produce cavities that expose the naive fan re-fill
    // (missing aerofoil edge, i.e. the "notch"); 42 is a benign control.
    for (int seed : {9, 11, 42}) {
        SCOPED_TRACE("srand seed " + std::to_string(seed));
        std::srand(seed);
        Mesh mesh;
        mesh.init(std::string(NT_DATA_DIR) + "/aerofoil.dat", 100.0);
        mesh.generateRandomNodes();
        DelaunayTriangulation algorithm;
        mesh.triangulate(algorithm);

        ASSERT_FALSE(mesh.elements.empty());

        // Constrained edges (outer box + aerofoil surface) bound exactly 1 triangle.
        int missingConstraints = 0, overSharedConstraints = 0;
        for (const auto& e : mesh.edges) {
            int count = countElementsWithEdge(mesh, e);
            if (count == 0) ++missingConstraints;
            if (count > 1)  ++overSharedConstraints;
        }
        EXPECT_EQ(missingConstraints, 0)
            << missingConstraints << " of " << mesh.edges.size()
            << " constrained edges are missing from the triangulation (notch)";
        EXPECT_EQ(overSharedConstraints, 0)
            << overSharedConstraints << " constrained edges bound more than one triangle";

        // Every triangle edge is shared by exactly 2 triangles, unless constrained
        // (then exactly 1). Anything else is a hole or an overlap.
        std::map<std::pair<int,int>, int> incidence;
        for (const auto& el : mesh.elements) {
            int ids[3][2] = {{el.n0_id, el.n1_id}, {el.n1_id, el.n2_id}, {el.n2_id, el.n0_id}};
            for (auto& p : ids)
                ++incidence[{std::min(p[0], p[1]), std::max(p[0], p[1])}];
        }
        int badEdges = 0;
        for (const auto& [key, count] : incidence) {
            if (count == 2) continue;
            bool constrained = false;
            for (const auto& e : mesh.edges)
                if (e == Edge{key.first, key.second, -1}) { constrained = true; break; }
            if (!(count == 1 && constrained)) ++badEdges;
        }
        EXPECT_EQ(badEdges, 0)
            << badEdges << " element edges are neither interior (2 triangles) "
            << "nor constrained-boundary (1 triangle)";
    }
}
