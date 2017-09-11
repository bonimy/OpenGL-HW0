#include "GL/freeglut.h"
#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>
#include "vecmath.h"
using namespace std;

// Define important constants.

// Define perspective constraints.
#define FIELD_OF_VIEW 50
#define NEAR_PERSPECTIVE 1
#define FAR_PERSPECTIVE 100

// The maximum buffer size for a single cin line.
#define MAX_BUFFER_SIZE 10000

// The rotation delta angle to rotate the mesh object every frame.
#define MESH_ROTATE_DEGREES 1

// Shift amount for hue color change.
#define HUE_SHIFT_DEGREES   1

// Shift amount for light position change.
#define LIGHT_SHIFT_DEGREES   15

// The mouse button to assign to world rotating.
#define MOUSE_ROTATE_BUTTON GLUT_LEFT_BUTTON

// The mouse rotation sensitivity when drag rotating an object.
#define MOUSE_ROTATE_FACTOR 4

// The mouse scaling factor when zooming in or out of an object.
#define MOUSE_SCALE_FACTOR 1.1

// Mathematical constant.
#define PI 3.14159265358972

// Convert radians to degrees.
#define rad2deg(rad) ((rad) * 180.0 / PI)

// Convert degrees to radians.
#define deg2rad(deg) ((deg) * PI / 180.0)

// Define global variables.

// GL list for rendering our static object.
GLuint mesh;

// GL list for rendering our grid;
GLuint grid;

// Defines the physical position of the camera. The object is always at the origin.
Vector3f position(0, 0, 5);

// The rotation tensor for the static object (used for mouse rotating).
Matrix4f space(Matrix4f::identity());

// This is the list of points (3D vectors).
vector<Vector3f> vecv;

// This is the list of normals (also 3D vectors)
vector<Vector3f> vecn;

// This is the list of faces (indices into vecv and vecn).
vector<vector<unsigned> > vecf;

// Light position for world.
Vector4f Lt0pos(1, 1, 5, 1);

// The current mouse state.
bool mouseRotationEnabled;

// The current mouse position.
Vector3f mousePos(0, 0, 0);

// The last mouse position.
Vector3f lastMousePos(0, 0, 0);

// The mouse delta change is used to get the X and Y rotation angles.
Vector3f mouseDelta(0, 0, 0);

// Millisecond wait times indexed by frame. Sums to 60 frames per second.
int timerInterval[] = { 17, 16, 16 };

// Determines whether the object should have an animated rotation.
bool meshSpinAnimate;

// Determines whether the object should have an animated color cycle.
bool diffuseColorAnimate;

// The current Y-rotated angle of the rendered object.
GLfloat spinAngleY = 0;

// Rotates the light transformation matrix.
void rotateLightMatrix(const Vector3f &direction, float radians)
{
    Lt0pos = Matrix4f::rotation(direction, radians) * Lt0pos;
}

// Represents a color by its hue, chroma, and luma.
struct HCY
{
    GLfloat alpha;
    GLfloat hue;
    GLfloat chroma;
    GLfloat luma;

    HCY(const GLfloat a, const GLfloat h, const GLfloat c, const GLfloat y)
    {
#define CLAMP(x) x < 0 ? 0 : (x > 1 ? 1 : x)

        alpha = CLAMP(a);
        hue = CLAMP(h);
        chroma = CLAMP(c);
        luma = CLAMP(y);

#undef CLAMP
    }

    // Rotate color's hue by given degrees.
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
} meshHcyColor(1, 0, 1, 0.5); // The current display color in HCY form.

// Represents an OpenGL-styled color of floats
union RGB
{
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

    // Bound hue to [0, 6) since we have different cases depending on which integers the hue will be between.
    GLfloat hue = hcy.hue * 6;

    // Initialize color to empty.
    GLfloat r = 0.f;
    GLfloat g = 0.f;
    GLfloat b = 0.f;

    // Alpha channel is a simple copy.
    rgb.alpha = hcy.alpha;

    // Only care about color if there is a present chroma.
    if (chroma > 0)
    {
        // If hue's bounds are [0, 1), we have a predominately red-to-yellow color.
        if (hue >= 0 && hue < 1)
        {
            r = chroma;
            g = chroma * hue;
        }

        // [1, 2) is a yellow-to-green.
        else if (hue >= 1 && hue < 2)
        {
            r = chroma * (2 - hue);
            g = chroma;
        }

        // Green-to-cyan.
        else if (hue >= 2 && hue < 3)
        {
            g = chroma;
            b = chroma * (hue - 2);
        }

        // Cyan-to-blue.
        else if (hue >= 3 && hue < 4)
        {
            g = chroma * (4 - hue);
            b = chroma;
        }

        // Blue-to-magenta.
        else if (hue >= 4 && hue < 5)
        {
            r = chroma * (hue - 4);
            b = chroma;
        }

        // This lost possible result is [5, 6) and is a magenta-to-red color.
        else //if (hue >= 5 && hue < 6)
        {
            r = chroma;
            b = chroma * (6 - hue);
        }
    }

