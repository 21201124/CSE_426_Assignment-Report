#include <GL/freeglut.h>
#include <cmath>
#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>
using namespace std;

// --- Window and Coordinate Settings ---
const int WINDOW_WIDTH = 1200;
const int WINDOW_HEIGHT = 800;
const int UI_HEADER_HEIGHT = 60;
const float COORD_RANGE_X = 600.0f;
const float COORD_RANGE_Y = 350.0f;

const int MODE_DEFINE_WINDOW = 1;
const int MODE_DRAW_LINES = 2;
const int MODE_CLIP = 3;

// --- Data Structures ---
struct Point {
    float x, y;
};
struct Line {
    Point p1, p2;
};

// --- Global Variables ---
int app_mode = MODE_DEFINE_WINDOW;
int click_count = 0;
bool drawing_new_line = true;
float xmin = 0, ymin = 0, xmax = 0, ymax = 0;
Point current_start;
vector<Line> lines;
vector<Point> clipped_points;

// Transformation factors
float x_center, y_center, x_scale, y_scale;

// --- Utility Functions ---
Point logical_to_screen(float lx, float ly) {
    Point p;
    p.x = x_center + lx * x_scale;
    p.y = y_center + ly * y_scale;
    return p;
}
Point screen_to_logical(float sx, float sy) {
    Point p;
    p.x = (sx - x_center) / x_scale;
    p.y = (sy - y_center) / y_scale;
    return p;
}
void draw_text(float x, float y, const string &text, void *font,
               float r = 0.0f, float g = 0.0f, float b = 0.0f) {
    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    for (char c : text) glutBitmapCharacter(font, c);
}

// --- Liang–Barsky Algorithm ---
bool liang_barsky(float x0, float y0, float x1, float y1,
                  float &clipped_x0, float &clipped_y0,
                  float &clipped_x1, float &clipped_y1) {
    float dx = x1 - x0;
    float dy = y1 - y0;
    float p[4] = {-dx, dx, -dy, dy};
    float q[4] = {x0 - xmin, xmax - x0, y0 - ymin, ymax - y0};
    float t0 = 0.0f, t1 = 1.0f;

    for (int i = 0; i < 4; i++) {
        if (fabs(p[i]) < 1e-6) {
            if (q[i] < 0) return false;
        } else {
            float t = q[i] / p[i];
            if (p[i] < 0)
                t0 = max(t0, t);
            else
                t1 = min(t1, t);
        }
    }
    if (t0 > t1) return false;

    clipped_x0 = x0 + t0 * dx;
    clipped_y0 = y0 + t0 * dy;
    clipped_x1 = x0 + t1 * dx;
    clipped_y1 = y0 + t1 * dy;
    return true;
}

// --- Drawing Functions ---
void draw_axes() {
    glColor3f(0.7f, 0.7f, 0.7f);
    glBegin(GL_LINES);
    glVertex2f(0, y_center);
    glVertex2f(WINDOW_WIDTH, y_center);
    glVertex2f(x_center, 0);
    glVertex2f(x_center, WINDOW_HEIGHT - UI_HEADER_HEIGHT);
    glEnd();
}
void draw_window() {
    Point p1 = logical_to_screen(xmin, ymin);
    Point p2 = logical_to_screen(xmax, ymin);
    Point p3 = logical_to_screen(xmax, ymax);
    Point p4 = logical_to_screen(xmin, ymax);
    glColor3f(0, 0, 0);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(p1.x, p1.y);
    glVertex2f(p2.x, p2.y);
    glVertex2f(p3.x, p3.y);
    glVertex2f(p4.x, p4.y);
    glEnd();
}
void draw_ui_header(const string &title, const string &hint, float r, float g, float b) {
    glColor3f(0.9f, 0.9f, 0.9f);
    glBegin(GL_QUADS);
    glVertex2f(0, WINDOW_HEIGHT - UI_HEADER_HEIGHT);
    glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT - UI_HEADER_HEIGHT);
    glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT);
    glVertex2f(0, WINDOW_HEIGHT);
    glEnd();

    glColor3f(0.4f, 0.4f, 0.4f);
    glBegin(GL_LINES);
    glVertex2f(0, WINDOW_HEIGHT - UI_HEADER_HEIGHT);
    glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT - UI_HEADER_HEIGHT);
    glEnd();

    draw_text(10, WINDOW_HEIGHT - 25, title, GLUT_BITMAP_HELVETICA_18, r, g, b);
    draw_text(10, WINDOW_HEIGHT - 45, hint, GLUT_BITMAP_HELVETICA_12, 0.1f, 0.1f, 0.1f);
}

// --- Display Modes ---
void draw_define_window_mode() {
    glClear(GL_COLOR_BUFFER_BIT);
    draw_ui_header("STEP 1: Define Clipping Window",
                   "Click two opposite corners (P1 and P2)", 0.0f, 0.5f, 0.0f);
    draw_axes();

    if (click_count == 1) {
        Point p = logical_to_screen(xmin, ymin);
        glColor3f(1.0f, 0.0f, 0.0f);
        glPointSize(6.0f);
        glBegin(GL_POINTS);
        glVertex2f(p.x, p.y);
        glEnd();
    }

    glFlush();
}

