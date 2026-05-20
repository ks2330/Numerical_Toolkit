#pragma once
#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <memory>
#include <fstream>
#include <iostream>
#include <concepts>

#include "mesh_generation/mesh_types.h"
#include "mesh_generation/mesh_geometry.h"
#include "mesh_generation/shape_generators.h"
#include "mesh_generation/quadtree.h"
#include "mesh_generation/triangulation_algorithm.h"




namespace meshgeneration {
template <typename ComputeFunc>
concept MetricFunction = requires(ComputeFunc compute, Node n0, Node n1, Node n2) {
    { compute(n0, n1, n2) } -> std::convertible_to<double>;
};

template <typename BinFunc>
concept BinFunction = requires(BinFunc toBinIndex, double metricValue) {
    { toBinIndex(metricValue) } -> std::convertible_to<int>;
};
class Mesh {
public:
    std::vector<Node> nodes;
    std::vector<Element> elements;
    std::vector<Edge> edges;
    std::vector<Edge> boundaryEdges;

    std::vector<Node> boundaryNodes;
    std::vector<Node> holeNodes;
    std::vector<Node> internalNodes;
    std::vector<std::vector<int>> neighbours;

    double chord;
    std::map<int, std::string> boundaryGroups;

    void registerGroup(int id, std::string name) { boundaryGroups[id] = std::move(name); }
    int groupId(const std::string& name) const {
        for (const auto& [id, n] : boundaryGroups)
            if (n == name) return id;
        return -1;
    }

    Node getNodeByID(int id) const { return nodes[id]; }
    size_t getNodeIndex(int id) const { return static_cast<size_t>(id); }
    void buildNodeIndexMap() {}
    const std::map<int, std::string>& getGroups() const { return boundaryGroups; }
    static void refineMesh() {}

    void init(const std::string& filename);
    void buildNeighbours();
    void printMeshInfo() const;
    int getMaxNodeRow() const;
    int getMaxNodeCol() const;

    void generateRandomNodes();
    void boundaryLayerSeeding();
    void triangulate(TriangulationAlgorithm& algorithm);
    void validateMesh() const;
    void runAdvancingFront();
    void improveMesh();
    void laplacianSmoothing(int iterations = 10);
    void enforceConstraint();
    void enforceOutsideConstraints();
    void deleteHoles();

    void metricAngles(const std::string& outputFile);
    void metricAspectRatios(const std::string& outputFile);


     template <MetricFunction ComputeFunc, BinFunction BinFunc>

    void writeMetric(int NumBins, const std::string& outputFile, ComputeFunc compute, BinFunc toBinIndex) {
        std::ofstream f(outputFile);
        if (!f.is_open()) {
            std::cerr << "Could not write to " << outputFile << "\n";
            return;
        }
        f << "Metric,Count\n";
        std::vector<int> bins(NumBins, 0);
        for (const auto& e : elements) {
            double metricValue = compute(nodes[e.n0_id], nodes[e.n1_id], nodes[e.n2_id]);
            int binIndex = toBinIndex(metricValue);
            if (binIndex >= 0 && binIndex < NumBins) {
                bins[binIndex]++;
            } else {
                std::cerr << "Warning: Metric value " << metricValue << " out of range for binning.\n";
            }
        }
        for (int i = 0; i < NumBins; ++i) {
            f << i << "," << bins[i] << "\n";
        }
        std::cout << "Metric distribution written to " << outputFile << "\n";
    }
    std::vector<Element> bowyerWatson();
    void advancingFront();

private:
    int element_id_counter = 0;
    int numRandomNodes = 0;
    double Area;

    std::unique_ptr<Quadtree> node_quadtree;

    std::vector<Node> initPoisson();

    void parseBoundaryCSV(const std::string& filename);
    void parseAerofoilDAT(const std::string& filename);
    void createOuterBoundary();
    void createAerofoilBoundary();
    void buildFlatNodeList();
    void buildEdges(const std::vector<Node>& poly, int group_id);
    void getInteriorNodeNumber();

    bool isSdistanceTooClose(const Node& node, double s, double s_boundary);
    double getClosestHoleDistance(const Node& node);
    std::vector<Node> nodesWithinDistanceAdvancingFront(const Node& node, double s);

    bool edgesIntersect(const Edge& e1, const Edge& e2);
    void insertNode(const Node& newNode);
    std::vector<Edge> findCavityEdges(const std::vector<Element>& badTriangles);
    void fillCavity(const std::vector<Edge>& polygon, const Node& apexNode,
                    std::vector<Element>& outElements, int& idCounter,
                    const std::vector<Node>& nodeList,
                    const std::map<int, size_t>& indexMap,
                    const std::vector<Node>* boundary = nullptr);
    static std::vector<Element> findBadTriangles(const std::vector<Element>& triangles,
                                                  const Node& point,
                                                  const std::vector<Node>& nodeList,
                                                  const std::map<int, size_t>& indexMap);

    Node computeCentroid(const Element& e);
    std::tuple<Node,Node,Node> computeEdgeMidpoint(const Element& e);

};

} // namespace meshgeneration