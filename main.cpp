#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <GL/gl.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <cstdlib>
#include <algorithm>

const int WIDTH = 1280;
const int HEIGHT = 720;
const int TERRAIN_SIZE = 128;
const float TILE_WORLD_SIZE = 400.0f;
const int TILES_AROUND = 2;

float posX = 0.0f, posY = 80.0f, posZ = 0.0f;
float pitch = 0.0f, yaw = 0.0f, roll = 0.0f;
float speed = 0.0f;
float pitchRate = 0, yawRate = 0, rollRate = 0;

std::vector<float> heightmap(TERRAIN_SIZE * TERRAIN_SIZE);

float noise(int x, int y) {
    x = (x % TERRAIN_SIZE + TERRAIN_SIZE) % TERRAIN_SIZE;
    y = (y % TERRAIN_SIZE + TERRAIN_SIZE) % TERRAIN_SIZE;
    int n = x + y * 57;
    n = (n << 13) ^ n;
    return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
}

float smoothNoise(int x, int y) {
    float corners = (noise(x-1, y-1) + noise(x+1, y-1) + noise(x-1, y+1) + noise(x+1, y+1)) / 16.0f;
    float sides   = (noise(x-1, y) + noise(x+1, y) + noise(x, y-1) + noise(x, y+1)) / 8.0f;
    float center  = noise(x, y) / 4.0f;
    return corners + sides + center;
}

float interpolatedNoise(float x, float y) {
    int ix = (int)std::floor(x);
    int iy = (int)std::floor(y);
    float fx = x - ix;
    float fy = y - iy;
    float v1 = smoothNoise(ix, iy);
    float v2 = smoothNoise(ix+1, iy);
    float v3 = smoothNoise(ix, iy+1);
    float v4 = smoothNoise(ix+1, iy+1);
    float i1 = v1 * (1-fx) + v2 * fx;
    float i2 = v3 * (1-fx) + v4 * fx;
    return i1 * (1-fy) + i2 * fy;
}

