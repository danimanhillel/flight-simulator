#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <GL/gl.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <cstdlib>

const int WIDTH = 1280;
const int HEIGHT = 720;
const int TERRAIN_SIZE = 256;      // heightmap resolution
const float TILE_WORLD_SIZE = 800.0f;
const int TILES_AROUND = 3;        // how many tiles in each direction

// Aircraft state
float posX = 0.0f, posY = 120.0f, posZ = 0.0f;
float pitch = 0.0f, yaw = 0.0f, roll = 0.0f;
float speed = 2.0f;
float pitchRate = 0, yawRate = 0, rollRate = 0;

// Heightmap
std::vector<float> heightmap(TERRAIN_SIZE * TERRAIN_SIZE);

// Simple value noise for heightmap
float noise(int x, int y) {
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
    int ix = (int)x;
    int iy = (int)y;
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
            float nx = x / (float)TERRAIN_SIZE * 6.0f;
            float ny = y / (float)TERRAIN_SIZE * 6.0f;
            float h = fractalNoise(nx, ny, 6);
            // Emphasize ridges a bit like the reference
            h = std::pow(std::abs(h), 1.4f) * (h > 0 ? 1.0f : -1.0f);
            heightmap[y * TERRAIN_SIZE + x] = h * 80.0f; // scale height
        }
    }
}

