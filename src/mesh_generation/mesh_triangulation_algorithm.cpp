#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
#include "mesh_generation/mesh_triangulation_algorithm.h"
#include "mesh_generation/mesh_generation.h"

namespace meshgen::triangulation
{

    Circumcircle drawCircle(meshgeneration::Node A, meshgeneration::Node B, meshgeneration::Node C) {
        double D  = 2 * (A.x * (B.y - C.y) + B.x * (C.y - A.y) + C.x * (A.y - B.y));
        double Ux = ((A.x*A.x + A.y*A.y) * (B.y - C.y) + (B.x*B.x + B.y*B.y) * (C.y - A.y) + (C.x*C.x + C.y*C.y) * (A.y - B.y)) / D;
        double Uy = ((A.x*A.x + A.y*A.y) * (C.x - B.x) + (B.x*B.x + B.y*B.y) * (A.x - C.x) + (C.x*C.x + C.y*C.y) * (B.x - A.x)) / D;

        Circumcircle result;
        result.center = {Ux, Uy, -1}; // Assign a temporary ID for the center
        result.radius = sqrt((result.center.x - A.x)*(result.center.x - A.x) +
                             (result.center.y - A.y)*(result.center.y - A.y));
        return result;
    }

    // Returns true if Point D is inside the circumcircle of ABC
    bool isInCircle(meshgeneration::Node A, meshgeneration::Node B, meshgeneration::Node C, meshgeneration::Node D) {
        double adx = A.x - D.x, ady = A.y - D.y;
        double bdx = B.x - D.x, bdy = B.y - D.y;
        double cdx = C.x - D.x, cdy = C.y - D.y;

        double det = (adx*adx + ady*ady) * (bdx*cdy - cdx*bdy) -
                     (bdx*bdx + bdy*bdy) * (adx*cdy - cdx*ady) +
                     (cdx*cdx + cdy*cdy) * (adx*bdy - bdx*ady);
        return det > 0;
    }

    bool isSameEdge(meshgeneration::Element t, Edge e2) {
        auto eq = [](Edge a, Edge b) {
            return (a.a.x == b.a.x && a.a.y == b.a.y && a.b.x == b.b.x && a.b.y == b.b.y) ||
                   (a.a.x == b.b.x && a.a.y == b.b.y && a.b.x == b.a.x && a.b.y == b.a.y);
        };
        return eq({t.a, t.b}, e2) || eq({t.b, t.c}, e2) || eq({t.c, t.a}, e2);
    }

    std::vector<meshgeneration::Node> generateLargeTriangle(double dim1, double dim2, double dim3, double sizeFactor) {
        std::vector<meshgeneration::Node> triangleNodes;
        double val1 = -1.0 * (dim1 * sizeFactor);
        double val2 = dim2 + (dim2 * sizeFactor);
        double val3 = dim3 + (dim3 * sizeFactor);
        triangleNodes.push_back({ val1, val2, -1 }); // Super-triangle nodes get negative IDs
        triangleNodes.push_back({ val2, val3, -2 });
        triangleNodes.push_back({ val3, val1, -3 });
        return triangleNodes;
    }

    // This function implements the Bowyer-Watson algorithm for Delaunay triangulation
    std::vector<meshgeneration::Element> bowyerWatson(const std::vector<meshgeneration::Node>& points, double width, double height) {
        // This Creates a super-triangle that encompasses all the points in the input set. 
        // The sizeFactor can be adjusted to ensure it is sufficiently large.
        std::vector<meshgeneration::Node> superTriangleNodes = generateLargeTriangle(width, height, height, 10);
        meshgeneration::Node n0 = superTriangleNodes[0];
        meshgeneration::Node n1 = superTriangleNodes[1];
        meshgeneration::Node n2 = superTriangleNodes[2];
        // This Creates a vector of triangles, initially containing just the super-triangle.
        std::vector<meshgeneration::Element> triangles = { {n0, n2, n1} };

        // This Iterates over each point in the input set and performs the following steps:
        // a. Identifies all triangles in the current triangulation whose circumcircles contain the point 
        // (these are the "bad" triangles that will be removed).
        // b. Constructs the polygonal hole formed by the edges of the bad triangles that are not shared with any other bad triangle.

        for(const auto& point : points){
            std::vector<meshgeneration::Element> badTriangles;
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

            triangles.erase(std::remove_if(triangles.begin(), triangles.end(), [&](const meshgeneration::Element& t){
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
        triangles.erase(std::remove_if(triangles.begin(), triangles.end(), [&](const meshgeneration::Element& t){
            return (t.a.x == n0.x && t.a.y == n0.y) || (t.b.x == n0.x && t.b.y == n0.y) || (t.c.x == n0.x && t.c.y == n0.y) ||
                   (t.a.x == n1.x && t.a.y == n1.y) || (t.b.x == n1.x && t.b.y == n1.y) || (t.c.x == n1.x && t.c.y == n1.y) ||
                   (t.a.x == n2.x && t.a.y == n2.y) || (t.b.x == n2.x && t.b.y == n2.y) || (t.c.x == n2.x && t.c.y == n2.y);
        }), triangles.end());

        return triangles;
    }

}
