#include "GL/freeglut.h"
#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>
#include "vecmath.h"
using namespace std;

// Globals
#define HUE_SHIFT_DEGREES   15

// This is the list of points (3D vectors)
vector<Vector3f> vecv;

// This is the list of normals (also 3D vectors)
vector<Vector3f> vecn;

// This is the list of faces (indices into vecv and vecn)
vector<vector<unsigned> > vecf;

// You will need more global variables to implement color and position changes

// Represents a color by its hue, chroma, and luma
struct HCY
{
    GLfloat alpha;  // 0 to 1 inclusive
    GLfloat hue;    // 0 to 1 inclusive
    GLfloat chroma; // 0 to 1 inclusive
    GLfloat luma;   // 0 to 1 inclusive
} diffuseHsl; // The current display color in HCY form.
int colorIndex;

           // Represents an OpenGL-styled color of floats
union RGB
{
    // 0 to 1 inclusive
    GLfloat values[4];
    struct
    {
        // Channels must be in this exact order to match OpenGL's system.
        GLfloat red, green, blue, alpha;
    };
};

// Get an RGB color from an HCY color
void hcy2rgb(const struct HCY &hcy, union RGB &rgb)
{
    GLfloat chroma = hcy.chroma;
    GLfloat hue = hcy.hue * 6;
    GLfloat r = 0.f;
    GLfloat g = 0.f;
    GLfloat b = 0.f;

    if (chroma > 0)
    {
        if (hue >= 0 && hue < 1)
        {
            r = chroma;
            g = chroma * hue;
        }
        else if (hue >= 1 && hue < 2)
        {
            r = chroma * (2 - hue);
            g = chroma;
        }
        else if (hue >= 2 && hue < 3)
        {
            g = chroma;
            b = chroma * (hue - 2);
        }
        else if (hue >= 3 && hue < 4)
        {
            g = chroma * (4 - hue);
            b = chroma;
        }
        else if (hue >= 4 && hue < 5)
        {
            r = chroma * (hue - 4);
            b = chroma;
        }
        else //if (hue >= 5 && hue < 6)
        {
            r = chroma;
            b = chroma * (6 - hue);
        }
    }

    // Color channel contributions (sums to unity).
#define REDWEIGHT   0.299f
#define GREENWEIGHT 0.587f
#define BLUEWEIGHT  0.114f

    GLfloat match = hcy.luma - (REDWEIGHT * r + GREENWEIGHT * g + BLUEWEIGHT * b);

#undef REDWEIGHT
#undef GREENWEIGHT
#undef BLUEWEIGHT

    rgb.alpha = hcy.alpha;
    rgb.red = r + match;
    rgb.green = g + match;
    rgb.blue = b + match;
}

// Shift the current hue value by the given amount.
void shifthue(GLfloat delta)
{
    // Add change to current hue.
    diffuseHsl.hue += delta;

    // Modulus clamp the value to be between 0 and 1.
    while (diffuseHsl.hue > 1)
        diffuseHsl.hue -= 1;
    while (diffuseHsl.hue < 0)
        diffuseHsl.hue += 1;
}

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
        shifthue(HUE_SHIFT_DEGREES / 360.0f);
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

    // Get current color in OpenGL-readable RGB format.
    RGB diffuseRgb;
    hcy2rgb(diffuseHsl, diffuseRgb);

    // Assign the current rgb color as the diffuse color.
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, diffuseRgb.values);

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

    // This GLUT method draws a teapot.  You should replace
    // it with code which draws the object you loaded.
    glutSolidTeapot(1.0);

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
}

// Main routine.
// Set up OpenGL, define the callbacks and start the main loop
int main(int argc, char** argv)
{
    // Initialize current HSL color.
    diffuseHsl.alpha = 1;
    diffuseHsl.hue = 0;
    diffuseHsl.chroma = 1;
    diffuseHsl.luma = 0.5f;

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