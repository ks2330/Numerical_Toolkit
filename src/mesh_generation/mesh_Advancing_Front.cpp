#include <vector>
#include <algorithm>
#include <iostream>
#include <cmath>
#include "mesh_generation/mesh_generation.h"
#include "mesh_generation/quadtree.h"

using namespace meshgeneration;

void run_Advancing_Front() {
    std::cout << "Advancing Front method is not yet implemented.\n";
}

void Mesh::AdvancingFront() {
    const double s = 0.01 * chord; // Minimum distance to consider for new node placement
    const int aerofoil_group_id = groupId("aerofoil");

    if (aerofoil_group_id == -1) {
        std::cerr << "Could not find the 'aerofoil' boundary group.\n";
        return;
    }

    std::vector<Edge> activeFront;


    for (const auto& edge : boundaryEdges) {
        if (edge.group_id == aerofoil_group_id) {
            activeFront.push_back(edge);
        }
    }

    for (const auto& edge : activeFront) {
        Node p_Ideal = RotateVector(getNodeByID(edge.n0_id), getNodeByID(edge.n1_id), M_PI / 3);
        std::vector<Node> nearbyNodes = NodesWithinDistanceAdvancingFront(p_Ideal, s);
        if (!nearbyNodes.empty()) {
            auto it = std::min_element(nearbyNodes.begin(), nearbyNodes.end(),
                [&p_Ideal](const Node& a, const Node& b) {
                    return distanceSquared(a, p_Ideal) < distanceSquared(b, p_Ideal);
                });
            p_Ideal = *it;
            // Check if p_Ideal is too close to the boundary or holes
            if (isSdistanceTooClose(p_Ideal, s, s) || GetClosestHoleDistance(p_Ideal) < s) {
                continue; // Skip this edge and try the next one
            }
            for (const auto& alledges : activeFront) {
                if (edgesIntersect({edge.n0_id, p_Ideal.Node_id, -1}, {alledges.n0_id, alledges.n1_id, -1})
                || edgesIntersect({edge.n1_id, p_Ideal.Node_id, -1}, {alledges.n0_id, alledges.n1_id, -1})) {
                    continue;
                }
            }
        }
        else {
            // No nearby nodes, so we can safely add p_Ideal
            Node newNode = {p_Ideal.x, p_Ideal.y, static_cast<int>(nodes.size()), NodeType::Internal, -1};
            insertNode(newNode);
            // Update active front and triangulate cavity
        }

    }
}
