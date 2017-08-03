#include "GL/freeglut.h"
#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>
#include "vecmath.h"
using namespace std;

// Globals

// The maximum buffer size for a single cin line.
// This should be more than enough.
#define MAX_BUFFER_SIZE 10000

// This is the list of points (3D vectors)
vector<Vector3f> vecv;

// This is the list of normals (also 3D vectors)
vector<Vector3f> vecn;

// This is the list of faces (indices into vecv and vecn)
vector<vector<unsigned> > vecf;

// You will need more global variables to implement color and position changes

// These are convenience functions which allow us to call OpenGL
// methods on Vec3d objects
inline void glVertex(const Vector3f &a)
{
    glVertex3fv(a);
}

inline void glNormal(const Vector3f &a)
{
    glNormal3fv(a);
}

// This function is called whenever a "Normal" key press is received.
void keyboardFunc(unsigned char key, int x, int y)
{
    switch (key)
    {
    case 27: // Escape key
        exit(0);
        break;
    case 'c':
        // add code to change color here
        cout << "Unhandled key press " << key << "." << endl;
        break;
    default:
        cout << "Unhandled key press " << key << "." << endl;
    }

    // this will refresh the screen so that the user sees the color change
    glutPostRedisplay();
}

// This function is called whenever a "Special" key press is received.
// Right now, it's handling the arrow keys.
void specialFunc(int key, int x, int y)
{
    switch (key)
    {
    case GLUT_KEY_UP:
        // add code to change light position
        cout << "Unhandled key press: up arrow." << endl;
        break;
    case GLUT_KEY_DOWN:
        // add code to change light position
        cout << "Unhandled key press: down arrow." << endl;
        break;
    case GLUT_KEY_LEFT:
        // add code to change light position
        cout << "Unhandled key press: left arrow." << endl;
        break;
    case GLUT_KEY_RIGHT:
        // add code to change light position
        cout << "Unhandled key press: right arrow." << endl;
        break;
    }

    // this will refresh the screen so that the user sees the light position
    glutPostRedisplay();
}

void drawTriangle(const vector<unsigned> &face)
{
#define DRAWPOINT(index) \
glNormal3d(vecn[face[index+2]][0], vecn[face[index+2]][1], vecn[face[index+2]][2]);\
glVertex3d(vecv[face[index+0]][0], vecv[face[index+0]][1], vecv[face[index+0]][2])

    // Draw first point
    DRAWPOINT(0);

    // Draw second point
    DRAWPOINT(3);

    // Draw last point
    DRAWPOINT(6);

#undef DRAWPOINT
}

void renderMesh()
{
    int fsize = vecf.size();
    if (fsize)
    {
        glBegin(GL_TRIANGLES);
        for (int i = 0; i < fsize; i++)
        {
            // Draw each triangle
            drawTriangle(vecf[i]);
        }
        glEnd();
    }
    else // Draw the teapot if no mesh is loaded.
        glutSolidTeapot(1.0);
}

// This function is responsible for displaying the object.
void drawScene(void)
{
    // Clear the rendering window
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Rotate the image
    glMatrixMode(GL_MODELVIEW);  // Current matrix affects objects positions
    glLoadIdentity();              // Initialize to the identity

    // Position the camera at [0,0,5], looking at [0,0,0],
    // with [0,1,0] as the up direction.
    gluLookAt(0.0, 0.0, 5.0,
        0.0, 0.0, 0.0,
        0.0, 1.0, 0.0);

    // Set material properties of object

    // Here are some colors you might use - feel free to add more
    GLfloat diffColors[4][4] = { {0.5f, 0.5f, 0.9f, 1.0f},
                                 {0.9f, 0.5f, 0.5f, 1.0f},
                                 {0.5f, 0.9f, 0.3f, 1.0f},
                                 {0.3f, 0.8f, 0.9f, 1.0f} };

    // Here we use the first color entry as the diffuse color
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, diffColors[0]);

    // Define specular color and shininess
    GLfloat specColor[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat shininess[] = { 100.0 };

    // Note that the specular color and shininess can stay constant
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specColor);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, shininess);

    // Set light properties

    // Light color (RGBA)
    GLfloat Lt0diff[] = { 1.0,1.0,1.0,1.0 };
    // Light position
    GLfloat Lt0pos[] = { 1.0f, 1.0f, 5.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_DIFFUSE, Lt0diff);
    glLightfv(GL_LIGHT0, GL_POSITION, Lt0pos);

    renderMesh();

    // Dump the image to the screen.
    glutSwapBuffers();
}

