#include <iostream>
#include <fstream>
#include <vector>

// #include <nlohmann/json.hpp>

#include "app_support/app_FEM.h"
#include "app_support/app_FEM_UI.h"
#include "nt/finite_element_methods/FEM_Potential_Flow.h"

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
    std::string aerfoilDAT;
    std::string pressureFieldCSV;
};

Config config = {
    .nx = 600,
    .ny = 200,
    .segsPerUnit = 12,
    .numRandomNodes = 100,
    .shape = "ushape", // "rectangle", "circle", "both", "triangle" (not implemented yet), "ushape"
    .shapeCSV = "results/csv/Nodes.csv",
    .boundaryCSV = "results/csv/boundary_nodes_rectangular.csv",
    .outputCSV = "results/csv/ushape_nodes.csv",
    .triangulationCSV = "results/csv/triangulation.csv",
    .solutionCSV = "results/csv/steady_state_nodes.csv",
    .aerfoilDAT = "results/dat/aerfoil.dat",
    .pressureFieldCSV = "results/csv/pressure_field.csv",
};

int main() {
    //meshgeneration::Mesh mesh = app_support::FEM::run::run_FEM(shape, nx, ny, segsPerUnit, numRandomNodes);
    meshgeneration::Mesh mesh = app_support::FEM::run::initialise_from_CSV(config.aerfoilDAT);
    app_support::FEM::run::run_Triangulation(mesh);
    app_support::FEM::UI::write_boundry_nodes_to_csv(mesh, mesh.nodes, config.boundaryCSV);
    app_support::FEM::UI::write_triangulation_to_csv(mesh, mesh.elements, mesh.nodes, config.triangulationCSV);

//  std::vector<double> T = app_support::FEM::run::run_FEM_Heat_Equation(
//  mesh, mesh.groupId("inlet"), 100.0, mesh.groupId("outlet"), 0.0);
//  app_support::FEM::UI::write_Solution_to_csv(T, config.solutionCSV, mesh.nodes.size(), mesh);

    const double U_inf = 1.0;
    std::vector<double> phi = app_support::FEM::run::run_Potential_Flow(mesh, U_inf, 0.0);
    auto velocity = nt::fem::computeVelocityField(mesh, phi);
    auto Cp       = nt::fem::computePressureCoefficients(velocity, U_inf);
    app_support::FEM::UI::write_pressure_field_to_csv(Cp, mesh, config.pressureFieldCSV);

    return 0;
}
