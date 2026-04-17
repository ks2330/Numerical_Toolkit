#pragma once

namespace meshgen::triangulation
{
    struct Vec2 {
    double x, y;
    };

    struct Triangle {
        Vec2 a, b, c;
    };

    bool isInCircle(Vec2 A, Vec2 B, Vec2 C, Vec2 D);


}


