#include "mesh_generation/mesh_generation.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <cstdio>

namespace meshgeneration {

void Mesh::init(const std::string& filename, double density) {
    nodes.clear(); edges.clear(); boundaryEdges.clear(); elements.clear();
    boundaryNodes.clear(); holeNodes.clear(); internalNodes.clear();
    boundaryGroups.clear();

    // File loading is CSV-only (the FEM domain boundary). 
    std::string ext = std::filesystem::path(filename).extension().string();
    if (ext != ".csv")
        throw std::runtime_error("Unsupported file format: " + ext);

    parseBoundaryCSV(filename);
    if (nodes.empty())
        throw std::runtime_error("No nodes loaded from " + filename);
    registerGroup(0, "outer");
    createOuterBoundary();
    buildFlatNodeList();
    buildEdges(boundaryNodes, 0);
    boundaryEdges = edges;

    boundaryLayerSeeding();
    getInteriorNodeNumber(density);
}

void Mesh::parseBoundaryCSV(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open())
        throw std::runtime_error("Failed to open file: " + filename);
    std::string line;
    bool first_line = true;
    while (std::getline(file, line)) {
        if (first_line) { first_line = false; continue; }
        size_t comma = line.find(',');
        if (comma != std::string::npos) {
            try {
                double x = std::stod(line.substr(0, comma));
                double y = std::stod(line.substr(comma + 1));
                nodes.push_back({x, y, static_cast<int>(nodes.size()), NodeType::Boundary, 0});
            } catch (const std::exception& e) {
                throw std::runtime_error("Error parsing line: " + line + " (" + e.what() + ")");
            }
        }
    }
}

void Mesh::generateNACA4(int digits4, int nPoints, double chord) {
    if (digits4 < 0 || digits4 > 9999)
        throw std::invalid_argument("NACA 4-digit code must be between 0000 and 9999");
    if (nPoints < 2)
        throw std::invalid_argument("NACA nPoints must be >= 2");
    char buffer[5];
    std::snprintf(buffer, sizeof(buffer), "%04d", digits4);
    double m = (buffer[0] - '0') / 100.0;                              
    double p = (buffer[1] - '0') / 10.0;                          
    double t = ((buffer[2] - '0') * 10 + (buffer[3] - '0')) / 100.0;   

    
    auto surfacePoint = [&](double x, bool upper) -> Node {
        double y_t = 5 * t * (0.2969 * std::sqrt(x) - 0.1260 * x - 0.3516 * x * x
                              + 0.2843 * x * x * x - 0.1036 * x * x * x * x);
        double y_c = 0.0, dyc_dx = 0.0;
        if (m > 0.0 && p > 0.0) {                                     
            if (x < p) {
                y_c    = m / (p * p) * (2 * p * x - x * x);
                dyc_dx = 2 * m / (p * p) * (p - x);
            } else {
                y_c    = m / ((1 - p) * (1 - p)) * ((1 - 2 * p) + 2 * p * x - x * x);
                dyc_dx = 2 * m / ((1 - p) * (1 - p)) * (p - x);
            }
        }
        double theta = std::atan(dyc_dx);
        double px = upper ? (x - y_t * std::sin(theta)) : (x + y_t * std::sin(theta));
        double py = upper ? (y_c + y_t * std::cos(theta)) : (y_c - y_t * std::cos(theta));
        return { px * chord, py * chord, static_cast<int>(holeNodes.size()), NodeType::Hole, 1 };
    };

    for (int i = nPoints; i >= 0; --i) {                              
        double x = 0.5 * (1.0 - std::cos(M_PI * i / nPoints));
        holeNodes.push_back(surfacePoint(x, true));
    }
    for (int i = 1; i < nPoints; ++i) {                               
        double x = 0.5 * (1.0 - std::cos(M_PI * i / nPoints));
        holeNodes.push_back(surfacePoint(x, false));
    }
}


void Mesh::buildAerofoilDomain(double density) {
    if (holeNodes.empty())
        throw std::runtime_error(
            "buildAerofoilDomain: no aerofoil surface (call generateNACA4 first)");
    registerGroup(0, "outer");
    registerGroup(1, "aerofoil");
    registerGroup(2, "inlet");
    registerGroup(3, "outlet");
    createAerofoilBoundary();
    buildFlatNodeList();
    buildEdges(boundaryNodes, 0);
    buildEdges(holeNodes, 1);
    boundaryEdges = edges;
    boundaryLayerSeeding();
    getInteriorNodeNumber(density);
}

void Mesh::buildRectangleDomain(double width, double height, double density) {
    nodes.clear(); edges.clear(); boundaryEdges.clear(); elements.clear();
    boundaryNodes.clear(); holeNodes.clear(); internalNodes.clear();
    boundaryGroups.clear();

    registerGroup(0, "outer");
    registerGroup(2, "inlet");
    registerGroup(3, "outlet");

    // Densified rectangle boundary; tag left edge = inlet, right edge = outlet, rest = outer,
    // so the heat solver's Dirichlet BCs land on the right nodes (it matches by group_id).
    std::vector<Node> boundary = shapegeneration::shapes::rectangle(width, height, 8);
    const double eps = 1e-9;
    for (auto& n : boundary) {
        n.type = NodeType::Boundary;
        if      (n.x <= eps)         n.group_id = 2;   // inlet  (left)
        else if (n.x >= width - eps) n.group_id = 3;   // outlet (right)
        else                         n.group_id = 0;   // outer  (top / bottom)
    }
    boundaryNodes = boundary;

    buildFlatNodeList();
    buildEdges(boundaryNodes, 0);
    boundaryEdges = edges;
    getInteriorNodeNumber(density);
}

