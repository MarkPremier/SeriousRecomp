/* Serious Sam manual dispatch and runtime diagnostics. */
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <xbox/xboxrecomp.h>

extern RECOMP_TLS uint32_t g_esp;
extern ptrdiff_t g_xbox_mem_offset;
extern void sub_001A6FF0_gen(void);
extern void sub_0019F1E0_gen(void);
extern void sub_0019D980(void);
extern void sub_0019D880(void);
extern void sub_002B7552(void);
extern void sub_002B78DF(void);
extern void sub_0019D290(void);
extern RECOMP_TLS uint32_t g_fs_base;
extern void sub_0019F5E0_gen(void);
extern void sub_00047B70_gen(void);
extern void sub_001A0420_gen(void);
extern void sub_003098A0_gen(void);
extern void sub_00309700(void);
extern RECOMP_TLS uint32_t g_eax, g_ecx, g_edx, g_ebx, g_esi, g_edi;
extern RECOMP_TLS uint32_t g_ebp, g_seh_ebp;

#define GUEST32(va) (*(uint32_t *)((uintptr_t)g_xbox_mem_offset + (uint32_t)(va)))

/* Writable CTFileStream pages normally enter the two-page flush window through
 * the engine's access-violation handler. Guest RAM is already committed here,
 * so that window stays empty. Publish every used page through the original
 * WritePage routine before Close releases the buffer and FILE. Read streams
 * (+0x5C != 0) and archive streams (no FILE at +0x50) are never written. */
void sub_0019F5E0(void)
{
    uint32_t stream = g_ecx;
    if (getenv("RECOMP_STREAM_PREFETCH") && stream >= 0x10000 &&
        stream < 0x08000000 - 0x60 && GUEST32(stream + 0x50) &&
        !GUEST32(stream + 0x5C)) {
        uint32_t base = GUEST32(stream + 0xC);
        uint32_t end = GUEST32(stream + 0x14);
        uint32_t high = GUEST32(stream + 0x1C);
        uint32_t limit = GUEST32(stream + 0x10);
        uint32_t page_size = GUEST32(0x004A1AD0);
        if (high > end) end = high;
        if (page_size && base >= 0x10000 && end >= base && end <= limit &&
            end <= 0x08000000) {
            uint32_t saved[] = {g_eax, g_ecx, g_edx, g_ebx, g_esi, g_edi,
                                g_esp, g_ebp, g_seh_ebp};
            uint32_t pages = (end - base + page_size - 1) / page_size;
            fprintf(stderr, "[STREAM] flush %u page(s), stream=%08X bytes=%u\n",
                    pages, stream, end - base);
            for (uint32_t page = 0; page < pages; ++page) {
                g_ecx = stream;
                g_esp = saved[6] - 8;
                GUEST32(g_esp) = 0;
                GUEST32(g_esp + 4) = page;
                sub_0019D880();
            }
            g_eax = saved[0]; g_ecx = saved[1]; g_edx = saved[2];
            g_ebx = saved[3]; g_esi = saved[4]; g_edi = saved[5];
            g_esp = saved[6]; g_ebp = saved[7]; g_seh_ebp = saved[8];
        }
    }
    sub_0019F5E0_gen();
}

/* XDK 4928 BlockOnTime can wait for an already-issued fence still in the
 * CPU's pending pushbuffer while the movie worker and asset loader run on
 * separate host cores. Submit that pending work through the title's own
 * KickOff before blocking. Completion still comes from the executor's real
 * semaphore release; never advance the fence value here. */
void sub_003098A0(void)
{
    uint32_t device = GUEST32(0x00313C48);
    uint32_t requested = GUEST32(g_esp + 4);
    if (device >= 0x10000 && device + 0x23C4 < 0x08000000 &&
        !(GUEST32(device + 8) & 4) &&
        requested != GUEST32(device + 0x30) &&
        (GUEST32(device + 0x30) - requested) <
            (GUEST32(device + 0x30) - GUEST32(GUEST32(device + 0x34))) &&
        GUEST32(device) != GUEST32(device + 0x2C)) {
        uint32_t saved[] = {g_eax, g_ecx, g_edx, g_ebx, g_esi, g_edi,
                            g_esp, g_ebp, g_seh_ebp};
        g_ecx = device;
        g_esp -= 4;
        GUEST32(g_esp) = 0;
        sub_00309700();
        g_eax = saved[0]; g_ecx = saved[1]; g_edx = saved[2];
        g_ebx = saved[3]; g_esi = saved[4]; g_edi = saved[5];
        g_esp = saved[6]; g_ebp = saved[7]; g_seh_ebp = saved[8];
    }
    sub_003098A0_gen();
}

