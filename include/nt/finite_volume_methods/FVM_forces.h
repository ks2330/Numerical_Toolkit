#pragma once
#include "nt/finite_volume_methods/FVM_flux.h"
#include <cmath>

namespace nt::fvm
{
    struct Forces {
        double fx;
        double fy;
        double lift;
        double drag;
        double cl;
        double cd;
    };

    inline Forces computeForces(const std::vector<ConservativeState>& state,
                                const std::vector<Face>& faces,
                                const GasModel& gm,
                                int wallGroup,
                                double alpha,
                                double qInf,
                                double chord,
                                double pInf = 0.0) {
        double fx = 0.0, fy = 0.0;
        for (const auto& f : faces) {
            if (f.bcType != wallGroup) continue;
            double p = gm.pressure(state[f.leftElement_id]) - pInf;
            fx += p * f.normal.x * f.length;
            fy += p * f.normal.y * f.length;
        }
        double ca = std::cos(alpha), sa = std::sin(alpha);
        Forces F;
        F.fx = fx;
        F.fy = fy;
        F.drag =  fx * ca + fy * sa;
        F.lift = -fx * sa + fy * ca;
        F.cl = F.lift / (qInf * chord);
        F.cd = F.drag / (qInf * chord);
        return F;
    }
}
