#include "grid.h"

namespace nt
{
    class Grid
    {
    private:
        double m_L;
        int m_nx;
        int m_dx;
    public:
        Grid(int N, double L) : m_L(L), m_nx(N) { 
            m_dx = L / (N - 1);
        }
        inline double dx() const { return m_dx; }
        inline int size() const { return m_nx; }
    };

}