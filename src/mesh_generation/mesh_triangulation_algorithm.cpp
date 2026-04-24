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

    bool isSameEdge(Triangle t, Edge e2) {
        auto eq = [](Edge a, Edge b) {
            return (a.a.x == b.a.x && a.a.y == b.a.y && a.b.x == b.b.x && a.b.y == b.b.y) ||
                   (a.a.x == b.b.x && a.a.y == b.b.y && a.b.x == b.a.x && a.b.y == b.a.y);
        };
        return eq({t.a, t.b}, e2) || eq({t.b, t.c}, e2) || eq({t.c, t.a}, e2);
    }

    std::vector<Vec2> generateLargeTriangle(double dim1, double dim2, double dim3, double sizeFactor) {
        std::vector<Vec2> triangleNodes;
        double val1 = -1.0 * (dim1 * sizeFactor);
        double val2 = dim2 + (dim2 * sizeFactor);
        double val3 = dim3 + (dim3 * sizeFactor);
        triangleNodes.push_back({ val1, val2 });
        triangleNodes.push_back({ val2, val3 });
        triangleNodes.push_back({ val3, val1 });
        return triangleNodes;
    }

    std::vector<Triangle> bowyerWatson(std::vector<Vec2> points, double width, double height) {
        std::vector<Vec2> superTriangleNodes = generateLargeTriangle(width, height, height, 10);
        Vec2 c0 = superTriangleNodes[0];
        Vec2 c1 = superTriangleNodes[1];
        Vec2 c2 = superTriangleNodes[2];

        std::vector<Triangle> triangles = { {c0, c2, c1} }; // CCW winding required by isInCircle

        for(const auto& point : points){
            std::vector<Triangle> badTriangles;
            for(const auto& triangle : triangles){
                if(isInCircle(triangle.a, triangle.b, triangle.c, point)){
                    badTriangles.push_back(triangle);
                }
            }

            std::vector<Edge> polygon;
            for (const auto& triangle : badTriangles){
                std::vector<Edge> edges = {{triangle.a, triangle.b}, {triangle.b, triangle.c}, {triangle.c, triangle.a}};
                for (const auto& edge : edges){
                    int count = 0;
                    for (const auto& other : badTriangles){
                        if (isSameEdge(other, edge)){
                            count++;
                        }
                    }
                    if (count == 1){
                        polygon.push_back(edge);
                    }
                }
            }

            triangles.erase(std::remove_if(triangles.begin(), triangles.end(), [&](const Triangle& t){
                for(const auto& bad : badTriangles){
                    if(t.a.x == bad.a.x && t.a.y == bad.a.y &&
                       t.b.x == bad.b.x && t.b.y == bad.b.y &&
                       t.c.x == bad.c.x && t.c.y == bad.c.y) return true;
                }
                return false;
            }), triangles.end());

            for (const auto& edge : polygon){
                triangles.push_back({edge.a, edge.b, point});
            }
        }

        // Remove all triangles that share a vertex with the super-triangle
        triangles.erase(std::remove_if(triangles.begin(), triangles.end(), [&](const Triangle& t){
            return (t.a.x == c0.x && t.a.y == c0.y) || (t.b.x == c0.x && t.b.y == c0.y) || (t.c.x == c0.x && t.c.y == c0.y) ||
                   (t.a.x == c1.x && t.a.y == c1.y) || (t.b.x == c1.x && t.b.y == c1.y) || (t.c.x == c1.x && t.c.y == c1.y) ||
                   (t.a.x == c2.x && t.a.y == c2.y) || (t.b.x == c2.x && t.b.y == c2.y) || (t.c.x == c2.x && t.c.y == c2.y);
        }), triangles.end());

        return triangles;
    }

}
