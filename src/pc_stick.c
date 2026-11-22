/* Optional modern stick deadzone (issue #5).
 *
 * The original game swallows about 40% of each stick axis, in two stages that
 * both run per axis (a square deadzone):
 *   1. The engine's pad code (setup 0x1DD1D0, poll 0x1DC9B0) gives every half
 *      axis min 8000 and max 0x7FFF: v1 = max(0, |raw| - 8000) / 24767.
 *   2. Every move/look control in Controls/Xbox/0?Axes.ctl (Main.gro) has
 *      "DeadZone: 0.2", applied at 0x45ABB as v2 = clamp((v1 - 0.2) / 0.8).
 * Nothing moves until |raw| > 8000 + 0.2 * 24767 = 12953.4.
 *
 * Modern choices apply a round deadzone to the host stick, stretch the
 * remaining circle to the game's square (a full diagonal still gives 1 on both
 * axes, as on the original), then emit the raw value whose v2 is exactly the
 * wanted deflection. Original leaves the stick untouched. */
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "pc_stick.h"

typedef void (*stick_filter)(unsigned long, short *, short *, short *, short *);
extern void xbox_InputSetStickFilter(stick_filter filter);

#define ENGINE_MIN 8000.0f
#define ENGINE_MAX 32767.0f
#define CONTROL_DEADZONE 0.2f
#define GUEST_FLOOR (ENGINE_MIN + CONTROL_DEADZONE * (ENGINE_MAX - ENGINE_MIN))

static const float deadzones[PC_STICK_CHOICES] = {0, 0.05f, 0.10f, 0.15f, 0.20f};
static volatile int choice;

static short to_guest(float o)
{
    float m = fabsf(o);
    long v;
    if (m == 0) return 0;
    if (m > 1) m = 1;
    v = lroundf(GUEST_FLOOR + m * (ENGINE_MAX - GUEST_FLOOR));
    if (v < 12954) v = 12954; /* the first step past the floor, never inside it */
    if (v > 32767) v = 32767;
    return (short)(o < 0 ? -v : v);
}

void pc_stick_map(short *x, short *y, float deadzone)
{
    float nx = *x / 32767.0f, ny = *y / 32767.0f, mag, r, k;
    if (nx < -1) nx = -1;
    if (ny < -1) ny = -1;
    mag = sqrtf(nx * nx + ny * ny);
    if (mag <= deadzone) {
        *x = *y = 0;
        return;
    }
    r = (mag - deadzone) / (1 - deadzone);
    if (r > 1) r = 1;
    nx /= mag;
    ny /= mag;
    k = r / (fabsf(nx) > fabsf(ny) ? fabsf(nx) : fabsf(ny));
    *x = to_guest(nx * k);
    *y = to_guest(ny * k);
}

static void filter(unsigned long port, short *lx, short *ly, short *rx, short *ry)
{
    int c = choice;
    (void)port;
    if (c <= 0 || c >= PC_STICK_CHOICES) return;
    pc_stick_map(lx, ly, deadzones[c]);
    pc_stick_map(rx, ry, deadzones[c]);
}

void pc_stick_install(void) { xbox_InputSetStickFilter(filter); }
int pc_stick_choice(void) { return choice; }
void pc_stick_set_choice(int c) { choice = c >= 0 && c < PC_STICK_CHOICES ? c : 0; }

void pc_stick_label(int c, char *out, size_t size)
{
    if (c <= 0 || c >= PC_STICK_CHOICES)
        snprintf(out, size, "Original");
    else
        snprintf(out, size, "%d", (int)lroundf(deadzones[c] * 100));
}

int pc_stick_parse(const char *text)
{
    char label[16];
    for (int c = 1; c < PC_STICK_CHOICES; ++c) {
        pc_stick_label(c, label, sizeof label);
        if (text && !strcmp(text, label)) return c;
    }
    return 0;
}
