#include <iostream>
#include <fstream>
#include <vector>
#include "mesh_generation/mesh_generation.h"

namespace app_support::FEM::UI

{
    void write_boundry_nodes_to_csv(const meshgeneration::Mesh& mesh, const std::vector<meshgeneration::Node>& nodes, const std::string& outputPath){
        std::ofstream nodesFile(outputPath);
        nodesFile << "id,x,y\n";
        for (size_t i = 0; i < mesh.nodes.size(); ++i) {
            nodesFile << i << ","
                    << mesh.nodes[i].x << ","
                    << mesh.nodes[i].y << "\n";
        }
        nodesFile.close();
        std::cout << "Intial mesh nodes: " << mesh.nodes.size() << " nodes written to " << outputPath << "\n";
    }

    void write_triangulation_to_csv(const meshgeneration::Mesh& mesh, const std::vector<meshgeneration::Element>& elements, const std::vector<meshgeneration::Node>& nodes, const std::string& outputPath){
        std::ofstream triFile(outputPath);
        triFile << "ax,ay,bx,by,cx,cy\n";
        for (const auto& T : mesh.elements) {
            const auto& n0 = mesh.getNodeByID(T.n0_id);
            const auto& n1 = mesh.getNodeByID(T.n1_id);
            const auto& n2 = mesh.getNodeByID(T.n2_id);
            triFile << n0.x << "," << n0.y << ","
                    << n1.x << "," << n1.y << ","
                    << n2.x << "," << n2.y << "\n";
        }
        triFile.close();
        std::cout << "Initial triangulation: " << mesh.elements.size() << " triangles written to " << outputPath << "\n";
    }

    void write_Solution_to_csv(std::vector<double>& T, const std::string& outputPath, int N, meshgeneration::Mesh& mesh){
        std::ofstream nodesFile(outputPath);
        nodesFile << "id,x,y,temperature\n";
        for (int i = 0; i < N; ++i) {
            nodesFile << i << ","
                    << mesh.nodes[i].x << ","
                    << mesh.nodes[i].y << ","
                    << T[i] << "\n";
        }
        nodesFile.close();

        // Derive elements path by replacing _nodes with _elements in outputPath
        std::string elemsPath = outputPath;
        size_t pos = elemsPath.rfind("steady_state_nodes");
        if (pos != std::string::npos)
            elemsPath.replace(pos, 18, "steady_state_elements");

        std::ofstream elemsFile(elemsPath);
        elemsFile << "n0,n1,n2\n";
        for (const auto& elem : mesh.elements) {
            elemsFile << elem.n0_id << ","
                     << elem.n1_id << ","
                     << elem.n2_id << "\n";
        }
        elemsFile.close();

        std::cout << "Steady-state solution written to:\n"
            << "  " << outputPath << "\n"
            << "  " << elemsPath << "\n";
    }

}
