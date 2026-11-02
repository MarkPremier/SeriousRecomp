/* Exercise page publication before the original close destroys the buffer. */
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
enum { STREAM = 0x10000, STACK = 0x11000, BUFFER = 0x20000, PAGE = 4096 };
static unsigned writes, closes;
static FILE *disk;
void sub_0019F5E0(void);

void sub_0019D880(void)
{
    uint32_t page = WORD(g_esp + 4), end = WORD(STREAM + 0x14);
    uint32_t high = WORD(STREAM + 0x1C), offset = page * PAGE, length;
    CHECK(g_ecx == STREAM && g_esp == STACK - 8 && WORD(g_esp) == 0);
    CHECK(page == writes);
    if (high > end) end = high;
    length = end - BUFFER - offset;
    if (length > PAGE) length = PAGE;
    CHECK(fseek(disk, offset, SEEK_SET) == 0);
    CHECK(fwrite((void *)(g_xbox_mem_offset + BUFFER + offset), 1, length, disk) == length);
    ++writes;
    g_eax = g_ecx = g_edx = g_ebx = g_esi = g_edi = g_ebp = g_seh_ebp = 0xBAD;
    g_esp += 8;
}
void sub_0019F5E0_gen(void)
{
    CHECK(g_eax == 1 && g_ecx == STREAM && g_edx == 3 && g_ebx == 4);
    CHECK(g_esi == 5 && g_edi == 6 && g_ebp == 7 && g_seh_ebp == 8);
    CHECK(g_esp == STACK);
    ++closes;
    /* Closing really releases the buffer: flushing afterwards cannot work. */
    memset((void *)(g_xbox_mem_offset + BUFFER), 0xEE, PAGE * 3);
    g_esp += 4;
}
void sub_001A6FF0_gen(void) { abort(); }
void sub_003041E0_gen(void) { abort(); }
int xbox_GetPresentInterval(void) { return 0; }
void sub_0019F1E0_gen(void) { abort(); }
void sub_0019D980(void) { abort(); }
void sub_002B7552(void) { abort(); }
void sub_002B78DF(void) { abort(); }
void sub_0019D290(void) { abort(); }
void sub_00367844_gen(void) { abort(); }
void sub_003098A0_gen(void) { abort(); }
void sub_00309700(void) { abort(); }

static void run(uint32_t file, uint32_t readonly, uint32_t cursor,
                uint32_t high, unsigned expected_pages, uint32_t expected_bytes)
{
    unsigned char actual[PAGE * 3], expected[PAGE * 3];
    for (unsigned i = 0; i < sizeof expected; ++i) expected[i] = (unsigned char)(i * 37 + 11);
    memcpy((void *)(g_xbox_mem_offset + BUFFER), expected, sizeof expected);
    WORD(STREAM + 0x50) = file;
    WORD(STREAM + 0x5C) = readonly;
    WORD(STREAM + 0xC) = BUFFER;
    WORD(STREAM + 0x10) = BUFFER + 0x8000000; /* Xbox reserve exceeds backed heap. */
    WORD(STREAM + 0x14) = BUFFER + cursor;
    WORD(STREAM + 0x1C) = BUFFER + high;
    WORD(0x4A1AD0) = PAGE;
    g_eax = 1; g_ecx = STREAM; g_edx = 3; g_ebx = 4;
    g_esi = 5; g_edi = 6; g_ebp = 7; g_seh_ebp = 8; g_esp = STACK;
    writes = closes = 0;
    disk = tmpfile(); CHECK(disk != NULL);
    sub_0019F5E0();
    CHECK(writes == expected_pages && closes == 1 && g_esp == STACK + 4);
    rewind(disk);
    CHECK(fread(actual, 1, sizeof actual, disk) == expected_bytes);
    CHECK(memcmp(actual, expected, expected_bytes) == 0);
    fclose(disk);
}
int main(void)
{
    g_xbox_mem_offset = (ptrdiff_t)calloc(1, 0x500000);
    CHECK(g_xbox_mem_offset != 0);
    _putenv_s("RECOMP_STREAM_PREFETCH", "1");
    run(1, 0, PAGE * 2 + 13, 0, 3, PAGE * 2 + 13);
    run(1, 0, 10, PAGE + 7, 2, PAGE + 7); /* Seek backwards retains high water. */
    run(1, 0, PAGE, 0, 1, PAGE); /* No extra page at an exact boundary. */
    run(1, 0, 0, 0, 0, 0);
    run(1, 1, PAGE + 7, 0, 0, 0); /* Read-only streams remain untouched. */
    run(0, 0, PAGE + 7, 0, 0, 0); /* Archive/no FILE. */
    _putenv_s("RECOMP_STREAM_PREFETCH", "");
    run(1, 0, PAGE + 7, 0, 0, 0);
    free((void *)g_xbox_mem_offset);
    puts("stream publication and guest ABI checks passed");
    return 0;
}
