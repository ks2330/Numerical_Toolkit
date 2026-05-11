#pragma once
#include <string>

namespace meshgeneration {

    enum class NodeType {
        Boundary,   // outer boundary polygon
        Hole,       // inner hole / aerofoil boundary
        Internal    // Poisson-sampled interior
    };

    struct Node {
        double x, y;
        int Node_id;
        NodeType type     = NodeType::Internal;
        int      group_id = -1;   // -1 = no group; maps to Mesh::boundaryGroups
    };

    struct Edge {
        int n0_id;
        int n1_id;
        int Edge_id;
        int group_id = -1;        // matches Node::group_id for the boundary it belongs to
    };

    struct Element {
        int n0_id, n1_id, n2_id;
        int Element_id;
    };

    struct Circumcircle {
        Node   center;
        double radius;
    };

}
