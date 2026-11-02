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

## Boot movie visible — 2026-11-02

This supersedes the black-screen status above. The boot movie now renders in
the actual game window and finishes naturally at a visible main menu, with no
keyboard/controller input sent during the playback probe.

### Cause and fix

Temporary tracing around XMV GetNextFrame (`0x003176C0`) returned successful
frames. The destination argument is a D3D surface object, not a pixel pointer:
its Data field named contiguous allocations `0x00569000` / `0x005FF000`, with
640x480 packed YUY2 format `0x24`. Dumping through the runtime DMA resolver and
converting a captured frame showed a valid animated intro logo. Decoding was
working. PVIDEO overlay registers were inactive because this title draws the
movie as a textured quad.

The D3D11 pushbuffer path deliberately treated all 16-bit render targets as
depth-only shadow passes. Serious Sam's display target is RGB565, so movie
draws were counted as successful hardware batches without writing any colour.
GPU reports showed zero colour surfaces and zero texture uploads despite
hundreds of movie draws.

`nv2a_pb_exec.c` now sends RGB565 draws with colour writes enabled through its
existing CPU rasterizer on Windows. It synchronizes pending native writes
before reading/updating guest memory. Depth-only passes and other formats keep
their previous native path. This also compiles the relevant CPU primitive path
on Windows; merely rejecting the native batch was insufficient, since Windows
previously skipped unsupported batches instead of falling back.

The temporary XMV trace/dump wrapper was removed from both manual and generated
code after diagnosis. The final executable uses the original translated movie
decoder; no replacement movie playback or game-data patch is involved.

### Verification

- `boot_movie_cpu_visible`: visibly rendered the Croteam animation in the game
  window. Reported presentation rates were about 23–28 fps while initialization
  ran; this is no longer a count of black frames.
- `boot_movie_natural75`: later decoded frames continued advancing, the movie
  worker exited successfully, and the main menu became visible without input.
  Subsequent CPU colour rendering measured roughly 14–19 fps. The visible menu
  has clipping/layout problems and then stops advancing; gameplay remains
  unverified. Sound quality has not been listened to or validated.
- Final Release build passes. `boot_movie_final` tests the final executable
  after removing the diagnostic movie wrapper, with native DSP and no mailbox
  bypass. It also records successful natural movie-worker termination, then
  ends at the configured 75-second watchdog (exit 3), without a logged crash or
  game fatal. Its matching linker map is saved in the probe directory.
- Focused `nv2a_shader_smoke` regression passes with
  `NV2A_GPU_RGB565_ONLY=1`. It exercises the real Windows pushbuffer renderer,
  checks an RGB565 colour pixel and following-memory guard, and checks a packed
  YUY2 movie texture produces the expected red RGB565 pixel. It also checks the
  preceding 32-bit draw after explicitly synchronizing native publication.
- Kernel bridge regression suite passes (1/1).

The broader legacy shader smoke run did not pass during exploration (including
pushbuffer packet checks); it is not claimed as passing. The focused RGB565
mode isolates this change from those unrelated checks.

```powershell
$env:NV2A_GPU_RGB565_ONLY = '1'
& .\build\xboxrecomp\src\d3d\d3d8_smoke\Release\nv2a_shader_smoke.exe
```

CPU colour rendering is a correctness fallback, not native RGB565 acceleration.
Native colour support for 16-bit targets is a future performance improvement.
The separate post-movie menu stall and clipping also remain to be investigated.
Runtime commits are `870420d` (RGB565 colour rendering and regression) and
`5f8ea14` (optional native watchdog sampling), both local on `serious-sam`.

### Diagnostic checkpoint

An optional Windows x64 `RECOMP_WATCH_NATIVE=1` watchdog sample now captures the
guest main thread's actual native RIP/RSP and candidate return addresses. It
resumes the thread before logging, so a held CRT output lock cannot deadlock
the sampler. `boot_movie_natural75` captured RIP `0x145A78CEF` after menu startup;
that address belongs to its instrumented build, whose map was subsequently
replaced. Do not symbolize it against a newer map.

