#pragma once
#include "mesh_generation/mesh_geometry.h"

namespace nt::fvm
{
    double cellArea(const meshgeneration::Node& n1, const meshgeneration::Node& n2, const meshgeneration::Node& n3){

        double area = orient2d(n1, n2, n3) / 2.0;

        return area;
    }
}