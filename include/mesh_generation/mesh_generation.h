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
#include "mesh_generation/mesh_types.h"
#include "mesh_generation/shape_generators.h"

namespace meshgeneration {


    class Mesh {
    public:
        std::vector<Node> nodes;
        std::vector<Element> elements;
        std::vector<Edge> edges;
        std::vector<Edge> boundaryEdges;

        // Loads boundary corner nodes from a 2-column CSV (x,y) and interpolates
        // edges between each consecutive pair of corners.
        void init(std::string filename) {
            nodes.clear(); edges.clear(); holes.clear(); outerBoundary.clear();
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

            // Interpolate additional nodes along each edge between corners
            std::vector<Node> withInterp;
            size_t n = nodes.size();
            int id = 0;
            for (size_t i = 0; i < n; ++i) {
                const Node& a = nodes[i];
                const Node& b = nodes[(i + 1) % n];
                withInterp.push_back({a.x, a.y, id++});
                double len = std::sqrt((b.x-a.x)*(b.x-a.x) + (b.y-a.y)*(b.y-a.y));
                int segs = std::max(1, static_cast<int>(len / 10.0));
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

            isRectangular = false;
            buildNodeIndexMap();
        }

        void setBoundary(double dim1, double dim2, int segsPerUnit) {
            nodes.clear();
            edges.clear();
            holes.clear();
            outerBoundary.clear();
            isRectangular = true;
            outerBoundary = shapegeneration::shapes::rectangle(dim1, dim2, segsPerUnit, 0);
            nodes.insert(nodes.end(), outerBoundary.begin(), outerBoundary.end());
            totalBoundaryNodes = static_cast<int>(outerBoundary.size());
            std::vector<Edge> boundaryEdges;
            for (int i = 0; i < totalBoundaryNodes; ++i) {
                // Do something with each boundary node if needed
                int a = nodes[i].Node_id;
                int b = nodes[(i + 1) % totalBoundaryNodes].Node_id;
                edges.push_back({ a, b, -1 });
                boundaryEdges.push_back({ a, b, -1 });
            }
            buildNodeIndexMap();
        }

        void AddHole(double radius, double cx, double cy, int numSegments) {
            std::vector<Node> holeNodes = shapegeneration::shapes::circle(radius, cx, cy, numSegments, static_cast<int>(nodes.size()));
            std::cout << "Adding hole with " << holeNodes.size() << " nodes\n";
            nodes.insert(nodes.end(), holeNodes.begin(), holeNodes.end());
            totalBoundaryNodes += static_cast<int>(holeNodes.size());
            std::vector<Edge> holeEdges;
            holes.push_back(holeNodes);
            for (size_t i = 0; i < holeNodes.size(); ++i) {
                int a = holeNodes[i].Node_id;
                int b = holeNodes[(i + 1) % holeNodes.size()].Node_id;
                edges.push_back({ a, b, -1 });
                holeEdges.push_back({ a, b, -1 });
            }
            buildNodeIndexMap();
        }

        void initialize(std::string shape, double dim1, double dim2, int segsPerUnit) {
            if (shape == "circle") {
                isRectangular = false;
                AddHole(dim1, dim2, 0, segsPerUnit);  

            } else if (shape == "rectangle") {
                isRectangular = true;
                setBoundary(dim1, dim2, segsPerUnit);

            } else if (shape == "triangle") {
                isRectangular = false;
                // generateLargeTriangle(dim1, dim2, dim2, 1.0);

            } else if (shape == "ushape") {
                isRectangular = false;
                std::vector<Node> uShapeNodes = shapegeneration::shapes::uShape(dim1, dim2, segsPerUnit, 0);
                nodes.insert(nodes.end(), uShapeNodes.begin(), uShapeNodes.end());
                totalBoundaryNodes = static_cast<int>(uShapeNodes.size());
                std::vector<Edge> boundaryEdges;
                for (int i = 0; i < totalBoundaryNodes; ++i) {
                    int a = nodes[i].Node_id;
                    int b = nodes[(i + 1) % totalBoundaryNodes].Node_id;
                    edges.push_back({ a, b, -1 });
                    boundaryEdges.push_back({ a, b, -1 });
                }
                buildNodeIndexMap();

            } else if (shape == "both") {
                isboth = true;
                setBoundary(dim1, dim2, segsPerUnit);
                double radius = std::min(dim1, dim2) / 3.0;
                double cx = dim1 / 2.0;
                double cy = dim2 / 2.0;
                AddHole(radius, cx, cy, segsPerUnit);
                totalBoundaryNodes = static_cast<int>(nodes.size());
            }
            buildNodeIndexMap();
        }
        //bool isBoundaryNode(){}

        //bool isBoundaryEdge(){}

        // Generates random interior nodes and adds them to the `nodes` vector.
        void generateRandomNodes(int numNodes, double dim1, double dim2) {
            int id_counter = nodes.size();
            std::vector<Node> boundary(nodes.begin(), nodes.begin() + totalBoundaryNodes);  
            std::vector<Node> boundingBoxNodes = GetBoundingBox(boundary);
            double minX = boundingBoxNodes[0].x;
            double maxX = boundingBoxNodes[1].x;
            double minY = boundingBoxNodes[0].y;
            double maxY = boundingBoxNodes[3].y;


            if (isboth) {
                int nodes_generated = 0;
                int attempts = 0;
                int maxAttempts = numNodes * 100;
                while (nodes_generated < numNodes && attempts < maxAttempts) {
                    attempts++;
                    double x = minX + static_cast<double>(rand()) / RAND_MAX * (maxX - minX);
                    double y = minY + static_cast<double>(rand()) / RAND_MAX * (maxY - minY);
                    Node randomNode = {x, y, id_counter};

                    bool insideOuter = isPointInPolygon(randomNode, outerBoundary);
                    bool insideAnyHole = false;
                    for (const auto& hole : holes) {
                        if (isPointInPolygon(randomNode, hole)) {
                            insideAnyHole = true;
                            break;
                        }
                    }
                    if (insideOuter && !insideAnyHole) {
                        nodes.push_back(randomNode);
                        id_counter++;
                        nodes_generated++;
                    }
                }
                if (nodes_generated < numNodes)
                    std::cout << "WARNING: only placed " << nodes_generated << " of " << numNodes << " nodes\n";
            } else if (isRectangular) {
                bool isInPolygon = true;
                int nodes_generated = 0;
                int attempts = 0;
                int maxAttempts = numNodes * 100;
                while (nodes_generated < numNodes && attempts < maxAttempts) {
                    attempts++;
                    double x = minX + static_cast<double>(rand()) / RAND_MAX * (maxX - minX);
                    double y = minY + static_cast<double>(rand()) / RAND_MAX * (maxY - minY);
                    Node randomNode = {x, y, id_counter};

                    if (isPointInPolygon(randomNode, boundary)) {
                        nodes.push_back(randomNode);
                        id_counter++;
                        nodes_generated++;
                    }
                }
                if (nodes_generated < numNodes){
                    std::cout << "WARNING: only placed " << nodes_generated 
                    << " of " << numNodes << " nodes\n";
                }
            } else if (!outerBoundary.empty()) {
                int nodes_generated = 0;
                int attempts = 0;
                int maxAttempts = numNodes * 100;
                while (nodes_generated < numNodes && attempts < maxAttempts) {
                    attempts++;
                    double x = minX + static_cast<double>(rand()) / RAND_MAX * (maxX - minX);
                    double y = minY + static_cast<double>(rand()) / RAND_MAX * (maxY - minY);
                    Node randomNode = {x, y, id_counter};
                    if (isPointInPolygon(randomNode, outerBoundary)) {
                        nodes.push_back(randomNode);
                        id_counter++;
                        nodes_generated++;
                    }
                }
                
            }
            buildNodeIndexMap();
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
                std::vector<Edge> cavityEdges;
                for (const auto& elem : intersected) {
                    Edge e[3] = {
                        {elem.n0_id, elem.n1_id, -1},
                        {elem.n1_id, elem.n2_id, -1},
                        {elem.n2_id, elem.n0_id, -1}
                    };
                    for (const auto& edge : e) {
                        int count = 0;
                        for (const auto& other : intersected) {
                            if (isSameEdge(other, edge)) count++;
                        }
                        if (count == 1) cavityEdges.push_back(edge);
                    }
                }
                for (const auto& cEdge : cavityEdges) {
                    if (cEdge.n0_id == edge.n0_id || cEdge.n1_id == edge.n0_id) continue;
                    Node na = getNodeByID(edge.n0_id);
                    Node nb = getNodeByID(cEdge.n0_id);
                    Node nc = getNodeByID(cEdge.n1_id);
                    double cross = (nb.x - na.x) * (nc.y - na.y) - (nb.y - na.y) * (nc.x - na.x);
                    if (cross < 0)
                        elements.push_back({edge.n0_id, cEdge.n1_id, cEdge.n0_id, element_id_counter++});
                    else
                        elements.push_back({edge.n0_id, cEdge.n0_id, cEdge.n1_id, element_id_counter++});
                }


            }
        }

