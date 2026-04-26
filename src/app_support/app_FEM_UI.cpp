#include <iostream>
#include <fstream>
#include <vector>
#include "mesh_generation/mesh_generation.h"

namespace app_support::FEM::UI

{
    void write_boundry_nodes_to_csv(const meshgeneration::Mesh& mesh, const std::vector<meshgeneration::Node>& nodes, const std::string& outputPath){
        const std::string outputPath_NODES = "boundary_nodes_rectangular.csv";
        std::ofstream nodesFile(outputPath_NODES);
        nodesFile << "id,x,y\n";
        for (size_t i = 0; i < mesh.nodes.size(); ++i) {
            nodesFile << i << ","
                    << mesh.nodes[i].x << ","
                    << mesh.nodes[i].y << "\n";
        }
        nodesFile.close();
        std::cout << "Intial mesh nodes: " << mesh.nodes.size() << " nodes written to " << outputPath_NODES << "\n";
    }
    void write_triangulation_to_csv(const meshgeneration::Mesh& mesh, const std::vector<meshgeneration::Element>& elements, const std::vector<meshgeneration::Node>& nodes, const std::string& outputPath){
        std::ofstream triFile("triangulation.csv");
        triFile << "ax,ay,bx,by,cx,cy\n";
        for (const auto& T : mesh.elements)
        // We can access the node coordinates using the node IDs stored in the element.
        // Assuming node IDs are 0-based and correspond to their index in the nodes vector:
            {
                const auto& n0 = mesh.nodes[T.n0_id];
                const auto& n1 = mesh.nodes[T.n1_id];
                const auto& n2 = mesh.nodes[T.n2_id];
                triFile << n0.x << "," << n0.y << ","
                        << n1.x << "," << n1.y << ","
                        << n2.x << "," << n2.y << "\n";
            }
        triFile.close();
        std::cout << "Initial triangulation: " << mesh.elements.size() << " triangles written to triangulation.csv\n";

    }
    
    void write_Solution_to_csv(std::vector<double>& T, const std::string& outputPath, int N, meshgeneration::Mesh& mesh){
        // ── Write node data ──────────────────────────────────────────────────
        std::ofstream nodesFile("steady_state_nodes.csv");
        nodesFile << "id,x,y,temperature\n";
        for (int i = 0; i < N; ++i) {
            nodesFile << i << ","
                    << mesh.nodes[i].x << ","
                    << mesh.nodes[i].y << ","
                    << T[i] << "\n";
        }
        nodesFile.close();

        // ── Write element data (node indices) ─────────────────────────────────
        std::ofstream elemsFile("steady_state_elements.csv");
        elemsFile << "n0,n1,n2\n";
        for (const auto& elem : mesh.elements) {
            elemsFile << elem.n0_id << ","
                     << elem.n1_id << ","
                     << elem.n2_id << "\n";
        }
        elemsFile.close();

        std::cout << "Steady-state solution written to:\n"
            << "  steady_state_nodes.csv\n"
            << "  steady_state_elements.csv\n"
            << "Run plot_steady_state.py to visualise.\n";
    }

}
