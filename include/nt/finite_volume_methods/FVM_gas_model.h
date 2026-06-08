#pragma once

namespace nt::fvm
{
    struct State {
        double rho; // Density
        double rho_u; // Momentum in x
        double rho_v; // Momentum in y
        double rho_e; // Total energy
    };
}