float fractalNoise(float x, float y, int octaves) {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float maxValue = 0.0f;
    for (int i = 0; i < octaves; ++i) {
        total += interpolatedNoise(x * frequency, y * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }
    return total / maxValue;
}

void generateHeightmap() {
    for (int y = 0; y < TERRAIN_SIZE; ++y) {
        for (int x = 0; x < TERRAIN_SIZE; ++x) {
            float nx = x / (float)TERRAIN_SIZE * 4.0f;
            float ny = y / (float)TERRAIN_SIZE * 4.0f;
            float h = fractalNoise(nx, ny, 5);
            h = std::pow(std::abs(h), 1.3f) * (h > 0 ? 1.0f : -1.0f);
            heightmap[y * TERRAIN_SIZE + x] = h * 100.0f + 10.0f;
        }
    }
    for (int i = 0; i < TERRAIN_SIZE; ++i) {
        heightmap[i] = heightmap[i + (TERRAIN_SIZE-1)*TERRAIN_SIZE];
        heightmap[i * TERRAIN_SIZE] = heightmap[i * TERRAIN_SIZE + (TERRAIN_SIZE-1)];
    }
    heightmap[0] = heightmap[(TERRAIN_SIZE-1)*TERRAIN_SIZE + (TERRAIN_SIZE-1)];
}

float getHeight(float worldX, float worldZ) {
    float tileX = std::fmod(worldX, TILE_WORLD_SIZE);
    float tileZ = std::fmod(worldZ, TILE_WORLD_SIZE);
    if (tileX < 0) tileX += TILE_WORLD_SIZE;
    if (tileZ < 0) tileZ += TILE_WORLD_SIZE;

    float u = tileX / TILE_WORLD_SIZE * (TERRAIN_SIZE - 1);
    float v = tileZ / TILE_WORLD_SIZE * (TERRAIN_SIZE - 1);

    int x0 = (int)u % TERRAIN_SIZE;
    int y0 = (int)v % TERRAIN_SIZE;
    float fx = u - (int)u;
    float fy = v - (int)v;

    auto sample = [&](int x, int y) {
        x = (x % TERRAIN_SIZE + TERRAIN_SIZE) % TERRAIN_SIZE;
        y = (y % TERRAIN_SIZE + TERRAIN_SIZE) % TERRAIN_SIZE;
        return heightmap[y * TERRAIN_SIZE + x];
    };

    float h00 = sample(x0, y0);
    float h10 = sample(x0+1, y0);
    float h01 = sample(x0, y0+1);
    float h11 = sample(x0+1, y0+1);

    float h0 = h00 * (1-fx) + h10 * fx;
    float h1 = h01 * (1-fx) + h11 * fx;
    return h0 * (1-fy) + h1 * fy;
}

void colorFromHeight(float h, float& r, float& g, float& b) {
    if (h < 30.0f) {
        r = 0.22f; g = 0.55f; b = 0.22f;
    } else if (h < 55.0f) {
        float t = (h - 30.0f) / 25.0f;
        r = 0.22f + t * 0.28f;
        g = 0.55f - t * 0.12f;
        b = 0.22f - t * 0.08f;
    } else if (h < 80.0f) {
        float t = (h - 55.0f) / 25.0f;
        r = 0.5f + t * 0.18f;
        g = 0.43f + t * 0.12f;
        b = 0.35f + t * 0.12f;
    } else {
        float t = std::min(1.0f, (h - 80.0f) / 40.0f);
        r = 0.68f + t * 0.25f;
        g = 0.62f + t * 0.25f;
        b = 0.55f + t * 0.3f;
    }
}

void drawTerrain() {
    int centerTileX = (int)std::floor(posX / TILE_WORLD_SIZE);
    int centerTileZ = (int)std::floor(posZ / TILE_WORLD_SIZE);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    GLfloat lightPos[] = {0.4f, 1.0f, 0.3f, 0.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    GLfloat diffuse[] = {1.0f, 1.0f, 0.95f, 1.0f};
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);

    // Disable culling so both sides are visible while we fix winding
    glDisable(GL_CULL_FACE);

    for (int tz = centerTileZ - TILES_AROUND; tz <= centerTileZ + TILES_AROUND; ++tz) {
        for (int tx = centerTileX - TILES_AROUND; tx <= centerTileX + TILES_AROUND; ++tx) {
            float originX = tx * TILE_WORLD_SIZE;
            float originZ = tz * TILE_WORLD_SIZE;

            glBegin(GL_TRIANGLES);
            const int step = 2;
            for (int y = 0; y < TERRAIN_SIZE - step; y += step) {
                for (int x = 0; x < TERRAIN_SIZE - step; x += step) {
                    auto emit = [&](int ix, int iy) {
                        float wx = originX + (ix / (float)(TERRAIN_SIZE-1)) * TILE_WORLD_SIZE;
                        float wz = originZ + (iy / (float)(TERRAIN_SIZE-1)) * TILE_WORLD_SIZE;
                        float h = getHeight(wx, wz);
                        float r,g,b;
                        colorFromHeight(h, r, g, b);
                        glColor3f(r, g, b);
                        glNormal3f(0, 1, 0);
                        glVertex3f(wx, h, wz);
                    };

                    // Reversed winding so the top face points upward toward the camera
                    emit(x, y); emit(x, y+step); emit(x+step, y);
                    emit(x+step, y); emit(x, y+step); emit(x+step, y+step);
                }
            }
            glEnd();
        }
    }
    glDisable(GL_LIGHTING);
}

void drawSky() {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glBegin(GL_QUADS);
    glColor3f(0.5f, 0.75f, 0.95f);
    glVertex3f(-6000, 700, -6000);
    glVertex3f( 6000, 700, -6000);
    glVertex3f( 6000, 700,  6000);
    glVertex3f(-6000, 700,  6000);
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
    glClearColor(0.5f, 0.75f, 0.95f, 1.0f);
    // Culling disabled in drawTerrain for safety

    generateHeightmap();

    yaw = 0.0f;

    bool running = true;
    SDL_Event e;
    const Uint8* keys = SDL_GetKeyboardState(nullptr);

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
        }

        const float controlRate = 35.0f;

        // Swapped A and D
        rollRate = 0;
        if (keys[SDL_SCANCODE_A]) rollRate =  controlRate;  // now right
        if (keys[SDL_SCANCODE_D]) rollRate = -controlRate;  // now left

        pitchRate = 0;
        if (keys[SDL_SCANCODE_W]) pitchRate = -controlRate * 0.65f;
        if (keys[SDL_SCANCODE_S]) pitchRate =  controlRate * 0.65f;

        yawRate = 0;
        if (keys[SDL_SCANCODE_Z]) yawRate = -controlRate * 0.55f;
        if (keys[SDL_SCANCODE_X]) yawRate =  controlRate * 0.55f;

        if (keys[SDL_SCANCODE_EQUALS] || keys[SDL_SCANCODE_KP_PLUS]) {
            speed += 0.08f;
        }
        if (keys[SDL_SCANCODE_MINUS] || keys[SDL_SCANCODE_KP_MINUS]) {
            speed -= 0.08f;
        }
        if (speed > 12.0f) speed = 12.0f;
        if (speed < -6.0f) speed = -6.0f;

        if (!keys[SDL_SCANCODE_EQUALS] && !keys[SDL_SCANCODE_KP_PLUS] &&
            !keys[SDL_SCANCODE_MINUS] && !keys[SDL_SCANCODE_KP_MINUS]) {
            speed *= 0.98f;
        }

        float dt = 0.016f;
        roll  += rollRate * dt;
        pitch += pitchRate * dt;
        yaw   += yawRate * dt;

        roll *= 0.985f;

        if (pitch > 55) pitch = 55;
        if (pitch < -55) pitch = -55;

        float radYaw   = yaw   * 3.14159265f / 180.0f;
        float radPitch = pitch * 3.14159265f / 180.0f;

        float forwardX = std::sin(radYaw) * std::cos(radPitch);
        float forwardY = -std::sin(radPitch);
        float forwardZ = std::cos(radYaw) * std::cos(radPitch);

        posX += forwardX * speed;
        posY += forwardY * speed;
        posZ += forwardZ * speed;

        float groundH = getHeight(posX, posZ);
        if (posY < groundH + 8.0f) posY = groundH + 8.0f;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        float aspect = (float)WIDTH / HEIGHT;
        float fov = 55.0f * 3.14159265f / 180.0f;
        float nearP = 0.5f, farP = 8000.0f;
        float top = nearP * std::tan(fov / 2);
        float bottom = -top;
        float right = top * aspect;
        float left = -right;
        glFrustum(left, right, bottom, top, nearP, farP);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glRotatef(-roll,  0, 0, 1);
        glRotatef(-pitch, 1, 0, 0);
        glRotatef(-yaw,   0, 1, 0);
        glTranslatef(-posX, -posY, -posZ);

        drawSky();
        drawTerrain();

        SDL_GL_SwapWindow(window);
        SDL_Delay(16);
    }

    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
