#pragma once
#include <cmath>
#include "FEM_Global_Stiffness_Matrix.h"
#include "mesh_generation/mesh_generation.h"

namespace nt::fem
{
    struct Velocity {
        double u;
        double v;
    };

    // Applies far-field Dirichlet BC for potential flow:
    // phi = U_inf * (x*cos(alpha) + y*sin(alpha))
    // Applied to outer boundary nodes (group "outer").
    // Aerofoil surface uses zero normal flux — naturwal BC, no action needed.
    inline void applyPotentialFlowBCs(
        const meshgeneration::Mesh& mesh,
        std::vector<std::vector<double>>& K,
        std::vector<double>& rhs,
        double U_inf,
        double alpha = 0.0)
    {
        for (int i = 0; i < static_cast<int>(mesh.nodes.size()); ++i) {
            if (mesh.nodes[i].type == meshgeneration::NodeType::Boundary) {
                double phi = U_inf * (mesh.nodes[i].x * std::cos(alpha)
                                    + mesh.nodes[i].y * std::sin(alpha));
                applyDirichletBC(K, rhs, i, phi);
            }
        }
    }

    inline std::vector<Velocity> computeVelocityField(
        const meshgeneration::Mesh& mesh, 
        const std::vector<double>& phi) 
    {
        std::vector<Velocity> velocityField;
        int N = static_cast<int>(mesh.nodes.size());
        std::vector<double> sum_u(N, 0.0), sum_v(N, 0.0), sum_area(N, 0.0);
        for (const auto& element : mesh.elements) {
            meshgeneration::Node n1 = mesh.getNodeByID(element.n0_id);
            meshgeneration::Node n2 = mesh.getNodeByID(element.n1_id);
            meshgeneration::Node n3 = mesh.getNodeByID(element.n2_id);


            double area = 0.5 * std::abs(n1.x * (n2.y - n3.y) + n2.x * (n3.y - n1.y) + n3.x * (n1.y - n2.y));
            
            if (area < 1e-14) {
                std::cerr << "Degenerate element " << element.Element_id
                        << " (area ≈ 0), skipping\n";
                continue;
            }

            double b[3] = { n2.y - n3.y, n3.y - n1.y, n1.y - n2.y };
            double c[3] = { n3.x - n2.x, n1.x - n3.x, n2.x - n1.x };

            double u_e = (b[0]*phi[element.n0_id] + b[1]*phi[element.n1_id] + b[2]*phi[element.n2_id]) / (2*area);
            double v_e = (c[0]*phi[element.n0_id] + c[1]*phi[element.n1_id] + c[2]*phi[element.n2_id]) / (2*area);


            sum_u[element.n0_id] += u_e * area;
            sum_u[element.n1_id] += u_e * area;
            sum_u[element.n2_id] += u_e * area;

            sum_v[element.n0_id] += v_e * area;
            sum_v[element.n1_id] += v_e * area;
            sum_v[element.n2_id] += v_e * area;

            sum_area[element.n0_id] += area;
            sum_area[element.n1_id] += area;
            sum_area[element.n2_id] += area;
        }
        for (int i = 0; i < static_cast<int>(mesh.nodes.size()); ++i) {
            velocityField.push_back({sum_u[i] / sum_area[i], sum_v[i] / sum_area[i]});
        }
        return velocityField;
    }
    inline std::vector<double> computePressureCoefficients(
        const std::vector<Velocity>& velocityField,
        double U_inf)
        {
            int N = static_cast<int>(velocityField.size());
            std::vector<double> Cp;
            for (int i = 0; i < N; i++)
            {
                Cp.push_back(1 - (velocityField[i].u * velocityField[i].u + velocityField[i].v * velocityField[i].v) / (U_inf * U_inf));
            }
            return Cp;
        }
}