/* FileExists uses an Open_t try/catch in the XBE. Preserve its false result
 * for an absent first-run GameSettings.gms without entering the throw path. */
void sub_001A0420(void)
{
    uint32_t name_object = GUEST32(g_esp + 4);
    uint32_t name = GUEST32(name_object);
    const char *root = getenv("RECOMP_GAME_DIR");
    if (root && getenv("RECOMP_OPTIONAL_DEFAULTS") &&
        name >= 0x10000 && name < 0x08000000 &&
        _stricmp((const char *)((uintptr_t)g_xbox_mem_offset + name),
                 "GameSettings.gms") == 0) {
        char path[MAX_PATH], override_path[MAX_PATH];
        snprintf(path, sizeof path, "%s\\GameSettings.gms", root);
        snprintf(override_path, sizeof override_path,
                 "%s\\TDATA\\54540004\\GameSettings.gms", root);
        if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES &&
            GetFileAttributesA(override_path) == INVALID_FILE_ATTRIBUTES) {
            fprintf(stderr, "[SETTINGS] first-run GameSettings.gms does not exist\n");
            g_eax = 0;
            g_esp += 4;
            return;
        }
    }
    sub_001A0420_gen();
}

/* First-run shell settings are optional: the original caller catches a missing
 * file and keeps its constructor defaults. Until guest C++ unwinding exists,
 * avoid entering that missing-file path; existing settings still use the game. */
void sub_00047B70(void)
{
    const char *root = getenv("RECOMP_GAME_DIR");
    if (root && getenv("RECOMP_OPTIONAL_DEFAULTS")) {
        char path[MAX_PATH], override_path[MAX_PATH];
        snprintf(path, sizeof path, "%s\\Data\\SeriousSam.gms", root);
        snprintf(override_path, sizeof override_path,
                 "%s\\TDATA\\54540004\\Data\\SeriousSam.gms", root);
        if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES &&
            GetFileAttributesA(override_path) == INVALID_FILE_ATTRIBUTES) {
            fprintf(stderr, "[SETTINGS] no shell settings file; retaining first-run defaults\n");
            g_esp += 4;
            return;
        }
    }
    sub_00047B70_gen();
}

/* Open_t reserves a lazy stream buffer; the console fills it through SEH page
 * faults. Committed guest RAM cannot fault here. Reuse the translated page-in
 * routine instead of reimplementing the engine's file/archive formats. */
void sub_0019F1E0(void)
{
    uint32_t stream = g_ecx, mode = GUEST32(g_esp + 8);
    if (getenv("RECOMP_STREAM_PREFETCH") && g_fs_base &&
        GUEST32(0x004A1AE4)) {
        uint32_t tls_vector = GUEST32(g_fs_base + 4);
        uint32_t tls_index = GUEST32(0x0047EF68);
        uint32_t tls = GUEST32(tls_vector + tls_index * 4);
        if (!GUEST32(tls + 0x10)) {
            uint32_t saved[] = {g_eax, g_ecx, g_edx, g_ebx, g_esi, g_edi,
                                g_esp, g_ebp, g_seh_ebp};
            g_esp -= 4;
            GUEST32(g_esp) = 0;
            sub_0019D290(); /* Enable the engine's stream context on this worker. */
            g_eax = saved[0]; g_ecx = saved[1]; g_edx = saved[2];
            g_ebx = saved[3]; g_esi = saved[4]; g_edi = saved[5];
            g_esp = saved[6]; g_ebp = saved[7]; g_seh_ebp = saved[8];
        }
    }
    sub_0019F1E0_gen();
    if ((mode == 1 || mode == 2) && getenv("RECOMP_STREAM_PREFETCH")) {
        uint32_t base = GUEST32(stream + 0xC), end = GUEST32(stream + 0x18);
        uint32_t page_size = GUEST32(0x004A1AD0);
        uint32_t saved[] = {g_eax, g_ecx, g_edx, g_ebx, g_esi, g_edi,
                            g_esp, g_ebp, g_seh_ebp};
        /* Append/update opens do not set a read end. Preserve the existing
         * bytes before a signature or an in-place edit is written into RAM. */
        if (mode == 2 && GUEST32(stream + 0x50)) {
            uint32_t file = GUEST32(stream + 0x50);
            g_esp = saved[6] - 16;
            GUEST32(g_esp) = 0;
            GUEST32(g_esp + 4) = file;
            GUEST32(g_esp + 8) = 0;
            GUEST32(g_esp + 12) = 2; /* SEEK_END */
            sub_002B7552();
            g_esp = saved[6] - 8;
            GUEST32(g_esp) = 0;
            GUEST32(g_esp + 4) = file;
            sub_002B78DF();
            if ((int32_t)g_eax >= 0 && g_eax <= 0x08000000 - base) {
                end = base + g_eax;
                GUEST32(stream + 0x1C) = end;
            }
        }
        if (page_size && base >= 0x10000 && end >= base && end < 0x08000000) {
            uint32_t count = (end - base + page_size - 1) / page_size;
            fprintf(stderr, "[STREAM] prefetch %u page(s), stream=%08X bytes=%u\n",
                    count, stream, end - base);
            for (uint32_t page = 0; page < count; page++) {
                g_ecx = stream;
                g_esp = saved[6] - 8;
                GUEST32(g_esp) = 0;
                GUEST32(g_esp + 4) = page;
                sub_0019D980();
            }
        }
        g_eax = saved[0]; g_ecx = saved[1]; g_edx = saved[2];
        g_ebx = saved[3]; g_esi = saved[4]; g_edi = saved[5];
        g_esp = saved[6]; g_ebp = saved[7]; g_seh_ebp = saved[8];
    }
}

