/* Optional visibility trial, based on the USA XBE's skeletal-shadow call.
 * Keep the lighting mask, projection, fog and GPU depth/blend paths intact. */
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <xbox/xboxrecomp.h>
#include "pc_shadows.h"

extern ptrdiff_t g_xbox_mem_offset;
extern RECOMP_TLS uint32_t g_ecx, g_esi, g_esp;
#define WORD(a) (*(uint32_t *)((uintptr_t)g_xbox_mem_offset + (uint32_t)(a)))
#define REAL(a) (*(float *)((uintptr_t)g_xbox_mem_offset + (uint32_t)(a)))
#define BIOMECH_VTABLE 0x003AD9D8u
#define SKELETAL_SHADOW_CALLER 0x00223235u
static float minimum;

void pc_shadows_load(void)
{
    const char *value = getenv("RECOMP_WALKER_SHADOW_MIN");
    minimum = 0;
    if (value && *value) {
        char *end;
        float parsed = strtof(value, &end);
        if (!*end && isfinite(parsed) && parsed >= 0.1f && parsed <= 1.0f)
            minimum = parsed;
    }
    if (minimum > 0.1f)
        fprintf(stderr, "[PC-SHADOWS] experimental walker minimum %.3g enabled\n", minimum);
}

void pc_shadows_simple(void (*original)(void))
{
    uint32_t argument = g_esp + 4, saved = 0;
    int changed = 0;
    /* This caller keeps its entity in ESI and passes that entity's skeletal
     * instance as ECX. The vtable is installed by the native Biomech ctor
     * 97760; its asset setup names WalkerSmall/WalkerBig. Other models and
     * other callers retain their exact original input. */
    if (minimum > 0.1f && WORD(g_esp) == SKELETAL_SHADOW_CALLER &&
        g_esi >= 0x10000u && g_esi < 0x08000000u - 0x80u &&
        WORD(g_esi) == BIOMECH_VTABLE && WORD(g_esi + 0x7C) == g_ecx &&
        REAL(argument) >= 0.1f && REAL(argument) < minimum) {
        saved = WORD(argument);
        REAL(argument) = minimum;
        changed = 1;
    }
    original();
    if (changed) WORD(argument) = saved;
}
