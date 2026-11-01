# First native build — 2026-11-01

The current target is native Windows x64 with the fork's D3D11 NV2A renderer.
The user chose D3D11 first; Linux/Vulkan work is deferred.

## Verified

- Original USA disc already extracted. XBE entry `0x0003D755`, title
  `0x54540004`, XDK 4928.
- Dedicated toolkit worktree `P:\Recomp\xboxrecomp_ss`, branch `serious-sam`,
  based on `1d9e929`. No Turok source/runtime edits were made.
- Local `.venv` with Capstone 5.0.7; upstream disassembler run with bytecode
  writes disabled and explicit per-game output paths.
- Full disassembly: 1,292,133 instructions, 34,814 function entries.
- Function identification and ABI analysis completed; 2,342 thiscall entries.
- All entries translated: 0 failed, 2,544,138 lines of C in 140 chunks,
  plus dispatch and 12 unresolved direct-call stubs.
- Visual Studio 2026 x64 Release compilation and linking passed, including
  all runtime libraries. Executable: `build/Release/serious_sam_recomp.exe`.
- Boot test used copied disc files in `test_game/`, saves in `test_game/saves/`,
  `RECOMP_FB_WINDOW=0`, `RECOMP_WATCHDOG_SECS=10`, `RECOMP_FPS_LOG=5`.
  It loaded the XBE, initialized kernel/APU/dispatch, and entered guest code.

## Current startup failure

The watchdog dumped state after 10 seconds and ended the process (exit 3).
There is no verified menu or rendered frame. Logs are local ignored files:
`stdout.txt`, `stderr.txt`, `build_log.txt`, `recomp_log.txt`.

The first unresolved indirect target was `0x002BAEA8`, an x87 helper
(`fstp; fstp; fldz; ret`). Many later failures are valid aligned thunks such as
`0x00303410` (`mov ecx, 0x004C11E0; jmp 0x001F90A0`) and `0x002FEDA0`
(`mov ecx, 0x0047DB08; jmp 0x0001A0F0`). These bytes were inspected in the XBE.
They appear to include static object cleanup callbacks. This is evidence of
missing dispatch entries, not yet proof of why startup takes a cleanup path.

The toolkit's `tools.seed_from_log` collected 97 executable/body-checked
candidates into `seeds/runtime_feedback.json`, separate from manual seeds.
It also captured thread context entry points. The current binary was built
before that feedback, so rerun `tools/generate.ps1` and rebuild before retesting.

Next steps:

1. Regenerate with both seed files, rebuild, and repeat the isolated boot test.
2. Check the earliest remaining startup error, rather than assuming later
   cleanup callbacks are the original cause. Inspect CRT initialization if the
   game still enters cleanup.
3. Review the 12 unresolved direct-call stubs individually; one target is
   `0xEA3AEFBC`, so blindly seeding every stub is inappropriate.
4. Enable a visible framebuffer only once boot progresses far enough to test
   graphics. Headless boot does not validate D3D11 rendering.

## Launcher changes

The template referenced upstream-only `xbox_VehMmioFault`, which the fork does
not implement. The launcher now routes APU and OHCI faults through the fork's
actual handlers. It defaults pushbuffer execution, AC97 readiness and framebuffer
presentation on, accepts game/save path overrides, and gives normal Serious Sam
launches a dedicated project-local save root.
Removed example overrides from the manual source comments so the generator's
address scanner does not treat template example addresses as real references.

No Croteam GPL source was copied or adapted. No game data or generated code is
tracked. No GitHub pushes or releases were made.

## Follow-up: kernel import lookbehind

The first boot reported six "extra" kernel imports with impossible ordinals
such as 1364322432. Inspection of the original XBE shows these addresses belong
to the tail of XPP machine code (`0x003A9808` through `0x003A9828`), immediately
before the real import table at `0x003A9840`. The runtime's lookbehind scan was
rewriting any word with its high bit set, including these instruction bytes.

The isolated `xboxrecomp_ss` runtime now restricts extra imports to nonzero
ordinals within the kernel export range. The kernel bridge regression harness
passes with a real extra import across a NULL separator and checks that ordinal
zero, an out-of-range ordinal, and an actual Serious Sam XPP word are preserved.
This proves the image-corruption fix; it does not establish the startup stall's
root cause. The original Turok runtime worktrees were not modified.

## Continuation checkpoint — 2026-11-02

