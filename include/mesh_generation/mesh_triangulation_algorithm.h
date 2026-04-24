#pragma once
#include <vector>

namespace meshgen::triangulation
{
    struct Vec2 {
        double x, y;
    };

    struct Edge {
        Vec2 a, b;
    };

    struct Triangle {
        Vec2 a, b, c;
    };

    struct Circumcircle {
        Vec2   center;
        double radius;
    };

    Circumcircle          drawCircle(Vec2 A, Vec2 B, Vec2 C);
    bool                  isInCircle(Vec2 A, Vec2 B, Vec2 C, Vec2 D);
    bool                  isSameEdge(Triangle t, Edge e2);
    std::vector<Vec2>    generateLargeTriangle(double dim1, double dim2, double dim3, double sizeFactor);
    std::vector<Triangle> bowyerWatson(std::vector<Vec2> points, double width, double height);
}
