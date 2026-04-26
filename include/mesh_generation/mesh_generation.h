#pragma once
#include <string>
#include <iostream>
#include <cmath> 
#include <algorithm>    
#include <tuple>
#include <vector>
#include <cmath>
#include <map>

namespace meshgeneration {
    struct Node {
        double x, y;
        int Node_id;
    };

    struct Edge {
        int n0_id;
        int n1_id;
        int Edge_id;
    };

    struct Element {
        int n0_id, n1_id, n2_id;
        int Element_id;
    };

    struct Circumcircle {
        Node   center;
        double radius;
    };


    class Mesh {
    public:
        std::vector<Node> nodes;
        std::vector<Element> elements;

        void initialize(std::string shape, double dim1, double dim2, int segsPerUnit) {
            if (shape == "circle") {
                isRectangular = false;
                double radius = dim1;
                int numSegments = static_cast<int>(dim2);
                double angleStep = 2.0 * M_PI / numSegments;
                for (int i = 0; i < numSegments; ++i) {
                    double angle = i * angleStep;                    
                    nodes.push_back({ radius * cos(angle), radius * sin(angle), i });

                }

            } else if (shape == "rectangle") {
                isRectangular = true;
                double width = dim1;
                double height = dim2;

                // 8 segments per unit length gives spacing of 0.125 — adjust for coarser/finer mesh
                // So in this example 
                const int nx = static_cast<int>(width  * segsPerUnit);
                const int ny = static_cast<int>(height * segsPerUnit);
                int id_counter = 0;
                // Bottom edge (left → right), corners included
                for (int i = 0; i <= nx; ++i)
                    nodes.push_back({ i * width / nx, 0.0, id_counter++ });
                // Right edge (bottom → top), skip bottom-right corner
                for (int j = 1; j <= ny; ++j)
                    nodes.push_back({ width, j * height / ny, id_counter++ });
                // Top edge (right → left), skip top-right corner
                for (int i = nx - 1; i >= 0; --i)
                    nodes.push_back({ i * width / nx, height, id_counter++ });
                // Left edge (top → bottom), skip both corner nodes
                for (int j = ny - 1; j >= 1; --j)
                    nodes.push_back({ 0.0, j * height / ny, id_counter++ });
                // const int totalNodes = static_cast<int>(nodes.size()); // This local variable is unused.
            }
            if (shape == "triangle") {
                isRectangular = false;
                // generateLargeTriangle(dim1, dim2, dim2, 1.0);
            }

            if (shape == "both") {
                isboth = true;
                initialize("rectangle", dim1, dim2, segsPerUnit);
                // std::min(dim1, dim2) / 2.0 gives radius for inscribed circle
                // but will this be touching the rectangle edge? We should esnure its always smaller than that to avoid degenerate elements in the mesh
                isRectangular = false;
                double radius = std::min(dim1, dim2) / 3.0;

                size_t starting_id = nodes.size();
                int numSegments = 12;
                double angleStep = 2.0 * M_PI / numSegments;
                for (int i = 0; i < numSegments; ++i) {
                    double angle = i * angleStep;
                    nodes.push_back({ radius * cos(angle) + dim1/2, radius * sin(angle) + dim2/2, static_cast<int>(starting_id + i) });
                }

            }
            buildNodeIndexMap();
        }

        // Generates random interior nodes and adds them to the `nodes` vector.
        void generateRandomNodes(int numNodes, double dim1, double dim2) {
            int id_counter = nodes.size();

            if (isboth) {
                double radius = std::min(dim1, dim2) / 3.0; // Match the radius from initialize()
                double radiusSq = radius * radius;
                int nodes_generated = 0;
                while (nodes_generated < numNodes) {
                    double x = static_cast<double>(rand()) / RAND_MAX * dim1;
                    double y = static_cast<double>(rand()) / RAND_MAX * dim2;

                    // Check if it's outside the inner circle
                    double dx = x - dim1/2;
                    double dy = y - dim2/2;
                    if (dx*dx + dy*dy >= radiusSq) {
                        nodes.push_back({x, y, id_counter++});
                        nodes_generated++;
                    }
                }
            } else if (isRectangular) {
                for (int i = 0; i < numNodes; ++i) {
                    double x = static_cast<double>(rand()) / RAND_MAX * dim1;
                    double y = static_cast<double>(rand()) / RAND_MAX * dim2;
                    nodes.push_back({ x, y, id_counter++ });
                }
            }
            buildNodeIndexMap();
        }

        // Runs the Bowyer-Watson algorithm on the mesh's nodes and populates the elements vector.
        void triangulate(double width, double height) {
            if (nodes.empty()) {
                return;
            }
            elements = bowyerWatson(width, height);
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



    private:
        std::map<int, size_t> id_to_index;

        bool isRectangular = false;
        bool isboth = false;


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
            int element_id_counter = 0;
            std::vector<Element> triangulation_elements = superTriangleElements;
            triangulation_elements[0].Element_id = element_id_counter++;

            for(const auto& point : all_points){
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

                for (const auto& edge : polygon){
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
