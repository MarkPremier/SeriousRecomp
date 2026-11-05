/* Verify the fence workaround's ABI and submission contract without game data. */
#include <xbox/xboxrecomp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

RECOMP_TLS uint32_t g_eax, g_ecx, g_edx, g_ebx, g_esi, g_edi, g_esp, g_ebp, g_seh_ebp;
RECOMP_TLS uint32_t g_fs_base;
ptrdiff_t g_xbox_mem_offset;
volatile uint32_t g_icall_trace[16], g_icall_trace_idx;
volatile uint64_t g_icall_count;
#define WORD(va) (*(uint32_t *)((uintptr_t)g_xbox_mem_offset + (va)))
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
enum { DEVICE = 0x3143E0, FENCE = 0x320000, STACK = 0x100000, REGS = 0x330000 };
static unsigned submissions, waits;
static uint32_t expected_fence;
void sub_003098A0(void);

/* KickOff (sub_00309700) is translated in recomp_manual.c; it publishes PUT
 * through the register block at device +0x23BC. Its wait loop runs only while
 * 0x313C4C is set, which these tests leave clear. */
void sub_00367741_gen(void) { abort(); }
void sub_00368B2C_gen(void) { abort(); }
void sub_0030B700_gen(void) { abort(); }
void sub_00304F90(void) { abort(); }
void sub_003098A0_gen(void)
{
    CHECK(g_eax == 1 && g_ecx == 2 && g_edx == 3 && g_ebx == 4);
    CHECK(g_esi == 5 && g_edi == 6 && g_ebp == 7 && g_seh_ebp == 8);
    CHECK(g_esp == STACK && WORD(g_esp + 8) == 0x10);
    CHECK(WORD(FENCE) == expected_fence); /* Submission must not fake completion. */
    waits++;
    g_esp += 12; /* Original stdcall return owns the caller's two arguments. */
}
void sub_001A6FF0_gen(void) { abort(); }
void sub_0019F1E0_gen(void) { abort(); }
void sub_0019D980(void) { abort(); }
void sub_0019D880(void) { abort(); }
void sub_0019F5E0_gen(void) { abort(); }
void sub_002B7552(void) { abort(); }
void sub_002B78DF(void) { abort(); }
void sub_0019D290(void) { abort(); }
void sub_00367844_gen(void) { abort(); }
void sub_003041E0_gen(void) { abort(); }
int xbox_GetPresentInterval(void) { return 0; }

static void run(uint32_t requested, uint32_t completed, uint32_t flags,
                int pending, unsigned expected_submissions)
{
    memset((void *)g_xbox_mem_offset, 0, 0x400000);
    WORD(0x313C48) = DEVICE;
    WORD(DEVICE + 8) = flags;
    WORD(DEVICE) = 0x8000FD28;
    WORD(DEVICE + 0x2C) = pending ? 0x8000FC84 : WORD(DEVICE);
    WORD(DEVICE + 0x30) = 0x1D3;
    WORD(DEVICE + 0x34) = FENCE;
    WORD(DEVICE + 0x23BC) = REGS;
    WORD(FENCE) = expected_fence = completed;
    WORD(STACK + 4) = requested;
    WORD(STACK + 8) = 0x10;
    g_eax = 1; g_ecx = 2; g_edx = 3; g_ebx = 4;
    g_esi = 5; g_edi = 6; g_ebp = 7; g_seh_ebp = 8; g_esp = STACK;
    waits = 0;
    sub_003098A0();
    submissions = WORD(REGS + 0x40) == (0x8000FD28u & 0x0FFFFFFFu);
    CHECK(submissions == expected_submissions && waits == 1);
    CHECK(WORD(DEVICE + 0x2C) == (pending && !expected_submissions ? 0x8000FC84u : WORD(DEVICE)));
    CHECK(g_esp == STACK + 12);
}
/* Both KickOff branches, called directly: no busy-bit wait, the original's
 * stores and returned registers. */
static void kickoff_test(void)
{
    memset((void *)g_xbox_mem_offset, 0, 0x400000);
    WORD(DEVICE) = 0x8000FD28; WORD(DEVICE + 8) = 2; WORD(DEVICE + 0x23BC) = REGS;
    g_ecx = DEVICE; g_esp = STACK; WORD(STACK) = 0x1234;
    sub_00309700();
    CHECK(g_esp == STACK + 4 && g_ecx == DEVICE && g_eax == 0);
    CHECK(WORD(DEVICE + 0x2C) == 0x8000FD28 && WORD(REGS + 0x40) == 0x0000FD28);
    CHECK(!(WORD(DEVICE + 8) & 0x2000));
    WORD(DEVICE + 8) = 4; WORD(DEVICE + 0x35C) = 0x8000FE00; /* recorded: PUT from +0x35C */
    g_esp = STACK;
    sub_00309700();
    CHECK(WORD(DEVICE + 0x2C) == 0x8000FE00 && WORD(REGS + 0x40) == 0x0000FE00);

    memset((void *)g_xbox_mem_offset, 0, 0x400000);
    WORD(DEVICE) = 0x8000FD28; WORD(DEVICE + 8) = 0x2000;
    WORD(DEVICE + 0x30) = 0x1D3; WORD(DEVICE + 0x34) = FENCE;
    WORD(DEVICE + 0x2C10) = 0x55; WORD(DEVICE + 0x40) = 0x66;
    g_ecx = DEVICE; g_esp = STACK;
    sub_00309700();
    CHECK(g_esp == STACK + 4 && g_ecx == DEVICE && g_eax == DEVICE && g_edx == 0x66);
    CHECK(WORD(DEVICE + 0x23BC) == 0x313C00 && WORD(0x313C40) == 0x0000FD28 && WORD(0x313C44) == 0x0000FD28);
    CHECK(WORD(FENCE) == 0x1D1 && WORD(DEVICE + 0x257C) == 0x55 && WORD(DEVICE + 0x2BD8) == 0x66);
}

int main(void)
{
    g_xbox_mem_offset = (ptrdiff_t)calloc(1, 0x400000);
    CHECK(g_xbox_mem_offset != 0);
    run(0x1D1, 0x1CF, 2, 1, 1); /* Captured deadlock: issued but unsubmitted. */
    run(0x1D1, 0x1D1, 2, 1, 0); /* Already complete. */
    run(0x1D1, 0x1CF, 2, 0, 0); /* Already submitted. */
    run(0x1D1, 0x1CF, 6, 1, 0); /* Recorded pushbuffer: preserve original path. */
    run(0x1D3, 0x1CF, 2, 1, 0); /* Next fence: original routine emits it. */
    kickoff_test();
    free((void *)g_xbox_mem_offset);
    puts("fence submission and guest ABI checks passed");
    return 0;
}
