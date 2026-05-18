#include <vector>
#include "mesh_generation/mesh_types.h"
#include "mesh_generation/quadtree.h"

namespace meshgeneration {

    void Quadtree::insert(const meshgeneration::Node& point) {
        insertRecursive(root, point);
    }


    void Quadtree::insertRecursive(QuadtreeBox* QuadBox, const meshgeneration::Node& point) {
        if (!contains(QuadBox, point)) return;

        if (QuadBox->children[0] != nullptr){
            for (size_t i = 0; i < 4; ++i) {
                if (!contains(QuadBox, point)) return;
                insertRecursive(QuadBox->children[i], point);
            }
        }
        else {
            if (QuadBox->points.size() < 4) {
                QuadBox->points.push_back(point);
            }
            else {
                subdivide(QuadBox);
                for (size_t i = 0; i < 4; ++i) {
                    insertRecursive(QuadBox, QuadBox->points[i]);
                }
                QuadBox->points.clear();
                QuadBox->points.shrink_to_fit();

                insertRecursive(QuadBox, point);
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

    bool Quadtree::withinRange(const AABB& range, const meshgeneration::Node& point) const {
        if (point.x >= range.x - range.half_width &&
            point.x <= range.x + range.half_width &&
            point.y >= range.y - range.half_height &&
            point.y <= range.y + range.half_height) {
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

    std::vector<meshgeneration::Node> Quadtree::query(const AABB& range) const {
        std::vector<meshgeneration::Node> foundPoints;
        queryRangeRecursive(root, range, foundPoints);
        return foundPoints;
    }

    void Quadtree::queryRangeRecursive(const QuadtreeBox* QuadBox, const AABB& range, std::vector<meshgeneration::Node>& foundPoints) const {
        if (QuadBox == nullptr) return;
        if (QuadBox->boundary.x + QuadBox->boundary.half_width < range.x - range.half_width ||
            QuadBox->boundary.x - QuadBox->boundary.half_width > range.x + range.half_width ||
            QuadBox->boundary.y + QuadBox->boundary.half_height < range.y - range.half_height ||
            QuadBox->boundary.y - QuadBox->boundary.half_height > range.y + range.half_height) {
            return;
        }
        if (QuadBox->children[0] != nullptr){
            for (size_t i = 0; i < 4; ++i) {
                queryRangeRecursive(QuadBox->children[i], range, foundPoints);
            }
        }
        else {
            for (const auto& point : QuadBox->points) {
                if (withinRange(range, point)) {
                    foundPoints.push_back(point);
                }
            }
        }
    }

}