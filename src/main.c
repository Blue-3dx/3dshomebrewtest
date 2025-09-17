\
    // Improved Wave-mode prototype for 3DS (citro2d)
    // - Fixed player X (world scrolls left)
    // - Trail effect (stores last positions and draws faded sprites)
    // - Score increments when passing pipes
    // - Speed ramps up over time to increase difficulty
    // - Smooth rotation based on vertical velocity
    //
    #include <citro2d.h>
    #include <3ds.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <time.h>
    #include <math.h>

    #define SCREEN_W 400
    #define SCREEN_H 240
    #define PIPE_WIDTH 48
    #define GAP_HEIGHT 80
    #define PIPE_SPACING 160
    #define BASE_PIPE_SPEED 2.0f
    #define MAX_PIPES 8
    #define TRAIL_LEN 10

    typedef struct {
        float x;
        int gapY; // top of gap (y coordinate)
        bool active;
        bool scored;
    } Pipe;

    static Pipe pipes[MAX_PIPES];

    static void spawnPipe(int i, float startX) {
        pipes[i].x = startX;
        pipes[i].gapY = 32 + rand() % (SCREEN_H - 64 - GAP_HEIGHT);
        pipes[i].active = true;
        pipes[i].scored = false;
    }

    static void initPipes() {
        for (int i = 0; i < MAX_PIPES; i++) { pipes[i].active = false; pipes[i].scored = false; }
        float x = SCREEN_W + 40;
        for (int i = 0; i < MAX_PIPES; i++) {
            spawnPipe(i, x + i * PIPE_SPACING);
        }
    }

    static bool aabb_collide(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh) {
        return (ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by);
    }

    int main(int argc, char* argv[]) {
        srand(time(NULL));
        romfsInit();
        gfxInitDefault();
        C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
        C2D_Init();
        C2D_Prepare();
        consoleInit(GFX_BOTTOM, NULL);

        C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

        C2D_SpriteSheet sheet = C2D_SpriteSheetLoad("romfs:/wave.t3x");
        if (!sheet) {
            printf("Failed to load romfs:/wave.t3x\n");
            printf("Make sure you converted gfx/wave.png -> romfs/wave.t3x\n");
            printf("Press START to exit.\n");
        }

        C2D_Sprite wave;
        if (sheet) C2D_SpriteFromSheet(&wave, sheet, 0);
        C2D_SpriteSetCenter(&wave, 0.5f, 0.5f);
        float px = 80.0f;                // fixed X for player
        float py = SCREEN_H/2.0f;

        // trail buffer
        float trailX[TRAIL_LEN];
        float trailY[TRAIL_LEN];
        for (int i = 0; i < TRAIL_LEN; i++) { trailX[i] = px; trailY[i] = py; }

        initPipes();

        bool alive = true;
        float pipeSpeed = BASE_PIPE_SPEED;
        int score = 0;
        float playTime = 0.0f;
        float vy = 0.0f; // vertical velocity used for rotation

        while (aptMainLoop()) {
            hidScanInput();
            u32 kHeld = hidKeysHeld();
            u32 kDown = hidKeysDown();
            if (kDown & KEY_START) break;
            bool pressing = (kHeld & KEY_A) || (kHeld & KEY_B);

            if (!alive) {
                // wait for A to restart
                if (kDown & KEY_A) {
                    alive = true;
                    score = 0;
                    pipeSpeed = BASE_PIPE_SPEED;
                    py = SCREEN_H/2.0f;
                    initPipes();
                }
                // still draw death screen; do not update world
            } else {
                // update player vertical movement
                // wave mode: constant diagonal motion simulated by changing vy
                float targetVy = pressing ? -2.4f : 2.4f;
                // smooth velocity change
                vy += (targetVy - vy) * 0.25f;
                py += vy;

                // clamp to screen (safe)
                if (py < 8) { py = 8; vy = 0; }
                if (py > SCREEN_H - 8) { py = SCREEN_H - 8; vy = 0; }

                // advance pipes (world moves left)
                for (int i = 0; i < MAX_PIPES; i++) {
                    if (!pipes[i].active) continue;
                    pipes[i].x -= pipeSpeed;
                    // scoring: when pipe passes player (and not yet scored)
                    if (!pipes[i].scored && (pipes[i].x + PIPE_WIDTH*0.5f) < px) {
                        pipes[i].scored = true;
                        score++;
                    }
                    if (pipes[i].x < -PIPE_WIDTH) {
                        // recycle pipe to right side
                        float maxx = 0;
                        for (int j = 0; j < MAX_PIPES; j++) if (pipes[j].active && pipes[j].x > maxx) maxx = pipes[j].x;
                        spawnPipe(i, maxx + PIPE_SPACING);
                    }
                }

                // speed ramps up slowly
                playTime += 1.0f/60.0f;
                if (playTime > 5.0f) {
                    pipeSpeed = BASE_PIPE_SPEED + playTime * 0.02f; // gradual increase
                }
            }

            // collision detection (only when alive)
            if (sheet && alive) {
                float sw = wave.params.pos.w;
                float sh = wave.params.pos.h;
                float sx = px - sw*0.5f;
                float sy = py - sh*0.5f;
                for (int i = 0; i < MAX_PIPES; i++) {
                    if (!pipes[i].active) continue;
                    float tx = pipes[i].x - PIPE_WIDTH*0.5f;
                    float tw = PIPE_WIDTH;
                    float th = pipes[i].gapY;
                    float ty = 0;
                    float bx = tx;
                    float by = pipes[i].gapY + GAP_HEIGHT;
                    float bh = SCREEN_H - by;
                    if (aabb_collide(sx, sy, sw, sh, tx, ty, tw, th) ||
                        aabb_collide(sx, sy, sw, sh, bx, by, tw, bh)) {
                        alive = false;
                        break;
                    }
                }
            }

            // update trail (shift buffer)
            for (int i = TRAIL_LEN-1; i > 0; i--) {
                trailX[i] = trailX[i-1];
                trailY[i] = trailY[i-1];
            }
            trailX[0] = px;
            trailY[0] = py;

            // rendering
            C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
            C2D_TargetClear(top, C2D_Color32f(0.03f, 0.03f, 0.06f, 1.0f));
            C2D_SceneBegin(top);

            // draw pipes
            for (int i = 0; i < MAX_PIPES; i++) {
                if (!pipes[i].active) continue;
                float tx = pipes[i].x - PIPE_WIDTH*0.5f;
                // top rect
                C2D_DrawRectSolid((C2D_S2D_Conf){ .x=tx, .y=0, .w=PIPE_WIDTH, .h=(float)pipes[i].gapY }, C2D_Color32(60,180,120,255));
                // bottom rect
                float by = pipes[i].gapY + GAP_HEIGHT;
                C2D_DrawRectSolid((C2D_S2D_Conf){ .x=tx, .y=by, .w=PIPE_WIDTH, .h=(float)(SCREEN_H - by) }, C2D_Color32(60,180,120,255));
            }

            // draw trail (faded sprites)
            if (sheet) {
                for (int i = TRAIL_LEN-1; i >= 0; i--) {
                    float t = (float)i / (float)(TRAIL_LEN - 1);
                    float scale = 1.0f - t*0.6f;
                    float alpha = 255 * (1.0f - t) * 0.6f;
                    C2D_SpriteCopy(&wave, &wave);
                    C2D_SpriteSetPos(&wave, trailX[i], trailY[i]);
                    C2D_SpriteSetScale(&wave, scale, scale);
                    C2D_DrawSprite(&wave);
                }
            }

            // draw player sprite with smooth rotation based on vy
            if (sheet) {
                float angleDeg = (-vy) * 6.0f; // vy negative => tilt up
                if (angleDeg > 40.0f) angleDeg = 40.0f;
                if (angleDeg < -40.0f) angleDeg = -40.0f;
                C2D_SpriteSetPos(&wave, px, py);
                C2D_SpriteSetRotation(&wave, C3D_AngleFromDegrees(angleDeg));
                C2D_SpriteSetScale(&wave, 1.0f, 1.0f);
                C2D_DrawSprite(&wave);
            }

            // HUD: score
            char buf[64];
            snprintf(buf, sizeof(buf), "Score: %d", score);
            C2D_DrawText(C2D_FontLoadSystem(), 12.0f, 12.0f, 0.0f, 1.0f, 1.0f, buf);

            // if dead, show message
            if (!alive) {
                const char *msg = "You died! Press A to restart.";
                C2D_DrawText(C2D_FontLoadSystem(), SCREEN_W/2 - 80, SCREEN_H/2 - 8, 0.0f, 1.0f, 1.0f, msg);
            }

            C2D_SceneEnd();
            C3D_FrameEnd(0);
        }

        if (sheet) C2D_SpriteSheetFree(sheet);
        C2D_Fini();
        C3D_Fini();
        gfxExit();
        romfsExit();
        return 0;
    }
