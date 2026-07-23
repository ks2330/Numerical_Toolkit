#pragma once
#include <cmath>
#include <stdexcept>

namespace nt::fvm
{
    struct ConservativeState {
        double rho; // Density
        double rho_u; // Momentum in x
        double rho_v; // Momentum in y
        double rho_e; // Total energy
        
        ConservativeState operator+(const ConservativeState& o) const {
            return {rho + o.rho, rho_u + o.rho_u, rho_v + o.rho_v, rho_e + o.rho_e};
        }
        ConservativeState operator-(const ConservativeState& o) const {
            return {rho - o.rho, rho_u - o.rho_u, rho_v - o.rho_v, rho_e - o.rho_e};
        }
        ConservativeState operator*(double s) const {
            return {rho * s, rho_u * s, rho_v * s, rho_e * s};
        }
    };
    inline ConservativeState operator*(double s, const ConservativeState& v) { return v * s; }

    struct PrimitiveState {
        double rho; // Density
        double u;   // Velocity in x
        double v;   // Velocity in y
        double p;   // Pressure
    };

    struct GasModel {
        double gamma; // Ratio of specific heats
        double R;   // Specific gas constant for air (J/kg*K)

        double pressure(const ConservativeState& s) const {
            if (s.rho <= 0.0)
                throw std::runtime_error("GasModel::pressure: non-positive density");
            double internal_energy = s.rho_e - kineticEnergy(s);
            double p = (gamma - 1.0) * internal_energy;
            if (p <= 0.0)
                throw std::runtime_error("GasModel::pressure: non-positive pressure (unphysical state)");
            return p;
        }

        double soundSpeed(const PrimitiveState& ps) const {
            if (ps.rho <= 0.0 || ps.p <= 0.0)
                throw std::runtime_error("GasModel::soundSpeed: non-positive density or pressure");
            return std::sqrt(gamma * ps.p / ps.rho);
        }

        PrimitiveState toPrimitive(const ConservativeState& s) const {
            if (s.rho <= 0.0)
                throw std::runtime_error("GasModel::toPrimitive: non-positive density");
            PrimitiveState ps;
            ps.rho = s.rho;
            ps.u = s.rho_u / s.rho;
            ps.v = s.rho_v / s.rho;
            ps.p = pressure(s);
            return ps;
        }

        ConservativeState toConservative(const PrimitiveState& ps) const {
            ConservativeState s;
            s.rho = ps.rho;
            s.rho_u = ps.rho * ps.u;
            s.rho_v = ps.rho * ps.v;
            double kinetic_energy = kineticEnergy(s);
            double internal_energy = ps.p / (gamma - 1.0);
            s.rho_e = internal_energy + kinetic_energy;
            return s;
        }

        private:
        double kineticEnergy(const ConservativeState& s) const {
            return 0.5 * (s.rho_u * s.rho_u + s.rho_v * s.rho_v) / s.rho;
        }

    };
}