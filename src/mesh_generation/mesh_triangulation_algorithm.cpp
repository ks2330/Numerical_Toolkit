#include "mesh_generation/mesh_triangulation_algorithm.h"

namespace meshgen::triangulation
{

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