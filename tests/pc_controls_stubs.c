/* Unrelated PC-menu integration for the existing focused wrapper regressions. */
#include <stdint.h>
#include <xbox/xboxrecomp.h>
extern RECOMP_TLS uint32_t g_esp;
unsigned pc_frame_calls,pc_menu_draw_calls;
uint32_t pc_hovered_shortcut,pc_text_effect;
unsigned pc_text_calls;
int pc_controls_shortcut_hovered(uint32_t gadget) {return gadget && gadget==pc_hovered_shortcut;}
void sub_0002A089_gen(void) {
    extern ptrdiff_t g_xbox_mem_offset;
    ++pc_text_calls;
    pc_text_effect=*(uint32_t *)((uintptr_t)g_xbox_mem_offset+g_esp+0x1C);
    g_esp+=4;
}
void pc_controls_build_menu(void) {}
void pc_controls_options_layout(void) {}
void pc_controls_frame(void) {++pc_frame_calls;}
void pc_controls_movie_frame(void) {}
void pc_controls_menu_drawn(void) {}
int pc_controls_input_begin(uint32_t profile) {(void)profile;return 0;}
void pc_controls_input_end(void) {}
void (*pc_controls_dispatch(uint32_t address))(void) {(void)address;return 0;}
void sub_00030710_gen(void) {}
void sub_00030D00_gen(void) {}
void sub_00070810_gen(void) {}
void sub_0002B6BE_gen(void) {++pc_menu_draw_calls;g_esp+=4;}
float pc_controls_view_wide(void) {return 1.0f;}
void sub_001D1C00_gen(void) {}
