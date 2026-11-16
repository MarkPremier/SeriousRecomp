#ifndef SAM_PC_CHEATS_H
#define SAM_PC_CHEATS_H
#include <stdint.h>
#include <stddef.h>

enum {
    CHEAT_PAUSE_MENU, CHEAT_HEALTH, CHEAT_ARMOR, CHEAT_AMMO, CHEAT_SPEED,
    CHEAT_DAMAGE, CHEAT_SUPER_JUMP, CHEAT_DOUBLE_JUMP, CHEAT_FLY,
    CHEAT_UNLOCK, CHEAT_FIRE_RATE, CHEAT_SNIPER, CHEAT_RESET, CHEAT_ROWS
};
void pc_cheats_load(const char *path);
const char *pc_cheats_label(int row);
void pc_cheats_value(int row, char *out, size_t size);
void pc_cheats_step(int row, int direction);
int pc_cheats_pause_menu(void);
int pc_cheats_single_player(void);
void pc_cheats_frontend(int main_menu);
void pc_cheats_player(void (*original)(void), void (*give_all)(void));
void pc_cheats_damage(void (*original)(void));
void pc_cheats_receive_damage(void (*original)(void));
void pc_cheats_timer(void (*original)(void));
void pc_cheats_native_tick(void (*original)(void));
#endif
