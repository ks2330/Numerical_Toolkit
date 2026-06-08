#pragma once
#include <vector>
#include "mesh_generation/mesh_types.h"

namespace meshgeneration {
    using meshgeneration::Node;

    struct AABB {
        double x, y; // Center of the bounding box
        double half_width, half_height; // Half dimensions of the bounding box

        // Helper to check for intersection with another AABB
        bool intersects(const AABB& other) const {
            // Check for non-intersection. If any of these are true, they don't overlap.
            if (x + half_width < other.x - other.half_width ||   // this is to the left of other
                x - half_width > other.x + other.half_width ||   // this is to the right of other
                y + half_height < other.y - other.half_height || // this is below other
                y - half_height > other.y + other.half_height) { // this is above other
                return false;
            }
            // Otherwise, they must overlap.
            return true;
        }
    };


    struct QuadtreeBox {

        AABB boundary; // The bounding box of this box
        std::vector<meshgeneration::Node> points; // Points contained in this box
        QuadtreeBox* children[4] = { nullptr, nullptr, nullptr, nullptr }; // Pointers to child boxes (NE, NW, SW, SE)

        // Constructor to initialize the bounding box for this QuadtreeBox
        QuadtreeBox(const AABB& boundary) : boundary(boundary) {}

    };  


    class Quadtree
    {   
        private:
            QuadtreeBox* root;
            bool contains(const QuadtreeBox* QuadBox, const meshgeneration::Node& point) const;
            void subdivide(QuadtreeBox* QuadBox);
            void insertRecursive(QuadtreeBox* QuadBox, const meshgeneration::Node& point);
            void queryRangeRecursive(const QuadtreeBox* QuadBox, const AABB& range, std::vector<meshgeneration::Node>& foundPoints) const;
            bool withinRange(const AABB& range, const meshgeneration::Node& point) const;
        public:

        Quadtree(const AABB& boundary) {
            root = new QuadtreeBox(boundary);
        }
        void insert(const meshgeneration::Node& point);
        std::vector<meshgeneration::Node> query(const AABB& range) const;

        
    };

    }