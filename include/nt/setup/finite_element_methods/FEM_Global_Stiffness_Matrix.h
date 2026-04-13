#pragma once
#include <vector>

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
            // Height (Y) goes from 0 to 2 (3 rows)
            for (int y = 0; y <= 2; ++y) { 
                // Width (X) goes from 0 to 6 (7 columns)
                for (int x = 0; x <= 6; ++x) { 
                    nodes.push_back({static_cast<double>(x), static_cast<double>(y)});
                }
            }
        }
    };
    
}