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

        Node operator+(const Node& other) const {
            return {x + other.x, y + other.y, -1};
        }

        Node operator-(const Node& other) const {
            return {x - other.x, y - other.y, -1};
        }

        Node operator*(double t) const {
            return {x * t, y * t, -1};
        }

        bool operator==(const Node& other) const {
            return Node_id == other.Node_id;
        }
    };

    struct Edge {
        int n0_id;
        int n1_id;
        int Edge_id;
        int group_id = -1;        // matches Node::group_id for the boundary it belongs to

        bool operator==(const Edge& other) const {
            return (n0_id == other.n0_id && n1_id == other.n1_id) ||
                   (n0_id == other.n1_id && n1_id == other.n0_id);
        }
    };

    struct Element {
        int n0_id, n1_id, n2_id;
        int Element_id;

        bool operator==(const Element& other) const {
            return Element_id == other.Element_id;
        }
    };

    struct Circumcircle {
        Node   center;
        double radius;
    };

}
