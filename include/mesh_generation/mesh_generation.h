#pragma once
#include <string>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <tuple>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <span>
#include <filesystem>

#include "mesh_generation/mesh_types.h"
#include "mesh_generation/shape_generators.h"

namespace meshgeneration {


    class Mesh {
    public:
        std::vector<Node> nodes; 
        std::vector<Element> elements;
        std::vector<Edge> edges;
        std::vector<Edge> boundaryEdges;

        std::vector<Node> boundaryNodes;   // outer boundary polygon
        std::vector<Node> holeNodes;       // inner hole / aerofoil boundary
        std::vector<Node> internalNodes;   // Poisson-sampled interior

        double chord;   // characteristic length for spacing (used for aerofoil and Poisson sampling)
        // Named boundary groups: group_id → label (e.g. {0,"outer"}, {1,"aerofoil"})
        // Solver applies BCs via: if (node.group_id == groupId("inlet"))
        std::map<int, std::string> boundaryGroups;

        void registerGroup(int id, std::string name) {
            boundaryGroups[id] = std::move(name);
        }

        int groupId(const std::string& name) const {
            for (const auto& [id, n] : boundaryGroups)
                if (n == name) return id;
            return -1;
        }

        void init(std::string filename) {
            nodes.clear(); edges.clear(); boundaryEdges.clear(); elements.clear();
            boundaryNodes.clear(); holeNodes.clear(); internalNodes.clear();
            boundaryGroups.clear();

            std::string ext = std::filesystem::path(filename).extension().string();
            if (ext == ".csv") {
                ParseBoundaryCSV(filename);
                std::cout << "Loaded " << nodes.size() << " corner nodes from " << filename << "\n";
                registerGroup(0, "outer");
                CreateOuterBoundary();
                buildFlatNodeList();
                buildEdges(boundaryNodes, 0);
                boundaryEdges = edges;
            } else if (ext == ".dat") {
                ParseAerofoilDAT(filename);
                std::cout << "Loaded " << holeNodes.size() << " aerofoil nodes from " << filename << "\n";
                registerGroup(0, "outer");
                registerGroup(1, "aerofoil");
                CreateAerofoilBoundary();
                std::cout << "Created bounding box with " << boundaryNodes.size() << " boundary nodes.\n";
                buildFlatNodeList();
                buildEdges(boundaryNodes, 0);
                buildEdges(holeNodes, 1);
                boundaryEdges = edges;
            } else {
                std::cerr << "Unsupported file format: " << ext << "\n";
                return;
            }
            BoundaryLayerSeeding();
            GetInteriorNodeNumber();
        }

