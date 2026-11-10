#ifndef SAM_PC_CONTROLS_H
#define SAM_PC_CONTROLS_H
#include <stdint.h>

/* Sam-only input and menu integration. No disc files or console profiles change. */
void pc_controls_load(void);
void pc_controls_build_menu(void);
void pc_controls_options_layout(void);
void pc_controls_frame(void);
/* Sam's menu renderer drew a menu this frame (which menu is shown; see pc_controls.c). */
void pc_controls_menu_drawn(void);
/* Footer labels use the normal selected-text animation while hovered. */
int pc_controls_shortcut_hovered(uint32_t gadget);
/* Keyboard/mouse movie skipping only (normally from its own thread); any thread. */
void pc_controls_movie_frame(void);
int pc_controls_input_begin(uint32_t profile);
void pc_controls_input_end(void);
/* After the original local-player alive action chooses its movement rates. */
void pc_controls_update_movement(uint32_t player, uint32_t action);
void (*pc_controls_dispatch(uint32_t address))(void);
/* How much wider than 4:3 the 3D view is now: the Aspect ratio setting's
 * width over 4:3's in a level (and its pause menu), 1 in the front end.
 * Main thread; the projection wrapper in recomp_manual.c reads it. */
float pc_controls_view_wide(void);
#endif
