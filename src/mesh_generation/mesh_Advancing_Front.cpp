#include <vector>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include "mesh_generation/mesh_generation.h"
#include "mesh_generation/quadtree.h"

using namespace meshgeneration;

void run_Advancing_Front() {
    std::cout << "Advancing Front method is not yet implemented.\n";
}

void Mesh::advancingFront() {

    const int aerofoil_group_id = groupId("aerofoil");
    const int boundary_group_id = groupId("outer");

    if (aerofoil_group_id == -1)
        throw std::runtime_error("advancingFront: 'aerofoil' boundary group not registered");

    if (boundary_group_id == -1)
        throw std::runtime_error("advancingFront: 'outer' boundary group not registered");

    std::cout << "Running Advancing Front\n";

    // Rebuild quadtree so ALL nodes (boundary, hole, boundary-layer internal) are
    // queryable without duplicates.  generateRandomNodes may have run before this and
    // only inserted its own Poisson nodes; boundaryLayerSeeding never inserts to the
    // quadtree at all, so those nodes would be invisible to the search otherwise.
    {
        std::vector<Node> bbox = GetBoundingBox(boundaryNodes);
        double cx = (bbox[0].x + bbox[1].x) / 2.0;
        double cy = (bbox[0].y + bbox[2].y) / 2.0;
        double hw = (bbox[1].x - bbox[0].x) / 2.0 * 1.1;
        double hh = (bbox[2].y - bbox[0].y) / 2.0 * 1.1;
        node_quadtree = std::make_unique<Quadtree>(AABB{cx, cy, hw, hh});
        for (const auto& n : nodes)
            node_quadtree->insert(n);
    }

    std::vector<Edge> activeFront;
    for (const auto& edge : boundaryEdges) {
        if (edge.group_id == aerofoil_group_id)
            activeFront.push_back(edge);
    }


    const bool boundary_ccw = isCCW(boundaryNodes);
    const bool hole_ccw     = !holeNodes.empty() && isCCW(holeNodes);

    int maxIterations = 10000;
    int iter = 0;
    int n_found = 0, n_rejected = 0, n_new = 0, n_degen = 0, n_outside = 0;
    while (!activeFront.empty() && iter++ < maxIterations) {
        Edge edge = activeFront.front();
        activeFront.erase(activeFront.begin());

        double edgeLen = distance(getNodeByID(edge.n0_id), getNodeByID(edge.n1_id));

        double s = 1.5 * edgeLen;

        double angle;
        if (edge.group_id == boundary_group_id)
            angle = boundary_ccw ?  M_PI/3 : -M_PI/3;
        else
            angle = hole_ccw    ? -M_PI/3 :  M_PI/3;

        Node p_Ideal = RotateVector(getNodeByID(edge.n0_id), getNodeByID(edge.n1_id), angle);
        std::vector<Node> nearbyNodes = nodesWithinDistanceAdvancingFront(p_Ideal, s);

        nearbyNodes.erase(std::remove_if(nearbyNodes.begin(), nearbyNodes.end(),
            [&](const Node& n) { return n.Node_id == edge.n0_id || n.Node_id == edge.n1_id; }),
            nearbyNodes.end());

        // Orientation filter: only keep candidates on the same (unmeshed) side of the
        // edge as p_Ideal. Without this, the search can pick nodes from the meshed side
        // (already-built triangles), creating overlapping elements and growing the front.
        {
            const Node& na = getNodeByID(edge.n0_id);
            const Node& nb = getNodeByID(edge.n1_id);
            const double ideal_side = orient2d(na, nb, p_Ideal);
            nearbyNodes.erase(std::remove_if(nearbyNodes.begin(), nearbyNodes.end(),
                [&](const Node& c) { return orient2d(na, nb, c) * ideal_side <= 0; }),
                nearbyNodes.end());
        }

        if (!nearbyNodes.empty()) {
            std::sort(nearbyNodes.begin(), nearbyNodes.end(),
                [&p_Ideal](const Node& a, const Node& b) {
                    return distanceSquared(a, p_Ideal) < distanceSquared(b, p_Ideal);
                });

            bool found = false;
            for (const auto& candidate : nearbyNodes) {
                bool intersects = false;
                for (const auto& front_edge : activeFront) {
                    if (edgesIntersect({edge.n0_id, candidate.Node_id, -1}, {front_edge.n0_id, front_edge.n1_id, -1})
                    || edgesIntersect({edge.n1_id, candidate.Node_id, -1}, {front_edge.n0_id, front_edge.n1_id, -1})) {
                        intersects = true;
                        break;
                    }
                }
                if (!intersects) {
                    p_Ideal = candidate;
                    found = true;
                    break;
                }
            }
            if (!found) {
                ++n_rejected;
                activeFront.push_back(edge);
                continue;
            }
            ++n_found;
        } else {
            // Reject new nodes that would land outside the outer domain.
            if (!isPointInPolygon(p_Ideal, boundaryNodes)) {
                ++n_outside;
                activeFront.push_back(edge);
                continue;
            }
            Node newNode = {p_Ideal.x, p_Ideal.y, static_cast<int>(nodes.size()), NodeType::Internal, -1};
            nodes.push_back(newNode);
            internalNodes.push_back(newNode);
            node_quadtree->insert(newNode);
            p_Ideal = newNode;
            ++n_new;
        }

        // Degenerate-triangle guard BEFORE touching activeFront.  The original code
        // modified the front first and then skipped on a near-zero orient2d, which left
        // orphaned half-edges in the active set and corrupted all subsequent iterations.
        const Node& na = getNodeByID(edge.n0_id);
        const Node& nb = getNodeByID(edge.n1_id);
        if (std::abs(orient2d(na, nb, p_Ideal)) < 1e-10) {
            ++n_degen;
            activeFront.push_back(edge);
            continue;
        }

        elements.push_back({edge.n0_id, edge.n1_id, p_Ideal.Node_id, element_id_counter++});

        // Propagate group_id to new front edges so they use the correct angle formula
        // for the boundary they originated from.  Without this every new edge got
        // group_id=-1 and fell into the hole branch, flipping the advance direction for
        // edges that came from the outer boundary.
        Edge newEdge1 = {edge.n0_id, p_Ideal.Node_id, -1, edge.group_id};
        auto it1 = std::find_if(activeFront.begin(), activeFront.end(),
            [&](const Edge& e) { return e == newEdge1; });
        if (it1 != activeFront.end())
            activeFront.erase(it1);
        else
            activeFront.push_back(newEdge1);

        Edge newEdge2 = {p_Ideal.Node_id, edge.n1_id, -1, edge.group_id};
        auto it2 = std::find_if(activeFront.begin(), activeFront.end(),
            [&](const Edge& e) { return e == newEdge2; });
        if (it2 != activeFront.end())
            activeFront.erase(it2);
        else
            activeFront.push_back(newEdge2);

        if (iter % 500 == 0)
            std::cout << "iter " << iter << " front size: " << activeFront.size() << "\n";
    }
    std::cout << "Loop exited. iter=" << iter << " front remaining=" << activeFront.size() << "\n";
    std::cout << "Size of Elements: " << elements.size() << "\n";
    std::cout << "Counters: found=" << n_found
              << " rejected=" << n_rejected
              << " new=" << n_new
              << " outside=" << n_outside
              << " degen=" << n_degen << "\n";
}