Work is confined to `serious_sam` and `xboxrecomp_ss`. The prior session finished
with project commit `354e21d`; it is not a competing active session. Runtime
import-scan changes were committed locally as `e944891` and `5cfd762`. Additional
runtime and project changes below are saved in the working trees, not yet committed.

### Verified progress

- Regenerated with the 97 observed seeds: 35,103 functions translated, zero
  failures, 10 unresolved direct-call stubs. The full pipeline took about 14 minutes.
- Native Release build passes. A 640x480 framebuffer window opens according to
  the runtime log. No menu or gameplay has been visually verified.
- Fixed `KeWaitForMultipleObjects` resolving guest dispatcher-object VAs as host
  handles. It now resolves shadow timers and title-created events. The kernel
  bridge regression harness passes, including timer timeout/signal and stdcall
  cleanup checks. This fixed the multimedia-timer initialization fatal error.
- Launcher enables `RECOMP_TITLE_KEVENTS`, `RECOMP_HEAP_RECLAIM`, and
  `RECOMP_NV2A_NATIVE_FENCES` by default. Reclamation removed the game's virtual
  memory release fatal; native fences released the first D3D initialization wait.
- Serious Engine reserves stream buffers and relies on page faults to fill them.
  The runtime's committed RAM returned zeros instead. The game-side Open_t wrapper
  at `0x0019F1E0` calls the translated page-in routine `0x0019D980` when
  `RECOMP_STREAM_PREFETCH=1`. Fonts and hundreds of scripts now load successfully.
  No Croteam source was copied or adapted; this uses the translated XBE routine.
- Fatal-error wrapper `0x001A6FF0` prints the game's own message. Arithmetic
  exceptions now produce diagnostics too. `RECOMP_EXCEPTION_TRACE=1` prints
  C++ exception text in the runtime; guest C++ unwinding remains unimplemented.

### Current test configuration and limitation

`tools/boot-test.ps1` uses `test_game` and its own saves, captures output beneath
`analysis/boot_<label>`, and enforces a watchdog/host timeout. It enables stream
prefetch, first-run setting fallbacks, and exception diagnostics. Example:

```powershell
pwsh -File P:\Recomp\serious_sam\tools\boot-test.ps1 -Seconds 30 -Label next -Visible -DspPassthrough
```

`-DspPassthrough` sets `RECOMP_DSP_ACK=0x803A4810`: a diagnostic bypass of the
observed DirectSound command-word wait. It does not emulate DSP command results
or establish working audio, and the address must be rechecked if allocation order
changes. Without it, the current build waits in `sub_00367052` on that mailbox.

Local probes added empty optional files in `test_game`: `DefaultMod.txt`,
`Scripts/PersistentSymbolsX.ini`, `Scripts/Game_startup.ini`, and
`Temp/ConsoleHistory.txt`. All 4,331 Main.gro entries were also extracted as loose
files there, preserving files already present. Standard font files were copied
under `test_game/TDATA/54540004/Fonts` for an isolation probe. These are ignored
test artifacts; original extracted disc files and archives are unchanged.
Whether the loose files/font overrides are still necessary after prefetch has
not been checked.

### Immediate next step

The last executed test (`analysis/boot_settings`) reached a missing optional
`T:\GameSettings.gms` after the shell-settings fallback. Inspection showed its
FileExists helper (`0x001A0420`) opens the file in a try/catch and should return
false when absent; the missing exception dispatch incorrectly lets it continue
as though the file existed. A narrow wrapper now returns false for an absent
first-run `GameSettings.gms`, letting the original caller take its default/save
path. This wrapper **has built but has not yet been boot-tested**. Run the probe
above next and inspect the first C++ exception/fatal, if any.

The other first-run wrapper, `0x00047B70`, retains constructor defaults only when
both shell-settings paths are absent. Both fallbacks require
`RECOMP_OPTIONAL_DEFAULTS=1` (the boot-test script sets it).

All wrapper generated bodies were renamed locally to `_gen`, with exact
`extern void sub_XXXXXXXX_gen(void);` declarations in `recomp_manual.c`.
Regeneration with `--exclude-manual` reproduces those wrapper/body names.
Generated chunks were backed up under `analysis/*_wrapper_backup/` before edits.
No GitHub pushes or releases were made.

## Continuation results — 2026-11-02 (second session)

This section supersedes the earlier immediate-next-step checkpoint. All work
remained in `serious_sam` and `xboxrecomp_ss`; Turok worktrees were only read.

### Verified fixes

