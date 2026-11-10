#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "pc_controls.h"
#include "pc_settings.h"
#include <xbox/xboxrecomp.h>

extern ptrdiff_t g_xbox_mem_offset;
extern RECOMP_TLS uint32_t g_eax, g_ecx, g_edx, g_ebx, g_esi, g_edi;
extern RECOMP_TLS uint32_t g_esp, g_ebp, g_seh_ebp;
extern uint32_t xbox_HeapAlloc(uint32_t size, uint32_t alignment);
extern void sub_0002F12B(void), sub_0002727C(void), sub_0002A807(void), sub_0002B52A(void);
extern void sub_0002B588(void);
extern void sub_0001AFBA(void);
extern void xbox_KbmSetMenuMode(int on), xbox_KbmSetRawMouse(int on), xbox_KbmSetCapture(int on);
extern void xbox_KbmSetBinding(const char *target, const int *keys, int count);
extern void xbox_KbmPulse(const char *target, int ms);
extern int xbox_KbmInputDown(int key), xbox_KbmMenuWheel(void), xbox_KbmWheelNotches(void);
extern int xbox_VideoActive(void), nv2a_gpu_frames_without_3d(void);
extern void nv2a_gpu_set_wide_factor(float factor), nv2a_gpu_set_wide_title_fov(int on);
extern int xbox_FramebufferKeyDown(int key), xbox_FramebufferMouseButton(int button);
extern int xbox_FramebufferMouseClicks(int button), xbox_FramebufferMouseDelta(int *dx, int *dy);
extern int xbox_FramebufferPointer(int *x, int *y, int *w, int *h);
extern void xbox_FramebufferSetMenuPointer(int on);

#define WORD(a)     (*(uint32_t *)((uintptr_t)g_xbox_mem_offset + (uint32_t)(a)))
#define FLOAT(a)    (*(float *)((uintptr_t)g_xbox_mem_offset + (uint32_t)(a)))
#define PTR(a)      ((void *)((uintptr_t)g_xbox_mem_offset + (uint32_t)(a)))
#define OPTIONS     0x00472568u
#define ACTIVE_MENU 0x0047011Cu
/* _pGame, and its NETRICSA state (the PC source's gm_csComputerState:
 * 0 off, 1 on, 2 turning on, 3 turning off), as NETRICSA's own code reads it. */
#define GAME                0x0047F6C4u
#define GAME_COMPUTER_STATE 0x0Cu
#define COMPUTER_ON         1
/* Set while the post-load briefing ("Please press START to continue") is up;
 * NETRICSA's update (sub_0005F1E0) reads its pad for this or COMPUTER_ON. */
#define BRIEFING            0x0047FE30u
/* CPlayer::RenderCameraView (0x7CAB0) tests m_penCamera at +0x3F0.
 * Action building continues while that scripted camera owns the view. */
#define NETWORK             0x004A2AE0u
#define PLAYER_VTABLE       0x003ABCD0u
#define PLAYER_CAMERA       0x3F0u
#define MOUSE1      0x101
#define MOUSE2      0x102
#define MOUSE3      0x103
#define WHEEL_UP    0x104
#define WHEEL_DOWN  0x105
#define MOUSE4      0x106
#define MOUSE5      0x107
/* Dispatch tokens in unused alignment padding after Options::Initialize.
 * These pass the runtime's code-range guard; no original code bytes change. */
#define OPEN_PAGE     0x00030CF7u
#define PAGE_KEY      0x00030CF8u
#define OPEN_SETTINGS 0x00030CF9u
#define SETTINGS_KEY  0x00030CFAu
#define OPEN_QUIT     0x00030CFBu
#define QUIT_KEY      0x00030CFCu
#define QUIT_YES      0x00030CFDu
#define QUIT_NO       0x00030CFEu
#define MAIN_MENU     0x0046F2A0u
#define PAUSE_MENU    0x0046E5B0u
#define VISIBLE_ROWS  9
#define TEXT_SIZE     128u

/* Sam's menu objects, laid out as the USA XBE's own code uses them (the
 * launcher refuses any other default.xbe). New buttons are clones of an
 * existing one and new pages copy an existing page's vtable, so these are the
 * only fields this file depends on; layout_ok() checks the templates before
 * anything is built. */
#define TEMPLATE_BUTTON 0x0046F3C8u /* the main menu's first text button */
#define MAIN_MENU_LAST  0x0046F6E8u /* the main menu's last original entry */
#define SCREEN_VTABLE   0x003AA210u /* a menu page's vtable, in .rdata */
#define EMPTY_STRING    0x004493F8u /* the title's shared empty CTString */
#define RDATA_START     0x003A9840u
#define RDATA_END       0x003F8A6Cu
#define TEXT_END        0x00303D30u

/* Text button. */
#define BUTTON_SIZE         0xC8u
#define GADGET_X            0x14u /* current position and scale ... */
#define GADGET_Y            0x18u
#define GADGET_SCALE        0x20u
#define GADGET_TARGET_X     0x28u /* ... and where the entrance animation ends */
#define GADGET_TARGET_Y     0x2Cu
#define GADGET_TARGET_SCALE 0x34u
#define GADGET_TEXT         0x4Cu /* char * */
#define GADGET_CENTRED      0x6Cu /* 0 centred on GADGET_X, -1 starting there */
#define GADGET_ENABLED      0x74u
#define GADGET_FOCUSED      0x7Cu
#define GADGET_VISIBLE      0x80u
#define GADGET_TRANSITION   0x8Cu /* entrance animation type (sub_0001AFBA) */
#define GADGET_LEFT         0x94u /* neighbours for d-pad navigation */
#define GADGET_RIGHT        0x98u
#define GADGET_UP           0x9Cu
#define GADGET_DOWN         0xA0u
#define GADGET_TRIGGER      0xBCu /* code address called on A / Enter */

/* Menu page: the object, its private copy of the vtable, then its title. */
#define SCREEN_FOCUS       0x14u /* focused button */
#define SCREEN_TITLE       0x20u /* char * */
#define SCREEN_VTABLE_COPY 0x40u
#define VTABLE_SIZE        0x2Cu
#define VTABLE_KEY         0x18u /* key handler slot */
#define SCREEN_TITLE_TEXT  (SCREEN_VTABLE_COPY + VTABLE_SIZE)
#define SCREEN_SIZE        SCREEN_TITLE_TEXT

enum {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    JUMP,
    CROUCH,
    FIRE,
    USE,
    COMPUTER,
    NEXT,
    PREVIOUS,
    RELOAD,
    FAVORITE1,
    FAVORITE2,
    THIRD_PERSON,
    PAUSE,
    BIND_COUNT
};
static const struct {
    const char *name, *label;
    int key;
    uint32_t address;
} actions[] = {
        {"forward", "Move forward", 'W', 0x480468},
        {"backward", "Move backward", 'S', 0x48046C},
        {"strafe_left", "Strafe left", 'A', 0x480470},
        {"strafe_right", "Strafe right", 'D', 0x480474},
        {"jump", "Jump / swim up", VK_SPACE, 0x480478},
        {"crouch", "Crouch / swim down", VK_CONTROL, 0x48047C},
        {"fire", "Fire", MOUSE1, 0x480530},
        {"use", "Use / sniper zoom", MOUSE2, 0x480538},
        {"netricsa", "NETRICSA", VK_TAB, 0x48053C},
        {"next_weapon", "Next weapon", WHEEL_UP, 0x480514},
        {"previous_weapon", "Previous weapon", WHEEL_DOWN, 0x480518},
        {"reload", "Reload", 'R', 0x480534},
        {"favorite_1", "Favourite weapon 1", '1', 0x480520},
        {"favorite_2", "Favourite weapon 2", '2', 0x480524},
        {"third_person", "Third-person view", 'H', 0x480548},
        {"pause", "Pause", VK_ESCAPE, 0},
};
enum { SPEED_ROW = BIND_COUNT, INVERT_ROW, MOVEMENT_ROW, DEFAULTS_ROW, ROW_COUNT };
/* The page's BACK button is the stop after the last row, so up/down (keys or a
 * pad) reach it as they reach the rows; Enter/A there goes back. */
#define BACK_SELECTED ROW_COUNT
static int keys[BIND_COUNT], speed = 5, invert_y, enabled;
static volatile LONG responsive_movement;
static volatile LONG keyboard_move_player, keyboard_move_until;
/* The thread that runs the title's main loop. The boot movie's worker presents
 * through the same calls as the main loop does, and the menu work here calls
 * Sam's menu code, which only the main thread may run. */
static DWORD main_thread;
static DWORD WINAPI movie_thread(LPVOID unused);

/* Which menu is on screen. ACTIVE_MENU alone is not it: B on the pause menu
 * resumes play but leaves it naming the page that started the level (the
 * Difficulty page), which is neither drawn nor meant to take keys. A menu is
 * shown while Sam's menu renderer (sub_0002B6BE, pc_controls_menu_drawn) keeps
 * drawing it. Until that has run once, ACTIVE_MENU is taken as it is. */
static volatile LONG menu_draw_ms, menu_draw_seen;

void pc_controls_menu_drawn(void)
{
    InterlockedExchange(&menu_draw_ms, (LONG)GetTickCount());
    InterlockedExchange(&menu_draw_seen, 1);
}

/* Whether a level is being played: Sam's action builder (sub_00070810,
 * pc_controls_input_begin) runs every game tick of play, and not while
 * NETRICSA (the briefing, the end-of-level statistics), a movie or a loading
 * screen is up. Until it has run once, nothing counts as play. */
static volatile LONG action_ms, action_seen;