    // Each color has a different defined weight in the luma color space.
#define REDWEIGHT   0.299f
#define GREENWEIGHT 0.587f
#define BLUEWEIGHT  0.114f

    // The match value defines the "colorful intensity" of the color.
    GLfloat match = hcy.luma - (REDWEIGHT * r + GREENWEIGHT * g + BLUEWEIGHT * b);

#undef REDWEIGHT
#undef GREENWEIGHT
#undef BLUEWEIGHT

    // The final color result.
    rgb.red = r + match;
    rgb.green = g + match;
    rgb.blue = b + match;
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

// Updates the current, last, and delta mouse positions.
void setMousePos(const GLdouble x, const GLdouble y, const GLdouble z)
{
    lastMousePos = mousePos;
    mousePos = Vector3f((float)x, (float)y, (float)z);
    mouseDelta = mousePos - lastMousePos;
}

// Unprojects screen coordinates and sets the result as the current mouse position.
void unprojectMouse(int x, int y)
{
    // Get the model view matrix.
    GLdouble model[16];
    glGetDoublev(GL_MODELVIEW_MATRIX, model);

    // Get the projection matrix.
    GLdouble projection[16];
    glGetDoublev(GL_PROJECTION_MATRIX, projection);

    // Get the viewport matrix.
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    // Get the projected window z-coordinate.
    GLdouble win[3];
    gluProject(
        0.0, 0.0, 0.0,
        model, projection, viewport,
        &win[0], &win[2], &win[2]
    );

    // Get the object's unprojected coordinates.
    GLdouble obj[3];
    gluUnProject(
        (double)x, (double)y, win[2],
        model, projection, viewport,
        &obj[0], &obj[1], &obj[2]
    );

    // Initialize mouse rotator vector with no z-component.
    setMousePos(obj[0], obj[1], 0.0);
}

// This function is called whenever a mouse button is pressed or released in the current window.
void mouseFunc(int button, int state, int x, int y)
{
    switch (state)
    {
        // Handle mouse presses.
    case GLUT_DOWN:
        switch (button)
        {
            // Enable and initialize mouse scrolling when the user presses the mouse scrolling button.
        case MOUSE_ROTATE_BUTTON:
            mouseRotationEnabled = true;
            unprojectMouse(x, y);
            break;

            // Don't break scrolling if the user simultaneously presses an unhandled mouse button.
        default:
            break;
        }
        break;

        // Handle mouse releases.
    case GLUT_UP:
        switch (button)
        {
            // Disable mouse rotation when the user releases the mouse scrolling button.
        case MOUSE_ROTATE_BUTTON:
            mouseRotationEnabled = false;
            break;

            // Don't remove scrolling if the user releases an unhandled mouse button.
        default:
            break;
        }
        break;
    }
}

// This function is called whenever a mouse button is held down and the mouse is moving.
void motionFunc(int x, int y)
{
    // Only handle if mouse rotation is enabled.
    if (mouseRotationEnabled)
    {
        // Update the current mouse position.
        unprojectMouse(x, y);

        // Rotate the view space by the mouse position change.
        space = Matrix4f::rotateY(mouseDelta[0] * (float)MOUSE_ROTATE_FACTOR / position.abs()) * space;
        space = Matrix4f::rotateX(mouseDelta[1] * (float)MOUSE_ROTATE_FACTOR / position.abs()) * space;

        // Redraw object to see our changes.
        glutPostRedisplay();
    }
}

void mouseWheel(int button, int dir, int x, int y)
{
    // Get the current camera-to-object length.
    float length = position.abs();

    // Zooming in
    if (dir > 0)
    {
        // Do not rescale if it puts us outside of our near perspective bounds.
        if (length / MOUSE_SCALE_FACTOR < NEAR_PERSPECTIVE)
            return;

        // Zooming in reduces the distance to the object.
        position *= 1 / (float)MOUSE_SCALE_FACTOR;
    }

    // Zooming out
    else if (dir < 0)
    {
        // Do not rescale if it puts us outside of our far perspective bounds.
        if (length * MOUSE_SCALE_FACTOR > FAR_PERSPECTIVE)
            return;

        position *= (float)MOUSE_SCALE_FACTOR;
    }

    // Redraw object to see our changes.
    glutPostRedisplay();
}

// Draws a triangle designated by a face vector, which specifies which
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
    // Determine the number of faces.
    int fsize = vecf.size();

    // If we have a nonzero number of faces, draw the selected object.
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

