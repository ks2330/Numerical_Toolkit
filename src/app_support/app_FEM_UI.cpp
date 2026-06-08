#include <iostream>
#include <fstream>
#include <vector>
#include "mesh_generation/mesh_generation.h"

namespace app_support::FEM::UI {

static std::ofstream openCSV(const std::string& path, const std::string& header) {
    std::ofstream f(path);
    if (!f.is_open()) { std::cerr << "Could not write to " << path << "\n"; return f; }
    f << header << "\n";
    return f;
}

void write_boundry_nodes_to_csv(const meshgeneration::Mesh& mesh, const std::vector<meshgeneration::Node>& nodes, const std::string& outputPath) {
    auto f = openCSV(outputPath, "id,x,y");
    if (!f.is_open()) return;
    for (size_t i = 0; i < mesh.nodes.size(); ++i)
        f << i << "," << mesh.nodes[i].x << "," << mesh.nodes[i].y << "\n";
    std::cout << "Intial mesh nodes: " << mesh.nodes.size() << " nodes written to " << outputPath << "\n";
}

void write_triangulation_to_csv(const meshgeneration::Mesh& mesh, const std::vector<meshgeneration::Element>& elements, const std::vector<meshgeneration::Node>& nodes, const std::string& outputPath) {
    auto f = openCSV(outputPath, "ax,ay,bx,by,cx,cy");
    if (!f.is_open()) return;
    for (const auto& T : mesh.elements) {
        const auto& n0 = mesh.getNodeByID(T.n0_id);
        const auto& n1 = mesh.getNodeByID(T.n1_id);
        const auto& n2 = mesh.getNodeByID(T.n2_id);
        f << n0.x << "," << n0.y << "," << n1.x << "," << n1.y << "," << n2.x << "," << n2.y << "\n";
    }
    std::cout << "Initial triangulation: " << mesh.elements.size() << " triangles written to " << outputPath << "\n";
}

void write_Solution_to_csv(std::vector<double>& T, const std::string& outputPath, int N, meshgeneration::Mesh& mesh) {
    auto f = openCSV(outputPath, "id,x,y,temperature");
    if (!f.is_open()) return;
    for (int i = 0; i < N; ++i)
        f << i << "," << mesh.nodes[i].x << "," << mesh.nodes[i].y << "," << T[i] << "\n";

    std::string elemsPath = outputPath;
    size_t pos = elemsPath.rfind("steady_state_nodes");
    if (pos != std::string::npos) elemsPath.replace(pos, 18, "steady_state_elements");

    auto ef = openCSV(elemsPath, "n0,n1,n2");
    if (!ef.is_open()) return;
    for (const auto& elem : mesh.elements)
        ef << elem.n0_id << "," << elem.n1_id << "," << elem.n2_id << "\n";

    std::cout << "Steady-state solution written to:\n  " << outputPath << "\n  " << elemsPath << "\n";
}

void write_pressure_field_to_csv(const std::vector<double>& Cp, const meshgeneration::Mesh& mesh, const std::string& outputPath) {
    auto f = openCSV(outputPath, "id,x,y,Cp");
    if (!f.is_open()) return;
    for (int i = 0; i < static_cast<int>(Cp.size()); ++i)
        f << i << "," << mesh.nodes[i].x << "," << mesh.nodes[i].y << "," << Cp[i] << "\n";
    std::cout << "Pressure field written to " << outputPath << "\n";
}

}