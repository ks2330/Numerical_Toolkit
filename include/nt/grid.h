#pragma once // Prevents the header from being included multiple times

#include <stdexcept> // For std::invalid_argument and std::out_of_range

namespace nt::matrix
{
    /**
     * @class Grid
     * @brief Represents a uniform 1D computational grid.
     *
     * This class encapsulates the properties of a one-dimensional grid,
     * such as its length, number of points, and grid spacing. It ensures
     * that the grid parameters are always consistent.
     */
    class Grid
    {
    private:
        double m_L;  // Physical length of the domain
        int m_nx;    // Number of grid points
        double m_dx; // Grid spacing (delta-x)

    public:
        /// @brief Constructs a Grid object.
        /// @param N The number of grid points (must be >= 2).
        /// @param L The physical length of the grid.
        Grid(int N, double L) : m_L(L), m_nx(N)
        {
            if (N < 2) {
                throw std::invalid_argument("Grid must have at least 2 points.");
            }
            // Ensure floating-point division to calculate dx correctly.
            m_dx = m_L / (static_cast<double>(m_nx) - 1.0);
        }

        /// @brief Returns the grid spacing, delta-x.
        inline double dx() const { return m_dx; }

        /// @brief Returns the number of points in the grid.
        inline int size() const { return m_nx; }
    };
}