/* Surface the game's fatal-error text before its guest CRT exit path runs. */
void sub_001A6FF0(void)
{
    const uint8_t *mem = (const uint8_t *)g_xbox_mem_offset;
    const uint32_t *args = (const uint32_t *)(mem + g_esp + 4);
    if (args[0] >= 0x10000 && args[0] < 0x08000000)
        fprintf(stderr, "[GAME FATAL] %.1024s; args=%08X %08X %08X %08X\n",
            (const char *)(mem + args[0]), args[1], args[2], args[3], args[4]);
    fflush(stderr);
    sub_001A6FF0_gen();
}

extern volatile uint32_t g_icall_trace[16];
extern volatile uint32_t g_icall_trace_idx;
extern volatile uint64_t g_icall_count;

typedef void (*recomp_func_t)(void);

recomp_func_t recomp_lookup_manual(uint32_t xbox_va)
{


    (void)xbox_va;
    return (recomp_func_t)0;
}

void recomp_icall_fail_log(uint32_t va)
{
    fprintf(stderr, "[ICALL] Failed to resolve VA 0x%08X (total calls: %llu)\n",
            va, (unsigned long long)g_icall_count);


    fprintf(stderr, "  Recent ICALL targets:\n");
    for (int i = 0; i < 16; i++) {
        int idx = (g_icall_trace_idx - 16 + i) & 15;
        if (g_icall_trace[idx])
            fprintf(stderr, "    [%2d] 0x%08X\n", i, g_icall_trace[idx]);
    }
    fflush(stderr);
}

void recomp_icall_not_code_log(uint32_t va)
{
    enum { SLOTS = 16 };
    static uint32_t seen[SLOTS];
    static uint64_t hits[SLOTS];
    static int count;
    int i;

    for (i = 0; i < count; i++)
        if (seen[i] == va)
            break;
    if (i == count) {
        if (count == SLOTS)
            return;
        seen[count] = va;
        hits[count] = 0;
        count++;
    }
    hits[i]++;

    {
        uint64_t n = hits[i];
        while (n >= 10 && n % 10 == 0)
            n /= 10;
        if (n != 1)
            return;
    }
    fprintf(stderr, "[ICALL] target 0x%08X is not code -- skipped %llu time(s) "
                    "(null or wild function pointer, at call #%llu)\n",
            va, (unsigned long long)hits[i],
            (unsigned long long)g_icall_count);
    fflush(stderr);
}

#include <stdlib.h>

void recomp_unimpl(const char *text, uint32_t va)
{
    static int printed;
    const char *trap = getenv("RECOMP_UNIMPL_TRAP");
    int stop = trap && *trap && *trap != '0';

    if (printed < 50 || stop) {
        printed++;
        fprintf(stderr,
                "[UNIMPL] untranslated instruction REACHED: `%s` at 0x%08X"
                " (a no-op; set RECOMP_UNIMPL_TRAP=1 to stop here)\n",
                text, va);
        fflush(stderr);
    }
    if (stop) abort();
}
