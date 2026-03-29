#include "nt/setup/p_cal.h"


namespace nt::nm
{
        double alpha_value(double k, double rho, double cp)
    {
        return k / (rho * cp);
    }

    double p_value(double alpha, double d_t, double dx)
    {
        return alpha * d_t / (dx * dx);
    }


}