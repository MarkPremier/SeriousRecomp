#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "pc_settings.h"

extern void nv2a_gpu_set_render_scale(float scale);
extern void nv2a_gpu_set_anisotropy(int level);
extern void nv2a_gpu_set_msaa(int samples);
extern void xbox_FramebufferSetFullscreen(int on);
extern int xbox_FramebufferFullscreen(void);
extern void xbox_SetPresentInterval(int interval);
extern int xbox_GetPresentInterval(void);
extern void xbox_SetFrameLimit(int hz);
extern int xbox_GetFrameLimit(void);
extern void xbox_FramebufferSetWindowAspect(float aspect);

static const char *const resolutions[] = {"Auto",        "640 x 480",   "1280 x 960",
                                          "1920 x 1440", "2560 x 1920", "2880 x 2160"};
static const float scales[] = {0, 1, 2, 3, 4, 4.5f};
/* As Turok's Video Options: "Auto" is the monitor's shape. 21:9 is the
 * marketing name of 64:27 (2560x1080); 3440x1440 monitors are 43:18. */
static const char *const aspects[] = {"4:3", "16:10", "16:9", "21:9", "Auto"};
static const float aspect_ratios[] = {4.0f / 3, 16.0f / 10, 16.0f / 9, 64.0f / 27, 0};
static const char *const displays[] = {"Windowed", "Fullscreen"};
static const char *const switches[] = {"Off", "On"};
static const char *const frames[] = {"Unlimited", "30", "60", "120", "144", "200", "240", "VBlank"};
static const int frame_hz[] = {0, 30, 60, 120, 144, 200, 240, -2};
static const char *const af[] = {"Game default", "2x", "4x", "8x", "16x"};
static const char *const aa[] = {"Off", "2x MSAA", "4x MSAA", "8x MSAA"};
#define COUNT(a) ((int)(sizeof(a) / sizeof((a)[0])))
static const struct {
    const char *key, *label;
    const char *const *values;
    int count, fallback;
} settings[] = {
        {"resolution", "Resolution", resolutions, COUNT(resolutions), 0},
        {"aspect", "Aspect ratio", aspects, COUNT(aspects), COUNT(aspects) - 1},
        {"display", "Display", displays, COUNT(displays), 0},
        {"vsync", "VSync", switches, COUNT(switches), 0},
        {"frame_limit", "Frame limit", frames, COUNT(frames), 0},
        {"anisotropy", "Anisotropic filtering", af, COUNT(af), 4},
        {"msaa", "Anti-aliasing", aa, COUNT(aa), 0},
};
static int choices[PC_VIDEO_DEFAULTS], observed_fullscreen;
static float custom_scale = -1;
static int custom_hz = -3;
static char settings_ini[MAX_PATH];

const char *pc_settings_label(int row)
{
    return row == PC_VIDEO_DEFAULTS ? "Restore defaults" : settings[row].label;
}

static int named_value(int row, const char *name)
{
    for (int i = 0; i < settings[row].count; ++i)
        if (!_stricmp(name, settings[row].values[i])) return i;
    return -1;
}

static void read_scale(const char *value)
{
    char *end;
    float scale = strtof(value, &end);
    if (!*value || *end || !isfinite(scale) || scale < 0 || scale > 8) return;
    for (int i = 0; i < COUNT(scales); ++i)
        if (fabsf(scales[i] - scale) < 0.0001f) {
            choices[PC_RESOLUTION] = i;
            custom_scale = -1;
            return;
        }
    custom_scale = scale;
}

static void read_hz(const char *value)
{
    char *end;
    long hz = strtol(value, &end, 10);
    if (!*value || *end || hz < 0 || hz > 1000) return;
    for (int i = 0; i < COUNT(frame_hz); ++i)
        if (frame_hz[i] == hz) {
            choices[PC_FRAMES] = i;
            custom_hz = -3;
            return;
        }
    custom_hz = (int)hz;
}

void pc_settings_value(int row, char *out, size_t size)
{
    if (row == PC_VIDEO_DEFAULTS)
        snprintf(out, size, "Enter / A");
    else if (row == PC_RESOLUTION && custom_scale >= 0)
        snprintf(out, size, "%gx scale", custom_scale);
    else if (row == PC_FRAMES && custom_hz >= 0)
        snprintf(out, size, "%d", custom_hz);
    else
        snprintf(out, size, "%s", settings[row].values[choices[row]]);
}