static int playing(void)
{
    return action_seen && GetTickCount() - (DWORD)action_ms < 250;
}

static uint32_t shown_menu(void)
{
    uint32_t menu = WORD(ACTIVE_MENU);
    if (menu && menu_draw_seen && GetTickCount() - (DWORD)menu_draw_ms > 250) return 0;
    return menu;
}
static char ini[MAX_PATH];
static uint32_t page, entry, rows[ROW_COUNT], values[ROW_COUNT], title, help, status, back_button;
static uint32_t settings_page, settings_entry, setting_rows[PC_SETTINGS_ROWS],
        setting_values[PC_SETTINGS_ROWS];
static int selected_setting;
static uint32_t settings_back;
/* As on the controls page, BACK is the stop after the last setting. */
#define SETTINGS_BACK PC_SETTINGS_ROWS
static uint32_t quit_entry, quit_page, quit_choices[2];
static int quit_selected;
static void build_quit(void), quit_frame(void), open_quit(void), quit_key(void);
static void quit_yes(void), quit_no(void);
static void quit_desktop(void);
static void update_settings_page(void), open_settings_page(void), settings_key(void), settings_frame(void);
static int selected, scroll, capturing = -1, menu_mode = -1;
static unsigned char previous[0x108], blocked[BIND_COUNT];
static DWORD capture_deadline;
static int pointer_x = -1, pointer_y = -1, delete_down;
static unsigned char menu_held[6];
static DWORD menu_repeat[4];
static int pause_held;
static const int menu_keys[6][3] = {
        {VK_UP, 'W', VK_UP},       {VK_DOWN, 'S', VK_DOWN},          {VK_LEFT, 'A', VK_LEFT},
        {VK_RIGHT, 'D', VK_RIGHT}, {VK_RETURN, VK_SPACE, VK_RETURN}, {VK_ESCAPE, VK_BACK, VK_ESCAPE}};
static RECOMP_TLS uint32_t saved_buttons[BIND_COUNT - FIRE];
static RECOMP_TLS int injected;

/* Translated callees share these TLS registers. Helpers restore every one,
 * including the frameless exception-chain frame, before returning to the title. */
static uint32_t call(void (*fn)(void), uint32_t self, const uint32_t *args, unsigned count)
{
    uint32_t save[] = {g_eax, g_ecx, g_edx, g_ebx, g_esi, g_edi, g_esp, g_ebp, g_seh_ebp};
    uint32_t result;
    g_esp -= 4 * (count + 1);
    WORD(g_esp) = 0;
    for (unsigned i = 0; i < count; ++i) WORD(g_esp + 4 + i * 4) = args[i];
    g_ecx = self;
    fn();
    result = g_eax;
    g_eax = save[0];
    g_ecx = save[1];
    g_edx = save[2];
    g_ebx = save[3];
    g_esi = save[4];
    g_edi = save[5];
    g_esp = save[6];
    g_ebp = save[7];
    g_seh_ebp = save[8];
    return result;
}