- Boot-tested the absent `GameSettings.gms` wrapper. `boot_continue_settings`
  logs both first-run fallbacks, then loads menu textures, player models and
  sounds without a C++ exception or game fatal. The 30-second watchdog exits 3.
- Traced the subsequent main-thread wait to `sub_00026B20`: it waits for the
  boot movie callback to finish. The movie worker was inside D3D Present,
  waiting for the guest event at `0x00316934`. A worker stack dump showed
  `sub_003041E0`, `sub_003097F0`, `sub_003110C0`, `sub_00311100`, and the
  `sub_001A9160` presentation path. Enabling `RECOMP_VBLANK=1` produces claimed
  GPU interrupts and advancing frame counters. The launcher now enables it.
- Switched the APU initializer to `mcpx_apu_init_standalone_mapped` with
  `xbox_ApuPhysicalPointer`. The previous flat RAM pointer missed contiguous
  DMA allocations. `boot_mapped_dsp` and `boot_mapped_native75` pass the initial
  audio command wait with the DSP56300 engine enabled and **without**
  `RECOMP_DSP_ACK` or `RECOMP_APU_DSP_ACK`. APU interrupts are claimed by the
  title. This establishes startup progress, not correct audible output.
- Corrected `-DspPassthrough` in `tools/boot-test.ps1` to also set
  `RECOMP_APU_DSP=0`. Previously it acknowledged a mailbox while still running
  the DSP interpreter; the flat-memory version raised a Windows assertion in
  `dsp_cpu.c:981`. No assertion was observed in the mapped-memory native probes.
- Release build and kernel bridge regression suite pass. The latter includes
  the previous session's guest KTIMER timeout/signal and stdcall cleanup checks.
  Runtime commits: `5317b3c` (multiple-object waits), `ebbdae4` (diagnostics).

### Rendering and remaining blocker

- `boot_visible_passthrough`: 90 seconds, advancing counters at roughly 32 fps,
  but the framebuffer was visually black and the boot movie did not finish.
- `boot_mapped_native75`: the native DSP path also showed black output, with
  counters around 16–31 fps. These are presentation counters, not validated
  game-performance measurements. No menu or gameplay has been verified.
- Backed up isolated saves under `analysis/saves_before_movie_input`, then sent
  one Enter (Start) through the observed game window during `mapped_native75`.
  The movie worker terminated successfully, and new texture setup methods
  appeared. Frames then stopped advancing. The watchdog sampled guest ESP
  `0x00F7EFC8`; saved stack entries include `0x001D87BF` (return from
  `sub_00304180`) and `0x001CE28D`. This is a checkpoint, not a proven hang site:
  cached guest registers may be stale in a tight translated loop. No crash,
  unresolved indirect-call report, or game fatal identifies the cause yet.
- The runtime's current pushbuffer presenter reads PVIDEO registers directly
  from the mapped aperture. An NV2A MMIO-hook initialization probe was removed
  after checking that implementation; the final launcher needs no extra GPU
  model allocation or PVIDEO page trap.

Next: investigate the first draw/texture path after the movie is skipped, using
the native DSP configuration. Capture the native instruction pointer or add a
targeted trace before assuming the saved guest stack names the active loop.
Keep movie decoding/black output as a separate unresolved issue.

### Reproduction and diagnostics

```powershell
pwsh -File tools/boot-test.ps1 -Seconds 30 -Label native -Visible
```

The probe sets stream prefetch, first-run defaults and exception tracing; those
remain explicit bring-up options. `-DspPassthrough` is optional, no longer needed
for the initial audio wait, and remains an incomplete diagnostic bypass.

Runtime diagnostics now include caller/guest ESP and matching returns for the
first 20 `NtWaitForSingleObjectEx` calls. `RECOMP_WATCH_STACK=<guest address>`
adds a bounded 1 KiB memory sample to the watchdog, checking mapped ranges before
each read. Example movie-worker range for the earlier allocation order:
`0x0159CC00`; recheck the spawned-worker stack log before reusing it.

The final build removes the temporary NV2A initialization; `boot_native_final`
tests that exact executable with native DSP and no mailbox bypass. All logs,
generated chunks, disc data and test-save backups remain ignored local artifacts.
The final 30-second probe exits through the watchdog (3), with no logged crash,
fatal, C++ exception or unresolved indirect call. Its movie stream advances to
`0x00403000` bytes (44 read requests), compared with only two requests in the
earlier passthrough snapshot. The movie is progressing internally, although its
visual output is still black.
