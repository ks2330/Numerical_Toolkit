#pragma once
#include <vector>
#include "mesh_generation/mesh_generation.h"

namespace meshgen::triangulation
{
    struct Edge {
        meshgeneration::Node a, b;
    };

    struct Triangle {
        meshgeneration::Node a, b, c;
    };

    struct Circumcircle {
        meshgeneration::Node center;
        double radius;
    };

    Circumcircle          drawCircle(meshgeneration::Node A, meshgeneration::Node B, meshgeneration::Node C);
    bool                  isInCircle(meshgeneration::Node A, meshgeneration::Node B, meshgeneration::Node C, meshgeneration::Node D);
    bool                  isSameEdge(meshgeneration::Element t, Edge e2);
    std::vector<meshgeneration::Node>   generateLargeTriangle(double dim1, double dim2, double dim3, double sizeFactor);
    std::vector<meshgeneration::Element> bowyerWatson(const std::vector<meshgeneration::Node>& points, double width, double height);
}
