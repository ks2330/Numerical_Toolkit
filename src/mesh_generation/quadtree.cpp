#pragma once
#include <vector>
#include "mesh_generation/mesh_types.h"
#include "mesh_generation/quadtree.h"

namespace meshgeneration {

    void Quadtree::insert(QuadtreeBox* QuadBox, const meshgeneration::Node& point) {
        if (!contains(QuadBox, point)) return;

        if (QuadBox->children[0] != nullptr){
            for (size_t i = 0; i < 4; ++i) {
                insert(QuadBox->children[i], point);
            }
        }
        else {
            if (QuadBox->points.size() < 4) {
                QuadBox->points.push_back(point);
            }
            else {
                subdivide(QuadBox);
                for (size_t i = 0; i < 4; ++i) {
                    insert(QuadBox->children[i], point);
                }
                QuadBox->points.clear();
                QuadBox->points.shrink_to_fit();

                insert(QuadBox, point);
            }
        }
        
    }

    bool Quadtree::contains(const QuadtreeBox* QuadBox, const meshgeneration::Node& point) const {
        if (point.x >= QuadBox->boundary.x - QuadBox->boundary.half_width &&
            point.x <= QuadBox->boundary.x + QuadBox->boundary.half_width &&
            point.y >= QuadBox->boundary.y - QuadBox->boundary.half_height &&
            point.y <= QuadBox->boundary.y + QuadBox->boundary.half_height) {
            return true;
        }
        else
            return false;
    }

    void Quadtree::subdivide(QuadtreeBox* QuadBox) {
        if (QuadBox->children[0] != nullptr) {
            return;
        }
        double x = QuadBox->boundary.x;
        double y = QuadBox->boundary.y;
        double hw = QuadBox->boundary.half_width / 2.0;
        double hh = QuadBox->boundary.half_height / 2.0;

        QuadBox->children[0] = new QuadtreeBox({ x + hw, y + hh, hw, hh }); // NE
        QuadBox->children[1] = new QuadtreeBox({ x - hw, y + hh, hw, hh }); // NW
        QuadBox->children[2] = new QuadtreeBox({ x - hw, y - hh, hw, hh }); // SW
        QuadBox->children[3] = new QuadtreeBox({ x + hw, y - hh, hw, hh }); // SE
    }
}