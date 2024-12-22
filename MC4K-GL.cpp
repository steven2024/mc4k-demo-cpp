#include <SDL.h>
#include <SDL_opengl.h>
#include <cmath>
#include <cstdlib>

const int w = 856;
const int h = 480;

SDL_Window* window = nullptr;
SDL_GLContext glContext;

static inline float math_sin(float x) { return sinf(x); }
static inline float math_cos(float x) { return cosf(x); }

int texmap[16 * 16 * 16 * 3];
char map[64 * 64 * 64];

static inline int random(int max) {
    return (rand() ^ (rand() << 16)) % max;
}

static void makeTextures() {
    for (int j = 0; j < 16; j++) {
        int k = 255 - random(96);
        for (int m = 0; m < 16 * 3; m++) {
            for (int n = 0; n < 16; n++) {
                int i1 = 0x966C4A;
                int i2 = 0;
                int i3 = 0;

                if (j == 4) i1 = 0x7F7F7F;
                if ((j != 4) || (random(3) == 0))
                    k = 255 - random(96);
                if (j == 1) {
                    if (m < (((n * n * 3 + n * 81) >> 2) & 0x3) + 18)
                        i1 = 0x6AAA40;
                    else if (m < (((n * n * 3 + n * 81) >> 2) & 0x3) + 19)
                        k = k * 2 / 3;
                }
                if (j == 7) {
                    i1 = 0x675231;
                    if ((n > 0) && (n < 15) && (((m > 0) && (m < 15)) || ((m > 32) && (m < 47)))) {
                        i1 = 0xBC9862;
                        i2 = n - 7;
                        i3 = (m & 0xF) - 7;
                        if (i2 < 0) i2 = 1 - i2;
                        if (i3 < 0) i3 = 1 - i3;
                        if (i3 > i2) i2 = i3;
                        k = 196 - random(32) + i2 % 3 * 32;
                    }
                    else if (random(2) == 0)
                        k = k * (150 - (n & 0x1) * 100) / 100;
                }
                if (j == 5) {
                    i1 = 0xB53A15;
                    if (((n + m / 4 * 4) % 8 == 0) || (m % 4 == 0))
                        i1 = 0xBCAFA5;
                }
                i2 = k;
                if (m >= 32) i2 /= 2;
                if (j == 8) {
                    i1 = 5298487;
                    if (random(2) == 0) {
                        i1 = 0;
                        i2 = 255;
                    }
                }
                i3 = ((((i1 >> 16) & 0xFF) * i2 / 255) << 16) |
                    ((((i1 >> 8) & 0xFF) * i2 / 255) << 8) |
                    ((i1 & 0xFF) * i2 / 255);
                texmap[n + m * 16 + j * 256 * 3] = i3;
            }
        }
    }
}

static void makeMap() {
    for (int x = 0; x < 64; x++) {
        for (int y = 0; y < 64; y++) {
            for (int z = 0; z < 64; z++) {
                int i = (z << 12) | (y << 6) | x;
                float yd = (y - 32.5) * 0.4;
                float zd = (z - 32.5) * 0.4;
                map[i] = random(16);

                float th = random(256) / 256.0f;

                if (th > sqrtf(sqrtf(yd * yd + zd * zd)) - 0.8f)
                    map[i] = 0;
            }
        }
    }
}

static void init() {
    makeTextures();
    makeMap();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w, h, 0, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
}

static inline void plot(int x, int y, int r, int g, int b) {
    glColor3ub(r, g, b);
    glVertex2i(x, y);
}

