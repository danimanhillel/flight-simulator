#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <GL/gl.h>
#include <iostream>
#include <cmath>

const int WIDTH = 1280;
const int HEIGHT = 720;

// Simple camera / aircraft state
float posX = 0.0f, posY = 50.0f, posZ = 0.0f;
float pitch = 0.0f, yaw = 0.0f, roll = 0.0f;
float speed = 0.5f;

void drawGround() {
    glBegin(GL_QUADS);
    glColor3f(0.2f, 0.6f, 0.2f);
    glVertex3f(-1000, 0, -1000);
    glVertex3f( 1000, 0, -1000);
    glVertex3f( 1000, 0,  1000);
    glVertex3f(-1000, 0,  1000);
    glEnd();

    // Simple grid
    glColor3f(0.3f, 0.5f, 0.3f);
    glBegin(GL_LINES);
    for (int i = -20; i <= 20; ++i) {
        glVertex3f(i * 50, 0.1f, -1000);
        glVertex3f(i * 50, 0.1f,  1000);
        glVertex3f(-1000, 0.1f, i * 50);
        glVertex3f( 1000, 0.1f, i * 50);
    }
    glEnd();
}

void drawSky() {
    glDisable(GL_DEPTH_TEST);
    glBegin(GL_QUADS);
    glColor3f(0.4f, 0.7f, 1.0f);
    glVertex3f(-2000, 500, -2000);
    glVertex3f( 2000, 500, -2000);
    glVertex3f( 2000, 500,  2000);
    glVertex3f(-2000, 500,  2000);
    glEnd();
    glEnable(GL_DEPTH_TEST);
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL init failed\n";
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Flight Simulator",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIDTH, HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(1);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.4f, 0.7f, 1.0f, 1.0f);

    bool running = true;
    SDL_Event e;
    const Uint8* keys = SDL_GetKeyboardState(nullptr);

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
        }

        // Controls
        if (keys[SDL_SCANCODE_W]) pitch -= 0.5f;
        if (keys[SDL_SCANCODE_S]) pitch += 0.5f;
        if (keys[SDL_SCANCODE_A]) yaw   -= 0.5f;
        if (keys[SDL_SCANCODE_D]) yaw   += 0.5f;
        if (keys[SDL_SCANCODE_Q]) roll  -= 0.5f;
        if (keys[SDL_SCANCODE_E]) roll  += 0.5f;
        if (keys[SDL_SCANCODE_UP])   speed += 0.05f;
        if (keys[SDL_SCANCODE_DOWN]) speed -= 0.05f;
        if (speed < 0.1f) speed = 0.1f;

        // Simple movement
        float radYaw   = yaw   * 3.14159f / 180.0f;
        float radPitch = pitch * 3.14159f / 180.0f;
        posX += std::sin(radYaw) * speed;
        posZ += std::cos(radYaw) * speed;
        posY -= std::sin(radPitch) * speed;

        if (posY < 2.0f) posY = 2.0f;

        // Render
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        float aspect = (float)WIDTH / HEIGHT;
        float fov = 60.0f * 3.14159f / 180.0f;
        float nearP = 0.1f, farP = 5000.0f;
        float top = nearP * std::tan(fov / 2);
        float bottom = -top;
        float right = top * aspect;
        float left = -right;
        glFrustum(left, right, bottom, top, nearP, farP);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // Camera transform
        glRotatef(-roll,  0, 0, 1);
        glRotatef(-pitch, 1, 0, 0);
        glRotatef(-yaw,   0, 1, 0);
        glTranslatef(-posX, -posY, -posZ);

        drawSky();
        drawGround();

        SDL_GL_SwapWindow(window);
        SDL_Delay(16);
    }

    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
