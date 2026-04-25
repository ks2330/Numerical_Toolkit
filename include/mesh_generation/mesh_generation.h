#pragma once
#include <string>
#include <iostream>
#include <cmath> 

// This file will be used to generate simple shapes which will then
// be discretized into meshes for testing our FEM solvers. We will start with creating
// a circle and a rectangle, and then we can move on to more complex shapes.

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
                // but will this be touching the rectangle edge? We should esnure its always smaller than that to avoid degenerate triangles in the mesh
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

        

    private:
        bool isRectangular = false;
        bool isboth = false;
    };
}
