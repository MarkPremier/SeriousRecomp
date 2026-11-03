#ifndef SAM_PC_CONTROLS_H
#define SAM_PC_CONTROLS_H
#include <stdint.h>

/* Sam-only input and menu integration. No disc files or console profiles change. */
void pc_controls_load(void);
void pc_controls_build_menu(void);
void pc_controls_options_layout(void);
void pc_controls_frame(void);
int pc_controls_input_begin(uint32_t profile);
void pc_controls_input_end(void);
void (*pc_controls_dispatch(uint32_t address))(void);
#endif
