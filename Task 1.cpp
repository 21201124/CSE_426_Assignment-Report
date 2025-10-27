#include <GL/freeglut.h>
#include <iostream>
#include <string>
#include <cmath>
using namespace std;

// --- Global Constants ---
const int WINDOW_SIZE = 700;
const int GRID_SPACING = 50;
const int MAX_COORD = WINDOW_SIZE / 2;
const int INPUT_MODE = 1;
const int DRAWING_MODE = 2;

// --- State Variables ---
int app_mode = INPUT_MODE; // 1: Input screen, 2: Drawing screen
int input_step = 1;        // Tracks input stage
string input_buffer;

// String storage for inputs
string choice_str, x1_str, y1_str, x2_str, y2_str, width_str;

// Parsed values
int choice = 0, x1_in = 0, y1_in = 0, x2_in = 0, y2_in = 0, width_in = 1;
bool has_input = false;

// --- Utility Functions ---
int to_screen_x(int x) { return x + MAX_COORD; }
int to_screen_y(int y) { return y + MAX_COORD; }

void draw_pixel(int x, int y, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_POINTS);
    glVertex2i(to_screen_x(x), to_screen_y(y));
    glEnd();
}

void draw_text(float x, float y, float r, float g, float b, const string &text, void *font) {
    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    for (char c : text) glutBitmapCharacter(font, c);
}

int parse_int(const string &str) {
    if (!str.empty() && !(str == "-")) {
        try {
            return stoi(str);
        } catch (...) {
            return 0;
        }
    }
    return 0;
}

// --- Bresenham’s Algorithm ---
void draw_line_bresenham(int x1, int y1, int x2, int y2) {
    int dx = abs(x2 - x1), dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        draw_pixel(x1, y1, 0.0, 0.0, 0.0);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx) { err += dx; y1 += sy; }
    }
}

void draw_thick_line(int x1, int y1, int x2, int y2, int w) {
    int half = (w - 1) / 2;
    int dx = abs(x2 - x1), dy = abs(y2 - y1);
    bool steep = dy > dx;

    for (int i = -half; i <= half; i++) {
        if (steep) draw_line_bresenham(x1 + i, y1, x2 + i, y2);
        else draw_line_bresenham(x1, y1 + i, x2, y2 + i);
    }
}

// --- Drawing Functions ---
void draw_axes() {
    glColor3f(0.0f, 0.0f, 1.0f);
    glBegin(GL_LINES);
    glVertex2i(0, MAX_COORD);
    glVertex2i(WINDOW_SIZE, MAX_COORD);
    glVertex2i(MAX_COORD, 0);
    glVertex2i(MAX_COORD, WINDOW_SIZE);
    glEnd();

    // Axis markers
    for (int coord = -MAX_COORD + GRID_SPACING; coord < MAX_COORD; coord += GRID_SPACING) {
        if (coord == 0) continue;
        int sx = to_screen_x(coord);
        int sy = to_screen_y(coord);
        glBegin(GL_LINES);
        glVertex2i(sx, MAX_COORD - 3);
        glVertex2i(sx, MAX_COORD + 3);
        glVertex2i(MAX_COORD - 3, sy);
        glVertex2i(MAX_COORD + 3, sy);
        glEnd();
    }
}

void draw_input_screen() {
    glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    string title = "Bresenham’s Line Drawing Input";
    draw_text(100, 620, 0, 0, 0, title, GLUT_BITMAP_HELVETICA_18);

    string prompt;
    switch (input_step) {
        case 1: prompt = "Enter 1 (Normal) or 2 (Thick): "; break;
        case 2: prompt = "Enter Start X1: "; break;
        case 3: prompt = "Enter Start Y1: "; break;
        case 4: prompt = "Enter End X2: "; break;
        case 5: prompt = "Enter End Y2: "; break;
        case 6: prompt = "Enter Line Width (>0): "; break;
    }

    draw_text(100, 500, 0.2, 0.2, 0.8, prompt + input_buffer + "_", GLUT_BITMAP_HELVETICA_18);
    glFlush();
}

// --- Input Handling ---
void process_input() {
    if (input_buffer.empty() && input_step != 1) return;

    switch (input_step) {
        case 1:
            if (input_buffer == "1" || input_buffer == "2") {
                choice_str = input_buffer;
                input_step = 2;
            } else {
                cerr << "Invalid choice.\n";
            }
            break;
        case 2: x1_str = input_buffer; input_step = 3; break;
        case 3: y1_str = input_buffer; input_step = 4; break;
        case 4: x2_str = input_buffer; input_step = 5; break;
        case 5:
            y2_str = input_buffer;
            if (choice_str == "1") { has_input = true; app_mode = DRAWING_MODE; }
            else input_step = 6;
            break;
        case 6: width_str = input_buffer; has_input = true; app_mode = DRAWING_MODE; break;
    }

    input_buffer.clear();

    if (app_mode == DRAWING_MODE) {
        choice = parse_int(choice_str);
        x1_in = parse_int(x1_str);
        y1_in = parse_int(y1_str);
        x2_in = parse_int(x2_str);
        y2_in = parse_int(y2_str);
        width_in = (choice == 2) ? max(1, parse_int(width_str)) : 1;
        cout << "Drawing: P1(" << x1_in << ", " << y1_in << ") P2(" << x2_in << ", " << y2_in
             << ") Width: " << width_in << endl;
    }

    glutPostRedisplay();
}

void keyboard(unsigned char key, int, int) {
    if (app_mode != INPUT_MODE) return;

    if (key == 13) process_input();              // ENTER
    else if (key == 8 || key == 127) {          // BACKSPACE/DEL
        if (!input_buffer.empty()) input_buffer.pop_back();
    }
    else if ((key >= '0' && key <= '9') || key == '-') {
        if (key == '-' && !input_buffer.empty()) return;
        if (input_buffer.size() < 5) input_buffer += key;
    }

    glutPostRedisplay();
}

// --- Display ---
void display() {
    if (app_mode == INPUT_MODE) {
        draw_input_screen();
    } else {
        glClearColor(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        glPointSize(1.5);
        draw_axes();

        if (has_input) {
            if (choice == 1) draw_line_bresenham(x1_in, y1_in, x2_in, y2_in);
            else draw_thick_line(x1_in, y1_in, x2_in, y2_in, width_in);
        }

        draw_text(10, 10, 0.4, 0, 0, "Line Drawn. Close window to exit.", GLUT_BITMAP_HELVETICA_10);
        glFlush();
    }
}

// --- Initialization ---
void init() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WINDOW_SIZE, 0, WINDOW_SIZE);
}

// --- Main ---
int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_SIZE, WINDOW_SIZE);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Bresenham’s Line Drawing");

    init();
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutMainLoop();
    return 0;
}
