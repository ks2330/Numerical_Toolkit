#pragma once
#include <string>
#include <iostream>
#include <cmath> 
#include <algorithm>    

#include <vector>
#include <cmath>

namespace meshgeneration {
    struct Node {
        double x, y;
        int Node_id;
    };


    struct Element {
        Node a, b, c;  
        int Element_id;
    };

    struct Edge {
        Node a, b;
        int Edge_id;
    };

    struct Circumcircle {
        Node   center;
        double radius;
    };


    class Mesh {
    public:
        std::vector<Node> nodes;
        std::vector<Element> elements;

        void initialize(std::string shape, double dim1, double dim2, int segsPerUnit) {
            if (shape == "circle") {
                isRectangular = false;
                double radius = dim1;
                int numSegments = static_cast<int>(dim2);
                double angleStep = 2.0 * M_PI / numSegments;
                for (int i = 0; i < numSegments; ++i) {
                    double angle = i * angleStep;                    
                    nodes.push_back({ radius * cos(angle), radius * sin(angle), i });

                }

            } else if (shape == "rectangle") {
                isRectangular = true;
                double width = dim1;
                double height = dim2;

                // 8 segments per unit length gives spacing of 0.125 — adjust for coarser/finer mesh
                // So in this example 
                const int nx = static_cast<int>(width  * segsPerUnit);
                const int ny = static_cast<int>(height * segsPerUnit);
                int id_counter = 0;
                // Bottom edge (left → right), corners included
                for (int i = 0; i <= nx; ++i)
                    nodes.push_back({ i * width / nx, 0.0, id_counter++ });
                // Right edge (bottom → top), skip bottom-right corner
                for (int j = 1; j <= ny; ++j)
                    nodes.push_back({ width, j * height / ny, id_counter++ });
                // Top edge (right → left), skip top-right corner
                for (int i = nx - 1; i >= 0; --i)
                    nodes.push_back({ i * width / nx, height, id_counter++ });
                // Left edge (top → bottom), skip both corner nodes
                for (int j = ny - 1; j >= 1; --j)
                    nodes.push_back({ 0.0, j * height / ny, id_counter++ });
                // const int totalNodes = static_cast<int>(nodes.size()); // This local variable is unused.
            }
            if (shape == "triangle") {
                isRectangular = false;
                // generateLargeTriangle(dim1, dim2, dim2, 1.0);
            }

            if (shape == "both") {
                isboth = true;
                initialize("rectangle", dim1, dim2, segsPerUnit);
                // std::min(dim1, dim2) / 2.0 gives radius for inscribed circle
                // but will this be touching the rectangle edge? We should esnure its always smaller than that to avoid degenerate elements in the mesh
                isRectangular = false;
                double radius = std::min(dim1, dim2) / 3.0;

                size_t starting_id = nodes.size();
                int numSegments = 12;
                double angleStep = 2.0 * M_PI / numSegments;
                for (int i = 0; i < numSegments; ++i) {
                    double angle = i * angleStep;
                    nodes.push_back({ radius * cos(angle) + dim1/2, radius * sin(angle) + dim2/2, static_cast<int>(starting_id + i) });
                }

            }
        }

        // Generates random interior nodes and adds them to the `nodes` vector.
        void generateRandomNodes(int numNodes, double dim1, double dim2) {
            int id_counter = nodes.size();

            if (isboth) {
                double radius = std::min(dim1, dim2) / 3.0; // Match the radius from initialize()
                double radiusSq = radius * radius;
                int nodes_generated = 0;
                while (nodes_generated < numNodes) {
                    double x = static_cast<double>(rand()) / RAND_MAX * dim1;
                    double y = static_cast<double>(rand()) / RAND_MAX * dim2;

                    // Check if it's outside the inner circle
                    double dx = x - dim1/2;
                    double dy = y - dim2/2;
                    if (dx*dx + dy*dy >= radiusSq) {
                        nodes.push_back({x, y, id_counter++});
                        nodes_generated++;
                    }
                }
            } else if (isRectangular) {
                for (int i = 0; i < numNodes; ++i) {
                    double x = static_cast<double>(rand()) / RAND_MAX * dim1;
                    double y = static_cast<double>(rand()) / RAND_MAX * dim2;
                    nodes.push_back({ x, y, id_counter++ });
                }
            }
        }

        // Runs the Bowyer-Watson algorithm on the mesh's nodes and populates the elements vector.
        void triangulate(double width, double height) {
            if (nodes.empty()) {
                return;
            }
            elements = bowyerWatson(nodes, width, height);
        }
        
    private:
        bool isRectangular = false;
        bool isboth = false;

