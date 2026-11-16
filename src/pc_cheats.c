/* Title-specific optional cheats. Layouts and call sites are from the USA XBE;
 * no public engine implementation is used. All effects recheck the session. */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <xbox/xboxrecomp.h>
#include "pc_cheats.h"

extern ptrdiff_t g_xbox_mem_offset;
extern RECOMP_TLS uint32_t g_eax, g_ecx, g_edx, g_ebx, g_esi, g_edi, g_esp, g_ebp, g_seh_ebp;
extern RECOMP_TLS double g_fp_stack[8];
extern RECOMP_TLS int g_fp_top;
#define CW(a) (*(uint32_t *)((uintptr_t)g_xbox_mem_offset + (uint32_t)(a)))
#define CF(a) (*(float *)((uintptr_t)g_xbox_mem_offset + (uint32_t)(a)))
#define NET 0x4A2AE0u
#define PLAYER_VT 0x3ABCD0u
static const struct { const char *key, *label; int max; } options[CHEAT_ROWS] = {
    {"pause_menu", "Show in pause menu", 1},
    {"health", "Unlimited health (200)", 1},
    {"armor", "Unlimited armor (200)", 1},
    {"ammo", "Unlimited ammo", 1},
    {"speed", "Movement speed", 4},
    {"damage", "Damage multiplier", 4},
    {"super_jump", "Super jump", 1},
    {"double_jump", "Double jump", 1},
    {"fly", "Flying", 1},
    {"unlock", "All weapons and levels", 1},
    {"fire_rate", "Fire rate", 4},
    {"sniper", "Skip sniper reload", 1},
    {NULL, "Disable all cheats", 0}
};
static int values[CHEAT_ROWS];
static const float multipliers[] = {1, 1.5f, 2, 3, 5};
static char cheats_ini[MAX_PATH];
static uint32_t jump_player, granted_player, granted_weapons;
static int jump_held, extra_jump;
static DWORD owner_thread;
static int valid(uint32_t a, uint32_t n) { return a >= 0x10000 && a < 0x08000000 && n <= 0x08000000 - a; }

static uint32_t network(void) {
    uint32_t net = CW(NET);
    return valid(net, 0xA68) ? net : 0;
}
int pc_cheats_single_player(void) {
    uint32_t net = network();
    /* CNetworkLibrary::GetSessionProperties is lea eax,[ecx+0x9CC].
     * The USA single-player initializer (0x80040) sets +0x10 to one and
     * +8 to zero. Cooperative multiplayer also sets +0x0C, so that flag
     * alone must not admit a multiplayer session. */
    return GetCurrentThreadId() == owner_thread && net && CW(net + 0x9CC + 0x10) == 1;
}
static uint32_t local_player(void) {
    uint32_t net = network(), session, players, player;
    if (!pc_cheats_single_player()) return 0;
    session = CW(net + 0x14);
    if (!valid(session, 0x14)) return 0;
    players = CW(session + 0x10);
    if (!valid(players, 8) || !CW(players)) return 0;
    player = CW(players + 4);
    return valid(player, 0x2AA0) && CW(player) == PLAYER_VT ? player : 0;
}
static int is_player(uint32_t p) { return p && p == local_player(); }
static uint32_t weapons(uint32_t p) {
    uint32_t w = CW(p + 0x390);
    return valid(w, 0x348) && CW(w + 0x108) == p ? w : 0;
}
void pc_cheats_load(const char *path) {
    snprintf(cheats_ini, sizeof cheats_ini, "%s", path);
    owner_thread = GetCurrentThreadId();
    for (int i = 0; i < CHEAT_RESET; ++i) {
        int v = (int)GetPrivateProfileIntA("cheats", options[i].key, 0, cheats_ini);
        values[i] = v >= 0 && v <= options[i].max ? v : 0;
    }
    jump_player = granted_player = granted_weapons = 0;
    jump_held = extra_jump = 0;
}
const char *pc_cheats_label(int row) { return row >= 0 && row < CHEAT_ROWS ? options[row].label : ""; }
void pc_cheats_value(int row, char *out, size_t size) {
    if (row < 0 || row >= CHEAT_ROWS || row == CHEAT_RESET) snprintf(out, size, "");
    else if (row == CHEAT_SPEED || row == CHEAT_DAMAGE || row == CHEAT_FIRE_RATE)
        snprintf(out, size, "%gx", multipliers[values[row]]);
    else snprintf(out, size, "%s", values[row] ? "On" : "Off");
}
static void save_options(void) {
    char v[16];
    for (int i = 0; i < CHEAT_RESET; ++i) {
        snprintf(v, sizeof v, "%d", values[i]);
        WritePrivateProfileStringA("cheats", options[i].key, v, cheats_ini);
    }
}
void pc_cheats_step(int row, int direction) {
    if (row < 0 || row >= CHEAT_ROWS) return;
    if (row == CHEAT_RESET) {
        /* Menu visibility is a preference, independent of gameplay effects. */
        for (int i = CHEAT_HEALTH; i < CHEAT_RESET; ++i) values[i] = 0;
    } else values[row] = (values[row] + (direction < 0 ? options[row].max : 1)) % (options[row].max + 1);
    if (!values[CHEAT_UNLOCK]) granted_player = granted_weapons = 0;
    save_options();
}
int pc_cheats_pause_menu(void) { return values[CHEAT_PAUSE_MENU]; }

