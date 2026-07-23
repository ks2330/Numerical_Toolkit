#pragma once
#include "nt/finite_volume_methods/FVM_flux.h"

namespace nt::fvm  
{

    class EulerSolver   
    {
    private:
        std::vector<Face>              faces_;
        std::vector<double>            volumes_;
        std::vector<ConservativeState> state_;
        GasModel                       gm_;
        ConservativeState              Uinf_;
        double                         cfl_;
        int                            farFieldGroup_;

    public:
        EulerSolver(const std::vector<meshgeneration::Element>& elements,
                    const std::vector<meshgeneration::Node>& nodes,
                    const std::vector<meshgeneration::Edge>& boundaryEdges,
                    GasModel gm, ConservativeState Uinf,
                    double cfl, int farFieldGroup)
            : gm_(gm), Uinf_(Uinf), cfl_(cfl), farFieldGroup_(farFieldGroup)
        {
            faces_   = buildFaces(elements, nodes, boundaryEdges);             
            volumes_ = computeAllCellArea(elements, nodes);
            state_   = std::vector<ConservativeState>(volumes_.size(), Uinf);
        }

        void step() {
            std::vector<ConservativeState> R = computeResidual(state_, faces_, gm_, Uinf_, farFieldGroup_);
            std::vector<double> dt = localTimeSteps();
            for (size_t i = 0; i < state_.size(); ++i)
                state_[i] = state_[i] + (dt[i] / volumes_[i]) * R[i];
        }


        std::vector<double> localTimeSteps() const {
            std::vector<double> accum(state_.size(), 0.0);  
            for (const auto& f : faces_) {
                double wave = (f.rightElement_id != -1)
                    ? maxWaveSpeed(state_[f.leftElement_id], state_[f.rightElement_id], f.normal, gm_)
                    : maxWaveSpeed(state_[f.leftElement_id], state_[f.leftElement_id], f.normal, gm_);
                accum[f.leftElement_id] += wave * f.length;
                if (f.rightElement_id != -1)
                    accum[f.rightElement_id] += wave * f.length;
            }
            std::vector<double> dt(state_.size(), 0.0);
            for (size_t i = 0; i < dt.size(); ++i)
                dt[i] = cfl_ * volumes_[i] / accum[i];
            return dt;
        }

        void solve(double tol, int maxIters) {
            for (int it = 0; it < maxIters; ++it) {
                if (residualNorm() < tol) break;
                step();
            }
        }

        double residualNorm() const {
            auto R = computeResidual(state_, faces_, gm_, Uinf_, farFieldGroup_);
            double sum = 0.0;
            for (const auto& r : R)
                sum += r.rho*r.rho + r.rho_u*r.rho_u + r.rho_v*r.rho_v + r.rho_e*r.rho_e;
            return std::sqrt(sum);
        }


        const std::vector<ConservativeState>& getState()   const { return state_; }
        const std::vector<double>&            getVolumes() const { return volumes_; }
        const std::vector<Face>&              getFaces()   const { return faces_; }
    

        void setState(const std::vector<ConservativeState>& s)
            { state_ = s; }
    };
}