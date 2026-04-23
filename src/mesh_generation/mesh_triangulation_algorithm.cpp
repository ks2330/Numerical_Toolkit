#include <iostream>
#include <cmath>
#include <vector>
#include "mesh_generation/mesh_triangulation_algorithm.h"

namespace meshgen::triangulation
{

    // Draw Circle ABC and 
    std::vector<Vec2> drawCircle(Vec2 A, Vec2 B, Vec2 C, int numSegments) {
        // Calculate circumcenter of triangle ABC
        double D = 2 * (A.x * (B.y - C.y) + B.x * (C.y - A.y) + C.x * (A.y - B.y));
        double Ux = ((A.x * A.x + A.y * A.y) * (B.y - C.y) + (B.x * B.x + B.y * B.y) * (C.y - A.y) + (C.x * C.x + C.y * C.y) * (A.y - B.y)) / D;
        double Uy = ((A.x * A.x + A.y * A.y) * (C.x - B.x) + (B.x * B.x + B.y * B.y) * (A.x - C.x) + (C.x * C.x + C.y * C.y) * (B.x - A.x)) / D;

        Vec2 center = { Ux, Uy };
        double radius = sqrt((center.x - A.x) * (center.x - A.x) + (center.y - A.y) * (center.y - A.y));

        // Generate points on the circumcircle
        std::vector<Vec2> circlePoints;
        for (int i = 0; i < numSegments; ++i) {
            double angle = 2.0 * M_PI * i / numSegments;
            double x = center.x + radius * cos(angle);
            double y = center.y + radius * sin(angle);
            circlePoints.push_back({x, y});
        }
        return circlePoints;
    }



    // This function returns true if Point D is inside the circumcircle of ABC
    bool isInCircle(meshgen::triangulation::Vec2 A, meshgen::triangulation::Vec2 B, meshgen::triangulation::Vec2 C, meshgen::triangulation::Vec2 D) {
        // Translate points so that D is at the origin (0,0)
        // This simplifies the matrix math significantly
        double adx = A.x - D.x;
        double ady = A.y - D.y;
        double bdx = B.x - D.x;
        double bdy = B.y - D.y;
        double cdx = C.x - D.x;
        double cdy = C.y - D.y;

        // Calculate the determinant of the 3x3 matrix
        double det = (adx * adx + ady * ady) * (bdx * cdy - cdx * bdy) -
                    (bdx * bdx + bdy * bdy) * (adx * cdy - cdx * ady) +
                    (cdx * cdx + cdy * cdy) * (adx * bdy - bdx * ady);

        return det > 0;
    }

} 