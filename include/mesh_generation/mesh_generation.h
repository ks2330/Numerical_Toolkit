#pragma once
#include <string>
#include <iostream>
// include f 

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

        void initialize(std::string shape, double dim1, double dim2, int segsPerUnit) {
            if (shape == "circle") {
                isRectangular = false;
                double radius = dim1;
                int numSegments = static_cast<int>(dim2);
                double angleStep = 2.0 * 3.14159265358979323846 / numSegments;
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
        }

        void generateRandomNodes(int numNodes, double maxX, double maxY) {
            if (isRectangular) {
                for (int i = 0; i < numNodes; ++i) {
                    double x = static_cast<double>(rand()) / RAND_MAX * maxX;
                    double y = static_cast<double>(rand()) / RAND_MAX * maxY;
                    nodes.push_back({ x, y });
                }
            }
        }
        
        void generateLargeTriangle(double dim1, double dim2, double dim3) {
            double sizeFactor = 10.0;
            double val1 = -1.0 * (dim1 * sizeFactor);
            double val2 = dim2 + (dim2 * sizeFactor);
            double val3 = dim3 + (dim3 * sizeFactor);

            nodes.push_back({ val1, val2 });
            nodes.push_back({ val2, val3 });
            nodes.push_back({ val3, val1 });
        }

    private:
        bool isRectangular = false;
    };
}
