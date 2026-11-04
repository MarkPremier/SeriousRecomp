#include <xbox/xboxrecomp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

RECOMP_TLS uint32_t g_eax, g_ecx, g_edx, g_ebx, g_esi, g_edi, g_esp, g_ebp, g_seh_ebp, g_fs_base;
ptrdiff_t g_xbox_mem_offset;
volatile uint32_t g_icall_trace[16], g_icall_trace_idx;
volatile uint64_t g_icall_count;
#define WORD(va) (*(uint32_t *)((uintptr_t)g_xbox_mem_offset + (va)))
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
enum { DSOUND = 0x10000, STACK = 0x20000 };
static unsigned notifications;
void sub_00367844(void);
void sub_00367844_gen(void) { ++notifications; g_esp += 8; }
void sub_001A6FF0_gen(void) { abort(); }
void sub_0019F1E0_gen(void) { abort(); }
void sub_0019F5E0_gen(void) { abort(); }
void sub_0019D980(void) { abort(); }
void sub_0019D880(void) { abort(); }
void sub_002B7552(void) { abort(); }
void sub_002B78DF(void) { abort(); }
void sub_0019D290(void) { abort(); }
void sub_003098A0_gen(void) { abort(); }
void sub_00304F90(void) { abort(); }
void sub_003041E0_gen(void) { abort(); }
int xbox_GetPresentInterval(void) { return 0; }
static void run(uint32_t handle, uint32_t owner, unsigned expected)
{
    g_ecx = DSOUND; g_eax = 1; g_edx = 2; g_esp = STACK;
    WORD(STACK + 4) = handle;
    if (handle < 256) WORD(DSOUND + 0x328 + handle * 4) = owner;
    notifications = 0;
    sub_00367844();
    CHECK(notifications == expected && g_esp == STACK + 8);
    CHECK(g_ecx == DSOUND && g_eax == 1 && g_edx == 2);
    if (handle < 256) CHECK(WORD(DSOUND + 0x328 + handle * 4) == owner);
}
int main(void)
{
    g_xbox_mem_offset = (ptrdiff_t)calloc(1, 0x30000);
    CHECK(g_xbox_mem_offset != 0);
    run(0, 0, 0); run(255, 0, 0); /* Retired voices: no invalid Unlink. */
    run(0, 0x12340, 1); run(255, 0x23450, 1); /* Live notifications retained. */
    run(256, 0, 1); run(UINT32_MAX, 0, 1); /* Original bounds handling retained. */
    free((void *)g_xbox_mem_offset);
    puts("idle voice ownership and guest ABI checks passed");
    return 0;
}