        // Runs the Bowyer-Watson algorithm on the mesh's nodes and populates the elements vector.
        void triangulate(double width, double height) {
            if (nodes.empty()) {
                return;
            }
            elements = bowyerWatson(width, height);
            enforceConstraint();
            deleteHoles();
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
                    if (isPointInPolygon(centroid, hole)) return true;
                }
                return false;
            }), elements.end());
        }

        void deleteElementsOutsideDomain(double circleCenterX, double circleCenterY, double circleRadius) {
            elements.erase(std::remove_if(elements.begin(), elements.end(), [&](const Element& e) {
                auto [mid01, mid12, mid20] = computeEdgeMidpoint(e);
                for (const auto& mid : {mid01, mid12, mid20}) {
                    double dx = mid.x - circleCenterX;
                    double dy = mid.y - circleCenterY;
                    double distance = dx * dx + dy * dy;
                    if (distance <= circleRadius * circleRadius) {
                        return true; // If any midpoint is inside the circle, we consider this element for deletion
                    }
                }
                return false; // If no midpoint is inside the circle, we keep this element
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

        bool isRectangular = false;
        bool isboth = false;
        int element_id_counter = 0;

        int totalBoundaryNodes = 0;
        std::vector<Node> outerBoundary;
        std::vector<std::vector<Node>> holes;

        static Circumcircle drawCircle(Node A, Node B, Node C) {
            double D  = 2 * (A.x * (B.y - C.y) + B.x * (C.y - A.y) + C.x * (A.y - B.y));
            double Ux = ((A.x*A.x + A.y*A.y) * (B.y - C.y) + (B.x*B.x + B.y*B.y) * (C.y - A.y) + (C.x*C.x + C.y*C.y) * (A.y - B.y)) / D;
            double Uy = ((A.x*A.x + A.y*A.y) * (C.x - B.x) + (B.x*B.x + B.y*B.y) * (A.x - C.x) + (C.x*C.x + C.y*C.y) * (B.x - A.x)) / D;

            Circumcircle result;
            result.center = {Ux, Uy, -1}; // Assign a temporary ID for the center
            result.radius = sqrt((result.center.x - A.x)*(result.center.x - A.x) +
                                (result.center.y - A.y)*(result.center.y - A.y));
            return result;
        }

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

        static std::tuple< std::vector<Node>, std::vector<Element>, std::vector<Edge> >generateLargeTriangle(double dim1, double dim2, double dim3, double sizeFactor) {
            std::vector<Node> triangleNodes;
            std::vector<Edge> triangleEdges;
            std::vector<Element> triangleElements;
            
            double val1 = -1.0 * (dim1 * sizeFactor);
            double val2 = dim2 + (dim2 * sizeFactor);
            double val3 = dim3 + (dim3 * sizeFactor);

            triangleNodes.push_back({ val1, val2, -1 }); // Super-triangle nodes get negative IDs
            triangleNodes.push_back({ val2, val3, -2 });
            triangleNodes.push_back({ val3, val1, -3 });

            // Ensure the triangle is oriented counter-clockwise for consistency in the algorithm
            double cross = (triangleNodes[1].x - triangleNodes[0].x) * (triangleNodes[2].y - triangleNodes[0].y)
             - (triangleNodes[1].y - triangleNodes[0].y) * (triangleNodes[2].x - triangleNodes[0].x);
            if (cross > 0)
                std::swap(triangleNodes[1], triangleNodes[2]);


            triangleEdges.push_back({ triangleNodes[0].Node_id, triangleNodes[1].Node_id, -1 });
            triangleEdges.push_back({ triangleNodes[1].Node_id, triangleNodes[2].Node_id, -2 });
            triangleEdges.push_back({ triangleNodes[2].Node_id, triangleNodes[0].Node_id, -3 });
            triangleElements.push_back({ triangleNodes[0].Node_id, triangleNodes[2].Node_id, triangleNodes[1].Node_id, -1 }); // Single super-triangle element

            return {triangleNodes, triangleElements, triangleEdges};
        }

        // This function implements the Bowyer-Watson algorithm for Delaunay triangulation
        std::vector<Element> bowyerWatson(double width, double height) {
            // Create a super-triangle that encompasses all the points.
            auto [superTriangleNodes, superTriangleElements, superTriangleEdges]  = generateLargeTriangle(width, height, height, 10);

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
                std::vector<Element> badTriangles;
                for(const auto& triangle : triangulation_elements){
                    Node n0 = all_points.at(id_to_index.at(triangle.n0_id));
                    Node n1 = all_points.at(id_to_index.at(triangle.n1_id));
                    Node n2 = all_points.at(id_to_index.at(triangle.n2_id));
                    if(isInCircle(n0, n1, n2, point)){
                        badTriangles.push_back(triangle);
                    }
                }

                std::vector<Edge> polygon;
                for (const auto& triangle : badTriangles){
                    // Create the three edges of the current bad triangle
                    Edge edges[3] = {
                        {triangle.n0_id, triangle.n1_id, -1},
                        {triangle.n1_id, triangle.n2_id, -1},
                        {triangle.n2_id, triangle.n0_id, -1}
                    };
                    for (const auto& edge : edges) {
                        int count = 0;
                        for (const auto& other : badTriangles){
                            if (isSameEdge(other, edge)){
                                count++;
                            }
                        }
                        if (count == 1){
                            // To avoid duplicates in the polygon, check if it's already there.
                            // This check is needed because this loop iterates over all edges of all bad triangles.
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

                triangulation_elements.erase(std::remove_if(triangulation_elements.begin(), triangulation_elements.end(), [&](const Element& t){
                    for(const auto& bad : badTriangles){
                        if(t.Element_id == bad.Element_id) return true;
                    }
                    return false;
                }), triangulation_elements.end());

                for (const auto& edge : polygon) {
                    Node na = all_points.at(id_to_index.at(edge.n0_id));
                    Node nb = all_points.at(id_to_index.at(edge.n1_id));
                    Node np = point;
                    double cross = (nb.x - na.x) * (np.y - na.y) - (nb.y - na.y) * (np.x - na.x);
                    if (cross < 0)
                        triangulation_elements.push_back({edge.n1_id, edge.n0_id, point.Node_id, element_id_counter++});
                    else
                        triangulation_elements.push_back({edge.n0_id, edge.n1_id, point.Node_id, element_id_counter++});
                }

            }

            // Remove all elements that share a vertex with the super-triangle
            triangulation_elements.erase(std::remove_if(triangulation_elements.begin(), triangulation_elements.end(), [&](const Element& t){
                return t.n0_id < 0 || t.n1_id < 0 || t.n2_id < 0;
            }), triangulation_elements.end());

            return triangulation_elements;
        }
    };
}
