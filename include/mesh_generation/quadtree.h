#pragma once
#include <vector>
#include "mesh_generation/mesh_types.h"

namespace meshgeneration {
    using meshgeneration::Node;

    struct AABB {
        double x, y; // Center of the bounding box
        double half_width, half_height; // Half dimensions of the bounding box
    };


    struct QuadtreeBox {
        AABB boundary; // The bounding box of this box
        std::vector<meshgeneration::Node> points; // Points contained in this box
        QuadtreeBox* children[4] = { nullptr, nullptr, nullptr, nullptr }; // Pointers to child boxes (NE, NW, SW, SE)


        QuadtreeBox(const AABB& boundary) : boundary(boundary) {}

    };  


    class Quadtree
    {   
        private:
            QuadtreeBox* root;

        public:

        Quadtree(const AABB& boundary) {
            root = new QuadtreeBox(boundary);
        }
        void insert(QuadtreeBox* QuadBox, const meshgeneration::Node& point);
        bool contains(const QuadtreeBox* QuadBox, const meshgeneration::Node& point) const;
        void subdivide(QuadtreeBox* QuadBox);

        
    };

    }