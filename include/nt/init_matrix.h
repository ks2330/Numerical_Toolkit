#pragma once
#include <vector>    // To use std::vector for the triplet list
#include <Eigen/Dense>


namespace nt::setup
{
    std::vector<Eigen::Triplet<double>> init_matrix(int N, double U0, double U1, double UL);
}