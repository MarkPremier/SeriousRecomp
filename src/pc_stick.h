#ifndef SAM_PC_STICK_H
#define SAM_PC_STICK_H
#include <stddef.h>
/* Optional controller stick deadzone. Choice 0 is the original game's own
 * (about 40% per axis); the others are round deadzones of 5/10/15/20%. */
#define PC_STICK_CHOICES 5
void pc_stick_install(void);
int pc_stick_choice(void);
void pc_stick_set_choice(int choice);
void pc_stick_label(int choice, char *out, size_t size);
/* INI text <-> choice ("Original", "5" ... "20"); unknown text is Original. */
int pc_stick_parse(const char *text);
void pc_stick_map(short *x, short *y, float deadzone);
#endif
