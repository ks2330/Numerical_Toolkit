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
        std::vector<Node> holes;

        // Loads boundary corner nodes from a 2-column CSV (x,y) and interpolates
        // edges between each consecutive pair of corners.
        void init(std::string filename) {
            nodes.clear(); edges.clear(); holes.clear(); outerBoundary.clear();
            std::string ext = std::filesystem::path(filename).extension().string();
            if (ext == ".csv") {
                ParseBoundaryCSV(filename);
                std::cout << "Loaded " << nodes.size() << " nodes from " << filename << "\n";
                CreateOuterBoundary();
            } else if (ext == ".dat") {
                ParseAeofoilDAT(filename);
                std::cout << "Loaded " << nodes.size() << " nodes from " << filename << "\n";
                CreateOuterBoundary();
            } else {
                std::cerr << "Unsupported file format: " << ext << "\n";
                return;
            }
            GetInteriorNodeNumber();
            buildNodeIndexMap();
            bool isGenerated = true;
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
                        nodes.push_back({x, y, static_cast<int>(nodes.size())});
                    } catch (const std::exception& e) {
                        std::cerr << "Error parsing line: " << line << " (" << e.what() << ")\n";
                    }
                }
            }
            file.close();
        }

        void ParseAeofoilDAT(std::string filename) {
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
                    nodes.push_back({x,y, static_cast<int>(nodes.size())});
                } else {
                    std::cerr << "Warning: COuld not parse line: " << line << "\n";
                }
            }
            file.close();
        }

        void CreateOuterBoundary() {
            if (nodes.empty()) return;
            // Interpolate additional nodes along each edge between corners
            std::vector<Node> withInterp;
            size_t n = nodes.size();
            int id = 0;
            for (size_t i = 0; i < n; ++i) {
                const Node& a = nodes[i];
                const Node& b = nodes[(i + 1) % n];
                withInterp.push_back({a.x, a.y, id++});
                double len = distance(a, b);
                int segs = std::max(1, static_cast<int>(len / 50.0));
                for (int s = 1; s < segs; ++s) {
                    double t = static_cast<double>(s) / segs;
                    withInterp.push_back({a.x + t*(b.x-a.x), a.y + t*(b.y-a.y), id++});
                }
            }
            nodes = withInterp;
            totalBoundaryNodes = static_cast<int>(nodes.size());
            outerBoundary = nodes;
            for (int i = 0; i < totalBoundaryNodes; ++i)
                edges.push_back({nodes[i].Node_id, nodes[(i+1) % totalBoundaryNodes].Node_id, -1});
            boundaryEdges = edges;
        }

        void CreateAeofoilBoundary() {
            if (holes.empty()) return;
            std::vector<Node> bbox = GetBoundingBox(holes);
            double minX = bbox[0].x * 1000, maxX = bbox[1].x * 1000;
            double minY = bbox[0].y * 1000, maxY = bbox[2].y * 1000;
            std::vector<Node> withInterp;
            std::vector<Node> boundaryNodes;
            std::vector<Node> TopLeft = {{minX, maxY, -1}};
            std::vector<Node> TopRight = {{maxX, maxY, -2}};
            std::vector<Node> BottomLeft = {{minX, minY, -3}};
            std::vector<Node> BottomRight = {{maxX, minY, -4}};
            boundaryNodes.push_back(TopLeft[0]);
            boundaryNodes.push_back(TopRight[0]);
            boundaryNodes.push_back(BottomLeft[0]);
            boundaryNodes.push_back(BottomRight[0]);
        }
        
        void GetInteriorNodeNumber(){
            auto bbox = GetBoundingBox(nodes);
            double minX = bbox[0].x, maxX = bbox[1].x;
            double minY = bbox[0].y, maxY = bbox[2].y;
            double dx = maxX - minX, dy = maxY - minY;
            numRandomNodes = static_cast<int>((boundaryEdges.size() * boundaryEdges.size()) / 2500);
        }


        // Generates random interior nodes and adds them to the `nodes` vector.
        void generateRandomNodes() {

                std::vector<Node> boundary(nodes.begin(), nodes.begin() + totalBoundaryNodes);  
                std::vector<Node> boundingBoxNodes = GetBoundingBox(boundary);

                double minX = boundingBoxNodes[0].x;
                double maxX = boundingBoxNodes[1].x;
                double minY = boundingBoxNodes[0].y;
                double maxY = boundingBoxNodes[3].y;

                int k = 30;
                double s = std::min((maxX - minX), (maxY - minY)) / std::sqrt(numRandomNodes) / std::sqrt(2.0);

                std::vector<Node> activeNodes = initPoisson();
                int id_counter = nodes.size();
                while(activeNodes.size() > 0 && nodes.size() - totalBoundaryNodes < static_cast<size_t>(numRandomNodes)) {
                    int idx = rand() % activeNodes.size();
                    Node activeNode = activeNodes[idx];
                    bool found = false;
                    for (int tries = 0; tries < k; ++tries) {
                        double angle = static_cast<double>(rand()) / RAND_MAX * 2 * M_PI;
                        double radius = s * (1 + static_cast<double>(rand()) / RAND_MAX);
                        Node newNode = {activeNode.x + radius * cos(angle), activeNode.y + radius * sin(angle), id_counter};
                        if (isPointInPolygon(newNode, boundary) && !isSdistanceTooClose(newNode, s)) {
                            nodes.push_back(newNode);
                            id_counter++;
                            activeNodes.push_back(newNode);
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        activeNodes.erase(activeNodes.begin() + idx);
                    }
                }
                
                buildNodeIndexMap();

        }

        std::vector<Node> initPoisson() {

            int id_counter = nodes.size();

            std::vector<Node> boundary(nodes.begin(), nodes.begin() + totalBoundaryNodes);  
            std::vector<Node> boundingBoxNodes = GetBoundingBox(boundary);

            double minX = boundingBoxNodes[0].x;
            double maxX = boundingBoxNodes[1].x;
            double minY = boundingBoxNodes[0].y;
            double maxY = boundingBoxNodes[3].y;
            std::vector<Node> activeNodes;

            if (!outerBoundary.empty()) {
                    bool node_placed = false;
                    int attempts = 0;
                    const int maxAttempts = 10000;
                    while(!node_placed && attempts < maxAttempts) {
                        attempts++;
                        double x = static_cast<double>(rand()) / RAND_MAX * (maxX - minX) + minX;
                        double y = static_cast<double>(rand()) / RAND_MAX * (maxY - minY) + minY;
                        Node randomNode = {x, y, id_counter};
                        if(isPointInPolygon(randomNode, outerBoundary)){
                            nodes.push_back(randomNode);
                            id_counter++;
                            node_placed = true;
                            activeNodes.push_back(randomNode);
                        }

                    }
                    if (!node_placed) {
                        std::cerr << "Failed to place initial Poisson node after " << maxAttempts << " attempts.\n";
                    }
                    if (node_placed){
                        std::cout << "Placed initial Poisson node at (" << activeNodes[0].x << ", " << activeNodes[0].y << ")\n";
                    }
                }
            return activeNodes;
        }

        bool isSdistanceTooClose(const Node& node, double s) {
            for (size_t i = totalBoundaryNodes; i < nodes.size(); ++i) {
                double dx = nodes[i].x - node.x;
                double dy = nodes[i].y - node.y;
                if (dx*dx + dy*dy < s*s) return true;
            }
            return false;
        }

        bool edgesIntersect(const Edge& e1, const Edge& e2){
            Node p1 = getNodeByID(e1.n0_id);
            Node p2 = getNodeByID(e1.n1_id);
            Node q1 = getNodeByID(e2.n0_id);
            Node q2 = getNodeByID(e2.n1_id);
            double rx = p2.x - p1.x;
            double ry = p2.y - p1.y;
            double qx = q2.x - q1.x;
            double qy = q2.y - q1.y;
            double det = rx * qy - ry * qx;
            if (std::abs(det) < 1e-10) {
                return false; // Lines are parallel
            }
            double t = ((q1.x - p1.x) * qy - (q1.y - p1.y) * qx) / det;
            double s = ((q1.x - p1.x) * ry - (q1.y - p1.y) * rx) / det;
            if (t > 1e-10 && t < 1 - 1e-10 && s > 1e-10 && s < 1 - 1e-10) {
                return true;
            }

            return false; // Lines do not intersect

        }

        void enforceConstraint() {

            // This function can be used to enforce any constraints on the mesh after triangulation, such as ensuring boundary edges are present.
            for (const auto& edge : edges) {
                std::vector<Element> intersected;
                bool existBoundaryEdge = false;
                // Check if this edge exists in the triangulation and if not, add it as a constraint.
                for (const auto& element : elements) {
                    if (isSameEdge(element, edge)) {
                        existBoundaryEdge = true;
                        break;
                    }
                }
                if (existBoundaryEdge) {
                        continue;
                    }
                // Now we need to check this edge against every edge in the triangulation to see if it intersects with any of them. If it does, we need to split the intersecting edge and add the constraint edge.
                for (const auto& element : elements) {
                    Edge e1 = {element.n0_id, element.n1_id, -1};
                    Edge e2 = {element.n1_id, element.n2_id, -1};
                    Edge e3 = {element.n2_id, element.n0_id, -1};
                    if (edgesIntersect(edge, e1) || edgesIntersect(edge, e2) || edgesIntersect(edge, e3)) {
                        intersected.push_back(element);
                    }
                }
               elements.erase(std::remove_if(elements.begin(), elements.end(), [&](const Element& e) {
                    for (const auto& interesectElements : intersected) {
                        if (e.Element_id == interesectElements.Element_id) return true;
                    }
                    return false;
                }), elements.end());
                std::vector<Edge> cavityEdges = findCavityEdges(intersected);
                for (const auto& cEdge : cavityEdges) {
                    if (cEdge.n0_id == edge.n0_id || cEdge.n1_id == edge.n0_id) continue;
                    Node na = getNodeByID(edge.n0_id);
                    Node nb = getNodeByID(cEdge.n0_id);
                    Node nc = getNodeByID(cEdge.n1_id);
                    double cross = orient2d(na, nb, nc);
                    if (cross < 0)
                        elements.push_back({edge.n0_id, cEdge.n1_id, cEdge.n0_id, element_id_counter++});
                    else
                        elements.push_back({edge.n0_id, cEdge.n0_id, cEdge.n1_id, element_id_counter++});
                }


            }
        }
        
        void enforceOutsideConstraints() {
            if (outerBoundary.empty()) return;
            elements.erase(std::remove_if(elements.begin(), elements.end(), [&](const Element& e) {
                Node centroid = computeCentroid(e);
                return !isPointInPolygon(centroid, outerBoundary);

            }), elements.end());
        }

        // Runs the Bowyer-Watson algorithm on the mesh's nodes and populates the elements vector.
        void triangulate() {
            if (nodes.empty()) {
                return;
            }
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
            int MaxIterations = 100;
            int iteration = 0;
            while (foundBadElement && iteration < MaxIterations) {
                foundBadElement = false;
                iteration++;
                for (auto& element : elements) {
                    double angle0 = minAngle(getNodeByID(element.n0_id), getNodeByID(element.n1_id), getNodeByID(element.n2_id));
                    double ratio = aspectRatio(getNodeByID(element.n0_id), getNodeByID(element.n1_id), getNodeByID(element.n2_id));
                    if (angle0 < 20 * M_PI / 180 || ratio > 10) {
                        Node centroid = computeCentroid(element);
                        insertNode(centroid);
                        buildNodeIndexMap();
                        foundBadElement = true;
                        break;
                    }

                }
            }

            enforceConstraint();
            enforceOutsideConstraints();
        }

        std::vector<Edge> findCavityEdges(const std::vector<Element>& badTriangles) {
            std::vector<Edge> polygon;
            for (const auto& triangle : badTriangles) {
                Edge edges[3] = {
                    {triangle.n0_id, triangle.n1_id, -1},
                    {triangle.n1_id, triangle.n2_id, -1},
                    {triangle.n2_id, triangle.n0_id, -1}
                };
                for (const auto& edge : edges) {
                    int count = 0;
                    for (const auto& other : badTriangles)
                        if (isSameEdge(other, edge)) count++;
                    if (count == 1) {
                        bool found = false;
                        for (const auto& poly_edge : polygon) {
                            if ((poly_edge.n0_id == edge.n0_id && poly_edge.n1_id == edge.n1_id) ||
                                (poly_edge.n0_id == edge.n1_id && poly_edge.n1_id == edge.n0_id)) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) polygon.push_back(edge);
                    }
                }
            }
            return polygon;
        }

        static std::vector<Element> findBadTriangles(
            const std::vector<Element>& triangles,
            const Node& point,
            const std::vector<Node>& nodeList,
            const std::map<int, size_t>& indexMap)
        {
            std::vector<Element> bad;
            for (const auto& t : triangles) {
                Node n0 = nodeList[indexMap.at(t.n0_id)];
                Node n1 = nodeList[indexMap.at(t.n1_id)];
                Node n2 = nodeList[indexMap.at(t.n2_id)];
                if (isInCircle(n0, n1, n2, point))
                    bad.push_back(t);
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
                Node na = nodeList[indexMap.at(edge.n0_id)];
                Node nb = nodeList[indexMap.at(edge.n1_id)];
                if (boundary) {
                    Node midpoint = {(na.x + nb.x + apexNode.x) / 3.0,
                                     (na.y + nb.y + apexNode.y) / 3.0, -1};
                    if (!isPointInPolygon(midpoint, *boundary)) continue;
                }
                double cross = orient2d(na, nb, apexNode);
                if (cross < 0)
                    outElements.push_back({edge.n1_id, edge.n0_id, apexNode.Node_id, idCounter++});
                else
                    outElements.push_back({edge.n0_id, edge.n1_id, apexNode.Node_id, idCounter++});
            }
        }

        void insertNode(const Node& newNode) {
            buildNodeIndexMap();
            Node nodeToInsert = newNode;
            nodeToInsert.Node_id = static_cast<int>(nodes.size());
            nodes.push_back(nodeToInsert);

            std::vector<Element> badElements = findBadTriangles(elements, nodeToInsert, nodes, id_to_index);
            std::vector<Edge> polygon = findCavityEdges(badElements);

            elements.erase(std::remove_if(elements.begin(), elements.end(), [&](const Element& t){
                for (const auto& bad : badElements)
                    if (t.Element_id == bad.Element_id) return true;
                return false;
            }), elements.end());

            fillCavity(polygon, nodeToInsert, elements, element_id_counter, nodes, id_to_index, &outerBoundary);
        }
        

        void MetricAngles(std::string outputFile) {
            std::vector<int> AngleBins(18, 0);
            for (const auto& element : elements) {
                double angle0 = minAngle(getNodeByID(element.n0_id), getNodeByID(element.n1_id), getNodeByID(element.n2_id));
                int bin = std::min(static_cast<int>(angle0 * 180.0 / M_PI) / 10, 17);
                AngleBins[bin]++;
            }
            std::ofstream angleFile(outputFile);
            angleFile << "Angle (degrees),Count\n";
            for (size_t i = 0; i < AngleBins.size(); ++i)
                angleFile << i * 10 << "," << AngleBins[i] << "\n";
            angleFile.close();
            std::cout << "Angle distribution written to " << outputFile << "\n";
        }

        void MetricAspectRatios(std::string outputFile) {
            std::vector<int> AspectRatioBins(10, 0);
            for (const auto& element : elements) {
                double ratio = aspectRatio(getNodeByID(element.n0_id), getNodeByID(element.n1_id), getNodeByID(element.n2_id));
                int bin = std::min(static_cast<int>(ratio / 10), 9);
                AspectRatioBins[bin]++;
            }
            std::ofstream aspectRatioFile(outputFile);
            aspectRatioFile << "Aspect Ratio,Count\n";
            for (size_t i = 0; i < AspectRatioBins.size(); ++i)
                aspectRatioFile << i * 10 << "," << AspectRatioBins[i] << "\n";
            aspectRatioFile.close();
            std::cout << "Aspect ratio distribution written to " << outputFile << "\n";
        }

        double minAngle(const Node& a, const Node& b, const Node& c) {
            double ab = std::sqrt((b.x-a.x)*(b.x-a.x) + (b.y-a.y)*(b.y-a.y));
            double bc = std::sqrt((c.x-b.x)*(c.x-b.x) + (c.y-b.y)*(c.y-b.y));
            double ca = std::sqrt((a.x-c.x)*(a.x-c.x) + (a.y-c.y)*(a.y-c.y));
            double A = std::acos(std::clamp((ab*ab + ca*ca - bc*bc) / (2*ab*ca), -1.0, 1.0));
            double B = std::acos(std::clamp((ab*ab + bc*bc - ca*ca) / (2*ab*bc), -1.0, 1.0));
            double C = std::acos(std::clamp((bc*bc + ca*ca - ab*ab) / (2*bc*ca), -1.0, 1.0));
            return std::min({A, B, C});
        }

        double aspectRatio(const Node& a, const Node& b, const Node& c) {
            double ab = distance(a, b);
            double bc = distance(b, c);
            double ca = distance(c, a);
            double longest = std::max({ab, bc, ca});
            double cross = orient2d(a, b, c);
            double area = 0.5 * std::abs(cross);
            return (0.433 * longest * longest) / area;
        }

        static double orient2d(const Node& a, const Node& b, const Node& c) {
            double val = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
            return val;
        }

        void buildNodeIndexMap() {
            id_to_index.clear();
            for (size_t i = 0; i < nodes.size(); ++i) {
                id_to_index[nodes[i].Node_id] = i;
            }
        }

        const std::map<int, size_t>& getNodeIndexMap() const {
            return id_to_index;
        }

        meshgeneration::Node getNodeByID(int id) const {
            return nodes[id_to_index.at(id)];
        }

        size_t getNodeIndex(int node_id) const {
            return id_to_index.at(node_id);
        }   

        static double distance(const Node& a, const Node& b) {
            return std::sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
        }

        static double distanceSquared(const Node& a, const Node& b) {
            return (b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y);
        }

        // Get the max Row Node
        int getMaxNodeRow() const{
            int maxRow = 0;
            for (const auto& node : nodes) {
                if (node.y > maxRow) {
                    maxRow = node.y;
                }
            }
            return maxRow;
        }

        int getMaxNodeCol() const{
            int maxCol = 0;
            for (const auto& node : nodes) {
                if (node.x > maxCol) {
                    maxCol = node.x;
                }
            }
            return maxCol;
        }

        static void refineMesh() {
            // This function can be used to refine the mesh by adding more nodes and re-triangulating, if needed.
        }

        Node computeCentroid(const Element& element) {   // drop the Mesh& parameter
            Node n1 = getNodeByID(element.n0_id);
            Node n2 = getNodeByID(element.n1_id);
            Node n3 = getNodeByID(element.n2_id);
            return { (n1.x + n2.x + n3.x) / 3.0, (n1.y + n2.y + n3.y) / 3.0, -1 };
        }

        std::tuple<Node, Node, Node>computeEdgeMidpoint(const Element& element) {   // drop the Mesh& parameter
            Node n1 = getNodeByID(element.n0_id);
            Node n2 = getNodeByID(element.n1_id);
            Node n3 = getNodeByID(element.n2_id);
            Node mid0 = { (n1.x + n2.x) / 2, (n1.y + n2.y) / 2, -1 };
            Node mid12 = { (n2.x + n3.x) / 2.0, (n2.y + n3.y) / 2.0, -1 };
            Node mid20 = { (n3.x + n1.x) / 2.0, (n3.y + n1.y) / 2.0, -1 };
            return { mid0, mid12, mid20 };
        }
        
        
        void deleteHoles() {
            if (holes.empty()) return;
            elements.erase(std::remove_if(elements.begin(), elements.end(), [&](const Element& e) {
                Node centroid = computeCentroid(e);
                for (const auto& hole : holes) {
                    if (isPointInPolygon(centroid, holes)) return true;
                }
                return false;
            }), elements.end());
        }

        bool isPointInPolygon(const Node& point, const std::vector<Node>& boundary) {
            if (boundary.empty()) {
                return false;
            }
            int numIntersections = 0;
            size_t n = boundary.size();
            for (size_t i = 0; i < n; ++i) {
                const Node& p1 = boundary[i];
                const Node& p2 = boundary[(i + 1) % n];
                if (point.y > std::min(p1.y, p2.y) && point.y <= std::max(p1.y, p2.y)) {
                    double xIntersection = p1.x + (point.y - p1.y) * (p2.x - p1.x) / (p2.y - p1.y);
                    if (xIntersection > point.x) {
                        numIntersections++;
                    }
                }
            }
            return numIntersections % 2 == 1;

        }

        void printMeshInfo() const {
            std::cout << "Mesh Info:\n";
            std::cout << "Number of Nodes: " << nodes.size() << "\n";
            std::cout << "Number of Elements: " << elements.size() << "\n";
        }


        std::vector<Node> GetBoundingBox(const std::vector<Node>& Boundry) const {
            if (Boundry.empty()) {
                return {};
            }
            double minX = Boundry[0].x, maxX = Boundry[0].x;
            double minY = Boundry[0].y, maxY = Boundry[0].y;

            for (const auto& node : Boundry) {
                if (node.x < minX) minX = node.x;
                if (node.x > maxX) maxX = node.x;
                if (node.y < minY) minY = node.y;
                if (node.y > maxY) maxY = node.y;
            }

            std::vector<Node> boundingBoxNodes = {
                {minX, minY, -1},
                {maxX, minY, -1},
                {maxX, maxY, -1},
                {minX, maxY, -1}
            };

            return boundingBoxNodes;


        }


    private:
        std::map<int, size_t> id_to_index;

        int element_id_counter = 0;
        bool isGenerated = false;
        int numRandomNodes = 0;

        int totalBoundaryNodes = 0;
        std::vector<Node> outerBoundary;

        // Returns true if Point D is inside the circumcircle of ABC
        static bool isInCircle(Node A, Node B, Node C, Node D) {
            double adx = A.x - D.x, ady = A.y - D.y;
            double bdx = B.x - D.x, bdy = B.y - D.y;
            double cdx = C.x - D.x, cdy = C.y - D.y;

            double det = (adx*adx + ady*ady) * (bdx*cdy - cdx*bdy) -
                        (bdx*bdx + bdy*bdy) * (adx*cdy - cdx*ady) +
                        (cdx*cdx + cdy*cdy) * (adx*bdy - bdx*ady);
            return det > 0;
        }

        // Checks if edge e2 is one of the three edges of triangle t.
        // This is an order-independent comparison based on node IDs.
        static bool isSameEdge(const Element& t, const Edge& e2) {
            auto edges_match = [](int id1, int id2, const Edge& e) {
                return (id1 == e.n0_id && id2 == e.n1_id) || (id1 == e.n1_id && id2 == e.n0_id);
            };
            return edges_match(t.n0_id, t.n1_id, e2) ||
                   edges_match(t.n1_id, t.n2_id, e2) ||
                   edges_match(t.n2_id, t.n0_id, e2);
        }


        // This function implements the Bowyer-Watson algorithm for Delaunay triangulation
        std::vector<Element> bowyerWatson() {
            // Create a super-triangle that encompasses all the points.
            // This is critical for the algorithm's correctness. A super-triangle that is
            // too small can lead to incorrect triangulation and the observed crash.
            std::vector<Node> superTriangleNodes;
            auto bbox = GetBoundingBox(nodes);
            double minX = bbox[0].x, maxX = bbox[1].x;
            double minY = bbox[0].y, maxY = bbox[2].y;
            double dx = maxX - minX, dy = maxY - minY;
            double M = std::max(dx, dy) * 2.0;
            double midX = (minX + maxX) / 2.0;
            double midY = (minY + maxY) / 2.0;

            superTriangleNodes.push_back({midX - M, midY - M, -1});
            superTriangleNodes.push_back({midX + M, midY - M, -2});
            superTriangleNodes.push_back({midX, midY + 2*M, -3});
            std::vector<Element> superTriangleElements = {{superTriangleNodes[0].Node_id, superTriangleNodes[1].Node_id, superTriangleNodes[2].Node_id, -1}};

            // The master list of nodes for the algorithm to use.
            // It includes the real points and the super-triangle vertices.
            std::vector<Node> all_points = nodes; 
            all_points.insert(all_points.end(), superTriangleNodes.begin(), superTriangleNodes.end());


            // Map node IDs to their index in the all_points vector for quick lookups.
            // This is robust for any integer ID, including the negative ones from the super-triangle.
            std::map<int, size_t> id_to_index;
            for (size_t i = 0; i < all_points.size(); ++i) {
                id_to_index[all_points[i].Node_id] = i;
            }

            // Initialize the triangulation with the super-triangle element.
            element_id_counter = 0;
            std::vector<Element> triangulation_elements = superTriangleElements;
            triangulation_elements[0].Element_id = element_id_counter++;

            for(const auto& point : nodes){
                std::vector<Element> badTriangles = findBadTriangles(triangulation_elements, point, all_points, id_to_index);
                std::vector<Edge> polygon = findCavityEdges(badTriangles);

                triangulation_elements.erase(std::remove_if(triangulation_elements.begin(), triangulation_elements.end(), [&](const Element& t){
                    for(const auto& bad : badTriangles)
                        if(t.Element_id == bad.Element_id) return true;
                    return false;
                }), triangulation_elements.end());

                fillCavity(polygon, point, triangulation_elements, element_id_counter, all_points, id_to_index);
            }

            // Remove all elements that share a vertex with the super-triangle
            triangulation_elements.erase(std::remove_if(triangulation_elements.begin(), triangulation_elements.end(), [&](const Element& t){
                return t.n0_id < 0 || t.n1_id < 0 || t.n2_id < 0;
            }), triangulation_elements.end());

            return triangulation_elements;
        }
    };
}
