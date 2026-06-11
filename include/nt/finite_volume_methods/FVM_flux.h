#pragma once
#include "nt/finite_volume_methods/FVM_gas_model.h"
#include "nt/finite_volume_methods/FVM_mesh.h"

namespace nt::fvm
{
    struct FluxVector {
        double mass;
        double x_mom;
        double y_mom;
        double energy;

        FluxVector operator+(const FluxVector& o) const {
            return {mass + o.mass, x_mom + o.x_mom, y_mom + o.y_mom, energy + o.energy};
        }
        FluxVector operator-(const FluxVector& o) const {
            return {mass - o.mass, x_mom - o.x_mom, y_mom - o.y_mom, energy - o.energy};
        }
        FluxVector operator*(double s) const {
            return {mass * s, x_mom * s, y_mom * s, energy * s};
        }
    };
    inline FluxVector operator*(double s, const FluxVector& f) { return f * s; }

    inline FluxVector computeFlux(const ConservativeState& s, const Vec2& normal, const GasModel& gm) {
        double u = s.rho_u / s.rho;
        double v = s.rho_v / s.rho;
        double p = gm.pressure(s);
        double un = u * normal.x + v * normal.y;

        FluxVector flux;
        flux.mass = s.rho * un;
        flux.x_mom = s.rho_u * un + p * normal.x;
        flux.y_mom = s.rho_v * un + p * normal.y;
        flux.energy = (s.rho_e + p) * un;
        return flux;
    }


    inline FluxVector CentralAverage(const FluxVector& fL, const FluxVector& fR) {
        FluxVector avg;
        avg = 0.5 * (fL + fR);
        return avg;
    }

    inline double maxWaveSpeed(const ConservativeState& ls, const ConservativeState& rs, const Vec2& normal, const GasModel& gm) {
        PrimitiveState ps_l = gm.toPrimitive(ls);
        PrimitiveState ps_r = gm.toPrimitive(rs);
        double un_l = ps_l.u * normal.x + ps_l.v * normal.y;
        double un_r = ps_r.u * normal.x + ps_r.v * normal.y;
        double a_l = gm.soundSpeed(ps_l);
        double a_r = gm.soundSpeed(ps_r);
        return std::max(std::abs(un_l) + a_l, std::abs(un_r) + a_r);
    }

    inline ConservativeState calculateDissipation(double s_max, const ConservativeState& rs, const ConservativeState& ls) {
        ConservativeState dissipation = 0.5 * s_max * (rs - ls);
        return dissipation;
    }

    inline FluxVector RusanovFlux(const ConservativeState& ls, const ConservativeState& rs, const Vec2& normal, const GasModel& gm) {
        FluxVector flux_L = computeFlux(ls, normal, gm);
        FluxVector flux_R = computeFlux(rs, normal, gm);
        double s_max = maxWaveSpeed(ls, rs, normal, gm);
        ConservativeState dissipation = calculateDissipation(s_max, rs, ls);

        FluxVector avg = CentralAverage(flux_L, flux_R);
        FluxVector flux;
        flux.mass   = avg.mass   - dissipation.rho;     
        flux.x_mom  = avg.x_mom  - dissipation.rho_u;  
        flux.y_mom  = avg.y_mom  - dissipation.rho_v;
        flux.energy = avg.energy - dissipation.rho_e;
        return flux;
    }

    inline std::vector<ConservativeState> computeResidual(const std::vector<ConservativeState>& state, const std::vector<Face>& faces, const GasModel& gm) {
        std::vector<ConservativeState> res(state.size(), {0.0, 0.0, 0.0, 0.0});
        for (const auto& face : faces) {
            if (face.rightElement_id == -1) continue; 

            const ConservativeState& sL = state[face.leftElement_id];
            const ConservativeState& sR = state[face.rightElement_id];
            FluxVector flux = RusanovFlux(sL, sR, face.normal, gm);

            ConservativeState contrib{ flux.mass   * face.length,
                                       flux.x_mom  * face.length,
                                       flux.y_mom  * face.length,
                                       flux.energy * face.length };

            res[face.leftElement_id]  = res[face.leftElement_id]  - contrib;  
            res[face.rightElement_id] = res[face.rightElement_id] + contrib;
        }
        return res;
    }
}