float pc_settings_aspect(void)
{
    float ratio = aspect_ratios[choices[PC_ASPECT]];
    if (!ratio) {
        /* The monitor the game window is on (the primary one before it opens). */
        DEVMODEA mode = {.dmSize = sizeof mode};
        if (EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &mode) && mode.dmPelsHeight)
            ratio = (float)mode.dmPelsWidth / (float)mode.dmPelsHeight;
    }
    /* Never narrower than the game's own 4:3, nor wider than the runtime's
     * widest targets (four times 4:3). */
    if (!(ratio >= 4.0f / 3)) ratio = 4.0f / 3;
    if (ratio > 16.0f / 3) ratio = 16.0f / 3;
    return ratio;
}

static void apply_setting(int row)
{
    switch (row) {
    case PC_RESOLUTION:
        nv2a_gpu_set_render_scale(custom_scale >= 0 ? custom_scale : scales[choices[row]]);
        break;
    /* Applied per frame by pc_controls_frame, which knows the front end
     * (kept 4:3) from a level; here only the window's shape follows. */
    case PC_ASPECT: xbox_FramebufferSetWindowAspect(pc_settings_aspect()); break;
    case PC_DISPLAY: xbox_FramebufferSetFullscreen(choices[row]); break;
    case PC_VSYNC: xbox_SetPresentInterval(choices[row]); break;
    case PC_FRAMES: xbox_SetFrameLimit(custom_hz >= 0 ? custom_hz : frame_hz[choices[row]]); break;
    case PC_AF: nv2a_gpu_set_anisotropy(1 << choices[row]); break;
    case PC_AA: nv2a_gpu_set_msaa(1 << choices[row]); break;
    }
}

static void save_settings(void)
{
    char value[64];
    for (int i = 0; i < PC_VIDEO_DEFAULTS; ++i) {
        pc_settings_value(i, value, sizeof value);
        if (i == PC_RESOLUTION && custom_scale >= 0) snprintf(value, sizeof value, "%g", custom_scale);
        WritePrivateProfileStringA("pc_settings", settings[i].key, value, settings_ini);
    }
}

void pc_settings_load(const char *controls_ini)
{
    char text[128];
    const char *override = getenv("RECOMP_PC_SETTINGS_FILE"), *env;
    snprintf(settings_ini, sizeof settings_ini, "%s", override && *override ? override : controls_ini);
    custom_scale = -1;
    custom_hz = -3;
    for (int i = 0; i < PC_VIDEO_DEFAULTS; ++i) {
        GetPrivateProfileStringA("pc_settings", settings[i].key, "", text, sizeof text, settings_ini);
        int value = named_value(i, text);
        choices[i] = value >= 0 ? value : settings[i].fallback;
        if (value < 0 && i == PC_RESOLUTION) read_scale(text);
        if (value < 0 && i == PC_FRAMES) read_hz(text);
    }
    /* Explicit diagnostic overrides win on launch and are reflected on the
     * page. A later choice on the page takes effect live. */
    env = getenv("RECOMP_RENDER_SCALE");
    if (env && *env) {
        if (!_stricmp(env, "auto")) {
            choices[PC_RESOLUTION] = 0;
            custom_scale = -1;
        } else
            read_scale(env);
    }
    env = getenv("RECOMP_VSYNC");
    if (env && *env) choices[PC_VSYNC] = atoi(env) > 0;
    env = getenv("RECOMP_FPS_CAP");
    if (env && *env) {
        if (!_stricmp(env, "vblank")) {
            choices[PC_FRAMES] = COUNT(frame_hz) - 1;
            custom_hz = -3;
        } else
            read_hz(env);
    }
    observed_fullscreen = xbox_FramebufferFullscreen();
    for (int i = 0; i < PC_VIDEO_DEFAULTS; ++i) apply_setting(i);
    fprintf(stderr, "[PC SETTINGS] %s:", settings_ini);
    for (int i = 0; i < PC_VIDEO_DEFAULTS; ++i) {
        pc_settings_value(i, text, sizeof text);
        fprintf(stderr, " %s=%s", settings[i].key, text);
    }
    fprintf(stderr, "\n");
}

void pc_settings_step(int row, int direction)
{
    if (row == PC_VIDEO_DEFAULTS) {
        custom_scale = -1;
        custom_hz = -3;
        for (int i = 0; i < PC_VIDEO_DEFAULTS; ++i) {
            choices[i] = settings[i].fallback;
            apply_setting(i);
        }
    } else {
        if (row == PC_RESOLUTION) custom_scale = -1;
        if (row == PC_FRAMES) custom_hz = -3;
        choices[row] = (choices[row] + settings[row].count + (direction < 0 ? -1 : 1)) % settings[row].count;
        apply_setting(row);
    }
    save_settings();
}

void pc_settings_poll(void)
{
    int actual = xbox_FramebufferFullscreen();
    if (actual != observed_fullscreen) {
        observed_fullscreen = actual;
        if (choices[PC_DISPLAY] != actual) {
            choices[PC_DISPLAY] = actual;
            save_settings();
        }
    }
}