void draw_line_input_mode() {
    glClear(GL_COLOR_BUFFER_BIT);
    draw_ui_header("STEP 2: Draw Line Segments",
                   "Click to set line endpoints. Press ENTER to clip. Press R to reset.",
                   0.8f, 0.3f, 0.0f);
    draw_axes();
    draw_window();

    // Draw all existing lines
    glColor3f(0.5f, 0.5f, 0.5f);
    glBegin(GL_LINES);
    for (auto &line : lines) {
        Point p1 = logical_to_screen(line.p1.x, line.p1.y);
        Point p2 = logical_to_screen(line.p2.x, line.p2.y);
        glVertex2f(p1.x, p1.y);
        glVertex2f(p2.x, p2.y);
    }
    glEnd();

    glFlush();
}

void draw_clipping_mode() {
    glClear(GL_COLOR_BUFFER_BIT);
    draw_ui_header("STEP 3: Clipping Results (Liang–Barsky)",
                   "Red: Original | Green: Clipped | Blue: Intersections | Press R to reset",
                   0.0f, 0.5f, 0.0f);
    draw_axes();
    draw_window();

    clipped_points.clear();

    for (auto &line : lines) {
        // Draw original (red)
        glColor3f(1.0f, 0.0f, 0.0f);
        glBegin(GL_LINES);
        Point sp1 = logical_to_screen(line.p1.x, line.p1.y);
        Point sp2 = logical_to_screen(line.p2.x, line.p2.y);
        glVertex2f(sp1.x, sp1.y);
        glVertex2f(sp2.x, sp2.y);
        glEnd();

        float cx0, cy0, cx1, cy1;
        if (liang_barsky(line.p1.x, line.p1.y, line.p2.x, line.p2.y, cx0, cy0, cx1, cy1)) {
            // Draw clipped portion
            Point cp1 = logical_to_screen(cx0, cy0);
            Point cp2 = logical_to_screen(cx1, cy1);
            glColor3f(0.0f, 0.8f, 0.0f);
            glLineWidth(3.0f);
            glBegin(GL_LINES);
            glVertex2f(cp1.x, cp1.y);
            glVertex2f(cp2.x, cp2.y);
            glEnd();

            // Draw intersection dots
            glColor3f(0.0f, 0.0f, 1.0f);
            glPointSize(7.0f);
            glBegin(GL_POINTS);
            glVertex2f(cp1.x, cp1.y);
            glVertex2f(cp2.x, cp2.y);
            glEnd();

            clipped_points.push_back({cx0, cy0});
            clipped_points.push_back({cx1, cy1});
        }
    }

    // Display coordinates list on right
    float start_y = WINDOW_HEIGHT - UI_HEADER_HEIGHT - 30;
    draw_text(900, start_y + 15, "Visible Points (x, y):", GLUT_BITMAP_HELVETICA_12);
    start_y -= 15;
    ostringstream ss;
    ss << fixed << setprecision(1);
    for (size_t i = 0; i < clipped_points.size(); ++i) {
        ss.str("");
        ss << "P" << (i + 1) << ": (" << clipped_points[i].x << ", " << clipped_points[i].y << ")";
        draw_text(900, start_y, ss.str(), GLUT_BITMAP_HELVETICA_10);
        start_y -= 15;
        if (start_y < 50) break;
    }

    glFlush();
}

void display() {
    if (app_mode == MODE_DEFINE_WINDOW)
        draw_define_window_mode();
    else if (app_mode == MODE_DRAW_LINES)
        draw_line_input_mode();
    else
        draw_clipping_mode();
}

// --- Input Handlers ---
void mouse(int button, int state, int x, int y) {
    if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN) return;

    float screen_x = (float)x;
    float screen_y = (float)(WINDOW_HEIGHT - y);
    Point logical = screen_to_logical(screen_x, screen_y);

    if (app_mode == MODE_DEFINE_WINDOW) {
        if (click_count == 0) {
            xmin = logical.x;
            ymin = logical.y;
            click_count = 1;
        } else {
            xmax = logical.x;
            ymax = logical.y;
            if (xmin > xmax) swap(xmin, xmax);
            if (ymin > ymax) swap(ymin, ymax);
            app_mode = MODE_DRAW_LINES;
            click_count = 0;
        }
    } else if (app_mode == MODE_DRAW_LINES) {
        if (drawing_new_line) {
            current_start = logical;
            drawing_new_line = false;
        } else {
            Line new_line = {current_start, logical};
            lines.push_back(new_line);
            current_start = logical;
        }
    }
    glutPostRedisplay();
}

void keyboard(unsigned char key, int, int) {
    if (key == 13) { // Enter
        if (app_mode == MODE_DRAW_LINES) app_mode = MODE_CLIP;
    } else if (key == ' ' && app_mode == MODE_DRAW_LINES) {
        drawing_new_line = true;
    } else if (key == 'r' || key == 'R') {
        app_mode = MODE_DEFINE_WINDOW;
        lines.clear();
        clipped_points.clear();
        xmin = ymin = xmax = ymax = 0;
        click_count = 0;
        drawing_new_line = true;
    }
    glutPostRedisplay();
}

// --- Initialization ---
void init() {
    glClearColor(1, 1, 1, 1);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);

    x_center = WINDOW_WIDTH * 0.35f;
    y_center = (WINDOW_HEIGHT - UI_HEADER_HEIGHT) / 2.0f;
    x_scale = (WINDOW_WIDTH * 0.6f) / (2 * COORD_RANGE_X);
    y_scale = (WINDOW_HEIGHT - UI_HEADER_HEIGHT) / (2 * COORD_RANGE_Y);
}

// --- Main ---
int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Liang–Barsky Line Clipping (All Quadrants)");

    init();
    glutDisplayFunc(display);
    glutMouseFunc(mouse);
    glutKeyboardFunc(keyboard);
    glutMainLoop();
    return 0;
}
