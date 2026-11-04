#ifndef SAM_PC_SETTINGS_H
#define SAM_PC_SETTINGS_H
#include <stddef.h>
enum { PC_RESOLUTION, PC_ASPECT, PC_DISPLAY, PC_VSYNC, PC_FRAMES, PC_AF, PC_AA, PC_VIDEO_DEFAULTS,
       PC_SETTINGS_ROWS };
void pc_settings_load(const char *controls_ini);
/* The chosen picture shape in a level, width over height (4:3 at least). */
float pc_settings_aspect(void);
const char *pc_settings_label(int row);
void pc_settings_value(int row, char *out, size_t size);
void pc_settings_step(int row, int direction);
void pc_settings_poll(void);
#endif
