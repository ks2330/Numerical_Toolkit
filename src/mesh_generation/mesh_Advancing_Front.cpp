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

    const int aerofoil_group_id = groupId("aerofoil");
    const int boundary_group_id = groupId("outer");


    if (aerofoil_group_id == -1) {
        std::cerr << "Could not find the 'aerofoil' boundary group.\n";
        return;
    }

    if (boundary_group_id == -1) {
        std::cerr << "Could not find the 'boundary' boundary group.\n";
        return;
    }

    std::cout << "Running Advancing Front \n";

    std::vector<Edge> activeFront;


    for (const auto& edge : boundaryEdges) {
        if (edge.group_id == aerofoil_group_id || edge.group_id == boundary_group_id) {
            activeFront.push_back(edge);
        }
    }

    int maxIterations = 100000;
    int iter = 0;
    while (!activeFront.empty() && iter++ < maxIterations) {
        Edge edge = activeFront.front();
        activeFront.erase(activeFront.begin());
        double edgeLen = distance(getNodeByID(edge.n0_id), getNodeByID(edge.n1_id));
        double s = 10 * edgeLen;


        bool ccw = (edge.group_id == 0) ? isCCW(boundaryNodes) : isCCW(holeNodes);
        double angle = ccw ? -M_PI/3 : M_PI/3;

        Node p_Ideal = RotateVector(getNodeByID(edge.n0_id), getNodeByID(edge.n1_id), angle);
        std::vector<Node> nearbyNodes = NodesWithinDistanceAdvancingFront(p_Ideal, s);


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
                activeFront.push_back(edge);
                continue;
            }
        }
        else {
            // No nearby nodes — add a new node directly (no Bowyer-Watson)
            Node newNode = {p_Ideal.x, p_Ideal.y, static_cast<int>(nodes.size()), NodeType::Internal, -1};
            nodes.push_back(newNode);
            internalNodes.push_back(newNode);
            node_quadtree->insert(newNode);
            p_Ideal = newNode;
        }
        Edge newEdge1 = {edge.n0_id, p_Ideal.Node_id, -1};
        auto it1 = std::find_if(activeFront.begin(), activeFront.end(),
            [&](const Edge& e) { return edgesMatch(e, newEdge1); });

        if (it1 != activeFront.end())
            activeFront.erase(it1);
        else
            activeFront.push_back(newEdge1);
        
        Edge newEdge2 = {p_Ideal.Node_id, edge.n1_id, -1};
        auto it2 = std::find_if(activeFront.begin(), activeFront.end(),
            [&](const Edge& e) { return edgesMatch(e, newEdge2); });

        if (it2 != activeFront.end())
            activeFront.erase(it2);
        else
            activeFront.push_back(newEdge2);

        elements.push_back({edge.n0_id, edge.n1_id, p_Ideal.Node_id, element_id_counter++});
        if (iter % 100 == 0)
        std::cout << "iter " << iter << " front size: " << activeFront.size() << "\n";

    }
}
