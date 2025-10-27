#include <GL/freeglut.h>
#include <cmath>
#include <iostream>
using namespace std;

// --- Constants ---
const int WINDOW_SIZE = 700;
const int CENTER_X = WINDOW_SIZE / 2;
const int CENTER_Y = WINDOW_SIZE / 2;
const int NUM_CIRCLES = 80;      // Number of circles
const int MIN_RADIUS = 20;       // Starting radius
const int MAX_RADIUS = 300;      // Outermost radius
const float MAX_THICKNESS = 6.0; // Max circle thickness

// --- Utility: HSV → RGB Conversion ---
void hsvToRgb(float h, float s, float v, float &r, float &g, float &b) {
    if (s == 0.0f) { r = g = b = v; return; }

    h = fmod(h, 360.0f) / 60.0f;
    int i = static_cast<int>(floor(h));
    float f = h - i;
    float p = v * (1 - s);
    float q = v * (1 - s * f);
    float t = v * (1 - s * (1 - f));

    switch (i % 6) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
    }
}

// --- Set Gradient Color ---
void setGradientColor(int index) {
    float t = (float)index / (NUM_CIRCLES - 1);
    float hue = t * 360.0f; // 0 → 360 color cycle
    float s = 1.0f, v = 1.0f;
    float r, g, b;
    hsvToRgb(hue, s, v, r, g, b);
    glColor3f(r, g, b);
}

// --- Draw Pixel ---
void drawPixel(int x, int y, int cx, int cy, float size) {
    glPointSize(size);
    glBegin(GL_POINTS);
    glVertex2i(cx + x, cy + y);
    glEnd();
}

// --- Plot 8 Symmetric Points ---
void plotCirclePoints(int cx, int cy, int x, int y, float size) {
    drawPixel(x, y, cx, cy, size);
    drawPixel(-x, y, cx, cy, size);
    drawPixel(x, -y, cx, cy, size);
    drawPixel(-x, -y, cx, cy, size);
    drawPixel(y, x, cx, cy, size);
    drawPixel(-y, x, cx, cy, size);
    drawPixel(y, -x, cx, cy, size);
    drawPixel(-y, -x, cx, cy, size);
}

// --- Midpoint Circle Algorithm ---
void drawCircle(int cx, int cy, int r, float thickness) {
    int x = 0;
    int y = r;
    int p = 1 - r;
    plotCirclePoints(cx, cy, x, y, thickness);

    while (x < y) {
        x++;
        if (p < 0) p += 2 * x + 1;
        else { y--; p += 2 * (x - y) + 1; }
        plotCirclePoints(cx, cy, x, y, thickness);
    }
}

// --- Display Function ---
void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    int radius_step = (MAX_RADIUS - MIN_RADIUS) / (NUM_CIRCLES - 1);
    for (int i = 0; i < NUM_CIRCLES; ++i) {
        int radius = MIN_RADIUS + i * radius_step;
        float t = (float)i / (NUM_CIRCLES - 1);
        float thickness = 1.0f + t * (MAX_THICKNESS - 1.0f);
        setGradientColor(i);
        drawCircle(CENTER_X, CENTER_Y, radius, thickness);
    }

    glFlush();
}

// --- Initialization ---
void init() {
    glClearColor(1, 1, 1, 1);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WINDOW_SIZE, 0, WINDOW_SIZE);

    // Enable smoothing for smoother visuals
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
}

// --- Main ---
int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_SIZE, WINDOW_SIZE);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Concentric Circles with Gradient Colors");

    init();
    glutDisplayFunc(display);
    glutMainLoop();
    return 0;
}
