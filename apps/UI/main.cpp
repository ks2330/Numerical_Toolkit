#include <iostream>
#include <fstream>
#include <vector>

// #include <nlohmann/json.hpp>

#include "app_support/app_FEM.h"
#include "app_support/app_FEM_UI.h"

// using json = nlohmann::json;

struct Config {
    int nx;
    int ny;
    int segsPerUnit;
    int numRandomNodes;
    std::string shape;
    std::string shapeCSV;
    std::string boundaryCSV;
    std::string outputCSV;
    std::string triangulationCSV;
    std::string solutionCSV;

};

Config config = {
    .nx = 600,
    .ny = 200,
    .segsPerUnit = 12,
    .numRandomNodes = 30,
    .shape = "ushape", // "rectangle", "circle", "both", "triangle" (not implemented yet), "ushape"
    .shapeCSV = "results/csv/Nodes.csv",
    .boundaryCSV = "results/csv/boundary_nodes_rectangular.csv",
    .outputCSV = "results/csv/ushape_nodes.csv",
    .triangulationCSV = "results/csv/triangulation.csv",
    .solutionCSV = "results/csv/steady_state_nodes.csv"
};

int main() {
    //meshgeneration::Mesh mesh = app_support::FEM::run::run_FEM(shape, nx, ny, segsPerUnit, numRandomNodes);
    meshgeneration::Mesh mesh = app_support::FEM::run::initialise_from_CSV(config.shapeCSV, config.nx, config.ny, config.numRandomNodes);
    app_support::FEM::run::run_Triangulation(mesh, config.nx, config.ny);
    app_support::FEM::UI::write_boundry_nodes_to_csv(mesh, mesh.nodes, config.boundaryCSV);
    app_support::FEM::UI::write_triangulation_to_csv(mesh, mesh.elements, mesh.nodes, config.triangulationCSV);

    std::vector<double> T = app_support::FEM::run::run_FEM_Heat_Equation(mesh);
    app_support::FEM::UI::write_Solution_to_csv(T, config.solutionCSV, mesh.nodes.size(), mesh);

    return 0;
}
