#pragma once
#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <memory>

#include "mesh_generation/mesh_types.h"
#include "mesh_generation/mesh_geometry.h"
#include "mesh_generation/shape_generators.h"
#include "mesh_generation/quadtree.h"

namespace meshgeneration {

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

    void init(std::string filename);
    void ParseBoundaryCSV(std::string filename);
    void ParseAerofoilDAT(std::string filename);
    void CreateOuterBoundary();
    void CreateAerofoilBoundary();
    void buildFlatNodeList();
    void buildEdges(const std::vector<Node>& poly, int group_id);
    void buildNeighbours();
    void GetInteriorNodeNumber();
    void printMeshInfo() const;
    int getMaxNodeRow() const;
    int getMaxNodeCol() const;

    void generateRandomNodes();
    void BoundaryLayerSeeding();
    bool isSdistanceTooClose(const Node& node, double s, double s_boundary);
    double GetClosestHoleDistance(const Node& node);
    std::vector<Node> NodesWithinDistanceAdvancingFront(const Node& node, double s);
    void triangulate(std::string method = "delaunay");
    void run_AdvancingFront();
    void ImproveMesh();
    void LaplacianSmoothing(int iterations = 10);
    void enforceConstraint();
    void enforceOutsideConstraints();
    void deleteHoles();
    void insertNode(const Node& newNode);
    bool edgesIntersect(const Edge& e1, const Edge& e2);
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

    void MetricAngles(std::string outputFile);
    void MetricAspectRatios(std::string outputFile);
    Node computeCentroid(const Element& e);
    std::tuple<Node,Node,Node> computeEdgeMidpoint(const Element& e);

private:
    int element_id_counter = 0;
    int numRandomNodes = 0;
    double Area;

    std::unique_ptr<Quadtree> node_quadtree;
    std::vector<Element> bowyerWatson();
    std::vector<Node> initPoisson();
    void AdvancingFront();
};

} // namespace meshgeneration