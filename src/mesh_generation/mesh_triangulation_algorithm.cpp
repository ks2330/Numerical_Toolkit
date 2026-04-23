#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
#include "mesh_generation/mesh_triangulation_algorithm.h"

namespace meshgen::triangulation
{

    Circumcircle drawCircle(Vec2 A, Vec2 B, Vec2 C) {
        double D  = 2 * (A.x * (B.y - C.y) + B.x * (C.y - A.y) + C.x * (A.y - B.y));
        double Ux = ((A.x*A.x + A.y*A.y) * (B.y - C.y) + (B.x*B.x + B.y*B.y) * (C.y - A.y) + (C.x*C.x + C.y*C.y) * (A.y - B.y)) / D;
        double Uy = ((A.x*A.x + A.y*A.y) * (C.x - B.x) + (B.x*B.x + B.y*B.y) * (A.x - C.x) + (C.x*C.x + C.y*C.y) * (B.x - A.x)) / D;

        Circumcircle result;
        result.center = {Ux, Uy};
        result.radius = sqrt((result.center.x - A.x)*(result.center.x - A.x) +
                             (result.center.y - A.y)*(result.center.y - A.y));
        return result;
    }

    // Returns true if Point D is inside the circumcircle of ABC
    bool isInCircle(Vec2 A, Vec2 B, Vec2 C, Vec2 D) {
        double adx = A.x - D.x, ady = A.y - D.y;
        double bdx = B.x - D.x, bdy = B.y - D.y;
        double cdx = C.x - D.x, cdy = C.y - D.y;

        double det = (adx*adx + ady*ady) * (bdx*cdy - cdx*bdy) -
                     (bdx*bdx + bdy*bdy) * (adx*cdy - cdx*ady) +
                     (cdx*cdx + cdy*cdy) * (adx*bdy - bdx*ady);
        return det > 0;
    }

    std::vector<Triangle> bowyerWatson(std::vector<Vec2> points, double width, double height) {
        Vec2 c0 = {0.0,   0.0  };
        Vec2 c1 = {width, 0.0  };
        Vec2 c2 = {width, height};
        Vec2 c3 = {0.0,   height};

        std::vector<Triangle> triangles = { {c0, c1, c2}, {c0, c2, c3} };

        // TODO: insert points one by one using the Bowyer-Watson algorithm

        return triangles;
    }

}
