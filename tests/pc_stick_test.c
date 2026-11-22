/* The optional stick deadzone, through a model of Sam's own two per-axis
 * stages: the engine's min 8000 / max 0x7FFF (0x1DD1D0, 0x1DC9B0) and the
 * controls' DeadZone 0.2 (Controls/Xbox/0?Axes.ctl, applied at 0x45ABB). */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "../src/pc_stick.h"
#define CHECK(x) do {if (!(x)) {fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);}} while(0)
static void *installed;
void xbox_InputSetStickFilter(void *filter) { installed = filter; }
/* What one direction of an axis does in the game: 0..1. */
static float game(int raw)
{
    float v = (raw > 8000 ? raw - 8000 : 0) / 24767.0f;
    v = (v - 0.2f) / 0.8f;
    return v < 0 ? 0 : v > 1 ? 1 : v;
}
static float axis(short raw) { return raw >= 0 ? game(raw) : -game(-raw); }
static void map(int x, int y, float dz, float *gx, float *gy)
{
    short sx = (short)x, sy = (short)y;
    pc_stick_map(&sx, &sy, dz);
    *gx = axis(sx); *gy = axis(sy);
}
int main(void)
{
    float gx, gy;
    /* The original: nothing until about 40% of travel, per axis. */
    CHECK(game(12900) == 0 && game(13100) > 0 && game(32767) == 1);
    /* Inside the round deadzone: still. */
    map(2500, 1500, 0.10f, &gx, &gy); CHECK(gx == 0 && gy == 0);
    /* Just past it the game moves, with no 40% jump. */
    map(3500, 0, 0.10f, &gx, &gy); CHECK(gx > 0 && gx < 0.01f && gy == 0);
    /* Linear in between: 55% travel with a 10% deadzone is 50% output. */
    map(-18022, 0, 0.10f, &gx, &gy); CHECK(fabsf(gx + 0.5f) < 0.002f && gy == 0);
    map(0, 18022, 0.10f, &gx, &gy); CHECK(fabsf(gy - 0.5f) < 0.002f && gx == 0);
    /* Small diagonals keep both axes (the original zeroes them). */
    CHECK(axis(6000) == 0);
    map(6000, 6000, 0.10f, &gx, &gy); CHECK(gx > 0 && gy > 0 && fabsf(gx - gy) < 1e-4f);
    /* Directions survive: 30 degrees off the X axis stays 30 degrees. */
    map((int)(20000 * cosf(0.5236f)), (int)(20000 * sinf(0.5236f)), 0.05f, &gx, &gy);
    CHECK(fabsf(atan2f(gy, gx) - 0.5236f) < 0.01f);
    /* A full diagonal still gives full speed on both axes, as the original. */
    map(23170, 23170, 0.10f, &gx, &gy); CHECK(gx > 0.999f && gy > 0.999f);
    map(-32768, -32768, 0.20f, &gx, &gy); CHECK(gx < -0.999f && gy < -0.999f);
    map(32767, 0, 0.05f, &gx, &gy); CHECK(gx == 1 && gy == 0);
    /* Monotonic along an axis, for every choice. */
    for (int c = 1; c < PC_STICK_CHOICES; ++c) {
        float last = 0, dz = c * 0.05f;
        for (int x = 0; x <= 32767; x += 64) {
            map(x, 0, dz, &gx, &gy);
            CHECK(gx >= last && gy == 0);
            last = gx;
        }
        map(32767, 0, dz, &gx, &gy); CHECK(gx == 1 && gx >= last);
    }
    /* Labels, parsing and choice bounds. */
    char label[16];
    pc_stick_label(0, label, sizeof label); CHECK(pc_stick_parse(label) == 0);
    for (int c = 1; c < PC_STICK_CHOICES; ++c) { pc_stick_label(c, label, sizeof label); CHECK(pc_stick_parse(label) == c); }
    CHECK(pc_stick_parse("bogus") == 0 && pc_stick_parse(NULL) == 0);
    pc_stick_set_choice(9); CHECK(pc_stick_choice() == 0);
    pc_stick_install(); CHECK(installed);
    puts("PASS: stick deadzone matches the game's own axis stages");
    return 0;
}
