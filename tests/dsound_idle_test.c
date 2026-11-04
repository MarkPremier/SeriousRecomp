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
void sub_00367741(void);
static uint32_t work_first, work_ecx;
void sub_00367741_gen(void) { work_ecx = g_ecx; work_first = WORD(DSOUND + 0x740); }
void sub_00368B2C_gen(void) { abort(); }
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
/* A released voice (base vtable 0x3BCCE8) queued between two live ones is
 * unlinked before DirectSound's work runs; the live ones stay in order. */
static void link(uint32_t head, uint32_t node)
{
    uint32_t last = WORD(head + 4);
    WORD(node) = head; WORD(node + 4) = last; WORD(last) = node; WORD(head + 4) = node;
}
static void work_test(void)
{
    enum { LIVE1 = 0x12000, DEAD = 0x13000, LIVE2 = 0x14000 };
    uint32_t head = DSOUND + 0x740;
    for (uint32_t list = DSOUND + 0x728; list <= head; list += 8) WORD(list) = WORD(list + 4) = list;
    WORD(LIVE1) = 0x3BCDDC; WORD(DEAD) = 0x3BCCE8; WORD(LIVE2) = 0x3BCE00;
    link(head, LIVE1 + 0x54); link(head, DEAD + 0x54); link(head, LIVE2 + 0x54);
    link(DSOUND + 0x730, DEAD + 0x4C); /* also on a voice list */
    g_ecx = DSOUND;
    sub_00367741();
    CHECK(work_ecx == DSOUND && work_first == LIVE1 + 0x54);
    CHECK(WORD(LIVE1 + 0x54) == LIVE2 + 0x54 && WORD(LIVE2 + 0x54 + 4) == LIVE1 + 0x54);
    CHECK(WORD(LIVE2 + 0x54) == head && WORD(head + 4) == LIVE2 + 0x54);
    CHECK(WORD(DEAD + 0x54) == DEAD + 0x54 && WORD(DEAD + 0x58) == DEAD + 0x54);
    CHECK(WORD(DSOUND + 0x730) == DSOUND + 0x730 && WORD(DEAD + 0x4C) == DEAD + 0x4C);
}

int main(void)
{
    g_xbox_mem_offset = (ptrdiff_t)calloc(1, 0x30000);
    CHECK(g_xbox_mem_offset != 0);
    run(0, 0, 0); run(255, 0, 0); /* Retired voices: no invalid Unlink. */
    run(0, 0x12340, 1); run(255, 0x23450, 1); /* Live notifications retained. */
    run(256, 0, 1); run(UINT32_MAX, 0, 1); /* Original bounds handling retained. */
    work_test();
    free((void *)g_xbox_mem_offset);
    puts("idle voice ownership and guest ABI checks passed");
    return 0;
}
