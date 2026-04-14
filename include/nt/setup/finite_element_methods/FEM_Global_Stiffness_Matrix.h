#pragma once
#include <vector>
#include <iostream>

namespace nt::fem
{
    struct Node {
        double x, y; // The physical location (e.g., 0.0, 1.0)
    };

    struct Element {
        // The indices (IDs) of the 3 nodes that form this triangle.
        // In our 2x6 grid, these will be numbers between 0 and 20.
        int nodeIDs[3]; 
    };

    class Mesh {
    public:
        std::vector<Node> nodes;
        std::vector<Element> elements;

        // A simple function to initialize the nodes in a 2x6 grid.
        void initialize() {
            std::cout << "Initializing mesh nodes..." << std::endl;
            for (int i = 0; i <= 2; ++i) {
                for (int j = 0; j <= 6; ++j) {
                    nodes.push_back({ static_cast<double>(j), static_cast<double>(i)});
                }
            }
            std::cout << "Mesh nodes initialized." << std::endl;
            std::cout << "Initializing mesh elements..." << std::endl;
            // Create elements (triangles) for the 2x6 grid.
            for (int i = 0; i <= 2; ++i) {
                for (int j = 0; j <= 5; ++j) {
                    Element element;
                    element.nodeIDs[0] = i * 7 + j;       // Node at bottom left 
                    element.nodeIDs[1] = i * 7 + (j + 1); // Node at bottom right
                    element.nodeIDs[2] = (i + 1) * 7 + j; // Node at top left
                    element.nodeIDs[3] = (i + 1) * 7 + (j + 1); // Node at top right
                    
                    // We can split the rectangle into two triangles:
                    // Triangle 1: (bottom left, bottom right, top left)
                    elements.push_back({ element.nodeIDs[0], element.nodeIDs[1], element.nodeIDs[2] });
                    // Triangle 2: (bottom right, top left, top right)
                    elements.push_back({ element.nodeIDs[1], element.nodeIDs[2], element.nodeIDs[3] });
                    std::cout << "Element created with nodes: " 
                              << element.nodeIDs[0] << ", " 
                              << element.nodeIDs[1] << ", " 
                              << element.nodeIDs[2] << std::endl;
                    std::cout << i*j << " " << i << " " << j << std::endl;
                }
            
            }
            std::cout << "Mesh elements initialized." << std::endl;
        
        };
    };

}