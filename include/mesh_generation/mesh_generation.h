#pragma once
#include <string>
#include <iostream>
// include f 

// This file will be used to generate simple shapes which will then
// be discretized into meshes for testing our FEM solvers. We will start with creating
// a circle and a rectangle, and then we can move on to more complex shapes.

#include <vector>
#include <cmath>

bool isRectangular = true; // Change to false to generate circular mesh


namespace meshgeneration {
    struct Node {
        double x, y; // The physical location (e.g., 0.0, 1.0)
    };

    struct Element {
        // The indices (IDs) of the 3 nodes that form this triangle.
        int nodeIDs[3]; 
    };

    class Mesh {
    public:
        std::vector<Node> nodes;
        std::vector<Element> elements;

        // A simple function to initialize the nodes in a 2x6 grid.
        void initialize(std::string shape, double dim1, double dim2) {
            if (shape == "circle") {
                // Initialize a circle mesh
                isRectangular = false;
                // Initialize a circle mesh
                double radius = dim1;
                int numSegments = static_cast<int>(dim2);
                double angleStep = 2.0 * 3.14159265358979323846 / numSegments;

                for (int i = 0; i < numSegments; ++i) {
                    double angle = i * angleStep;
                    nodes.push_back({ radius * cos(angle), radius * sin(angle) });
                }

            } else if (shape == "rectangle") {
                isRectangular = true;
                // Initialize a rectangle mesh
                double width = dim1;
                double height = dim2;
                nodes.push_back({ 0.0, 0.0 });
                nodes.push_back({ width, 0.0 });
                nodes.push_back({ width, height });
                nodes.push_back({ 0.0, height });
                // add more nodes to create a finer mesh
                for (int i = 1; i < 6; ++i) {
                    nodes.push_back({ i * width / 6.0, 0.0 });
                    nodes.push_back({ i * width / 6.0, height });
                }
                for (int j = 1; j < 2; ++j) {
                    nodes.push_back({ 0.0, j * height / 10.0 });
                    nodes.push_back({ width, j * height / 10.0 });
                }

            }
        }

        void generateRandomNodes(int numNodes, double maxX, double maxY) {
            if (isRectangular) {
                // Initialize a rectangle mesh

                for (int i = 0; i < numNodes; ++i) {
                    double x = static_cast<double>(rand()) / RAND_MAX * maxX;
                    double y = static_cast<double>(rand()) / RAND_MAX * maxY;
                    nodes.push_back({ x, y });
                }
            }
        }
    };
}