`boot-test.ps1` now saves the matching linker map as `executable.map` inside each
probe directory before launching, so later rebuilds will not invalidate future
watchdog/crash investigations. Decoded frame captures and the temporary trace
wrapper are ignored local artifacts under `analysis/movie_raw` and
`analysis/movie_trace_wrapper.c`. No game data is tracked and no GitHub pushes
or releases were made.

## Normal launcher and Turok workflow (2026-11-02)

The user asked to follow the completed Turok project's approach. Reviewed its
README, launcher, file locator and current runtime history read-only. The intended
milestones are an ordinary executable launch with working defaults and useful
logs, then correct menus and gameplay at native GPU performance. The RGB565 CPU
fallback remains an intermediate correctness measure.

Adapted Turok's game-file locator to Serious Sam's USA XBE SHA-1
`5463f81814860649ce6fae754e34d84b02ed93cb`, `Main.gro` and the movie archives.
Normal startup now selects and remembers the data folder, loads its XBE, and
publishes the same folder to the stream/settings wrappers. It defaults stream
prefetch and optional settings alongside the existing hardware defaults. The local
build uses prepared `test_game` as its developer fallback, with its own saves.
Portable discovery, image extraction and a picker are available, but the complete
fresh-disc installation path has not been verified: the successful local folder
contains the earlier loose assets, fonts and first-run files described above.
The extractor uses a unique temporary directory and never recursively deletes a
pre-existing game directory.

Explorer launches also create and rotate `serious_sam_log.txt` and
`serious_sam_log.prev.txt` beside the executable. Normal disk output is buffered;
fatal/exception reports flush explicitly. Existing redirected stderr is retained
for probes. Log buffering alone did not resolve the startup stall.

### Pending GPU fence

Plain launches exposed a timing-dependent stall during texture loading. Native
watchdog samples identify `sub_003098A0` (XDK BlockOnTime). A read-only process
memory capture gave these concrete values:

- Device `0x003143E0`, flags `2`, next CPU fence `0x1D3`, completed fence
  `[0x80007000] = 0x1CF`.
- Actual submitted/consumed DMA PUT/GET both `0xFC84`.
- The command buffer already contains semaphore release `0x1D1` at `0x8000FD10`,
  beyond PUT. The wait asks for this issued, unsubmitted work.

The manual `sub_003098A0` wrapper submits pending commands using the translated
`sub_00309700` KickOff before the original wait. It only does so for an outstanding
already-issued fence with unsubmitted data, outside pushbuffer recording mode.
Already completed/submitted fences and the next-fence case retain the original
path. Guest registers and the stack are restored before entering the original
wait. The wrapper never writes the completion value: the executor must process
the real semaphore command. The deeper scheduling cause between the movie and
asset-loader threads is not yet proven; this is a targeted bring-up workaround.

### Verification

- Release build succeeds. A normal launch with no probe environment visibly
  played Croteam's animation and naturally reached the partially rendered menu
  (frame 966). Its log/map are saved as `analysis/launcher_fence_fixed_first.*`.
- The final build narrows submission to outstanding fences. Launched this build
  by double-clicking the executable in the user's Release Explorer window;
  Croteam frames visibly advance, without a watchdog or probe settings. It
  naturally reaches the partial menu at frame 965 and stalls there. Its log and
  matching map are saved as `analysis/launcher_final_explorer.*`.
- `tests/fence_submit_test.c` exercises the captured pending-fence condition,
  verifies the guest ABI survives submission, verifies completion is not forged,
  and checks completed, submitted, recording and next-fence cases. Passes 1/1:

```powershell
& $cmake -S tests -B build/manual_tests
& $cmake --build build/manual_tests --config Release --parallel 6
ctest --test-dir build/manual_tests -C Release --output-on-failure
```

Menu layout/clipping and a subsequent stall remain unresolved. Gameplay and audio
quality remain unverified. No changes were made to Turok or its runtime worktree.

## Complete menus and native RGB565 performance (2026-11-02)

The previously incomplete menu was a frame stopped during visibility-query
retrieval, rather than a reason to alter menu positions. A read-only native thread
sample found repeated `sub_00304180` (GetVisibilityTestResult) calls from
`sub_001D8720`, waiting for report byte +12 to stop being `0xFFFFFFFF`.