// Initialize OpenGL's rendering modes
void initRendering()
{
    glEnable(GL_DEPTH_TEST);   // Depth testing must be turned on
    glEnable(GL_LIGHTING);     // Enable lighting calculations
    glEnable(GL_LIGHT0);       // Turn on light #0.
}

// Called when the window is resized
// w, h - width and height of the window in pixels.
void reshapeFunc(int w, int h)
{
    // Always use the largest square viewport possible
    if (w > h) {
        glViewport((w - h) / 2, 0, h, h);
    }
    else {
        glViewport(0, (h - w) / 2, w, w);
    }

    // Set up a perspective view, with square aspect ratio
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // 50 degree fov, uniform aspect ratio, near = 1, far = 100
    gluPerspective(50.0, 1.0, 1.0, 100.0);
}

void loadInput()
{
    // load the OBJ file here

    // Current line
    char buffer[MAX_BUFFER_SIZE];

    // Current parsed vector
    Vector3f v;

    // Current token ("v", "vn", or "f").
    string s;

    const int fSize = 9;

    // Current face array
    unsigned f[fSize];

    do
    {
        // Get current line
        cin.getline(buffer, MAX_BUFFER_SIZE);

        // Replace forward slashes with spaces (to better parse faces)
        for (int i = 0; buffer[i]; i++)
        {
            if (buffer[i] == '/')
                buffer[i] = ' ';
        }

        // Initialize new string stream.
        stringstream ss(buffer);

        // Get token
        ss >> s;

        // This is a vertex token.
        if (s == "v")
        {
            // Get vector from line
            ss >> v[0] >> v[1] >> v[2];

            // Store to vertex array
            vecv.push_back(v);
            continue;
        }

        // This is a vector-normal token.
        if (s == "vn")
        {
            // Get vector from line
            ss >> v[0] >> v[1] >> v[2];

            // Store to vertex array
            vecn.push_back(v);
            continue;
        }

        // This is a face token.
        if (s == "f")
        {
            // Initialize face vector
            vector<unsigned> face;

            // Iterate for the number of elements we expect (9)
            for (int i = 0; i < fSize; i++)
            {
                // Get face index
                ss >> f[i];

                // Store to current face vector.
                // We subtract one to account for the zero index of C++.
                face.push_back(f[i] - 1);
            }

            // push complete face vector
            vecf.push_back(face);
            continue;
        }
    } while (!cin.eof());
}

// Main routine.
// Set up OpenGL, define the callbacks and start the main loop
int main(int argc, char** argv)
{
    loadInput();

    glutInit(&argc, argv);

    // We're going to animate it, so double buffer
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

    // Initial parameters for window position and size
    glutInitWindowPosition(60, 60);
    glutInitWindowSize(360, 360);
    glutCreateWindow("Assignment 0");

    // Initialize OpenGL parameters.
    initRendering();

    // Set up callback functions for key presses
    glutKeyboardFunc(keyboardFunc); // Handles "normal" ascii symbols
    glutSpecialFunc(specialFunc);   // Handles "special" keyboard keys

     // Set up the callback function for resizing windows
    glutReshapeFunc(reshapeFunc);

    // Call this whenever window needs redrawing
    glutDisplayFunc(drawScene);

    // Start the main loop.  glutMainLoop never returns.
    glutMainLoop();

    return 0;	// This line is never reached.
}