/* The level selector reads this native symbol. Restore only our own override;
 * do not clear levels the player had already unlocked by the console cheat. */
void pc_cheats_frontend(int main_menu) {
    static uint32_t saved_unlock;
    static int owned;
    if (GetCurrentThreadId() != owner_thread) return;
    if (values[CHEAT_UNLOCK] && main_menu) {
        if (!owned) { saved_unlock = CW(0x47C5E0); owned = 1; }
        CW(0x47C5E0) = 1;
    } else if (owned && (!values[CHEAT_UNLOCK] || !main_menu)) {
        CW(0x47C5E0) = saved_unlock;
        owned = 0;
    }
}
/* Invoke an original thiscall helper without leaking its scratch registers. */
static void give_weapons(void (*fn)(void), uint32_t w) {
    uint32_t regs[] = {g_eax,g_ecx,g_edx,g_ebx,g_esi,g_edi,g_esp,g_ebp,g_seh_ebp};
    g_esp -= 4; CW(g_esp) = 0; g_ecx = w; fn();
    g_eax=regs[0];g_ecx=regs[1];g_edx=regs[2];g_ebx=regs[3];g_esi=regs[4];g_edi=regs[5];
    g_esp=regs[6];g_ebp=regs[7];g_seh_ebp=regs[8];
}
void pc_cheats_player(void (*original)(void), void (*give_all)(void)) {
    uint32_t p = g_ecx, action = CW(g_esp + 4);
    if (!is_player(p) || !valid(action, 44) || CW(p + 0x3F0) || !(CF(p + 0xE8) > 0)) {
        jump_player = 0;
        original(); return;
    }
    uint32_t speed = CW(0x407A98), jump = CW(0x407AE0), fly = CW(0x480754);
    uint32_t props = network() + 0x9CC, ammo = CW(props + 0x50);
    if (values[CHEAT_SPEED]) CF(0x407A98) = multipliers[values[CHEAT_SPEED]];
    if (values[CHEAT_SUPER_JUMP]) CF(0x407AE0) *= 2;
    if (values[CHEAT_FLY]) CW(0x480754) = 1;
    if (values[CHEAT_AMMO]) CW(props + 0x50) = 1;
    if (values[CHEAT_HEALTH]) CF(p + 0xE8) = 200;
    if (values[CHEAT_ARMOR]) CF(p + 0x37C) = 200;
    uint32_t w = weapons(p);
    if (values[CHEAT_UNLOCK] && w && (granted_player != p || granted_weapons != w || (CW(w + 0x120) & 0x7FFF) != 0x7FFF)) {
        give_weapons(give_all, w); granted_player = p; granted_weapons = w;
    }
    if (values[CHEAT_AMMO] && w) {
        /* The original GiveAll copies each ammo capacity (odd word) to its
         * count (even word). Keep the inventory/weapon mask intact here. */
        for (uint32_t a = 0x300; a <= 0x340; a += 8) CW(w + a) = CW(w + a + 4);
    }
    int down = CF(action + 4) > 0.5f;
    int ground = CW(p + 0x138) != 0;
    if (jump_player != p) { jump_player = p; jump_held = down; extra_jump = 0; }
    if (ground) extra_jump = 0;
    int boost = values[CHEAT_DOUBLE_JUMP] && !values[CHEAT_FLY] && !ground && down && !jump_held && !extra_jump;
    jump_held = down;
    original();
    if (boost && CF(p + 0xE8) > 0) {
        CF(p + 0x348) = CF(0x407AE0); extra_jump = 1;
    }
    if (values[CHEAT_HEALTH] && CF(p + 0xE8) > 0) CF(p + 0xE8) = 200;
    if (values[CHEAT_ARMOR] && CF(p + 0xE8) > 0) CF(p + 0x37C) = 200;
    CW(0x407A98)=speed;CW(0x407AE0)=jump;CW(0x480754)=fly;CW(props+0x50)=ammo;
}
void pc_cheats_damage(void (*original)(void)) {
    uint32_t source = CW(g_esp + 4);
    int apply = values[CHEAT_DAMAGE] && is_player(source);
    original();
    if (apply) g_fp_stack[g_fp_top & 7] *= multipliers[values[CHEAT_DAMAGE]];
}
void pc_cheats_receive_damage(void (*original)(void)) {
    uint32_t p = g_ecx;
    int apply = is_player(p) && CF(p + 0xE8) > 0;
    if (apply && values[CHEAT_HEALTH]) {
        /* This is CPlayer's damage receiver, vtable +0xE4. Avoid the death
         * path even for a single hit exceeding 200, without reviving corpses. */
        CF(p + 0xE8) = 200;
        if (values[CHEAT_ARMOR]) CF(p + 0x37C) = 200;
        g_esp += 24; /* five thiscall arguments and the return address */
        return;
    }
    if (apply && values[CHEAT_ARMOR]) CF(p + 0x37C) = 200;
    original();
    if (apply && values[CHEAT_ARMOR] && CF(p + 0xE8) > 0) CF(p + 0x37C) = 200;
}
void pc_cheats_timer(void (*original)(void)) {
    uint32_t w = g_ecx, arg = g_esp + 4, caller = CW(g_esp);
    uint32_t saved = CW(arg);
    if ((values[CHEAT_FIRE_RATE] || values[CHEAT_SNIPER]) && valid(w, 0x348) && is_player(CW(w + 0x108)) && weapons(CW(w + 0x108)) == w &&
        caller >= 0xCA000 && caller < 0xE0000 && isfinite(CF(arg)) && CF(arg) > 0) {
        float delay = CF(arg);
        if (values[CHEAT_FIRE_RATE]) {
            delay /= multipliers[values[CHEAT_FIRE_RATE]];
            if (delay < 0.001f) delay = 0.001f;
        }
        /* The two timed post-shot sniper states: bolt cycle and recovery.
         * Retain one simulation tick so transitions cannot spin in one tick. */
        if (values[CHEAT_SNIPER] && (caller == 0xDB9C2 || caller == 0xDBCA5)) delay = 0.05f;
        CF(arg) = delay;
    }
    original(); CW(arg) = saved;
}
void pc_cheats_native_tick(void (*original)(void)) {
    uint32_t saved = CW(0x480754);
    if (values[CHEAT_FLY] && is_player(g_ecx)) CW(0x480754) = 1;
    original(); CW(0x480754) = saved;
}
