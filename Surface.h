#pragma once

#include "GL/freeglut.h"
#include <vector>
#include <unordered_set>
#include "vecmath.h"
using namespace std;


union Edge
{
    struct
    {
        int v1;
        int v2;
    };

    int v[2];

    Edge(int v1, int v2)
    {
        this->v1 = v1;
        this->v2 = v2;
    }
};

bool operator ==(const Edge &left, const Edge &right);

    namespace std
    {
        template<>
        struct hash<Edge>
        {
            using argument_type = Edge;
            using result_type = int;
            int operator()(const Edge &_Keyval) const _NOEXCEPT
            {
                return _Keyval.v1 ^ _Keyval.v2;
            }
        };
    }

class Surface
{
    // This is the list of points (3D vectors).
    vector<Vector3f> vertices;

    // This is the list of normals (also 3D vectors)
    vector<Vector3f> normals;

    // This is the list of faces (indices into vecv and vecn).
    vector<vector<unsigned>> faces;

    // This is the list of edges (indices into vecv).
    vector<Edge> vece;

    unordered_set<Edge> edges;

    // Draws a triangle designated by a face vector, which specifies which
    // vertices and normals to index to.
    void drawTriangle(const vector<unsigned> &face);

public:
    Surface();

    void drawSurface();

    void addVertex(Vector3f &v);

    void addNormal(Vector3f &n);

    void addFace(vector<unsigned> face);
};