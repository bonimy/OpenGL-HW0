#include "GL/freeglut.h"
#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>
#include "vecmath.h"
using namespace std;

// Defines

// The maximum buffer size for a single cin line.
// This should be more than enough.
#define MAX_BUFFER_SIZE 10000

// The rotation delta angle to rotate the mesh object every frame.
#define MESH_ROTATE_DEGREES 1

// Shift amount for hue color change.
#define HUE_SHIFT_DEGREES   1

// Shift amount for light position change.
#define LIGHT_SHIFT_DEGREES   15

// Mathematical constant
#define PI 3.14159265358972

// Convert radians to degrees
#define rad2deg(rad) ((rad) * 180.0 / PI)

// Convert degrees to radians
#define deg2rad(deg) ((deg) * PI / 180.0)

// Globals

// GL list for rendering our static object
GLuint mesh;

// This is the list of points (3D vectors)
vector<Vector3f> vecv;

// This is the list of normals (also 3D vectors)
vector<Vector3f> vecn;

// This is the list of faces (indices into vecv and vecn)
vector<vector<unsigned> > vecf;

// Light position
Vector4f Lt0pos(1, 1, 5, 1);

// Millisecond wait times for update functions. Gives us 60FPS.
int timerInterval[] = { 17, 16, 16 };

// Determines whether the object is being rotated by an 'r' key press.
bool meshSpinAnimate;

// Determines whether to cycle through hue colors.
bool diffuseColorAnimate;

// The current Y-rotated angle of the rendered object.
GLfloat spinAngleY = 0;

// Rotates the light transformation matrix
void rotateLightMatrix(const Vector3f &direction, float radians)
{
    Lt0pos = Matrix4f::rotation(direction, radians) * Lt0pos;
}

// Represents a color by its hue, chroma, and luma
struct HCY
{
    GLfloat alpha;  // 0 to 1 inclusive
    GLfloat hue;    // 0 to 1 inclusive
    GLfloat chroma; // 0 to 1 inclusive
    GLfloat luma;   // 0 to 1 inclusive

    HCY(GLfloat a, GLfloat h, GLfloat c, GLfloat y)
    {
#define CLAMP(x) x < 0 ? 0 : (x > 1 ? 1 : x)
        alpha  = CLAMP(a);
        hue    = CLAMP(h);
        chroma = CLAMP(c);
        luma   = CLAMP(y);
#undef CLAMP
    }

    // Rotate color's hue by given degrees
    void rotateHue(const GLfloat degrees)
    {
        float delta = degrees / 360.0f;

        // Add change to current hue.
        hue += delta;

        // Modulus clamp the value to be between 0 and 1.
        while (hue > 1)
            hue -= 1;
        while (hue < 0)
            hue += 1;
    }

} diffuseHsl(1, 0, 1, 0.5); // The current display color in HCY form.

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

// Toggles smooth color change animation on or off.
void toggleDiffuseColorAnimate()
{
    if (diffuseColorAnimate ^= true)
    {
        cout << "Auto shifting hue: Enabled" << endl;
    }
    else
    {
        cout << "Auto shifting hue: Disabled" << endl;
    }
}

// Toggles spinning of mesh object on or off.
void toggleMeshSpinAnimate()
{
    if (meshSpinAnimate ^= true)
    {
        cout << "Auto rotating: Enabled" << endl;
    }
    else
    {
        cout << "Auto rotating: Disabled" << endl;
    }
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
        toggleDiffuseColorAnimate();
        break;

    case 'r':
        toggleMeshSpinAnimate();
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
    // Calculate diffuse light position's change in radians.
    float lightShiftRads = (float)deg2rad(LIGHT_SHIFT_DEGREES);

    switch (key)
    {
    case GLUT_KEY_UP:
        rotateLightMatrix(Vector3f::RIGHT, -lightShiftRads);
        break;
    case GLUT_KEY_DOWN:
        rotateLightMatrix(Vector3f::RIGHT, +lightShiftRads);
        break;
    case GLUT_KEY_LEFT:
        rotateLightMatrix(Vector3f::UP, -lightShiftRads);
        break;
    case GLUT_KEY_RIGHT:
        rotateLightMatrix(Vector3f::UP, +lightShiftRads);
        break;
    }

    // this will refresh the screen so that the user sees the light position
    glutPostRedisplay();
}

// Draws an OpenGL triangle designated by a face vector, which specifies which
// vertices and normals to index to.
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

// Renders the loaded mesh, or the GL solid teapot if no file was specified.
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

    // Initialize the model-view matrix
    glMatrixMode(GL_MODELVIEW);  // Current matrix affects objects positions
    glLoadIdentity();

    // Position the camera at [0,0,5], looking at [0,0,0],
    // with [0,1,0] as the up direction.
    gluLookAt(0.0, 0.0, 5.0,
        0.0, 0.0, 0.0,
        0.0, 1.0, 0.0);

    // Rotate model-view matrix for rendering model, then restore matrix
    glPushMatrix();
    glRotatef(spinAngleY, 0, 1, 0);
    renderMesh();
    glPopMatrix();

    // Set material properties of object

    // Get current color in OpenGL-readable RGB format.
    RGB diffuseRgb;
    hcy2rgb(diffuseHsl, diffuseRgb);

    // Assign the current RGB color as the diffuse color.
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

    glLightfv(GL_LIGHT0, GL_DIFFUSE, Lt0diff);
    glLightfv(GL_LIGHT0, GL_POSITION, Lt0pos);

    // Dump the image to the screen.
    glutSwapBuffers();
}

// Static object mesh for object to render.
void initRenderMesh()
{
    mesh = glGenLists(1);
    glNewList(mesh, GL_COMPILE);

    // The outcome of this depends on which .obj is loaded, but it won't change
    // after program start, so we can determine it as static.
    renderMesh();
    glEndList();
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
    // Always use the largest square view-port possible
    if (w > h) {
        glViewport((w - h) / 2, 0, h, h);
    }
    else {
        glViewport(0, (h - w) / 2, w, w);
    }

    // Set up a perspective view, with square aspect ratio
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // 50 degree FOV, uniform aspect ratio, near = 1, far = 100
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

void update(int code)
{
    // Assume animation is disabled until a flag enables it.
    bool redraw = false;

    // Update mesh rotation angle
    if (meshSpinAnimate)
    {
        spinAngleY += MESH_ROTATE_DEGREES;
        redraw = true;
    }

    // Update diffuse color
    if (diffuseColorAnimate)
    {
        diffuseHsl.rotateHue(HUE_SHIFT_DEGREES);
        redraw = true;
    }

    // Redraw if an animation flag was set.
    if (redraw)
        glutPostRedisplay();

    // Update timer to next interval.
    glutTimerFunc(timerInterval[code], update, (code + 1) % (sizeof(timerInterval) / sizeof(int)));
}

// Main routine.
// Set up OpenGL, define the callbacks and start the main loop
int main(int argc, char** argv)
{
    // Load an .OBJ file if one was specified.
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

    // Initialize static object mesh.
    initRenderMesh();

    // Set up callback functions for key presses
    glutKeyboardFunc(keyboardFunc); // Handles "normal" ASCII symbols
    glutSpecialFunc(specialFunc);   // Handles "special" keyboard keys

     // Set up the callback function for resizing windows
    glutReshapeFunc(reshapeFunc);

    // Call this whenever window needs redrawing
    glutDisplayFunc(drawScene);

    // Initialize timer update function with 17ms delay.
    glutTimerFunc(17, update, 0);

    // Start the main loop.  glutMainLoop never returns.
    glutMainLoop();

    return 0;	// This line is never reached.
}