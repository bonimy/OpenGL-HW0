
#include "GL/freeglut.h"
#include "Surface.h"


bool operator ==(const Edge &left, const Edge &right)
{
    return (left.v1 == right.v1 && left.v2 == right.v2) ||
        (left.v1 == right.v2 && left.v2 == right.v1);
}

Surface::Surface()
{
    vertices = vector<Vector3f>();
    normals = vector<Vector3f>();
    faces = vector<vector<unsigned>>();
    vece = vector<Edge>();
    edges = unordered_set<Edge>();
}

void Surface::drawTriangle(const vector<unsigned> &face)
{
#define DRAWPOINT(index) \
glNormal3d(normals[face[index+2]][0], normals[face[index+2]][1], normals[face[index+2]][2]);\
glVertex3d(vertices[face[index+0]][0], vertices[face[index+0]][1], vertices[face[index+0]][2])

    // Draw first point
    DRAWPOINT(0);

    // Draw second point
    DRAWPOINT(3);

    // Draw last point
    DRAWPOINT(6);

#undef DRAWPOINT
}

void Surface::drawSurface()
{
    for (int i = 0, fsize = faces.size(); i < fsize; i++)
    {
        // Draw each triangle
        drawTriangle(faces[i]);
    }
}

void Surface::addVertex(Vector3f &v)
{
    vertices.push_back(v);
}

void Surface::addNormal(Vector3f &n)
{
    normals.push_back(n);
}

void Surface::addFace(vector<unsigned> face)
{
    faces.push_back(face);

    for (int i = 0; i < (int)face.size(); i += 3)
    {
        Edge edge = Edge(face[i], face[(i + 3) % 9]);

        if (!edges.count(edge))
        {
            edges.insert(edge);
            vece.push_back(edge);
        }
    }
}