Two runtime bugs prevented completion:

- GET_REPORT masks bits 24–31 for its type, but compared the still-shifted result
  with the unshifted ZPASS enum `1`. No valid ZPASS command could match.
- The report context was treated as a direct RAMIN offset when numerically small.
  Serious Sam uses handle `12`: actual RAMHT lookup resolves it to instance
  `0x11A0`, descriptor `0002B03D 10000000 00000003`. Reading RAMIN +12 instead
  produced an invalid descriptor. Report contexts now always resolve through RAMHT.

After these fixes, the full background, text and Sam animation render. The user
confirmed that Sam rising from below the screen is the original entrance
animation. No position/transform changes were made for it.

A later infinite wait at `0x00316944` revealed the missing D3D software-method
callback. The title's ISR invokes `sub_0030B9C0` with ECX `0x003167A0` and one stack
argument. Its type-5 case calls KeSetEvent at context +`0x1A4` (device +`0x2564`).
The launcher now registers this routine after dispatch initialization, following
Turok's integration pattern with Serious Sam's own addresses. This also restores
the driver's deferred pushbuffer fixups. A 100-second probe sustains menu animation
instead of sleeping forever in the completion event.

### Performance

RGB565 colour targets now use the native D3D11 renderer. Internally their colour
textures are BGRA8; uploads expand RGB565, and required guest-memory publications
pack it back while preserving guest pitch, row padding and written regions.
Snapshots and cache identity include the guest byte width. GPU rendering and
direct presentation avoid the old CPU per-pixel rasterizer. Native 16-bit passes
without colour writes retain the depth-only path. The CPU fallback remains
available with `RECOMP_CPU_RGB565=1` for comparison.

Measured with `RECOMP_FPS_LOG=5`, native DSP, the same prepared game data and no
per-frame trace:

| Probe | Sustained menu samples |
|---|---|
| `boot_menu_software_methods` — CPU RGB565 | approximately 10.7–12.1 fps |
| `boot_menu_native_rgb565` — native, automatic 3× scale | approximately 43.3–48.2 fps |
| `boot_menu_native_1x` — native, original resolution | approximately 43.7–48.8 fps |

The early movie samples in those logs are separate from menu performance. Lowering
render scale gives little menu benefit here, so automatic scaling remains the
default. More performance work should profile guest/DSP execution and synchronization
before sacrificing image resolution. These are menu measurements, not gameplay.

### Verification and remaining work

- Final x64 Release build succeeds. The normal executable, without probe settings,
  visibly reaches the complete animated menu and later enters its idle attract
  movie. The captured normal-run log/map are `analysis/menu_native_final_normal.*`.
- Focused `NV2A_GPU_REPORT_ONLY=1` shader test passes, including unsupported report
  type, the real small-handle lookup, ready/result fields and an adjacent guard.
- Focused `NV2A_GPU_RGB565_ONLY=1` passes on native rendering and with the CPU
  fallback enabled. It checks exact colour and YUY2 conversion, padded rows,
  trailing guards, clear/background preservation and a guest CPU edit between
  partial draws. A direct `nv2a_gpu_draw` assertion ensures native acceptance;
  silently falling back cannot satisfy that check.
- The normal-run startup log still contains occasional recoverable pushbuffer
  desync reports. They did not prevent this run reaching the complete menu, but
  stability across level loads remains unverified.
- Menu input and actual gameplay are not yet verified. Keyboard input attempts
  did not establish menu navigation; no claim of playable controls is made.
  The later action-looking screen was the recorded attract movie, not gameplay.
- Runtime commits: `2dd4d90` (query completion) and `f8f9936` (native RGB565).
  Turok and its runtime worktree remain unchanged. No pushes or releases were made.

## Controller input and first level (2026-11-02)

