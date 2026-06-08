#include "mesh_generation/mesh_generation.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <stdexcept>

namespace meshgeneration {

void Mesh::init(const std::string& filename, double density) {
    nodes.clear(); edges.clear(); boundaryEdges.clear(); elements.clear();
    boundaryNodes.clear(); holeNodes.clear(); internalNodes.clear();
    boundaryGroups.clear();

    std::string ext = std::filesystem::path(filename).extension().string();
    if (ext == ".csv") {
        parseBoundaryCSV(filename);
        if (nodes.empty())
            throw std::runtime_error("No nodes loaded from " + filename);
        registerGroup(0, "outer");
        createOuterBoundary();
        buildFlatNodeList();
        buildEdges(boundaryNodes, 0);
        boundaryEdges = edges;
    } else if (ext == ".dat") {
        parseAerofoilDAT(filename);
        if (holeNodes.empty())
            throw std::runtime_error("No nodes loaded from " + filename);
        registerGroup(0, "outer");
        registerGroup(1, "aerofoil");
        registerGroup(2, "inlet");
        registerGroup(3, "outlet");
        createAerofoilBoundary();
        buildFlatNodeList();
        buildEdges(boundaryNodes, 0);
        buildEdges(holeNodes, 1);
        boundaryEdges = edges;
    } else {
        throw std::runtime_error("Unsupported file format: " + ext);
    }
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

void Mesh::parseAerofoilDAT(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open())
        throw std::runtime_error("Failed to open file: " + filename);
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        double x, y;
        if (iss >> x >> y) {
            holeNodes.push_back({x, y, static_cast<int>(holeNodes.size()), NodeType::Hole, 1});
        } else {
            std::cerr << "Warning: skipping line: " << line << "\n";
        }
    }
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

    double domainMinX = bbox[0].x - 1.0 * chord;
    double domainMaxX = bbox[1].x + 1.50 * chord;
    double domainMinY = bbox[0].y - 1.0 * chord;
    double domainMaxY = bbox[2].y + 1.0 * chord;

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