        void ParseBoundaryCSV(std::string filename) {
            std::ifstream file(filename);
            if (!file.is_open()) {
                std::cerr << "Failed to open file: " << filename << "\n";
                return;
            }
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
                        std::cerr << "Error parsing line: " << line << " (" << e.what() << ")\n";
                    }
                }
            }
            file.close();
        }

        void ParseAerofoilDAT(std::string filename) {
            std::ifstream file(filename);
            if (!file.is_open()) {
                std::cerr << "Failed to open file: " << filename << "\n";
                return;
            }
            std::string line;
            while (std::getline(file, line)) {
                std::istringstream iss(line);
                double x, y;
                if (iss >> x >> y) {
                    holeNodes.push_back({x, y, static_cast<int>(holeNodes.size()), NodeType::Hole, 1});
                } else {
                    std::cerr << "Warning: Could not parse line: " << line << "\n";
                }

            }
            file.close();
        }

        void CreateOuterBoundary() {
            if (nodes.empty()) return;
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
                    withInterp.push_back({a.x + t*(b.x-a.x), a.y + t*(b.y-a.y), -1, NodeType::Boundary, 0});
                }
            }
            boundaryNodes = withInterp;
            nodes.clear();
        }

        void CreateAerofoilBoundary() {
            if (holeNodes.empty()) return;
            std::vector<Node> bbox = GetBoundingBox(holeNodes);

            chord = bbox[1].x - bbox[0].x;   // aerofoil chord length
            std::cout << "Aerofoil chord length is: " << chord << "\n";
            if (chord <= 0) chord = 1.0;

            // Additive offsets based on chord — works regardless of where the aerofoil sits
            double domainMinX = bbox[0].x - 1.0 * chord;   // 10c upstream
            double domainMaxX = bbox[1].x + 1.50 * chord;   // 15c downstream (wake)
            double domainMinY = bbox[0].y - 1.0 * chord;   // 10c below
            double domainMaxY = bbox[2].y + 1.0 * chord;   // 10c above

            Node TL = {domainMinX, domainMaxY, -1};
            Node TR = {domainMaxX, domainMaxY, -1};
            Node BR = {domainMaxX, domainMinY, -1};
            Node BL = {domainMinX, domainMinY, -1};

            // Segment spacing scales with chord so density is consistent
            double segLen = chord * 0.5;
            auto interpolate = [&](const Node& a, const Node& b) {
                std::vector<Node> result;
                double len = distance(a, b);
                int segs = std::max(1, static_cast<int>(len / segLen));
                for (int s = 0; s < segs; ++s) {
                    double t = static_cast<double>(s) / segs;
                    result.push_back({a.x + t*(b.x-a.x), a.y + t*(b.y-a.y), -1, NodeType::Boundary, 0});
                }
                return result;
            };

            // CCW: TL → TR → BR → BL
            std::vector<Node> boxNodes;
            for (auto& seg : {interpolate(TL, TR), interpolate(TR, BR),
                               interpolate(BR, BL), interpolate(BL, TL)})
                boxNodes.insert(boxNodes.end(), seg.begin(), seg.end());

            boundaryNodes = boxNodes;
            // Edges are built after buildFlatNodeList assigns final IDs
        }

        // Assigns sequential IDs (0..N-1) so that node.Node_id == index in `nodes`.
        // Must be called before buildEdges.
        void buildFlatNodeList() {
            nodes.clear();
            int id = 0;

            for (auto n : boundaryNodes) {
                n.Node_id = id; n.type = NodeType::Boundary;
                nodes.push_back(n); boundaryNodes[id] = nodes.back(); ++id;
            }
            // Correct the named vector IDs in one pass
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

        // Builds closed polygon edges for a node group using their final IDs.
        void buildEdges(const std::vector<Node>& poly, int group_id) {
            int n = static_cast<int>(poly.size());
            for (int i = 0; i < n; ++i)
                edges.push_back({poly[i].Node_id, poly[(i + 1) % n].Node_id, -1, group_id});
        }

        void GetInteriorNodeNumber() {
            auto bbox = GetBoundingBox(boundaryNodes.empty() ? nodes : boundaryNodes);
            numRandomNodes = static_cast<int>((boundaryEdges.size() * boundaryEdges.size()) / 1000);
        }

        // Generates random interior nodes via Poisson disc sampling.
        void generateRandomNodes() {
            std::vector<Node> boundingBoxNodes = GetBoundingBox(boundaryNodes);
            double minX = boundingBoxNodes[0].x, maxX = boundingBoxNodes[1].x;
            double minY = boundingBoxNodes[0].y, maxY = boundingBoxNodes[3].y;

            int k = 30;
            double s_min = std::min((maxX - minX), (maxY - minY)) / std::sqrt(numRandomNodes) * 0.05;  // base spacing tuned for good results at 100-200 nodes; scales with domain size and node count
            std::vector<Node> activeNodes = initPoisson();

            while (!activeNodes.empty() && internalNodes.size() < static_cast<size_t>(numRandomNodes)) {
                int idx = rand() % activeNodes.size();
                Node activeNode = activeNodes[idx];
                double d = GetClosestHoleDistance(activeNode);
                double growth_rate = 10.0;
                double s_max = s_min * 5.0;
                double s_varied = std::clamp(s_min + growth_rate * d, s_min, s_max);

                bool found = false;
                for (int tries = 0; tries < k; ++tries) {
                    double angle = static_cast<double>(rand()) / RAND_MAX * 2 * M_PI;
                    double radius = s_varied * (1 + static_cast<double>(rand()) / RAND_MAX);
                    int nextId = static_cast<int>(nodes.size());
                    Node newNode = {activeNode.x + radius * cos(angle),
                                    activeNode.y + radius * sin(angle),
                                    nextId, NodeType::Internal, -1};
                    bool inOuter = isPointInPolygon(newNode, boundaryNodes);
                    bool inHole  = !holeNodes.empty() && isPointInPolygon(newNode, holeNodes);
                    if (inOuter && !inHole && !isSdistanceTooClose(newNode, s_varied, s_min)) {
                        internalNodes.push_back(newNode);
                        nodes.push_back(newNode);   // ID == index since appended at end
                        activeNodes.push_back(newNode);
                        found = true;
                        break;
                    }
                }
                if (!found) activeNodes.erase(activeNodes.begin() + idx);
            }
        }

        std::vector<Node> initPoisson() {
            int nextId = static_cast<int>(nodes.size());

            double minX, maxX, minY, maxY;
            if (!boundaryNodes.empty()) {
                auto bbox = GetBoundingBox(boundaryNodes);
                minX = bbox[0].x; maxX = bbox[1].x; minY = bbox[0].y; maxY = bbox[3].y;
            } else if (!holeNodes.empty()) {
                auto bbox = GetBoundingBox(holeNodes);
                minX = bbox[0].x; maxX = bbox[1].x; minY = bbox[0].y; maxY = bbox[3].y;
            } else {
                return {};  // nothing to work with
            }

            std::vector<Node> activeNodes;

            if (boundaryNodes.empty()) return activeNodes;

            bool placed = false;
            int attempts = 0;
            while (!placed && attempts < 10000) {
                ++attempts;
                double x = static_cast<double>(rand()) / RAND_MAX * (maxX - minX) + minX;
                double y = static_cast<double>(rand()) / RAND_MAX * (maxY - minY) + minY;
                Node seed = {x, y, nextId, NodeType::Internal, -1};
                bool inOuter = isPointInPolygon(seed, boundaryNodes);
                bool inHole  = !holeNodes.empty() && isPointInPolygon(seed, holeNodes);
                if (inOuter && !inHole) {
                    internalNodes.push_back(seed);
                    nodes.push_back(seed);
                    activeNodes.push_back(seed);
                    placed = true;
                }
            }
            if (!placed)
                std::cerr << "Failed to place initial Poisson node after 10000 attempts.\n";
            return activeNodes;
        }

        inline double getVectorLength(const Node& a, const Node& b) {
            double dx = b.x - a.x, dy = b.y - a.y;
            double len = std::sqrt(dx*dx + dy*dy);
            return len;
        }

        void BoundaryLayerSeeding(){
            if (boundaryEdges.empty()) return;
            if (holeNodes.empty()) return;

            int numLayers = 3;
            std::pair<double, double> perpendicular;
            std::pair<double, double> tangent;
            std::pair<double, double> unitVec;

            bool isCCW_BOOL = false;
            if (isCCW(holeNodes)) {
                isCCW_BOOL = true;
            } else {
                isCCW_BOOL = false;
            }

            double ratio = 1.5;
            double h0 = chord * 0.01;   // initial layer spacing based on chord length; can be tuned for different results
            std::cout << "Chord Length is:" << chord << "\n";
            std::cout << "Initial Layer Spacing is:" << h0 << "\n";

            for (int j = 0; j < static_cast<int>(holeNodes.size()); ++j) {
                double layerDistance = 0;
                if (isCCW_BOOL) {
                    perpendicular = edgeDirection(holeNodes[j], holeNodes[(j + 1) % holeNodes.size()]);
                    tangent = {perpendicular.second, -perpendicular.first};
                    double len = getVectorLength(holeNodes[j], holeNodes[(j + 1) % holeNodes.size()]); 
                    unitVec = {tangent.first / len, tangent.second / len};
                } else {
                    perpendicular = edgeDirection(holeNodes[(j + 1) % holeNodes.size()], holeNodes[j]);
                    tangent = {-perpendicular.second, perpendicular.first};
                    double len = getVectorLength(holeNodes[(j + 1) % holeNodes.size()], holeNodes[j]);
                    unitVec = {tangent.first / len, tangent.second / len};
                }    
                for (int i = 0; i < numLayers; ++i) {
                    layerDistance += h0 * std::pow(ratio, i); // geometric growth
                    Node NewNode = {holeNodes[j].x + unitVec.first * layerDistance, holeNodes[j].y + unitVec.second * layerDistance, static_cast<int>(nodes.size()), NodeType::Internal, -1};
                    if (isPointInPolygon(NewNode, boundaryNodes) && !isPointInPolygon(NewNode, holeNodes)) {
                        internalNodes.push_back(NewNode);
                        nodes.push_back(NewNode);
                    } else {
                        break;
                    }   
                }
            }

        }

        bool isSdistanceTooClose(const Node& node, double s, double s_boundary) {
            for (const auto& b : boundaryNodes) {
                double dx = b.x - node.x, dy = b.y - node.y;
                if (dx*dx + dy*dy < s_boundary*s_boundary) return true;
            }
            for (const auto& n : internalNodes) {
                double dx = n.x - node.x, dy = n.y - node.y;
                if (dx*dx + dy*dy < s*s) return true;
            }
            return false;
        }


        double GetClosestHoleDistance(const Node& node) {
            double minDist = std::numeric_limits<double>::max();
            for (const auto& n : holeNodes) {
                double dx = n.x - node.x, dy = n.y - node.y;
                double distSq = dx*dx + dy*dy;
                if (distSq < minDist) minDist = distSq;
            }
            return std::sqrt(minDist);
        }

        bool edgesIntersect(const Edge& e1, const Edge& e2) {
            const Node& p1 = nodes[e1.n0_id]; const Node& p2 = nodes[e1.n1_id];
            const Node& q1 = nodes[e2.n0_id]; const Node& q2 = nodes[e2.n1_id];
            double rx = p2.x - p1.x, ry = p2.y - p1.y;
            double qx = q2.x - q1.x, qy = q2.y - q1.y;
            double det = rx * qy - ry * qx;
            if (std::abs(det) < 1e-10) return false;
            double t = ((q1.x - p1.x) * qy - (q1.y - p1.y) * qx) / det;
            double u = ((q1.x - p1.x) * ry - (q1.y - p1.y) * rx) / det;
            return t > 1e-10 && t < 1 - 1e-10 && u > 1e-10 && u < 1 - 1e-10;
        }

        void enforceConstraint() {
            for (const auto& edge : edges) {
                std::vector<Element> intersected;
                bool exists = false;
                for (const auto& element : elements) {
                    if (isSameEdge(element, edge)) { exists = true; break; }
                }
                if (exists) continue;

                for (const auto& element : elements) {
                    Edge e1 = {element.n0_id, element.n1_id, -1};
                    Edge e2 = {element.n1_id, element.n2_id, -1};
                    Edge e3 = {element.n2_id, element.n0_id, -1};
                    if (edgesIntersect(edge, e1) || edgesIntersect(edge, e2) || edgesIntersect(edge, e3))
                        intersected.push_back(element);
                }
                elements.erase(std::remove_if(elements.begin(), elements.end(), [&](const Element& e) {
                    for (const auto& ie : intersected)
                        if (e.Element_id == ie.Element_id) return true;
                    return false;
                }), elements.end());

                std::vector<Edge> cavity = findCavityEdges(intersected);
                for (const auto& cEdge : cavity) {
                    if (cEdge.n0_id == edge.n0_id || cEdge.n1_id == edge.n0_id) continue;
                    double cross = orient2d(nodes[edge.n0_id], nodes[cEdge.n0_id], nodes[cEdge.n1_id]);
                    if (cross < 0)
                        elements.push_back({edge.n0_id, cEdge.n1_id, cEdge.n0_id, element_id_counter++});
                    else
                        elements.push_back({edge.n0_id, cEdge.n0_id, cEdge.n1_id, element_id_counter++});
                }
            }
        }

        void enforceOutsideConstraints() {
            if (boundaryNodes.empty()) return;
            elements.erase(std::remove_if(elements.begin(), elements.end(), [&](const Element& e) {
                return !isPointInPolygon(computeCentroid(e), boundaryNodes);
            }), elements.end());
        }

        void triangulate() {
            if (nodes.empty()) return;
            elements = bowyerWatson();
            enforceConstraint();
            deleteHoles();
            enforceOutsideConstraints();
            MetricAngles("angle_distribution.csv");
            MetricAspectRatios("aspect_ratio_distribution.csv");
            ImproveMesh();
            MetricAngles("angle_distribution_improved.csv");
            MetricAspectRatios("aspect_ratio_distribution_improved.csv");
        }

        void ImproveMesh() {
            std::cout << "Improving mesh quality...\n";
            bool foundBadElement = true;
            int iteration = 0;
            while (foundBadElement && iteration < 100) {
                foundBadElement = false; ++iteration;
                for (auto& element : elements) {
                    double angle0 = minAngle(nodes[element.n0_id], nodes[element.n1_id], nodes[element.n2_id]);
                    double ratio  = aspectRatio(nodes[element.n0_id], nodes[element.n1_id], nodes[element.n2_id]);
                    if (angle0 < 20 * M_PI / 180 || ratio > 10) {
                        insertNode(computeCentroid(element));
                        foundBadElement = true;
                        break;
                    }
                }
            }
            enforceConstraint();
            enforceOutsideConstraints();
            deleteHoles();
        }

        std::vector<Edge> findCavityEdges(const std::vector<Element>& badTriangles) {
            std::vector<Edge> polygon;
            for (const auto& tri : badTriangles) {
                Edge tedges[3] = {{tri.n0_id, tri.n1_id, -1},
                                   {tri.n1_id, tri.n2_id, -1},
                                   {tri.n2_id, tri.n0_id, -1}};
                for (const auto& e : tedges) {
                    int count = 0;
                    for (const auto& other : badTriangles)
                        if (isSameEdge(other, e)) ++count;
                    if (count == 1) {
                        bool dup = false;
                        for (const auto& pe : polygon)
                            if ((pe.n0_id == e.n0_id && pe.n1_id == e.n1_id) ||
                                (pe.n0_id == e.n1_id && pe.n1_id == e.n0_id)) { dup = true; break; }
                        if (!dup) polygon.push_back(e);
                    }
                }
            }
            return polygon;
        }

        // Uses indexMap so it works inside bowyerWatson where all_points includes super-triangle nodes.
        static std::vector<Element> findBadTriangles(
            const std::vector<Element>& triangles,
            const Node& point,
            const std::vector<Node>& nodeList,
            const std::map<int, size_t>& indexMap)
        {
            std::vector<Element> bad;
            for (const auto& t : triangles) {
                const Node& n0 = nodeList[indexMap.at(t.n0_id)];
                const Node& n1 = nodeList[indexMap.at(t.n1_id)];
                const Node& n2 = nodeList[indexMap.at(t.n2_id)];
                if (isInCircle(n0, n1, n2, point)) bad.push_back(t);
            }
            return bad;
        }

        void fillCavity(
            const std::vector<Edge>& polygon,
            const Node& apexNode,
            std::vector<Element>& outElements,
            int& idCounter,
            const std::vector<Node>& nodeList,
            const std::map<int, size_t>& indexMap,
            const std::vector<Node>* boundary = nullptr)
        {
            for (const auto& edge : polygon) {
                const Node& na = nodeList[indexMap.at(edge.n0_id)];
                const Node& nb = nodeList[indexMap.at(edge.n1_id)];
                if (boundary) {
                    Node mid = {(na.x + nb.x + apexNode.x) / 3.0,
                                (na.y + nb.y + apexNode.y) / 3.0, -1};
                    if (!isPointInPolygon(mid, *boundary)) continue;
                }
                double cross = orient2d(na, nb, apexNode);
                if (cross < 0)
                    outElements.push_back({edge.n1_id, edge.n0_id, apexNode.Node_id, idCounter++});
                else
                    outElements.push_back({edge.n0_id, edge.n1_id, apexNode.Node_id, idCounter++});
            }
        }

        void insertNode(const Node& newNode) {
            if (!holeNodes.empty() && isPointInPolygon(newNode, holeNodes)) return;
            Node n  = newNode;
            n.Node_id   = static_cast<int>(nodes.size());  // appended at end → ID == index
            n.type      = NodeType::Internal;
            nodes.push_back(n);
            internalNodes.push_back(n);

            // Build identity map for this call (ID == index for all real nodes)
            std::map<int, size_t> idMap;
            for (size_t i = 0; i < nodes.size(); ++i) idMap[nodes[i].Node_id] = i;

            std::vector<Element> bad     = findBadTriangles(elements, n, nodes, idMap);
            std::vector<Edge>    polygon = findCavityEdges(bad);

            elements.erase(std::remove_if(elements.begin(), elements.end(), [&](const Element& t) {
                for (const auto& b : bad) if (t.Element_id == b.Element_id) return true;
                return false;
            }), elements.end());

            fillCavity(polygon, n, elements, element_id_counter, nodes, idMap, &boundaryNodes);
        }

        void MetricAngles(std::string outputFile) {
            std::vector<int> bins(18, 0);
            for (const auto& e : elements) {
                double a = minAngle(nodes[e.n0_id], nodes[e.n1_id], nodes[e.n2_id]);
                bins[std::min(static_cast<int>(a * 180.0 / M_PI) / 10, 17)]++;
            }
            std::ofstream f(outputFile);
            f << "Angle (degrees),Count\n";
            for (size_t i = 0; i < bins.size(); ++i) f << i * 10 << "," << bins[i] << "\n";
            std::cout << "Angle distribution written to " << outputFile << "\n";
        }

        void MetricAspectRatios(std::string outputFile) {
            std::vector<int> bins(10, 0);
            for (const auto& e : elements) {
                double r = aspectRatio(nodes[e.n0_id], nodes[e.n1_id], nodes[e.n2_id]);
                bins[std::min(static_cast<int>(r / 10), 9)]++;
            }
            std::ofstream f(outputFile);
            f << "Aspect Ratio,Count\n";
            for (size_t i = 0; i < bins.size(); ++i) f << i * 10 << "," << bins[i] << "\n";
            std::cout << "Aspect ratio distribution written to " << outputFile << "\n";
        }

        double minAngle(const Node& a, const Node& b, const Node& c) {
            double ab = distance(a, b), bc = distance(b, c), ca = distance(c, a);
            double A = std::acos(std::clamp((ab*ab + ca*ca - bc*bc) / (2*ab*ca), -1.0, 1.0));
            double B = std::acos(std::clamp((ab*ab + bc*bc - ca*ca) / (2*ab*bc), -1.0, 1.0));
            double C = std::acos(std::clamp((bc*bc + ca*ca - ab*ab) / (2*bc*ca), -1.0, 1.0));
            return std::min({A, B, C});
        }

        double aspectRatio(const Node& a, const Node& b, const Node& c) {
            double ab = distance(a, b), bc = distance(b, c), ca = distance(c, a);
            double longest = std::max({ab, bc, ca});
            double area = 0.5 * std::abs(orient2d(a, b, c));
            return (0.433 * longest * longest) / area;
        }

        static double orient2d(const Node& a, const Node& b, const Node& c) {
            return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
        }

        // ID == index, so lookup is direct array access.
        Node getNodeByID(int id) const { return nodes[id]; }
        size_t getNodeIndex(int id) const { return static_cast<size_t>(id); }

        // Kept for API compatibility — no longer needed since ID == index.
        void buildNodeIndexMap() {}

        const std::map<int, std::string>& getGroups() const { return boundaryGroups; }

        static double distance(const Node& a, const Node& b) {
            return std::sqrt((b.x-a.x)*(b.x-a.x) + (b.y-a.y)*(b.y-a.y));
        }
        static double distanceSquared(const Node& a, const Node& b) {
            return (b.x-a.x)*(b.x-a.x) + (b.y-a.y)*(b.y-a.y);
        }

        int getMaxNodeRow() const {
            int m = 0; for (const auto& n : nodes) if (n.y > m) m = n.y; return m;
        }
        int getMaxNodeCol() const {
            int m = 0; for (const auto& n : nodes) if (n.x > m) m = n.x; return m;
        }

        static void refineMesh() {}

        Node computeCentroid(const Element& e) {
            const Node& n1 = nodes[e.n0_id];
            const Node& n2 = nodes[e.n1_id];
            const Node& n3 = nodes[e.n2_id];
            return {(n1.x+n2.x+n3.x)/3.0, (n1.y+n2.y+n3.y)/3.0, -1};
        }

        std::tuple<Node,Node,Node> computeEdgeMidpoint(const Element& e) {
            const Node& n1 = nodes[e.n0_id];
            const Node& n2 = nodes[e.n1_id];
            const Node& n3 = nodes[e.n2_id];
            return { {(n1.x+n2.x)/2.0, (n1.y+n2.y)/2.0, -1},
                     {(n2.x+n3.x)/2.0, (n2.y+n3.y)/2.0, -1},
                     {(n3.x+n1.x)/2.0, (n3.y+n1.y)/2.0, -1} };
        }

        void deleteHoles() {
            if (holeNodes.empty()) return;
            elements.erase(std::remove_if(elements.begin(), elements.end(), [&](const Element& e) {
                return isPointInPolygon(computeCentroid(e), holeNodes);
            }), elements.end());
        }

        bool isPointInPolygon(const Node& point, const std::vector<Node>& boundary) {
            if (boundary.empty()) return false;
            int hits = 0;
            size_t n = boundary.size();
            for (size_t i = 0; i < n; ++i) {
                const Node& p1 = boundary[i];
                const Node& p2 = boundary[(i + 1) % n];
                if (point.y > std::min(p1.y, p2.y) && point.y <= std::max(p1.y, p2.y)) {
                    double xInt = p1.x + (point.y - p1.y) * (p2.x - p1.x) / (p2.y - p1.y);
                    if (xInt > point.x) ++hits;
                }
            }
            return hits % 2 == 1;
        }

        void printMeshInfo() const {
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

        std::vector<Node> GetBoundingBox(const std::vector<Node>& b) const {
            if (b.empty()) return {};
            double minX = b[0].x, maxX = b[0].x, minY = b[0].y, maxY = b[0].y;
            for (const auto& n : b) {
                minX = std::min(minX, n.x); maxX = std::max(maxX, n.x);
                minY = std::min(minY, n.y); maxY = std::max(maxY, n.y);
            }
            return {{minX,minY,-1},{maxX,minY,-1},{maxX,maxY,-1},{minX,maxY,-1}};
        }


    private:
        int element_id_counter = 0;
        int numRandomNodes = 0;
        double Area;


        static bool isCCW(std::vector<Node> NodeList){
            double LocalSum = 0;
            for (int i = 0; i < NodeList.size(); ++i) {
                LocalSum += (NodeList[(i+1)%NodeList.size()].x - NodeList[i].x) * (NodeList[(i+1)%NodeList.size()].y + NodeList[i].y);
            }
            return LocalSum < 0;
        }

        static std::pair<double, double> edgeDirection(const Node& a, const Node& b){
            return {b.x - a.x, b.y - a.y};
        }

        static bool isInCircle(Node A, Node B, Node C, Node D) {
            double adx = A.x-D.x, ady = A.y-D.y;
            double bdx = B.x-D.x, bdy = B.y-D.y;
            double cdx = C.x-D.x, cdy = C.y-D.y;
            double det = (adx*adx+ady*ady)*(bdx*cdy-cdx*bdy)
                        -(bdx*bdx+bdy*bdy)*(adx*cdy-cdx*ady)
                        +(cdx*cdx+cdy*cdy)*(adx*bdy-bdx*ady);
            return det > 0;
        }

        static bool isSameEdge(const Element& t, const Edge& e) {
            auto match = [](int a, int b, const Edge& e) {
                return (a == e.n0_id && b == e.n1_id) || (a == e.n1_id && b == e.n0_id);
            };
            return match(t.n0_id, t.n1_id, e) ||
                   match(t.n1_id, t.n2_id, e) ||
                   match(t.n2_id, t.n0_id, e);
        }

        std::vector<Element> bowyerWatson() {
            auto bbox = GetBoundingBox(nodes);
            double minX = bbox[0].x, maxX = bbox[1].x;
            double minY = bbox[0].y, maxY = bbox[2].y;
            double M = std::max(maxX-minX, maxY-minY) * 2.0;
            double midX = (minX+maxX)/2.0, midY = (minY+maxY)/2.0;

            // Super-triangle nodes use negative IDs — kept separate from real nodes
            std::vector<Node> superNodes = {
                {midX-M, midY-M, -1},
                {midX+M, midY-M, -2},
                {midX,   midY+2*M, -3}
            };

            std::vector<Node> all = nodes;
            all.insert(all.end(), superNodes.begin(), superNodes.end());

            // Local map needed because super-triangle IDs are negative
            std::map<int, size_t> localMap;
            for (size_t i = 0; i < all.size(); ++i) localMap[all[i].Node_id] = i;

            element_id_counter = 0;
            std::vector<Element> tris = {{-1, -2, -3, element_id_counter++}};

            for (const auto& point : nodes) {
                std::vector<Element> bad     = findBadTriangles(tris, point, all, localMap);
                std::vector<Edge>    polygon = findCavityEdges(bad);

                tris.erase(std::remove_if(tris.begin(), tris.end(), [&](const Element& t) {
                    for (const auto& b : bad) if (t.Element_id == b.Element_id) return true;
                    return false;
                }), tris.end());

                fillCavity(polygon, point, tris, element_id_counter, all, localMap);
            }

            tris.erase(std::remove_if(tris.begin(), tris.end(), [](const Element& t) {
                return t.n0_id < 0 || t.n1_id < 0 || t.n2_id < 0;
            }), tris.end());

            return tris;
        }
    };
}