static void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    float now = (float)(SDL_GetTicks() % 10000) / 10000.f;

    const int lowResWidth = 214;
    const int lowResHeight = 120;
    const float scaleX = (float)w / lowResWidth;
    const float scaleY = (float)h / lowResHeight;

    static int buffer[lowResWidth * lowResHeight];

    float xRot = math_sin(now * math_pi * 2) * 0.4 + math_pi / 2;
    float yRot = math_cos(now * math_pi * 2) * 0.4;
    float yCos = math_cos(yRot);
    float ySin = math_sin(yRot);
    float xCos = math_cos(xRot);
    float xSin = math_sin(xRot);

    float ox = 32.5 + now * 64.0;
    float oy = 32.5;
    float oz = 32.5;

    for (int x = 0; x < lowResWidth; x++) {
        float ___xd = ((float)x - (float)lowResWidth / 2.f) / (float)lowResHeight; 
        for (int y = 0; y < lowResHeight; y++) {
            float __yd = ((float)y - (float)lowResHeight / 2.f) / (float)lowResHeight; 
            float __zd = 1;  

            float ___zd = __zd * yCos + __yd * ySin;
            float _yd = __yd * yCos - __zd * ySin;
            float _xd = ___xd * xCos + ___zd * xSin;
            float _zd = ___zd * xCos - ___xd * xSin;

            int col = 0;       
            int br = 255;      
            float closest = 32.f;  

            for (int d = 0; d < 3; d++) {
                float dimLength = (d == 0) ? _xd : (d == 1) ? _yd : _zd;
                float ll = 1.0f / fabs(dimLength);
                float xd = _xd * ll;
                float yd = _yd * ll;
                float zd = _zd * ll;

                float initial = (d == 0 ? ox : (d == 1 ? oy : oz)) - floor((d == 0 ? ox : (d == 1 ? oy : oz)));
                if (dimLength > 0) initial = 1 - initial;
                float dist = ll * initial;

                float xp = ox + xd * initial;
                float yp = oy + yd * initial;
                float zp = oz + zd * initial;

                if (dimLength < 0) {
                    if (d == 0) xp--;
                    if (d == 1) yp--;
                    if (d == 2) zp--;
                }

                while (dist < closest) {
                    int tex = map[(((int)zp & 63) << 12) | (((int)yp & 63) << 6) | ((int)xp & 63)];
                    if (tex > 0) {
                        int u = ((int)((xp + zp) * 16.f)) & 15;
                        int v = ((int)(yp * 16.f) & 15) + 16;

                        if (d == 1) {
                            u = ((int)(xp * 16.f)) & 15;
                            v = ((int)(zp * 16.f)) & 15;
                            if (yd < 0) v += 32;
                        }

                        int cc = texmap[u + v * 16 + tex * 256 * 3];
                        if (cc > 0) {
                            col = cc;
                            br = 255 * (255 - ((d + 2) % 3) * 50) / 255;
                            closest = dist;
                        }
                    }
                    xp += xd;
                    yp += yd;
                    zp += zd;
                    dist += ll;
                }
            }

            int r = ((col >> 16) & 0xFF) * br / 255;
            int g = ((col >> 8) & 0xFF) * br / 255;
            int b = (col & 0xFF) * br / 255;

            buffer[x + y * lowResWidth] = (r << 16) | (g << 8) | b;
        }
    }

    glBegin(GL_QUADS);
    for (int x = 0; x < lowResWidth; x++) {
        for (int y = 0; y < lowResHeight; y++) {
            int color = buffer[x + y * lowResWidth];
            int r = (color >> 16) & 0xFF;
            int g = (color >> 8) & 0xFF;
            int b = color & 0xFF;

            glColor3ub(r, g, b);

            float x0 = x * scaleX;
            float y0 = y * scaleY;
            float x1 = (x + 1) * scaleX;
            float y1 = (y + 1) * scaleY;

            glVertex2f(x0, y0);
            glVertex2f(x1, y0);
            glVertex2f(x1, y1);
            glVertex2f(x0, y1);
        }
    }
    glEnd();

    SDL_GL_SwapWindow(window);
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);

    window = SDL_CreateWindow(
        "Minecraft4k OpenGL Fixed-Function", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h,
        SDL_WINDOW_OPENGL);

    glContext = SDL_GL_CreateContext(window);
    init();

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }

        render();
    }

    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