static void key_name(int key, char *out, size_t size)
{
    const char *name = NULL;
    switch (key) {
    case 0: name = "None"; break;
    case VK_SPACE: name = "Space"; break;
    case VK_RETURN: name = "Enter"; break;
    case VK_ESCAPE: name = "Escape"; break;
    case VK_BACK: name = "Backspace"; break;
    case VK_CONTROL: name = "Ctrl"; break;
    case VK_SHIFT: name = "Shift"; break;
    case VK_MENU: name = "Alt"; break;
    case VK_TAB: name = "Tab"; break;
    case MOUSE1: name = "Mouse 1"; break;
    case MOUSE2: name = "Mouse 2"; break;
    case MOUSE3: name = "Mouse 3"; break;
    case MOUSE4: name = "Mouse 4"; break;
    case MOUSE5: name = "Mouse 5"; break;
    case WHEEL_UP: name = "Wheel up"; break;
    case WHEEL_DOWN: name = "Wheel down"; break;
    }
    if (name)
        snprintf(out, size, "%s", name);
    else if ((key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9'))
        snprintf(out, size, "%c", key);
    else {
        UINT scan = MapVirtualKeyA(key, MAPVK_VK_TO_VSC);
        if (key >= VK_PRIOR && key <= VK_DOWN) scan |= 0x100;
        if (!GetKeyNameTextA((LONG)(scan << 16), out, (int)size)) snprintf(out, size, "Key %d", key);
    }
}

static int named_key(const char *name)
{
    char label[64];
    for (int k = 0; k <= MOUSE5; ++k) {
        if (k == 0x100) continue;
        if (k == VK_F11 || k == VK_F12) continue;
        key_name(k, label, sizeof label);
        if (!_stricmp(name, label)) return k;
    }
    return -1;
}

static void save(void)
{
    char label[64];
    for (int i = 0; i < BIND_COUNT; ++i) {
        key_name(keys[i], label, sizeof label);
        WritePrivateProfileStringA("pc_controls", actions[i].name, label, ini);
    }
    snprintf(label, sizeof label, "%d", speed);
    WritePrivateProfileStringA("pc_controls", "mouse_speed", label, ini);
    WritePrivateProfileStringA("pc_controls", "invert_y", invert_y ? "1" : "0", ini);
    WritePrivateProfileStringA("pc_controls", "keyboard_movement", responsive_movement ? "Responsive" : "Original", ini);
}

static void apply(void)
{
    /* Actions enter Sam's own action builder, independently of the console
     * player's selected stick/button layout. Pause pulses the normal Start
     * input when its configured PC key is pressed. */
    xbox_KbmSetBinding("START", NULL, 0);
    xbox_KbmSetRawMouse(1);
}

void pc_controls_load(void)
{
    char text[128];
    const char *flag = getenv("RECOMP_KBM"), *path = getenv("RECOMP_PC_CONTROLS_FILE");
    enabled = !flag || (*flag && *flag != '0');
    if (!flag) _putenv_s("RECOMP_KBM", "1");
    if (!getenv("RECOMP_KBM_LOOK_DIRECT")) _putenv_s("RECOMP_KBM_LOOK_DIRECT", "0");
    if (!getenv("RECOMP_KBM_WHEEL_MOVE")) _putenv_s("RECOMP_KBM_WHEEL_MOVE", "0");
    /* The runtime's own key maps start empty: this file binds every action
     * (apply), so menu keys keep working after any gameplay rebinding. No
     * game-specific camera addresses or controller defaults from Turok are
     * used. Either map can still be set from the environment for diagnosis. */
    if (!getenv("RECOMP_KBM_MAP"))
        _putenv_s("RECOMP_KBM_MAP", "A=None,B=None,X=None,Y=None,BLACK=None,WHITE=None,LT=None,RT=None,"
                                    "START=None,BACK=None,LTHUMB=None,RTHUMB=None,UP=None,DOWN=None,"
                                    "LEFT=None,RIGHT=None,MOVE_UP=None,MOVE_DOWN=None,MOVE_LEFT=None,"
                                    "MOVE_RIGHT=None");
    if (!getenv("RECOMP_KBM_MENU_MAP"))
        _putenv_s("RECOMP_KBM_MENU_MAP", "A=None,B=None,START=None,UP=None,DOWN=None,LEFT=None,RIGHT=None,"
                                         "MOVE_UP=None,MOVE_DOWN=None,MOVE_LEFT=None,MOVE_RIGHT=None");
    main_thread = GetCurrentThreadId();
    /* Sam widens its own view for wide targets (sub_001D1C00 in recomp_manual.c). */
    nv2a_gpu_set_wide_title_fov(1);
    if (enabled && !getenv("RECOMP_PC_CONTROLS_NO_THREAD")) {
        HANDLE thread = CreateThread(NULL, 0, movie_thread, NULL, 0, NULL);
        if (thread) CloseHandle(thread);
    }
    if (path && *path)
        snprintf(ini, sizeof ini, "%s", path);
    else {
        DWORD n = GetModuleFileNameA(NULL, ini, sizeof ini);
        while (n && ini[n - 1] != '\\' && ini[n - 1] != '/') --n;
        snprintf(ini + n, sizeof ini - n, "serious_sam_settings.ini");
    }
    for (int i = 0; i < BIND_COUNT; ++i) {
        GetPrivateProfileStringA("pc_controls", actions[i].name, "", text, sizeof text, ini);
        int key = named_key(text);
        keys[i] = key >= 0 ? key : actions[i].key;
    }
    speed = (int)GetPrivateProfileIntA("pc_controls", "mouse_speed", 5, ini);
    if (speed < 1 || speed > 10) speed = 5;
    invert_y = GetPrivateProfileIntA("pc_controls", "invert_y", 0, ini) == 1;
    GetPrivateProfileStringA("pc_controls", "keyboard_movement", "Original", text, sizeof text, ini);
    InterlockedExchange(&responsive_movement, !_stricmp(text, "Responsive"));
    InterlockedExchange(&keyboard_move_player, 0);
    pc_settings_load(ini);
    fprintf(stderr, "[PC CONTROLS] keyboard/mouse %s; settings %s\n", enabled ? "enabled" : "disabled", ini);
}

static uint32_t alloc(uint32_t bytes)
{
    uint32_t va = xbox_HeapAlloc(bytes, 16);
    if (va) memset(PTR(va), 0, bytes);
    return va;
}

static void text(uint32_t gadget, const char *label)
{
    if (gadget) snprintf((char *)PTR(WORD(gadget + GADGET_TEXT)), TEXT_SIZE, "%s", label);
}

static void position(uint32_t gadget, float x, float y, float scale)
{
    FLOAT(gadget + GADGET_X) = FLOAT(gadget + GADGET_TARGET_X) = x;
    FLOAT(gadget + GADGET_Y) = FLOAT(gadget + GADGET_TARGET_Y) = y;
    FLOAT(gadget + GADGET_SCALE) = FLOAT(gadget + GADGET_TARGET_SCALE) = scale;
}

/* As position, but leaves a button alone when it is already there: rewriting
 * both endpoints every frame would cancel a slide-in in progress. */
static void place(uint32_t gadget, float x, float y, float scale)
{
    if (FLOAT(gadget + GADGET_X) != x || FLOAT(gadget + GADGET_Y) != y || FLOAT(gadget + GADGET_SCALE) != scale)
        position(gadget, x, y, scale);
}

/* The entrance animation Sam's Options rows have: row by row from the left and
 * the right in turn (transition types 2 and 3), over the menu change's own
 * clock (sub_00029396, restarted by every ChangeToMenu). A row's label and
 * value come in together. */
static void slide_in(uint32_t gadget, int row)
{
    uint32_t transition = 2 + (row & 1);
    if (gadget) call(sub_0001AFBA, gadget, &transition, 1);
}

static void options_position(uint32_t gadget, float y, float scale, int row)
{
    position(gadget, 140, y, scale);
    /* Position sets both endpoints. Reapply Sam's own transition afterwards:
     * types 2/3 offset the animated endpoint by -/+640 pixels. The original
     * Options start routine supplies the animation clock and easing. */
    uint32_t transition = 2 + (row & 1);
    call(sub_0001AFBA, gadget, &transition, 1);
}

static uint32_t button(uint32_t screen, const char *label, float x, float y, float scale)
{
    uint32_t va = alloc(BUTTON_SIZE + TEXT_SIZE);
    if (!va) return 0;
    /* Sam's ordinary text button, preserving its font and focus renderer. The
     * clone owns its text and is retained for the lifetime of this process. */
    memcpy(PTR(va), PTR(TEMPLATE_BUTTON), BUTTON_SIZE);
    WORD(va + 8) = WORD(va + 12) = 0;
    WORD(va + GADGET_TEXT) = va + BUTTON_SIZE;
    WORD(va + 0x50) = WORD(va + 0x90) = EMPTY_STRING; /* title's empty CTString */
    WORD(va + 0x58) = WORD(va + 0x5C) = WORD(va + 0x60) = WORD(va + 0x64) = 0;
    WORD(va + GADGET_FOCUSED) = 0;
    WORD(va + GADGET_VISIBLE) = WORD(va + 0x74) = 1;
    WORD(va + 0x94) = WORD(va + 0x98) = WORD(va + GADGET_UP) = WORD(va + GADGET_DOWN) = 0;
    WORD(va + 0xA4) = WORD(va + 0xA8) = WORD(va + GADGET_TRIGGER) = WORD(va + 0xC0) = 0;
    WORD(va + GADGET_CENTRED) = 0xFFFFFFFFu;
    WORD(va + GADGET_TRANSITION) = 2;
    position(va, x, y, scale);
    text(va, label);
    call(sub_0002727C, screen, &va, 1);
    return va;
}

static void update_page(void)
{
    char label[128];
    if (selected < ROW_COUNT) {
        if (selected < scroll) scroll = selected;
        if (selected >= scroll + VISIBLE_ROWS) scroll = selected - VISIBLE_ROWS + 1;
    }
    for (int i = 0; i < ROW_COUNT; ++i) {
        int visible = i >= scroll && i < scroll + VISIBLE_ROWS;
        WORD(rows[i] + GADGET_VISIBLE) = WORD(values[i] + GADGET_VISIBLE) = visible;
        WORD(rows[i] + GADGET_FOCUSED) = WORD(values[i] + GADGET_FOCUSED) = i == selected;
        place(rows[i], 90, 145 + (i - scroll) * 26.0f, 0.57f);
        place(values[i], 410, 145 + (i - scroll) * 26.0f, 0.57f);
        if (i < BIND_COUNT) {
            if (i == capturing)
                snprintf(label, sizeof label, "Press a key...");
            else
                key_name(keys[i], label, sizeof label);
        } else if (i == SPEED_ROW)
            snprintf(label, sizeof label, "< %d / 10 >", speed);
        else if (i == INVERT_ROW)
            snprintf(label, sizeof label, "< %s >", invert_y ? "On" : "Off");
        else if (i == MOVEMENT_ROW)
            snprintf(label, sizeof label, "< %s >", responsive_movement ? "Responsive" : "Original");
        else
            snprintf(label, sizeof label, "Enter / A");
        text(values[i], label);
    }
    WORD(back_button + GADGET_FOCUSED) = selected == BACK_SELECTED;
    WORD(page + SCREEN_FOCUS) = selected == BACK_SELECTED ? back_button : rows[selected];
    snprintf(label, sizeof label, "%d - %d of %d", scroll + 1,
             scroll + VISIBLE_ROWS < ROW_COUNT ? scroll + VISIBLE_ROWS : ROW_COUNT, ROW_COUNT);
    text(status, label);
    text(help, capturing >= 0 ? "Press a key, mouse button or wheel. Esc cancels."
                              : "Arrows: select   Enter/A: change   Delete: clear   Esc/B: back");
}

/* The templates this file clones must look like Sam's own: a constructed text
 * button and a page vtable, both pointing into .rdata at real code. Anything
 * else means the layout above does not hold, and building on it would corrupt
 * the menus, so the PC pages are left out instead. */
static int vtable_ok(uint32_t vtable, int slots)
{
    if (vtable < RDATA_START || vtable + slots * 4 > RDATA_END) return 0;
    for (int i = 0; i < slots; ++i)
        if (WORD(vtable + i * 4) < 0x10000 || WORD(vtable + i * 4) >= TEXT_END) return 0;
    return 1;
}

static int layout_ok(void)
{
    if (vtable_ok(WORD(TEMPLATE_BUTTON), 4) && vtable_ok(SCREEN_VTABLE, VTABLE_SIZE / 4) &&
        vtable_ok(WORD(MAIN_MENU), 4) && WORD(TEMPLATE_BUTTON + GADGET_TEXT) >= 0x10000)
        return 1;
    fprintf(stderr, "[PC CONTROLS] menu templates are not as expected; PC Controls, PC Settings and "
                    "Quit Game are disabled\n");
    return 0;
}

void pc_controls_build_menu(void)
{
    static int checked, usable;
    if (page) return;
    if (!checked) {
        checked = 1;
        usable = layout_ok();
    }
    if (!usable) return;
    page = alloc(SCREEN_SIZE + TEXT_SIZE);
    if (!page) return;
    call(sub_0002F12B, page, NULL, 0);
    memcpy(PTR(page + SCREEN_VTABLE_COPY), PTR(SCREEN_VTABLE), VTABLE_SIZE);
    WORD(page) = page + SCREEN_VTABLE_COPY;
    WORD(page + SCREEN_VTABLE_COPY + VTABLE_KEY) = PAGE_KEY;
    WORD(page + SCREEN_TITLE) = page + SCREEN_TITLE_TEXT;
    snprintf((char *)PTR(page + SCREEN_TITLE_TEXT), TEXT_SIZE, "PC Controls");
    title = button(page, "PC CONTROLS", 90, 88, 1.05f);
    for (int i = 0; i < ROW_COUNT; ++i) {
        const char *label = i < BIND_COUNT    ? actions[i].label
                            : i == SPEED_ROW  ? "Mouse speed"
                            : i == INVERT_ROW ? "Invert mouse Y"
                            : i == MOVEMENT_ROW ? "Keyboard move"
                                              : "Restore defaults";
        rows[i] = button(page, label, 90, 145, 0.57f);
        values[i] = button(page, "", 410, 145, 0.57f);
        if (!rows[i] || !values[i]) {
            fprintf(stderr, "[PC CONTROLS] menu allocation failed\n");
            return;
        }
    }
    status = button(page, "", 90, 397, 0.43f);
    help = button(page, "", 55, 430, 0.38f);
    back_button = button(page, "BACK", 510, 397, 0.5f);
    entry = button(OPTIONS, "PC CONTROLS", 140, 372, 0.65f);
    if (!entry || !title || !status || !help || !back_button) return;
    WORD(entry + GADGET_TRIGGER) = OPEN_PAGE;
    settings_page = alloc(SCREEN_SIZE + TEXT_SIZE);
    if (!settings_page) return;
    call(sub_0002F12B, settings_page, NULL, 0);
    memcpy(PTR(settings_page + SCREEN_VTABLE_COPY), PTR(SCREEN_VTABLE), VTABLE_SIZE);
    WORD(settings_page) = settings_page + SCREEN_VTABLE_COPY;
    WORD(settings_page + SCREEN_VTABLE_COPY + VTABLE_KEY) = SETTINGS_KEY;
    WORD(settings_page + SCREEN_TITLE) = settings_page + SCREEN_TITLE_TEXT;
    snprintf((char *)PTR(settings_page + SCREEN_TITLE_TEXT), TEXT_SIZE, "PC Settings");
    if (!button(settings_page, "PC SETTINGS", 90, 88, 1.05f)) return;
    for (int i = 0; i < PC_SETTINGS_ROWS; ++i) {
        setting_rows[i] = button(settings_page, pc_settings_label(i), 90, 145 + i * 26.0f, 0.57f);
        setting_values[i] = button(settings_page, "", 410, 145 + i * 26.0f, 0.57f);
        if (!setting_rows[i] || !setting_values[i]) return;
    }
    if (!button(settings_page, "Changes apply immediately", 90, 370, 0.43f) ||
        !button(settings_page, "Arrows: select   Left/Right/A: change   Esc/B: back", 55, 430, 0.38f) ||
        !(settings_back = button(settings_page, "BACK", 510, 397, 0.5f)))
        return;
    settings_entry = button(OPTIONS, "PC SETTINGS", 140, 400, 0.65f);
    if (!settings_entry) return;
    WORD(settings_entry + GADGET_TRIGGER) = OPEN_SETTINGS;
    update_settings_page();
    update_page();
    apply();
    build_quit();
    fprintf(stderr, "[PC CONTROLS] Options entry %08X, page %08X\n", entry, page);
}

void pc_controls_options_layout(void)
{
    if (!entry || !settings_entry) return;
    static const uint32_t original[] = {0x472768, 0x472830, 0x4728F8, 0x4729E8, 0x472AB0, 0x472BA0};
    uint32_t first = 0, last = 0;
    int visible = 0;
    for (unsigned i = 0; i < sizeof original / sizeof original[0]; ++i) {
        uint32_t row = original[i];
        if (!WORD(row + GADGET_VISIBLE)) continue;
        options_position(row, 157 + 30.0f * visible, FLOAT(row + GADGET_SCALE), visible);
        ++visible;
        if (!first) first = row;
        last = row;
    }
    if (!last) return;
    options_position(entry, 157 + 30.0f * visible - 19, 0.65f, visible);
    options_position(settings_entry, 157 + 30.0f * (visible + 1) - 19, 0.65f, visible + 1);
    WORD(last + GADGET_DOWN) = entry;
    WORD(first + GADGET_UP) = settings_entry;
    WORD(entry + GADGET_UP) = last;
    WORD(entry + GADGET_DOWN) = settings_entry;
    WORD(settings_entry + GADGET_UP) = entry;
    WORD(settings_entry + GADGET_DOWN) = first;
}

static int physical_down(int key)
{
    if (key >= MOUSE1 && key <= MOUSE5) {
        int button_index = key == MOUSE1   ? 0
                           : key == MOUSE2 ? 1
                           : key == MOUSE3 ? 2
                           : key == MOUSE4 ? 3
                           : key == MOUSE5 ? 4
                                           : -1;
        return button_index >= 0 ? xbox_FramebufferMouseButton(button_index) : 0;
    }
    return key > 0 && key < 256 && xbox_FramebufferKeyDown(key);
}

static void remember_menu_keys(void)
{
    DWORD now = GetTickCount();
    for (int i = 0; i < 6; ++i) {
        menu_held[i] = (unsigned char)(physical_down(menu_keys[i][0]) || physical_down(menu_keys[i][1]));
        if (i < 4) menu_repeat[i] = now + 350;
    }
}

static void cancel_capture(void)
{
    capturing = -1;
    xbox_KbmSetCapture(0);
    remember_menu_keys();
}

static void begin_capture(void)
{
    capturing = selected;
    capture_deadline = GetTickCount() + 6000;
    for (int k = 0; k <= MOUSE5; ++k) previous[k] = (unsigned char)physical_down(k);
    for (int i = 0; i < 5; ++i) xbox_FramebufferMouseClicks(i);
    xbox_KbmMenuWheel();
    xbox_KbmSetCapture(1);
}

static void set_key(int row, int key)
{
    int old = keys[row];
    for (int i = 0; i < BIND_COUNT; ++i)
        if (i != row && key && keys[i] == key) keys[i] = old;
    keys[row] = key;
    apply();
    save();
}

static void change_row(int direction)
{
    if (selected < BIND_COUNT)
        begin_capture();
    else {
        if (selected == SPEED_ROW) {
            speed += direction;
            if (speed < 1) speed = 1;
            if (speed > 10) speed = 10;
        } else if (selected == INVERT_ROW)
            invert_y = !invert_y;
        else if (selected == MOVEMENT_ROW)
            InterlockedExchange(&responsive_movement, !responsive_movement);
        else {
            for (int i = 0; i < BIND_COUNT; ++i) keys[i] = actions[i].key;
            speed = 5;
            invert_y = 0;
            InterlockedExchange(&responsive_movement, 0);
            InterlockedExchange(&keyboard_move_player, 0);
        }
        apply();
        save();
    }
    update_page();
}

static void open_page(void)
{
    uint32_t args[] = {page, 0};
    if (page && entry) {
        selected = scroll = 0;
        cancel_capture();
        update_page();
        for (int i = 0; i < VISIBLE_ROWS && i < ROW_COUNT; ++i) {
            slide_in(rows[i], i);
            slide_in(values[i], i);
        }
        slide_in(status, VISIBLE_ROWS);
        slide_in(back_button, VISIBLE_ROWS + 1);
        int w, h;
        xbox_FramebufferPointer(&pointer_x, &pointer_y, &w, &h);
        for (int i = 0; i < 5; ++i) xbox_FramebufferMouseClicks(i);
        xbox_KbmMenuWheel();
        delete_down = xbox_FramebufferKeyDown(VK_DELETE);
        call(sub_0002A807, 0, args, 2);
    }
    g_esp += 4;
}

static void page_key(void)
{
    uint32_t key = WORD(g_esp + 4);
    if (key == VK_ESCAPE) {
        if (capturing >= 0)
            cancel_capture();
        else
            call(sub_0002B52A, 0, NULL, 0);
    } else if (capturing < 0) {
        if (key == VK_UP)
            selected = (selected + ROW_COUNT) % (ROW_COUNT + 1);
        else if (key == VK_DOWN)
            selected = (selected + 1) % (ROW_COUNT + 1);
        else if (selected == BACK_SELECTED) {
            if (key == VK_RETURN || key == 0x64) call(sub_0002B52A, 0, NULL, 0);
        } else if (key == VK_RETURN || key == 0x64 || key == VK_LEFT || key == VK_RIGHT)
            change_row(key == VK_LEFT ? -1 : 1);
        else if (key == VK_DELETE && selected < BIND_COUNT)
            set_key(selected, 0);
    }
    update_page();
    g_eax = 1;
    g_esp += 12; /* thiscall ret 8 */
}

void (*pc_controls_dispatch(uint32_t address))(void)
{
    if (address == OPEN_PAGE) return open_page;
    if (address == PAGE_KEY) return page_key;
    if (address == OPEN_SETTINGS) return open_settings_page;
    if (address == SETTINGS_KEY) return settings_key;
    if (address == OPEN_QUIT) return open_quit;
    if (address == QUIT_KEY) return quit_key;
    if (address == QUIT_YES) return quit_yes;
    if (address == QUIT_NO) return quit_no;
    return NULL;
}

static void follow_mode(void)
{
    int in_menu = shown_menu() != 0, dx, dy;
    if (menu_mode != in_menu) {
        menu_mode = in_menu;
        xbox_KbmSetMenuMode(in_menu);
        xbox_FramebufferSetMenuPointer(in_menu);
        xbox_FramebufferMouseDelta(&dx, &dy);
        for (int i = 0; i < BIND_COUNT; ++i) blocked[i] = (unsigned char)physical_down(keys[i]);
        remember_menu_keys();
        pause_held = physical_down(keys[PAUSE]);
        if (in_menu) xbox_KbmPulse("START", 0);
    }
}

/* PC menu events use Sam's own key handler on the render thread. This keeps
 * short taps responsive independently of USB report timing, while real Xbox
 * controllers retain their original input path. */
static void keyboard_frame(void)
{
    if (!enabled) return;
    if (!shown_menu()) {
        int down = physical_down(keys[PAUSE]);
        if (down && !pause_held) xbox_KbmPulse("START", 350);
        pause_held = down;
        return;
    }
    if (capturing >= 0) return;
    DWORD now = GetTickCount();
    for (int i = 0; i < 6; ++i) {
        int down = physical_down(menu_keys[i][0]) || physical_down(menu_keys[i][1]);
        int press = down && (!menu_held[i] || (i < 4 && (int32_t)(now - menu_repeat[i]) >= 0));
        int was_held = menu_held[i];
        menu_held[i] = (unsigned char)down;
        if (!press) continue;
        if (i < 4) menu_repeat[i] = now + (was_held ? 100 : 350);
        uint32_t args[] = {(uint32_t)menu_keys[i][2], 0};
        call(sub_0002B588, 0, args, 2);
        if (capturing >= 0 || !shown_menu()) break;
    }
}

/* Escape, Enter, Space or a left click skips a movie, as Start does on a pad;
 * passes the "Please press START to continue" briefing that follows the level
 * load (NETRICSA, with its preview in a small box, so not a full-screen video,
 * on a screen with no 3D world); and continues the end-of-level statistics as
 * A does ("A Continue": that screen has 3D models, but no play is running).
 * None of these happen with a menu shown or in play, so the keys keep their
 * in-game meaning there. The boot movies play on
 * their own worker while the main thread loads, and do not reliably pass
 * through any presenter this file hooks, so a small thread of its own watches
 * for the press (movie_thread). It only reads key and screen state and pulses
 * the virtual pad, all safe from any thread. */
void pc_controls_movie_frame(void)
{
    static const int skip_keys[] = {VK_ESCAPE, VK_RETURN, VK_SPACE, MOUSE1};
    static volatile LONG held;
    LONG down = 0;
    if (!enabled) return;
    for (int i = 0; i < (int)(sizeof skip_keys / sizeof skip_keys[0]); ++i)
        if (physical_down(skip_keys[i])) down = 1;
    if (InterlockedExchange(&held, down) || !down) return;
    if (xbox_VideoActive() || (!shown_menu() && nv2a_gpu_frames_without_3d() >= 2)) {
        xbox_KbmPulse("START", 150);
        fprintf(stderr, "[PC CONTROLS] movie skipped from the keyboard/mouse\n");
    } else if (!shown_menu() && !playing()) {
        xbox_KbmPulse("A", 150);
        fprintf(stderr, "[PC CONTROLS] NETRICSA screen continued from the keyboard/mouse\n");
    }
}

/* NETRICSA's own pad reader (sub_00058E20) scrolls the text with up/down on
 * the d-pad or either stick and turns messages with left/right, repeating
 * while held. The arrows (or W/A/S/D) hold the d-pad for as long as they are
 * down, so Sam's own repeat applies; each wheel notch is one tap of up or
 * down, with a gap after it so the next notch is a press of its own. Only
 * while that reader runs (the briefing, or NETRICSA on) and no menu is shown. */
static void netricsa_frame(void)
{
    static const char *const targets[4] = {"UP", "DOWN", "LEFT", "RIGHT"};
    static const int arrow_keys[4][2] = {{VK_UP, 'W'}, {VK_DOWN, 'S'}, {VK_LEFT, 'A'}, {VK_RIGHT, 'D'}};
    static int pending;
    static DWORD last_notch;
    int notches = xbox_KbmWheelNotches();
    if (!enabled) return;
    /* Before the title runs there is no guest memory to read. */
    if (!InterlockedCompareExchange(&menu_draw_seen, 0, 0) && !InterlockedCompareExchange(&action_seen, 0, 0))
        return;
    uint32_t game = WORD(GAME);
    int computer = game ? (int)WORD(game + GAME_COMPUTER_STATE) : 0;
    if ((computer != COMPUTER_ON && !WORD(BRIEFING)) || shown_menu()) {
        pending = 0;
        return;
    }
    for (int i = 0; i < 4; ++i)
        if (physical_down(arrow_keys[i][0]) || physical_down(arrow_keys[i][1])) xbox_KbmPulse(targets[i], 50);
    pending += notches;
    if (pending > 10) pending = 10;
    if (pending < -10) pending = -10;
    DWORD now = GetTickCount();
    if (pending && now - last_notch >= 100) {
        xbox_KbmPulse(pending > 0 ? "UP" : "DOWN", 50);
        pending += pending > 0 ? -1 : 1;
        last_notch = now;
    }
}

static void wide_frame(void);

static DWORD WINAPI movie_thread(LPVOID unused)
{
    (void)unused;
    for (;;) {
        pc_controls_movie_frame();
        netricsa_frame();
        /* Only once the title runs: this thread starts before guest memory. */
        if (InterlockedCompareExchange(&menu_draw_seen, 0, 0) || InterlockedCompareExchange(&action_seen, 0, 0))
            wide_frame();
        Sleep(10);
    }
}

/* Widescreen policy. The front end is drawn for 4:3 (its backdrop, and Sam's
 * head as a 3D model) and stays 4:3; a level, and the pause menu drawn over
 * it, use the Aspect ratio setting. The pages the pause menu opens (Options
 * and the rest) are full-screen front-end pages, with their own backdrop and
 * 3D bomb, so they are 4:3 as well. A level is running from the first frame
 * with no menu and no movie until the main menu comes back. */
static float view_wide = 1.0f;

static int guest_range(uint32_t address, uint32_t bytes)
{
    return address >= 0x10000u && address <= 0x08000000u - bytes;
}

static int cinematic_camera_active(void)
{
    /* The same first-player lookup as GetPlayerEntity(0), 0x1E6FB0.
     * Startup, loading and disconnected players need not have these objects. */
    uint32_t net = WORD(NETWORK);
    if (!guest_range(net, 0x18)) return 0;
    uint32_t session = WORD(net + 0x14);
    if (!guest_range(session, 0x14)) return 0;
    uint32_t players = WORD(session + 0x10);
    if (!guest_range(players, 8) || !WORD(players)) return 0;
    uint32_t player = WORD(players + 4);
    return guest_range(player, PLAYER_CAMERA + 4) && WORD(player) == PLAYER_VTABLE && WORD(player + PLAYER_CAMERA) != 0;
}

float pc_controls_view_wide(void)
{
    return view_wide;
}

/* Called on the main thread every frame it presents through a hooked path,
 * and from movie_thread: NETRICSA's end-of-level statistics present through
 * neither vblank hook, so play's widescreen stayed on there. */
static SRWLOCK wide_lock = SRWLOCK_INIT;

static void wide_frame_locked(void);
static void wide_frame(void)
{
    AcquireSRWLockExclusive(&wide_lock);
    wide_frame_locked();
    ReleaseSRWLockExclusive(&wide_lock);
}

static void wide_frame_locked(void)
{
    static int in_level;
    static uint32_t last_menu = 1, from_menu, last_page, resume_page;
    uint32_t menu = shown_menu();
    if (menu != last_menu) {
        if (getenv("RECOMP_MENU_MOUSE_TRACE"))
            fprintf(stderr, "[MENU] active %08X (was %08X) at %lu ms\n", menu, last_menu, GetTickCount());
        from_menu = last_menu;
    }
    last_menu = menu;
    if (menu) last_page = menu;
    if (menu == MAIN_MENU)
        in_level = 0;
    else if (!menu && !xbox_VideoActive()) {
        in_level = 1;
        resume_page = last_page;
    }
    /* Leaving the pause menu, Sam names the page play last came back from
     * (the one the level was started from) for the 500 ms its entries take
     * to slide away over the paused level, then clears it and play resumes.
     * That is still the pause menu on screen. The pages it opens are not. */
    int pause_menu = menu == PAUSE_MENU || (menu && menu == resume_page && from_menu == PAUSE_MENU);
    /* NETRICSA's pages (the end-of-level statistics) are laid out for 4:3 as
     * the menus are, so wide is play and the pause menu over it only. */
    /* Scripted camera views retain the console's framing. Their original
     * letterbox drawport is shorter than full screen, so gameplay's Hor+
     * projection hook cannot compensate for stretching the render target.
     * Use the existing 4:3 presentation path, which adds black side bars. */
    int cinematic = cinematic_camera_active() || xbox_VideoActive();
    float wide = in_level && !cinematic && ((!menu && playing()) || pause_menu) ? pc_settings_aspect() * 3.0f / 4.0f : 1.0f;
    if (fabsf(wide - view_wide) > 0.0001f) {
        view_wide = wide;
        nv2a_gpu_set_wide_factor(wide);
        fprintf(stderr, "[PC SETTINGS] view %.3f times as wide as 4:3\n", wide);
    }
}

/* ── Mouse in Sam's own menus ──────────────────────────────────────────────
 *
 * As in Turok's: moving over an entry focuses it, a left click chooses it (on
 * an option row, the next value), a right click is Back (on an option row, the
 * previous value), and the wheel moves up and down (over an option row, its
 * value). The PC pages above handle their own pointer.
 *
 * A page's buttons are a list from page +0x04 through each button's node at
 * +0x08 (sub_0002727C adds one). Focus works as the page's own OnKeyDown
 * (0x0002AC71) moves it: the old button's KillFocus (vtable +0x10), the new
 * one's SetFocus (+0x0C). Only buttons reachable from the focused one through
 * the up/down/left/right links can take focus, which leaves out titles,
 * labels and button hints. A button's own OnKeyDown (+0x00) takes Left/Right
 * when it is an option row (a slider or a choice); anything else answers 0. */
#define GADGET_NODE        0x08u
#define SCREEN_GADGETS     0x04u
#define VTABLE_GADGET_KEY  0x00u
#define VTABLE_SET_FOCUS   0x0Cu
#define VTABLE_KILL_FOCUS  0x10u
#define MENU_SOUND_CHANGE  0x0047013Cu /* what sub_0002B588 plays for a handled key */
#define SHORTCUT_VTABLE    0x003A9F34u /* A/B/X/Y footer, rendered by sub_0001C999 */
#define SHORTCUT_BUTTON    0xB0u /* ASCII controller button */
#define SHORTCUT_ICON_LEFT 0xC0u
#define MAX_MENU_GADGETS   64
static uint32_t hovered_shortcut, hovered_shortcut_menu;
extern void sub_00028A11(void);
extern void (*recomp_lookup(uint32_t va))(void);

static int guest_object(uint32_t va)
{
    return va >= 0x10000 && va < 0x08000000;
}

int pc_controls_shortcut_hovered(uint32_t gadget)
{
    int x, y, w, h;
    return enabled && GetCurrentThreadId() == main_thread && guest_object(gadget) &&
           gadget == hovered_shortcut && shown_menu() == hovered_shortcut_menu &&
           WORD(gadget + GADGET_VISIBLE) && WORD(gadget + GADGET_ENABLED) &&
           xbox_FramebufferPointer(&x, &y, &w, &h);
}

static uint32_t virtual_call(uint32_t object, uint32_t slot, const uint32_t *args, unsigned count)
{
    void (*fn)(void) = recomp_lookup(WORD(WORD(object) + slot));
    return fn ? call(fn, object, args, count) : 0;
}

static void menu_change_sound(void)
{
    uint32_t sound = WORD(MENU_SOUND_CHANGE);
    call(sub_00028A11, 0, &sound, 1);
}

/* Footer hints are shortcuts, not focusable rows. Keep the selected profile
 * (or other row) focused and send the same key the controller would send. */
static uint32_t shortcut_key(uint32_t gadget)
{
    if (WORD(gadget) != SHORTCUT_VTABLE) return 0;
    switch (WORD(gadget + SHORTCUT_BUTTON)) {
    case 'A': return VK_RETURN;
    case 'B': return VK_ESCAPE;
    case 'X': return 0xA8C;
    case 'Y': return 0xAF0;
    default: return 0;
    }
}

static int menu_shortcuts(uint32_t menu, uint32_t *out)
{
    int n = 0;
    for (uint32_t node = WORD(menu + SCREEN_GADGETS), guard = 0; guest_object(node) && WORD(node) && guard < 256;
         node = WORD(node), ++guard) {
        uint32_t g = node - GADGET_NODE;
        if (WORD(g + GADGET_VISIBLE) && WORD(g + GADGET_ENABLED) && shortcut_key(g) && n < MAX_MENU_GADGETS)
            out[n++] = g;
    }
    return n;
}

/* The focusable buttons of `menu`: those linked, directly or not, to the
 * focused one. Returns their count; *focused is 0 if none has the focus. */
static int menu_buttons(uint32_t menu, uint32_t *out, uint32_t *focused)
{
    int n = 0;
    *focused = 0;
    for (uint32_t node = WORD(menu + SCREEN_GADGETS), guard = 0; guest_object(node) && WORD(node) && guard < 256;
         node = WORD(node), ++guard)
        if (WORD(node - GADGET_NODE + GADGET_FOCUSED)) *focused = node - GADGET_NODE;
    if (!*focused) return 0;
    out[n++] = *focused;
    for (int i = 0; i < n; ++i)
        for (uint32_t link = GADGET_LEFT; link <= GADGET_DOWN; link += 4) {
            uint32_t next = WORD(out[i] + link);
            int seen = !guest_object(next) || !WORD(next + GADGET_VISIBLE);
            for (int j = 0; j < n && !seen; ++j) seen = out[j] == next;
            if (!seen && n < MAX_MENU_GADGETS) out[n++] = next;
        }
    return n;
}

/* Where a button is, in the 640x480 menu space, as Sam's own Render
 * (0x0001B3D2) places it: sub_0001B237 gives the top left (its anchor moved
 * by the alignment at +0x6C / +0x70), vtable +0x20 the text's size in its own
 * font, and the page it is on adds its offset (+0x24, +0x28). A row starting
 * at its anchor with no other button beside it reaches across to its value
 * (a slider's bar, a choice's setting), which its text box does not cover. */
#define GADGET_SCREEN     0x10u
#define VTABLE_TEXT_SIZE  0x20u
#define SCREEN_OFFSET_X   0x24u
#define SCREEN_OFFSET_Y   0x28u
extern void sub_0001B237(void);

static void button_box(uint32_t button, const uint32_t *all, int n, float box[4])
{
    static uint32_t scratch;
    if (!scratch) scratch = alloc(16);
    uint32_t corner[] = {scratch, scratch + 4}, size[] = {scratch + 8, scratch + 12, 1};
    call(sub_0001B237, button, corner, 2);
    virtual_call(button, VTABLE_TEXT_SIZE, size, 3);
    uint32_t screen = WORD(button + GADGET_SCREEN);
    int shortcut = shortcut_key(button) != 0;
    /* The footer renderer uses the animated anchor directly; unlike text
     * rows it does not add the page offset. Its background bars overlap, so
     * trim the overlap toward the adjacent label while retaining both icons. */
    float dx = !shortcut && guest_object(screen) ? FLOAT(screen + SCREEN_OFFSET_X) : 0.0f;
    float dy = !shortcut && guest_object(screen) ? FLOAT(screen + SCREEN_OFFSET_Y) : 0.0f;
    float y = FLOAT(button + GADGET_Y);
    int alone = 1;
    for (int i = 0; i < n; ++i)
        if (all[i] != button && fabsf(FLOAT(all[i] + GADGET_Y) - y) < 10.0f) alone = 0;
    box[0] = FLOAT(scratch) + dx;
    box[1] = FLOAT(scratch + 4) + dy;
    box[2] = box[0] + FLOAT(scratch + 8);
    box[3] = box[1] + FLOAT(scratch + 12);
    if (shortcut) {
        for (int i = 0; i < n; ++i) {
            uint32_t other = all[i];
            if (other == button || !shortcut_key(other) || fabsf(FLOAT(other + GADGET_Y) - y) >= 10.0f) continue;
            call(sub_0001B237, other, corner, 2);
            float ox = FLOAT(scratch);
            if (ox > FLOAT(button + GADGET_X) && WORD(button + SHORTCUT_ICON_LEFT))
                box[2] = fminf(box[2], ox);
            else if (ox < FLOAT(button + GADGET_X) && !WORD(other + SHORTCUT_ICON_LEFT)) {
                virtual_call(other, VTABLE_TEXT_SIZE, size, 3);
                box[0] = fmaxf(box[0], ox + FLOAT(scratch + 8));
            }
        }
        return;
    }
    if (alone && WORD(button + GADGET_CENTRED) == 0xFFFFFFFFu && box[2] < 620.0f) box[2] = 620.0f;
}

static uint32_t button_at(const uint32_t *all, int n, float gx, float gy)
{
    uint32_t best = 0;
    float best_distance = 1e9f;
    for (int i = 0; i < n; ++i) {
        if (!WORD(all[i] + GADGET_ENABLED)) continue;
        float box[4];
        button_box(all[i], all, n, box);
        int shortcut = shortcut_key(all[i]) != 0;
        float pad = shortcut ? 0.0f : 6.0f;
        if (gx < box[0] - pad || gx > box[2] + pad || (shortcut && gx == box[2]) ||
            gy < box[1] - 3 || gy > box[3] + 3) continue;
        float dx = gx - (box[0] + box[2]) / 2, dy = gy - (box[1] + box[3]) / 2;
        if (dx * dx + dy * dy < best_distance) {
            best_distance = dx * dx + dy * dy;
            best = all[i];
        }
    }
    return best;
}

/* Send a key to `button` alone; nonzero when it took it (an option row). */
static int button_key(uint32_t button, uint32_t key)
{
    uint32_t args[] = {key};
    if (!virtual_call(button, VTABLE_GADGET_KEY, args, 1)) return 0;
    menu_change_sound();
    return 1;
}

/* A key to the whole menu, as the keyboard sends it. */
static void menu_key(uint32_t key)
{
    uint32_t args[] = {key, 0};
    call(sub_0002B588, 0, args, 2);
}

static void native_mouse_frame(void)
{
    static int last_x = -1, last_y = -1;
    uint32_t menu = shown_menu(), buttons[MAX_MENU_GADGETS], focused, hit;
    uint32_t shortcuts[MAX_MENU_GADGETS];
    int x, y, w, h, n;
    if (!menu || menu == page || menu == settings_page || menu == quit_page || capturing >= 0) return;
    int click = xbox_FramebufferMouseClicks(0), right = xbox_FramebufferMouseClicks(1);
    int wheel = xbox_KbmMenuWheel();
    if (!xbox_FramebufferPointer(&x, &y, &w, &h)) {
        hovered_shortcut = hovered_shortcut_menu = 0;
        if ((click || right) && getenv("RECOMP_MENU_MOUSE_TRACE"))
            fprintf(stderr, "[MENU MOUSE] menu %08X: no pointer\n", menu);
        return;
    }
    int moved = x != last_x || y != last_y;
    last_x = x;
    last_y = y;
    if (!moved && !click && !right && !wheel) return;
    n = menu_buttons(menu, buttons, &focused);
    int shortcut_count = menu_shortcuts(menu, shortcuts);
    /* Menus are drawn for 4:3, centred in the window (also over a widescreen
     * level, where the 2D is squeezed back to the middle). */
    float view_w = (float)w, view_h = (float)w * 3 / 4;
    if (view_h > h) {
        view_h = (float)h;
        view_w = (float)h * 4 / 3;
    }
    float gx = (x - ((float)w - view_w) / 2) * 640 / view_w, gy = (y - ((float)h - view_h) / 2) * 480 / view_h;
    uint32_t shortcut = button_at(shortcuts, shortcut_count, gx, gy);
    if (shortcut && shortcut != hovered_shortcut) menu_change_sound();
    hovered_shortcut = shortcut;
    hovered_shortcut_menu = menu;
    if (shortcut) {
        if (click) menu_key(shortcut_key(shortcut));
        else if (right) menu_key(VK_ESCAPE);
        return;
    }
    hit = button_at(buttons, n, gx, gy);
    if ((click || right) && getenv("RECOMP_MENU_MOUSE_TRACE")) {
        fprintf(stderr, "[MENU MOUSE] menu %08X at %.0f,%.0f: %d buttons, focused %08X, hit %08X\n", menu, gx, gy,
                n, focused, hit);
        for (int i = 0; i < n; ++i) {
            float box[4];
            button_box(buttons[i], buttons, n, box);
            fprintf(stderr, "[MENU MOUSE]   %08X vt %08X %.0f,%.0f - %.0f,%.0f\n", buttons[i], WORD(buttons[i]),
                    box[0], box[1], box[2], box[3]);
        }
    }
    if (hit && hit != focused && (moved || click || right || wheel)) {
        virtual_call(focused, VTABLE_KILL_FOCUS, NULL, 0);
        virtual_call(hit, VTABLE_SET_FOCUS, NULL, 0);
        menu_change_sound();
    }
    if (click && hit && !button_key(hit, VK_RIGHT)) menu_key(VK_RETURN);
    if (right && !(hit && button_key(hit, VK_LEFT))) menu_key(VK_ESCAPE);
    if (wheel && !(hit && button_key(hit, wheel > 0 ? VK_RIGHT : VK_LEFT))) menu_key(wheel > 0 ? VK_UP : VK_DOWN);
}

void pc_controls_frame(void)
{
    if (GetCurrentThreadId() != main_thread) return;
    if (!enabled || shown_menu() != hovered_shortcut_menu) hovered_shortcut = hovered_shortcut_menu = 0;
    wide_frame();
    if (quit_entry) WORD(quit_entry + GADGET_VISIBLE) = shown_menu() == MAIN_MENU;
    if (quit_entry && shown_menu() == MAIN_MENU) {
        WORD(MAIN_MENU_LAST + GADGET_DOWN) = quit_entry;
        WORD(TEMPLATE_BUTTON + GADGET_UP) = quit_entry;
    }
    follow_mode();
    keyboard_frame();
    pc_settings_poll();
    if (quit_page && shown_menu() == quit_page) {
        quit_frame();
        return;
    }
    if (settings_page && shown_menu() == settings_page) {
        if (capturing >= 0) cancel_capture();
        settings_frame();
        return;
    }
    if (!page || shown_menu() != page) {
        if (capturing >= 0) cancel_capture();
        if (enabled) native_mouse_frame();
        return;
    }
    int old_selected = selected;
    int wheel = xbox_KbmMenuWheel();
    if (capturing >= 0) {
        int key = wheel > 0 ? WHEEL_UP : wheel < 0 ? WHEEL_DOWN : 0;
        static const int mouse_keys[] = {MOUSE1, MOUSE2, MOUSE3, MOUSE4, MOUSE5};
        for (int i = 0; i < 5; ++i)
            if (xbox_FramebufferMouseClicks(i)) key = mouse_keys[i];
        for (int k = 1; k <= MOUSE5; ++k) {
            int down = physical_down(k);
            if (down && !previous[k] && k != VK_F11 && k != VK_F12) key = k;
            previous[k] = (unsigned char)down;
        }
        if (key == VK_ESCAPE || (int32_t)(GetTickCount() - capture_deadline) >= 0)
            cancel_capture();
        else if (key) {
            int row = capturing;
            cancel_capture();
            set_key(row, key);
        }
    } else {
        int x, y, w, h;
        int click = xbox_FramebufferMouseClicks(0), right = xbox_FramebufferMouseClicks(1);
        int del = xbox_FramebufferKeyDown(VK_DELETE);
        if (del && !delete_down && selected < BIND_COUNT) set_key(selected, 0);
        delete_down = del;
        if (xbox_FramebufferPointer(&x, &y, &w, &h)) {
            /* Sam is presented as 4:3, with pillarboxing in wide windows. */
            float view_w = (float)w, view_h = (float)w * 3 / 4;
            if (view_h > h) {
                view_h = (float)h;
                view_w = (float)h * 4 / 3;
            }
            float left = ((float)w - view_w) / 2, top = ((float)h - view_h) / 2;
            float gx = (x - left) * 640 / view_w, gy = (y - top) * 480 / view_h;
            /* The title font's glyphs sit below the text button's anchor;
             * use their visible 26-pixel rows, rather than the anchor itself. */
            int row = scroll + (int)((gy - 154) / 26);
            int moved = x != pointer_x || y != pointer_y;
            if (gx >= 495 && gx <= 610 && gy >= 390 && gy < 425) {
                if (moved) selected = BACK_SELECTED;
                if (click) call(sub_0002B52A, 0, NULL, 0);
            } else if (gx >= 75 && gx <= 610 && gy >= 154 && gy < 154 + VISIBLE_ROWS * 26 && row < ROW_COUNT &&
                     (x != pointer_x || y != pointer_y || click || right)) {
                selected = row;
                if (right && selected < BIND_COUNT)
                    set_key(selected, 0);
                else if (click)
                    change_row(1);
            }
            pointer_x = x;
            pointer_y = y;
        }
        if (wheel) {
            selected += wheel > 0 ? -1 : 1;
            if (selected < 0) selected = 0;
            if (selected >= ROW_COUNT) selected = ROW_COUNT - 1;
        }
    }
    if (selected != old_selected) menu_change_sound();
    update_page();
}

static void update_quit(void)
{
    for (int i = 0; i < 2; ++i) WORD(quit_choices[i] + GADGET_FOCUSED) = i == quit_selected;
    WORD(quit_page + SCREEN_FOCUS) = quit_choices[quit_selected];
}

static void build_quit(void)
{
    quit_page = alloc(SCREEN_SIZE + TEXT_SIZE);
    if (!quit_page) return;
    call(sub_0002F12B, quit_page, NULL, 0);
    memcpy(PTR(quit_page + SCREEN_VTABLE_COPY), PTR(SCREEN_VTABLE), VTABLE_SIZE);
    WORD(quit_page) = quit_page + SCREEN_VTABLE_COPY;
    WORD(quit_page + SCREEN_VTABLE_COPY + VTABLE_KEY) = QUIT_KEY;
    WORD(quit_page + SCREEN_TITLE) = quit_page + SCREEN_TITLE_TEXT;
    snprintf((char *)PTR(quit_page + SCREEN_TITLE_TEXT), TEXT_SIZE, "Quit Game");
    if (!button(quit_page, "QUIT GAME", 90, 115, 0.85f) ||
        !button(quit_page, "Return to the desktop?", 125, 190, 0.7f))
        return;
    quit_choices[0] = button(quit_page, "YES", 200, 265, 0.7f);
    quit_choices[1] = button(quit_page, "NO", 410, 265, 0.7f);
    if (!quit_choices[0] || !quit_choices[1]) return;
    WORD(quit_choices[0] + GADGET_TRIGGER) = QUIT_YES;
    WORD(quit_choices[1] + GADGET_TRIGGER) = QUIT_NO;
    for (int i = 0; i < 2; ++i) {
        WORD(quit_choices[i] + GADGET_CENTRED) = 0;
        for (uint32_t link = GADGET_LEFT; link <= GADGET_DOWN; link += 4) WORD(quit_choices[i] + link) = quit_choices[1 - i];
    }
    quit_entry = button(MAIN_MENU, "QUIT GAME", 391, 339, 0.875f);
    if (!quit_entry) return;
    WORD(quit_entry + GADGET_CENTRED) = 0;
    WORD(quit_entry + GADGET_TRIGGER) = OPEN_QUIT;
    WORD(quit_entry + GADGET_UP) = MAIN_MENU_LAST;
    WORD(quit_entry + GADGET_DOWN) = TEMPLATE_BUTTON;
    quit_selected = 1;
    update_quit();
}

static void open_quit(void)
{
    uint32_t args[] = {quit_page, 0};
    quit_selected = 1;
    update_quit();
    remember_menu_keys();
    WORD(quit_entry + GADGET_VISIBLE) = 0;
    int w, h;
    xbox_FramebufferPointer(&pointer_x, &pointer_y, &w, &h);
    xbox_FramebufferMouseClicks(0);
    call(sub_0002A807, 0, args, 2);
    g_esp += 4;
}
/* The runtime maps an Xbox dashboard return to desktop exit. Sam's unused
 * shell Quit instead tears down the entire engine and can stall in shutdown.
 * This entry is only on the main menu; native PC choices persist on change. */
static void quit_desktop(void)
{
    xbox_HalReturnToFirmware(2);
    g_esp += 4;
}
static void quit_yes(void)
{
    call(quit_desktop, 0, NULL, 0);
    g_esp += 4;
}
static void quit_no(void)
{
    call(sub_0002B52A, 0, NULL, 0);
    g_esp += 4;
}
static void quit_key(void)
{
    uint32_t key = WORD(g_esp + 4);
    if (key == VK_ESCAPE)
        call(sub_0002B52A, 0, NULL, 0);
    else if (key == VK_UP || key == VK_DOWN || key == VK_LEFT || key == VK_RIGHT)
        quit_selected ^= 1;
    else if (key == VK_RETURN || key == 0x64)
        call(quit_selected ? sub_0002B52A : quit_desktop, 0, NULL, 0);
    update_quit();
    g_eax = 1;
    g_esp += 12;
}
static void quit_frame(void)
{
    int x, y, w, h, click = xbox_FramebufferMouseClicks(0);
    if (!xbox_FramebufferPointer(&x, &y, &w, &h) || !w || !h) return;
    float vw = (float)w, vh = vw * 3 / 4;
    if (vh > h) {
        vh = (float)h;
        vw = vh * 4 / 3;
    }
    float gx = (x - (w - vw) / 2) * 640 / vw, gy = (y - (h - vh) / 2) * 480 / vh;
    if (gy >= 265 && gy < 310 && ((gx >= 140 && gx < 270) || (gx >= 350 && gx < 470)) &&
        (x != pointer_x || y != pointer_y || click)) {
        int old_selected = quit_selected;
        quit_selected = gx < 305 ? 0 : 1;
        if (quit_selected != old_selected) menu_change_sound();
        if (click) call(quit_selected ? sub_0002B52A : quit_desktop, 0, NULL, 0);
        update_quit();
    }
    pointer_x = x;
    pointer_y = y;
}

static void update_settings_page(void)
{
    char value[96], label[128];
    for (int i = 0; i < PC_SETTINGS_ROWS; ++i) {
        WORD(setting_rows[i] + GADGET_FOCUSED) = WORD(setting_values[i] + GADGET_FOCUSED) = i == selected_setting;
        pc_settings_value(i, value, sizeof value);
        if (i == PC_VIDEO_DEFAULTS)
            snprintf(label, sizeof label, "%s", value);
        else
            snprintf(label, sizeof label, "< %s >", value);
        text(setting_values[i], label);
    }
    WORD(settings_back + GADGET_FOCUSED) = selected_setting == SETTINGS_BACK;
    WORD(settings_page + SCREEN_FOCUS) =
            selected_setting == SETTINGS_BACK ? settings_back : setting_rows[selected_setting];
}

static void open_settings_page(void)
{
    if (settings_page && settings_entry) {
        int w, h;
        uint32_t args[] = {settings_page, 0};
        selected_setting = 0;
        cancel_capture();
        update_settings_page();
        for (int i = 0; i < PC_SETTINGS_ROWS; ++i) {
            slide_in(setting_rows[i], i);
            slide_in(setting_values[i], i);
        }
        slide_in(settings_back, PC_SETTINGS_ROWS);
        xbox_FramebufferPointer(&pointer_x, &pointer_y, &w, &h);
        for (int i = 0; i < 5; ++i) xbox_FramebufferMouseClicks(i);
        xbox_KbmMenuWheel();
        call(sub_0002A807, 0, args, 2);
    }
    g_esp += 4;
}

static void settings_key(void)
{
    uint32_t key = WORD(g_esp + 4);
    if (key == VK_ESCAPE)
        call(sub_0002B52A, 0, NULL, 0);
    else if (key == VK_UP)
        selected_setting = (selected_setting + PC_SETTINGS_ROWS) % (PC_SETTINGS_ROWS + 1);
    else if (key == VK_DOWN)
        selected_setting = (selected_setting + 1) % (PC_SETTINGS_ROWS + 1);
    else if (selected_setting == SETTINGS_BACK) {
        if (key == VK_RETURN || key == 0x64) call(sub_0002B52A, 0, NULL, 0);
    } else if (key == VK_RETURN || key == 0x64 || key == VK_LEFT || key == VK_RIGHT)
        pc_settings_step(selected_setting, key == VK_LEFT ? -1 : 1);
    update_settings_page();
    g_eax = 1;
    g_esp += 12;
}

static void settings_frame(void)
{
    int old_selected = selected_setting;
    int x, y, w, h, click = xbox_FramebufferMouseClicks(0), right = xbox_FramebufferMouseClicks(1);
    int wheel = xbox_KbmMenuWheel();
    if (xbox_FramebufferPointer(&x, &y, &w, &h)) {
        float view_w = (float)w, view_h = (float)w * 3 / 4;
        if (view_h > h) {
            view_h = (float)h;
            view_w = (float)h * 4 / 3;
        }
        float gx = (x - ((float)w - view_w) / 2) * 640 / view_w;
        float gy = (y - ((float)h - view_h) / 2) * 480 / view_h;
        int row = (int)((gy - 154) / 26);
        if (gx >= 495 && gx <= 610 && gy >= 390 && gy < 425) {
            if (x != pointer_x || y != pointer_y) selected_setting = SETTINGS_BACK;
            if (click) call(sub_0002B52A, 0, NULL, 0);
        } else if (gx >= 75 && gx <= 630 && gy >= 154 && gy < 154 + PC_SETTINGS_ROWS * 26 &&
                 (x != pointer_x || y != pointer_y || click || right)) {
            selected_setting = row;
            if (click || right) pc_settings_step(row, right ? -1 : 1);
        }
        pointer_x = x;
        pointer_y = y;
    }
    if (wheel) {
        selected_setting += wheel > 0 ? -1 : 1;
        if (selected_setting < 0) selected_setting = 0;
        if (selected_setting >= PC_SETTINGS_ROWS) selected_setting = PC_SETTINGS_ROWS - 1;
    }
    if (selected_setting != old_selected) menu_change_sound();
    update_settings_page();
}

int pc_controls_input_begin(uint32_t profile)
{
    injected = 0;
    InterlockedExchange(&action_ms, (LONG)GetTickCount());
    InterlockedExchange(&action_seen, 1);
    /* The action builder runs every game tick, also when no presenter this
     * file hooks does (after Back on the pause menu the engine plays on with
     * ACTIVE_MENU still naming the Difficulty page and neither vblank wait
     * nor the menu renderer called), so the widescreen policy follows here
     * too. */
    if (GetCurrentThreadId() == main_thread) wide_frame();
    follow_mode();
    if (!shown_menu()) keyboard_frame();
    if (!enabled || capturing >= 0 || shown_menu() || profile < 0x10000 || profile > 0x07FFFFC0 ||
        WORD(profile + 0x38) != 0)
        return 0;
    /* The original action builder returns early without consuming controls
     * when this player has no entity. Leave those loading/spectator calls alone. */
    uint32_t net = WORD(0x4A2AE0), session = net ? WORD(net + 0x14) : 0,
             players = session ? WORD(session + 0x10) : 0;
    if (!players || !WORD(players) || !WORD(players + 4)) return 0;
    int controller_move = 0, keyboard_move = 0;
    for (int i = FORWARD; i <= RIGHT; ++i)
        if (fabsf(FLOAT(actions[i].address)) > 0.0001f) controller_move = 1;
    for (int i = FIRE; i < PAUSE; ++i) saved_buttons[i - FIRE] = WORD(actions[i].address);
    for (int i = 0; i < PAUSE; ++i) {
        int down = xbox_KbmInputDown(keys[i]);
        if (blocked[i]) {
            if (!down) blocked[i] = 0;
            continue;
        }
        if (down) {
            if (i <= RIGHT) keyboard_move = 1;
            if (i < FIRE) {
                if (FLOAT(actions[i].address) < 1) FLOAT(actions[i].address) = 1;
            } else
                WORD(actions[i].address) = 1;
        }
    }
    /* Controller/script axes are sampled before adding the PC bindings. A
     * simultaneous stick keeps its original acceleration. Retain keyboard
     * ownership briefly after release so Responsive also shortens braking. */
    if (controller_move)
        InterlockedExchange(&keyboard_move_player, 0);
    else if (keyboard_move) {
        InterlockedExchange(&keyboard_move_until, (LONG)(GetTickCount() + 200));
        InterlockedExchange(&keyboard_move_player, (LONG)WORD(players + 4));
    }
    int dx = 0, dy = 0;
    xbox_FramebufferMouseDelta(&dx, &dy);
    /* The delta piles up whenever this does not run -- NETRICSA pages, the
     * outro, loading -- and the first tick back in play turned Sam by all of
     * it: steering the pointer to Continue on the statistics started the
     * next level looking at the pitch limit (and the level-start autosave
     * kept it). Movement from before play resumed is not look input. */
    static DWORD last_look;
    DWORD now = GetTickCount();
    if (now - last_look > 250) dx = dy = 0;
    last_look = now;
    /* ctl_fTurn* are angles, not stick deflections. Cancel the console
     * profile's sensitivity/invert here, then let Sam build the normal action.
     * Equal mouse counts turn equally at 50 or 300 fps, without a stick cap. */
    int sensitivity = *(int8_t *)PTR(profile + 0x22);
    float gain = 0.10f * powf(1.4f, (float)(speed - 5));
    float h = 1 + (float)sensitivity * (0.75f / 127), v = 1 + (float)sensitivity * (0.5f / 127);
    float pitch_sign = (WORD(profile + 0x1C) & 2) ? -1.0f : 1.0f;
    FLOAT(0x480484) += (float)dx * gain / h;
    FLOAT(0x480488) -= (float)dy * gain * (invert_y ? -1.0f : 1.0f) / v * pitch_sign;
    injected = 1;
    return 1;
}

void pc_controls_input_end(void)
{
    if (!injected) return;
    /* Keyboard-held buttons must not remain set in the console script state
     * after release; preserve a simultaneously held controller button. */
    for (int i = FIRE; i < PAUSE; ++i) WORD(actions[i].address) = saved_buttons[i - FIRE];
    injected = 0;
}

void pc_controls_update_movement(uint32_t player, uint32_t action)
{
    if (!enabled || !InterlockedCompareExchange(&responsive_movement, 0, 0)) return;
    uint32_t owner = (uint32_t)InterlockedCompareExchange(&keyboard_move_player, 0, 0);
    if (!owner) return;
    if ((LONG)(GetTickCount() - (DWORD)InterlockedCompareExchange(&keyboard_move_until, 0, 0)) >= 0) {
        InterlockedCompareExchange(&keyboard_move_player, 0, (LONG)owner);
        return;
    }
    if (!guest_range(player, 0x2BE8) || !guest_range(owner, 0x2BE8) || !guest_range(action, 12) ||
        WORD(player) != PLAYER_VTABLE || WORD(owner) != PLAYER_VTABLE) return;
    uint32_t flags = WORD(player + 0x10);
    /* Prediction copies of this local entity have the same persistent ID.
     * Remote players and unrelated entities retain the original rates. */
    if (player != owner && (!(flags & 0x20000u) || WORD(player + 0x1C) != WORD(owner + 0x1C))) return;
    /* Standing/crouching on a supporting entity only. The state can remain
     * standing for half a second after takeoff; +0x138 is the ground reference
     * that AliveActions uses to reset its fall timer. Keep the jump-action
     * check too, before that reference clears on the first airborne step. */
    if (!(flags & 8) || !WORD(player + 0x138) || WORD(player + 0x3D0) > 1 || WORD(player + PLAYER_CAMERA) ||
        WORD(player + 0x1074) || FLOAT(action + 4) != 0.0f) return;
    /* AliveActions has just restored the original rates (100 / 60). Raise
     * acceleration and braking enough to reach the same target speed within
     * one 50 ms simulation step; do not alter velocity or maximum speed. */
    if (FLOAT(player + 0x1A4) > 0 && FLOAT(player + 0x1A4) < 1000) FLOAT(player + 0x1A4) = 1000;
    if (FLOAT(player + 0x1A8) > 0 && FLOAT(player + 0x1A8) < 1000) FLOAT(player + 0x1A8) = 1000;
}
