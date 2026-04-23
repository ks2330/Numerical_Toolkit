#pragma once
#include <vector>

namespace meshgen::triangulation
{
    struct Vec2 {
    double x, y;
    };

    struct Triangle {
        Vec2 a, b, c;
    };

    std::vector<Vec2> drawCircle(Vec2 A, Vec2 B, Vec2 C, int numSegments);

    bool isInCircle(Vec2 A, Vec2 B, Vec2 C, Vec2 D);


}


