#pragma once

namespace meshgeneration {

    struct Node {
        double x, y;
        int Node_id;
    };

    struct Edge {
        int n0_id;
        int n1_id;
        int Edge_id;
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