The launcher previously handled OHCI faults but never initialized the controller.
It now initializes host input and OHCI after mapping guest memory and defaults
USB on with four root ports, pad on root port index 2, and automatic XInput slot
selection, following Turok's bus setup. An Xbox/XInput pad is detected in host
slot 0. Live test reports injected through the existing USB controller path
verify D-pad and left-stick menu movement, A opening Options, B returning, Start
skipping the movie, and left-stick movement in Hatshepsut. The diagnostic input
file is only enabled by `RECOMP_PAD_LIVE`; ordinary launches use the host pad.

Selecting Options exposed a stream-save bug: committed guest RAM never invokes
the engine's access-violation handler, leaving its two-page flush window empty.
Close at `0x0019F5E0` now calls the original WritePage (`0x0019D880`) for all used
pages of writable file streams before the translated close releases the buffer.
Read-only and archive streams are excluded. Seek high-water and partial final
pages are retained. A focused regression publishes multiple patterned pages to a
temporary file before destroying the buffer and verifies exact bytes and guest
register/stack preservation, including a backwards seek and exact page boundary.

Update mode (`Open_t` mode 2) also needs existing data loaded before modifications.
Without this, appending the signature rewrote the prior contents as zero bytes.
The wrapper measures the original FILE with the translated seek/tell routines,
prefetches through the original ReadPage and retains the file high-water mark.
Settings now contain 49,152 bytes plus the 20-byte signature; profiles contain
4,194,304 bytes plus the signature. A profile initially contains the game's TEMP
placeholder; after a successful first level it begins `GAMESESV` and has a real
saved state. Diagnostic failed files/UDATA were preserved under `analysis/`.

A loader worker also opened message streams with its engine TLS stream flag
unset, causing an uncaught C++ exception. When the global stream system exists,
Open_t now invokes the translated per-thread stream initializer `0x0019D290`
on such a worker while preserving the caller's guest state. The prepared test
data additionally needs an empty optional `Data/ExcludedEditorModels.lst`.

`boot_first_level` reaches real Hatshepsut gameplay with HUD, pistol and rendered
world; forward input moves from the spawn platform toward the temple ramp.
Initial diagnostic captures are approximately 50 fps. They include GPU reporting
and sampling overhead and are not the final performance baseline. CPU sampling
finds the main thread fully occupied, with `recomp_fp_round24` a prominent self
hotspot; the APU/DSP thread is also busy. The user requests 120+ fps gameplay.
No claim of that target being achieved is made. Extended play, combat, audio
quality, clean archive-only setup, and stability across levels remain to verify.

## Release optimization and 120+ fps first-level measurements (2026-11-02)

The original local CMake cache had empty `CMAKE_C_FLAGS_RELEASE` (and empty C
flags for the other configurations). Selecting Release and enabling LTCG did
not itself enable the compiler optimizer. Native disassembly confirmed an
out-of-line helper with stack spills and a CRT memcpy call for an eight-byte
copy. C++ GPU code already had its normal Release optimization flags.

The project now explicitly adds `/O2`, `/Ob2`, and `NDEBUG` to Release builds
before adding the runtime subdirectory. This fixes both translated game C and
runtime C even with that existing cache. The executable shrank from approximately
106 MB to 31 MB. Floating-point precision behavior is unchanged; an experimental
rounding-helper rewrite gave no benefit on the unoptimized build and was removed.
The isolated runtime worktree has no changes from this experiment.

Measured with native DSP, native D3D11 RGB565, automatic 3× rendering on the
3440×1440 monitor, `RECOMP_FPS_LOG=5`, and no GPU/input reporting:

| Run / scene | Five-second FPS samples |
|---|---|
| `boot_gameplay_baseline`, active Hatshepsut spawn | 46.6–51.5 (mostly 48–49) |
| `boot_gameplay_optimized`, same active spawn | 150.3–186.3 |
| Same optimized run, moving onto ramp / active enemy | 194.6–204.5 |
| Normal executable launch, active spawn | 140.2–175.2 initially, 157.5–161.5 after resuming |

Loading movies, the loading-screen flyover, and paused frames are excluded from
the gameplay comparison. The 120+ fps target is reached in this first-level
sample, not established across the complete game. The normal launch reloaded the
signed GAME profile without a settings exception. A subsequent idle-voice crash
is tracked separately below; it must be fixed before calling this stable play.