float getHeight(float worldX, float worldZ) {
    // Tile wrapping
    float tileX = std::fmod(worldX, TILE_WORLD_SIZE);
    float tileZ = std::fmod(worldZ, TILE_WORLD_SIZE);
    if (tileX < 0) tileX += TILE_WORLD_SIZE;
    if (tileZ < 0) tileZ += TILE_WORLD_SIZE;

    float u = tileX / TILE_WORLD_SIZE * (TERRAIN_SIZE - 1);
    float v = tileZ / TILE_WORLD_SIZE * (TERRAIN_SIZE - 1);

    int x0 = (int)u;
    int y0 = (int)v;
    float fx = u - x0;
    float fy = v - y0;

    auto sample = [&](int x, int y) {
        x = std::clamp(x, 0, TERRAIN_SIZE-1);
        y = std::clamp(y, 0, TERRAIN_SIZE-1);
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
    // Match the aerial photo: green valleys, brown mid, gray rock, light peaks
    if (h < 15.0f) {          // low green
        r = 0.25f; g = 0.55f; b = 0.25f;
    } else if (h < 35.0f) {   // mid green-brown
        float t = (h - 15.0f) / 20.0f;
        r = 0.25f + t * 0.25f;
        g = 0.55f - t * 0.15f;
        b = 0.25f - t * 0.1f;
    } else if (h < 55.0f) {   // rock gray
        float t = (h - 35.0f) / 20.0f;
        r = 0.5f + t * 0.2f;
        g = 0.4f + t * 0.15f;
        b = 0.35f + t * 0.15f;
    } else {                  // high light rock
        float t = std::min(1.0f, (h - 55.0f) / 25.0f);
        r = 0.7f + t * 0.25f;
        g = 0.65f + t * 0.25f;
        b = 0.6f + t * 0.3f;
    }
}

void drawTerrain() {
    // Draw tiles around the player
    int centerTileX = (int)std::floor(posX / TILE_WORLD_SIZE);
    int centerTileZ = (int)std::floor(posZ / TILE_WORLD_SIZE);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    GLfloat lightPos[] = {0.3f, 1.0f, 0.5f, 0.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    for (int tz = centerTileZ - TILES_AROUND; tz <= centerTileZ + TILES_AROUND; ++tz) {
        for (int tx = centerTileX - TILES_AROUND; tx <= centerTileX + TILES_AROUND; ++tx) {
            float originX = tx * TILE_WORLD_SIZE;
            float originZ = tz * TILE_WORLD_SIZE;

            glBegin(GL_TRIANGLES);
            const int step = 4; // coarser for performance
            for (int y = 0; y < TERRAIN_SIZE - step; y += step) {
                for (int x = 0; x < TERRAIN_SIZE - step; x += step) {
                    auto emit = [&](int ix, int iy) {
                        float wx = originX + (ix / (float)(TERRAIN_SIZE-1)) * TILE_WORLD_SIZE;
                        float wz = originZ + (iy / (float)(TERRAIN_SIZE-1)) * TILE_WORLD_SIZE;
                        float h = getHeight(wx, wz);
                        float r,g,b;
                        colorFromHeight(h, r, g, b);
                        glColor3f(r, g, b);
                        glNormal3f(0, 1, 0); // simple normal
                        glVertex3f(wx, h, wz);
                    };

                    // two triangles
                    emit(x, y); emit(x+step, y); emit(x, y+step);
                    emit(x+step, y); emit(x+step, y+step); emit(x, y+step);
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
    glColor3f(0.45f, 0.7f, 0.95f);
    glVertex3f(-5000, 800, -5000);
    glVertex3f( 5000, 800, -5000);
    glVertex3f( 5000, 800,  5000);
    glVertex3f(-5000, 800,  5000);
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
    glClearColor(0.45f, 0.7f, 0.95f, 1.0f);
    glEnable(GL_CULL_FACE);

    generateHeightmap();

    // Start looking forward (positive Z direction feels natural)
    yaw = 0.0f;

    bool running = true;
    SDL_Event e;
    const Uint8* keys = SDL_GetKeyboardState(nullptr);

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
        }

        // Controls - rates
        const float controlRate = 40.0f; // degrees per second-ish

        // Ailerons: A left, D right
        rollRate = 0;
        if (keys[SDL_SCANCODE_A]) rollRate = -controlRate;
        if (keys[SDL_SCANCODE_D]) rollRate =  controlRate;

        // Elevator: W nose up, S nose down
        pitchRate = 0;
        if (keys[SDL_SCANCODE_W]) pitchRate = -controlRate * 0.7f;
        if (keys[SDL_SCANCODE_S]) pitchRate =  controlRate * 0.7f;

        // Rudder: Z left, X right
        yawRate = 0;
        if (keys[SDL_SCANCODE_Z]) yawRate = -controlRate * 0.6f;
        if (keys[SDL_SCANCODE_X]) yawRate =  controlRate * 0.6f;

        // Speed
        if (keys[SDL_SCANCODE_EQUALS] || keys[SDL_SCANCODE_KP_PLUS]) speed += 0.05f;
        if (keys[SDL_SCANCODE_MINUS] || keys[SDL_SCANCODE_KP_MINUS]) speed -= 0.05f;
        if (speed < 0.5f) speed = 0.5f;
        if (speed > 15.0f) speed = 15.0f;

        // Apply rates (simple physics)
        float dt = 0.016f;
        roll  += rollRate * dt;
        pitch += pitchRate * dt;
        yaw   += yawRate * dt;

        // Mild damping
        roll  *= 0.98f;
        // pitch and yaw keep their value (more plane-like)

        // Clamp pitch to avoid flipping too hard
        if (pitch > 60) pitch = 60;
        if (pitch < -60) pitch = -60;

        // Movement in the direction the plane is facing
        float radYaw   = yaw   * 3.14159265f / 180.0f;
        float radPitch = pitch * 3.14159265f / 180.0f;

        // Forward is positive Z when yaw=0
        float forwardX = std::sin(radYaw) * std::cos(radPitch);
        float forwardY = -std::sin(radPitch);
        float forwardZ = std::cos(radYaw) * std::cos(radPitch);

        posX += forwardX * speed;
        posY += forwardY * speed;
        posZ += forwardZ * speed;

        // Keep above ground
        float groundH = getHeight(posX, posZ);
        if (posY < groundH + 8.0f) posY = groundH + 8.0f;

        // Render
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        float aspect = (float)WIDTH / HEIGHT;
        float fov = 60.0f * 3.14159265f / 180.0f;
        float nearP = 1.0f, farP = 8000.0f;
        float top = nearP * std::tan(fov / 2);
        float bottom = -top;
        float right = top * aspect;
        float left = -right;
        glFrustum(left, right, bottom, top, nearP, farP);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // Camera follows aircraft orientation
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