void Mesh::createOuterBoundary() {
    if (nodes.empty())
        throw std::runtime_error("No boundary nodes to create outer boundary.");
    std::vector<Node> withInterp;
    size_t n = nodes.size();
    for (size_t i = 0; i < n; ++i) {
        const Node& a = nodes[i];
        const Node& b = nodes[(i + 1) % n];
        withInterp.push_back({a.x, a.y, -1, NodeType::Boundary, 0});
        double len = distance(a, b);
        int segs = std::max(1, static_cast<int>(len / 50.0));
        for (int s = 1; s < segs; ++s) {
            double t = static_cast<double>(s) / segs;
            Node interp = a + (b - a) * t;
            withInterp.push_back({interp.x, interp.y, -1, NodeType::Boundary, 0});
        }
    }
    boundaryNodes = withInterp;
    nodes.clear();
}

void Mesh::createAerofoilBoundary() {
    if (holeNodes.empty()) {
        throw std::runtime_error("No hole nodes to create aerofoil boundary.");
    }
    std::vector<Node> bbox = GetBoundingBox(holeNodes);

    chord = bbox[1].x - bbox[0].x;
    if (chord <= 0) chord = 1.0;

    double domainMinX = bbox[0].x - 5.0 * chord;
    double domainMaxX = bbox[1].x + 5.0 * chord;
    double domainMinY = bbox[0].y - 5.0 * chord;
    double domainMaxY = bbox[2].y + 5.0 * chord;

    Node TL = {domainMinX, domainMaxY, -1};
    Node TR = {domainMaxX, domainMaxY, -1};
    Node BR = {domainMaxX, domainMinY, -1};
    Node BL = {domainMinX, domainMinY, -1};

    double segLen = chord * 0.5;
    auto interpolate = [&](const Node& a, const Node& b, int group_id) {
        std::vector<Node> result;
        double len = distance(a, b);
        int segs = std::max(1, static_cast<int>(5 * len / segLen));
        for (int s = 0; s < segs; ++s) {
            double t = static_cast<double>(s) / segs;
            Node interp = a + (b - a) * t;
            result.push_back({interp.x, interp.y, -1, NodeType::Boundary, group_id});
        }
        return result;
    };

    std::vector<Node> boxNodes;
    for (auto& seg : {interpolate(TL, TR, 0), interpolate(TR, BR, 3),
                       interpolate(BR, BL, 0), interpolate(BL, TL, 2)})
        boxNodes.insert(boxNodes.end(), seg.begin(), seg.end());

    boundaryNodes = boxNodes;
}

void Mesh::buildFlatNodeList() {
    nodes.clear();
    int id = 0;

    for (auto n : boundaryNodes) {
        n.Node_id = id; n.type = NodeType::Boundary;
        nodes.push_back(n); boundaryNodes[id] = nodes.back(); ++id;
    }
    for (int i = 0; i < static_cast<int>(boundaryNodes.size()); ++i)
        boundaryNodes[i].Node_id = i;

    int bOffset = static_cast<int>(boundaryNodes.size());
    for (auto n : holeNodes) {
        n.Node_id = id; n.type = NodeType::Hole;
        nodes.push_back(n); ++id;
    }
    for (int i = 0; i < static_cast<int>(holeNodes.size()); ++i)
        holeNodes[i].Node_id = bOffset + i;

    int hOffset = bOffset + static_cast<int>(holeNodes.size());
    for (auto n : internalNodes) {
        n.Node_id = id; n.type = NodeType::Internal;
        nodes.push_back(n); ++id;
    }
    for (int i = 0; i < static_cast<int>(internalNodes.size()); ++i)
        internalNodes[i].Node_id = hOffset + i;
}

void Mesh::buildEdges(const std::vector<Node>& poly, int group_id) {
    int n = static_cast<int>(poly.size());
    for (int i = 0; i < n; ++i)
        edges.push_back({poly[i].Node_id, poly[(i + 1) % n].Node_id, -1, group_id});
}

void Mesh::buildNeighbours() {
    neighbours.clear();
    neighbours.resize(nodes.size());
    for (const auto& e : elements) {
        neighbours[e.n0_id].push_back(e.n1_id);
        neighbours[e.n0_id].push_back(e.n2_id);
        neighbours[e.n1_id].push_back(e.n0_id);
        neighbours[e.n1_id].push_back(e.n2_id);
        neighbours[e.n2_id].push_back(e.n0_id);
        neighbours[e.n2_id].push_back(e.n1_id);
    }
    for (auto& n : neighbours) {
        std::sort(n.begin(), n.end());
        n.erase(std::unique(n.begin(), n.end()), n.end());
    }
}

void Mesh::getInteriorNodeNumber(double density) {
    double area = polygonArea(boundaryNodes);
    if (!holeNodes.empty())
        area -= polygonArea(holeNodes);
    numRandomNodes = std::max(1, static_cast<int>(density * area));
}

int Mesh::getMaxNodeRow() const {
    int m = 0; for (const auto& n : nodes) if (n.y > m) m = n.y; return m;
}

int Mesh::getMaxNodeCol() const {
    int m = 0; for (const auto& n : nodes) if (n.x > m) m = n.x; return m;
}

void Mesh::printMeshInfo() const {
    std::cout << "Mesh Info:\n"
              << "  Boundary nodes : " << boundaryNodes.size() << " (IDs 0.."
              << static_cast<int>(boundaryNodes.size())-1 << ")\n"
              << "  Hole nodes     : " << holeNodes.size() << " (IDs "
              << boundaryNodes.size() << ".."
              << boundaryNodes.size()+holeNodes.size()-1 << ")\n"
              << "  Internal nodes : " << internalNodes.size() << "\n"
              << "  Total nodes    : " << nodes.size() << "\n"
              << "  Elements       : " << elements.size() << "\n";
}

} // namespace meshgeneration