    // Draw the teapot if no mesh is loaded.
    else
        glutSolidTeapot(1.0);
}

// Renders a 20 x 20 square grid.
void renderGrid()
{
    glBegin(GL_LINES);

#define GRID_SIZE 10

    // Remove possibility of floating errors by doing an integer for-loop.
    for (int i = -GRID_SIZE; i <= GRID_SIZE; i++)
    {
        // Get the relative position of the current line.
        float step = (float)i / GRID_SIZE;

        // Draw both sets of lines at once.
        glVertex3f(step, 0, -1.0);
        glVertex3f(step, 0, +1.0);

        glVertex3f(-1.0, 0, step);
        glVertex3f(+1.0, 0, step);
    }

#undef GRID_SIZE

    glEnd();
}


// Draws a gray XY grid to help orient the user.
void drawGrid()
{
    // Save the current lighting attributes.
    glPushAttrib(GL_LIGHTING_BIT);

    // Disable lighting effects for the grid.
    glDisable(GL_LIGHTING);

    // Draw grid of unit thickness.
    glLineWidth(1.0f);

    // Draw grid with a solid gray color.
    glColor3f(0.5, 0.5, 0.5);

    // Save the current modelview matrix.
    glPushMatrix();

    // Scale the grid to an appropriate size.
    glScalef(10, 10, 10);

    // Draw the static grid.
    glCallList(grid);

    // Restore the modelview matrix.
    glPopMatrix();

    // Restore the lighting attributes.
    glPopAttrib();
}

// Draws the mesh at the grid origin according and appropriately rotated.
void drawMesh()
{
    // Save the current modelview matrix.
    glPushMatrix();

    // Rotate the object according to how much it has spun.
    glRotatef(spinAngleY, 0, 1, 0);

    // Render the static mesh object.
    glCallList(mesh);

    // Restore the modelview matrix.
    glPopMatrix();
}

// This function is responsible for displaying the object.
void drawScene(void)
{
    // Clear the rendering window
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Initialize the model-view matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Position the camera at [0,0,5], looking at [0,0,0],
    // with [0,1,0] as the up direction.
    gluLookAt(position[0], position[1], position[2],
        0.0, 0.0, 0.0,
        0.0, 1.0, 0.0);

    // Save the modelview matrix. This is our home-world space (useful for mouse scrolling).
    glPushMatrix();

    // Rotate the world space.
    glMultMatrixf(space);

    //Draw a grid to show how our world is oriented.
    drawGrid();

    // Set material properties of object

    // Get current color in OpenGL-readable RGB format.
    RGB meshRgbColor;
    hcy2rgb(meshHcyColor, meshRgbColor);

    // Set the material color for our static object.
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, meshRgbColor.values);

    // Define specular color and shininess
    GLfloat specColor[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat shininess[] = { 100.0 };

    // Note that the specular color and shininess can stay constant
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specColor);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, shininess);

    // Set light properties

    // Light color (RGBA)
    GLfloat Lt0diff[] = { 1.0,1.0,1.0,1.0 };

    // Set light diffusiion and position.
    glLightfv(GL_LIGHT0, GL_DIFFUSE, Lt0diff);
    glLightfv(GL_LIGHT0, GL_POSITION, Lt0pos);

    // Draw our static object.
    drawMesh();

    // Restore our modelview matrix.
    glPopMatrix();

    // Dump the image to the screen.
    glutSwapBuffers();
}

// Creates a static mesh using the supplied render function.
void createStaticList(GLuint &glList, void func())
{
    // Generate the new single list.
    glList = glGenLists(1);

    // Initialize list function.
    glNewList(glList, GL_COMPILE);

    // Do the list function.
    func();

    // End list call.
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
    // Set up viewport to use full screen.
    glViewport(0, 0, w, h);

    // Set up a perspective view.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(
        FIELD_OF_VIEW,
        (double)w / (double)h,
        NEAR_PERSPECTIVE,
        FAR_PERSPECTIVE
    );
}

// Loads an OBJ file mesh from the standard input stream.
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

// This function is called every timer update (60 times per second).
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
        meshHcyColor.rotateHue(HUE_SHIFT_DEGREES);
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

    // Create static object mesh.
    createStaticList(mesh, renderMesh);

    // Create static grid mesh.
    createStaticList(grid, renderGrid);

    // Set up callback functions for key presses
    glutKeyboardFunc(keyboardFunc); // Handles "normal" ASCII symbols
    glutSpecialFunc(specialFunc);   // Handles "special" keyboard keys

    // Set up callback functions for mouse input
    glutMouseFunc(mouseFunc);       // Handles mouse button input
    glutMotionFunc(motionFunc);     // Handles mouse movement
    glutMouseWheelFunc(mouseWheel);

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