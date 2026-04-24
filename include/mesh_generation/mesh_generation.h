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
    };


    struct Element {
        int nodeIDs[3];   
    };


    class Mesh {
    public:
        std::vector<Node> nodes;
        std::vector<Element> elements;
        std::vector<Node> randomNodes;
        std::vector<Node> triangleNodes;

        void initialize(std::string shape, double dim1, double dim2, int segsPerUnit) {
            if (shape == "circle") {
                isRectangular = false;
                double radius = dim1;
                int numSegments = static_cast<int>(dim2);
                double angleStep = 2.0 * M_PI / numSegments;
                for (int i = 0; i < numSegments; ++i) {
                    double angle = i * angleStep;
                    nodes.push_back({ radius * cos(angle), radius * sin(angle) });
                }

            } else if (shape == "rectangle") {
                isRectangular = true;
                double width = dim1;
                double height = dim2;

                // 8 segments per unit length gives spacing of 0.125 — adjust for coarser/finer mesh
                // So in this example 
                const int nx = static_cast<int>(width  * segsPerUnit);
                const int ny = static_cast<int>(height * segsPerUnit);
                
                // Bottom edge (left → right), corners included
                for (int i = 0; i <= nx; ++i)
                    nodes.push_back({ i * width / nx, 0.0 });
                // Right edge (bottom → top), skip bottom-right corner
                for (int j = 1; j <= ny; ++j)
                    nodes.push_back({ width, j * height / ny });
                // Top edge (right → left), skip top-right corner
                for (int i = nx - 1; i >= 0; --i)
                    nodes.push_back({ i * width / nx, height });
                // Left edge (top → bottom), skip both corner nodes
                for (int j = ny - 1; j >= 1; --j)
                    nodes.push_back({ 0.0, j * height / ny });
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

                int numSegments = 12;
                double angleStep = 2.0 * M_PI / numSegments;
                for (int i = 0; i < numSegments; ++i) {
                    double angle = i * angleStep;
                    nodes.push_back({ radius * cos(angle) + dim1/2, radius * sin(angle) + dim2/2 });
                }

            }
        }

        std::vector<Node> generateRandomNodes(int numNodes, double dim1, double dim2) {

            if (isRectangular) {
                 for (int i = 0; i < numNodes; ++i) {
                    double x = static_cast<double>(rand()) / RAND_MAX * dim1;
                    double y = static_cast<double>(rand()) / RAND_MAX * dim2;
                    randomNodes.push_back({ x, y });
                }
            }
            if (isboth) {
                for (int i = 0; i < numNodes; ++i) {
                    double x = static_cast<double>(rand()) / RAND_MAX * dim1;
                    double y = static_cast<double>(rand()) / RAND_MAX * dim2;
                    randomNodes.push_back({ x, y });
                }
                double radius = std::min(dim1, dim2) / 2.0;
                for (auto it = randomNodes.begin(); it != randomNodes.end();) {
                    double dx = it->x - dim1/2;
                    double dy = it->y - dim2/2;
                    double distSq = dx*dx + dy*dy;
                    if (it->x < 0 || it->x > dim1 || it->y < 0 || it->y > dim2 || distSq < radius*radius) {
                        it = randomNodes.erase(it);
                    } else {
                        ++it;
                    }
                }   
            }
            return randomNodes;
        }

    private:
        bool isRectangular = false;
        bool isboth = false;
    };
}