        static Circumcircle drawCircle(Node A, Node B, Node C) {
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
        static bool isInCircle(Node A, Node B, Node C, Node D) {
            double adx = A.x - D.x, ady = A.y - D.y;
            double bdx = B.x - D.x, bdy = B.y - D.y;
            double cdx = C.x - D.x, cdy = C.y - D.y;

            double det = (adx*adx + ady*ady) * (bdx*cdy - cdx*bdy) -
                        (bdx*bdx + bdy*bdy) * (adx*cdy - cdx*ady) +
                        (cdx*cdx + cdy*cdy) * (adx*bdy - bdx*ady);
            return det > 0;
        }

        static bool isSameEdge(Element t, Edge e2) {
            auto eq = [](Edge a, Edge b) {
                return (a.a.x == b.a.x && a.a.y == b.a.y && a.b.x == b.b.x && a.b.y == b.b.y) ||
                    (a.a.x == b.b.x && a.a.y == b.b.y && a.b.x == b.a.x && a.b.y == b.a.y);
            };
            return eq({t.a, t.b, -1}, e2) || eq({t.b, t.c, -1}, e2) || eq({t.c, t.a, -1}, e2);
        }

        static std::vector<Node> generateLargeTriangle(double dim1, double dim2, double dim3, double sizeFactor) {
            std::vector<Node> triangleNodes;
            double val1 = -1.0 * (dim1 * sizeFactor);
            double val2 = dim2 + (dim2 * sizeFactor);
            double val3 = dim3 + (dim3 * sizeFactor);
            triangleNodes.push_back({ val1, val2, -1 }); // Super-triangle nodes get negative IDs
            triangleNodes.push_back({ val2, val3, -2 });
            triangleNodes.push_back({ val3, val1, -3 });
            return triangleNodes;
        }

        // This function implements the Bowyer-Watson algorithm for Delaunay triangulation
        static std::vector<Element> bowyerWatson(const std::vector<Node>& points, double width, double height) {
            // This Creates a super-triangle that encompasses all the points in the input set. 
            // The sizeFactor can be adjusted to ensure it is sufficiently large.
            std::vector<Node> superTriangleNodes = generateLargeTriangle(width, height, height, 10);
            Node n0 = superTriangleNodes[0];
            Node n1 = superTriangleNodes[1];
            Node n2 = superTriangleNodes[2];
            // This Creates a vector of elements, initially containing just the super-triangle.
            std::vector<Element> triangulation_elements = { {n0, n2, n1, -1} };

            // This Iterates over each point in the input set and performs the following steps:
            // a. Identifies all elements in the current triangulation whose circumcircles contain the point 
            // (these are the "bad" elements that will be removed).
            // b. Constructs the polygonal hole formed by the edges of the bad elements that are not shared with any other bad triangle.

            for(const auto& point : points){
                std::vector<Element> badTriangles;
                for(const auto& triangle : triangulation_elements){
                    if(isInCircle(triangle.a, triangle.b, triangle.c, point)){
                        badTriangles.push_back(triangle);
                    }
                }

                std::vector<Edge> polygon;
                for (const auto& triangle : badTriangles){
                    std::vector<Edge> edges = {{triangle.a, triangle.b, -1}, {triangle.b, triangle.c, -1}, {triangle.c, triangle.a, -1}};
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

                triangulation_elements.erase(std::remove_if(triangulation_elements.begin(), triangulation_elements.end(), [&](const Element& t){
                    for(const auto& bad : badTriangles){
                        if(t.a.x == bad.a.x && t.a.y == bad.a.y &&
                        t.b.x == bad.b.x && t.b.y == bad.b.y &&
                        t.c.x == bad.c.x && t.c.y == bad.c.y) return true;
                    }
                    return false;
                }), triangulation_elements.end());

                for (const auto& edge : polygon){
                    triangulation_elements.push_back({edge.a, edge.b, point, -1});
                }
            }

            // Remove all elements that share a vertex with the super-triangle
            triangulation_elements.erase(std::remove_if(triangulation_elements.begin(), triangulation_elements.end(), [&](const Element& t){
                return (t.a.x == n0.x && t.a.y == n0.y) || (t.b.x == n0.x && t.b.y == n0.y) || (t.c.x == n0.x && t.c.y == n0.y) ||
                    (t.a.x == n1.x && t.a.y == n1.y) || (t.b.x == n1.x && t.b.y == n1.y) || (t.c.x == n1.x && t.c.y == n1.y) ||
                    (t.a.x == n2.x && t.a.y == n2.y) || (t.b.x == n2.x && t.b.y == n2.y) || (t.c.x == n2.x && t.c.y == n2.y);
            }), triangulation_elements.end());

            return triangulation_elements;
        }
    };
}
