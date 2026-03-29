#pragma once

namespace app_support::forward_euler
{
    void run_forward_euler(const int N, const double L, const double U0, const double U1, const double UL, 
                          const double k, const double rho, const double cp, const double dt, int max_steps);
}