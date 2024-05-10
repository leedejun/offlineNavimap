#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
using namespace std;


namespace df
{

    namespace RTIN
    {

        // a class to represent a 2D point
        class Point {
        public:
            int x, y;
            Point(int x, int y) {
                this->x = x;
                this->y = y;
            }
        };

        // a class to represent a triangle
        class Triangle {
        public:
            Point a, b, c;
            Triangle(Point a, Point b, Point c) {
                this->a = a;
                this->b = b;
                this->c = c;
            }
        };

        // a class to represent a RTIN node
        class Node {
        public:
            int x0, y0, x1, y1; // the bounding box of the node
            int mx, my; // the midpoint of the diagonal
            int i; // the index of the vertex in the vertex array
            Node *parent; // the parent node
            Node *children[4]; // the four children nodes
            Node(int x0, int y0, int x1, int y1, int i, Node *parent) {
                this->x0 = x0;
                this->y0 = y0;
                this->x1 = x1;
                this->y1 = y1;
                this->mx = (x0 + x1) / 2;
                this->my = (y0 + y1) / 2;
                this->i = i;
                this->parent = parent;
                for (int j = 0; j < 4; j++) {
                    this->children[j] = NULL;
                }
            }
        };

        // a function to create a RTIN node from a height map
        Node* createNode(vector<vector<int>> &heights, int x0, int y0, int x1, int y1, Node *parent) {
            // find the max and min height in the bounding box
            int max = heights[y0][x0];
            int min = heights[y0][x0];
            for (int y = y0; y <= y1; y++) {
                for (int x = x0; x <= x1; x++) {
                    int h = heights[y][x];
                    if (h > max) max = h;
                    if (h < min) min = h;
                }
            }
            // calculate the error as the height difference
            int error = max - min;
            // find the middle point and its height
            int mx = (x0 + x1) / 2;
            int my = (y0 + y1) / 2;
            int mh = heights[my][mx];
            // create a new node with the given parameters
            Node *node = new Node(x0, y0, x1, y1, -1, parent);
            // if the node is not a leaf, recursively create its children
            if (x1 - x0 > 1 || y1 - y0 > 1) {
                node->children[0] = createNode(heights, x0, y0, mx, my, node); // top-left
                node->children[1] = createNode(heights, mx, y0, x1, my, node); // top-right
                node->children[2] = createNode(heights, x0, my, mx, y1, node); // bottom-left
                node->children[3] = createNode(heights, mx, my, x1, y1, node); // bottom-right
                // find the child with the minimum error
                int minError = node->children[0]->i;
                for (int i = 1; i < 4; i++) {
                    int e = node->children[i]->i;
                    if (e < minError) minError = e;
                }
                // update the node's error
                node->i = error + minError;
            } else {
                // if the node is a leaf, store the height as the error
                node->i = mh;
            }
            return node;
        }

        // a function to assign an index to each node in the RTIN tree
        void assignIndex(Node *node, vector<Point> &vertices, int &index) {
            // if the node is not a leaf, recursively assign index to its children
            if (node->children[0] != NULL) {
                for (int i = 0; i < 4; i++) {
                    assignIndex(node->children[i], vertices, index);
                }
            }
            // assign an index to the node and store its vertex in the vertex array
            node->i = index++;
            vertices.push_back(Point(node->mx, node->my));
        }

        // a function to split a node into two triangles and store them in the triangle array
        void splitNode(Node *node, vector<Triangle> &triangles, bool diagonal) {
            // get the four corner points of the node
            Point a(node->x0, node->y0);
            Point b(node->x1, node->y0);
            Point c(node->x1, node->y1);
            Point d(node->x0, node->y1);
            // get the middle point of the node
            Point m(node->mx, node->my);
            // create two triangles based on the diagonal direction
            if (diagonal) {
                triangles.push_back(Triangle(a, b, m));
                triangles.push_back(Triangle(c, d, m));
            } else {
                triangles.push_back(Triangle(b, c, m));
                triangles.push_back(Triangle(d, a, m));
            }
        }

        // a function to refine a node based on the given error threshold
        void refineNode(Node *node, vector<Triangle> &triangles, int error) {
            // if the node is a leaf, split it into two triangles
            if (node->children[0] == NULL) {
                splitNode(node, triangles, true);
            } else {
                // if the node's error is less than the threshold, split it into two triangles
                if (node->i <= error) {
                    splitNode(node, triangles, false);
                } else {
                    // otherwise, recursively refine its children
                    for (int i = 0; i < 4; i++) {
                        refineNode(node->children[i], triangles, error);
                    }
                }
            }
        }

        // a function to create a RTIN mesh from a height map
        vector<Triangle> createRTINMesh(vector<vector<int>> &heights, int error) {
            // get the size of the height map
            int n = heights.size();
            int m = heights[0].size();
            // create a RTIN tree from the height map
            Node *root = createNode(heights, 0, 0, m - 1, n - 1, NULL);
            // create a vertex array and assign an index to each node
            vector<Point> vertices;
            int index = 0;
            assignIndex(root, vertices, index);
            // create a triangle array and refine the RTIN tree based on the error threshold
            vector<Triangle> triangles;
            refineNode(root, triangles, error);
            // return the triangle array
            return triangles;
        }

    }

}