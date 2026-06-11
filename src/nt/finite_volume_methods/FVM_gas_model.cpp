#include "nt/finite_volume_methods/FVM_gas_model.h"

namespace nt::fvm {

ConservativeState GasModel::eulerFlux(const ConservativeState& s, double nx, double ny) const {
    // TODO: implement
    // u_n = (rho_u*nx + rho_v*ny) / rho
    // p   = pressure(s)
    // F   = [ rho*u_n,  rho_u*u_n + p*nx,  rho_v*u_n + p*ny,  (rho_e+p)*u_n ]
    return {0, 0, 0, 0};
}

ConservativeState GasModel::laxFriedrichsFlux(const ConservativeState& L, const ConservativeState& R,
                                               double nx, double ny, double maxWaveSpeed) const {
    // TODO: implement
    // F_LF = 0.5*(eulerFlux(L,n) + eulerFlux(R,n)) - 0.5*maxWaveSpeed*(R - L)
    return {0, 0, 0, 0};
}

} // namespace nt::fvm
