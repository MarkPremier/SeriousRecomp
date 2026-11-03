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

## Takeover: exception handling and clean installs (2026-11-02, Claude Code)

Codex ran out of credits mid-task; this section picks up from its last state.

### DirectSound idle-voice guard (committed `c17fe13`)

Codex's uncommitted wrapper for `sub_00367844` was correct: the crash at return
`0x00367891` was Unlink with `this=0` for a voice whose owner had left
DirectSound's table at `+0x328`. Its `ret 4` handling matches the generated
body (`esp += 8; return;`). With it, `boot_audio_idle_fixed` ran 50 s further.

That run then crashed at `sub_002B6E33 + 0x269`, the C runtime's heap
allocator (called from the engine's file loading, `0x00197B9B` -> `0x00194DF9`),
reading guest `0xFFFFFFF5` with `esi = 0`. This looks like heap corruption or a
bad free and is **not yet investigated**. It happened after firing in
Hatshepsut (`rt:10000`), about 105 s into the run.

### Release flags

The old `build/` cache had every `CMAKE_C_FLAGS_*` empty, including Debug and
the base `/DWIN32 /D_WINDOWS`, from a broken first configure. A fresh configure
gets `/O2 /Ob2 /DNDEBUG`. The forced options are replaced by a configure-time
check that stops with instructions if `/O2` is missing.

### Guest C++ exceptions (toolkit `3f45f57`)

`RtlRaiseException` used to log a throw and return to the throw site. Serious
Engine reports a missing file by throwing and catching, so every optional file
on `T:` needed a placeholder or a wrapper. The toolkit now:

- walks the SEH chain for C++ throws as the kernel does, pushing the
  dispatcher's own registration that `_UnwindNestedFrames` and
  `_JumpToContinuation` (`0x002B4625`) expect;
- in the generator, finds try-block registrations through their
  `mov eax, FuncInfo; jmp __CxxFrameHandler` (`0x002B46AE`) thunks, labels each
  catch continuation, and arms a native `setjmp` anchor where the registration
  is linked into `fs:[0]`;
- turns `_JumpToContinuation`'s `jmp eax` into a `longjmp` back to that anchor.

Coverage: 160 anchors; 181 of 190 continuations (167 by label, 14 by tail call
where the analysis split the continuation into its own function). The other 9
are inside a different piece of a split function and log
`[EH] continuation ... is not a label` if reached.

The two settings wrappers (`sub_001A0420` GameSettings.gms, `sub_00047B70`
shell settings) said they existed "until guest C++ unwinding exists" and are
removed, along with `RECOMP_OPTIONAL_DEFAULTS`. They also had a bug: they
tested `getenv(...)` for presence, so `RECOMP_OPTIONAL_DEFAULTS=0` still
enabled them.

### Clean install verified

`test_clean/` holds only `default.xbe`, `Main.gro` and `Movies0-5.gro`
(hard links to the extracted disc). `clean_play3`: empty save folder, no
placeholders or wrappers. Player Name -> intro story -> Hatshepsut with HUD and
knife at about 185 fps. Seven throws, all caught: `DefaultMod.txt`,
`Scripts\PersistentSymbolsX.ini`, `Scripts\Game_startup.ini`,
`Temp\ConsoleHistory.txt`, `Data\SeriousSam.gms`,
`Data\ExcludedEditorModels.lst`, and "Chunk ID validation failed" (first-run
settings).

### Open: intermittent fence stall at the boot movie

4 of 15 clean runs froze during the Croteam movie while the main thread loaded
textures. Native samples put the main thread in `sub_003098A0_gen` (XDK
BlockOnTime) every time: requested fence `0x4E5`, completed `0x4E3`, GPU idle
(`PUT == GET`). The executor writes every semaphore release it sees without
filtering, so the releases for `0x4E4`/`0x4E5` never reached it. Codex's
wrapper only covers a fence issued but not yet kicked off at entry. The
prepared `test_game/` (loose files, faster loads) did not stall in 4 runs on
the same build, which points to a timing race between the movie worker and
the loader rather than to exception handling. Next step: dump the device
(`0x003143E0`: `+0` put, `+0x2C`, `+0x30` next fence) and the pushbuffer
around `PUT` when it stalls (`RECOMP_WATCH_STACK=0x003143E0` is wired up for
this but did not reproduce in 5 runs).

## Thread identity and intro-race investigation (2026-11-02, Codex)

Toolkit commit `6c87b8c` fixes a synchronization defect found while investigating
the intro stall. The title's XAPI `GetCurrentThreadId` at `0x0003E560` reads
`fs:[0x28]`, then `[record + 0x12C]`. The main record's ID was zero and workers
inherited the same record pointer. A live pre-fix memory read confirmed
`[0x4028] = 0x00760000`, `[0x0076012C] = 0`.

Serious Engine's recursive lock routines (`0x001F76A0`, `0x001F75F0`) compare
that ID with the owner at lock `+4`. Distinct threads therefore mistook an
already-held lock for recursion and entered together. This can race shared
engine state. It is a plausible contributor to the intro submission race and
shell-parser/allocator failure, not proof of either failure's cause.

The loader now stamps the main record with its host thread ID. Each worker's
TIB allocation includes a private current-thread record with that worker's ID.
The existing shared RW pointer and private image TLS/SEH initialization are
preserved. The allocator's existing no-TLS early return is unchanged; this
regression covers images with a TLS directory, including Serious Sam.

Validation:

- `tests/thread_identity` in the toolkit loads a synthetic XBE, starts three
  workers and checks distinct nonzero IDs, private records, the shared RW
  compatibility pointer, SEH heads and recursive-lock exclusion. The test fails
  against the original committed memory implementation and passes with the fix.
- Release game build succeeds. Kernel bridge regression passes; all three game
  manual-wrapper regressions pass.
- Memory regressions: default, heap reclaim and 128 MB extended-VMA modes pass.
  The 64 MB extended-VMA test fails eight assertions because it never selects
  64 MB and the runtime defaults to 128 MB. An isolated build using the original
  committed memory implementation reproduces the same eight failures. No memory
  allocation or RAM-size policy was changed by this fix.
- Before the fix, 20 archive-only startup probes of 25–30 s, plus two longer
  probes, did not reproduce the freeze this session. After the fix, 20 more
  30-second probes completed with advancing fences and no detected stall or
  crash. Each run uses archive-only assets in `test_clean` and its own empty
  save directory. Startup FPS samples were 31.4–32.3; these are not gameplay
  measurements and establish no performance improvement.
- A separate 75-second launch from `build/Release`, with all inherited
  `RECOMP_*` switches cleared except an isolated save directory, FPS logging
  and the watchdog, automatically discovered `test_clean`. Frame presentation
  continued throughout with no crash; later samples were 26.7–29.2 fps. The
  phase was not visually verified. This confirms normal file discovery and
  continuing presentation, not menu correctness or playable performance.

Evidence is retained in `analysis/intro_race_baseline*`,
`analysis/intro_identity_fixed`, `analysis/test_identity_baseline.txt`, and
`analysis/build_thread_identity*.txt`; the normal-launch logs are under
`analysis/intro_identity_normal`. The local `analysis/repro_intro.py`
monitors next/completed fences and ring PUT/GET, captures a stalled device and
ring, and leaves only a detected stalled probe alive for inspection.

Actual-window verification is pending this session: the Windows Computer Use
helper fails to connect to its native pipe, including after its documented
retry/reset procedure. Fence progress and FPS logs are diagnostic evidence,
not a substitute for visual confirmation. Do not close the intro-freeze or
post-firing-crash issue yet, or claim a menu/movie performance improvement.

## Options screen: bounded clears and RGB565 (2026-11-02)

Toolkit `0938eda` fixes the cyan Options background and missing upper rows in
the user's screenshot. The same defect reproduced in a fresh-save pause menu
and the main menu, on both D3D11 and the diagnostic CPU RGB565 path.

The slider code was requesting sensible small fills. The pushbuffer contained
inclusive clear edges, for example x=351..436, y=174..187, and packed RGB565
colour `0xFFE0` for yellow or `0xCCC0` for gold. The executor recorded
`SET_CLEAR_RECT_HORIZONTAL/VERTICAL` but ignored them when clearing. Every
slider erased the whole screen, including the preceding labels and background.
It also interpreted the packed colour as ARGB; `0x0000CCC0` produces cyan.

The executor now intersects the requested clear rectangle with the surface
clip, handles inclusive maximum edges, and preserves surrounding pixels. The
software path writes the packed 16-bit clear value directly; the D3D11 adapter
expands RGB565 to its ARGB clear API. Native partial clears use the existing
scissored clear shader, preserving GPU rendering rather than reading back the
whole surface for each slider. Full clears retain the fast path. Depth/stencil
clears use the same bounded region and keep the unselected component.

Validation:

- New toolkit `tests/nv2a_clear`: real NV097 dispatch and pixel-byte checks for
  RGB565, ARGB, inclusive rectangles, unchanged surrounding pixels/pitch padding,
  off-surface rectangles, later full clears, Z16, and separate Z24/stencil
  clears. Both CPU and GPU/readback tests pass. An isolated build using the
  original executor fails both at pixel (0,0), which was wrongly overwritten.
- Release game builds successfully. Temporary diagnostics in generated chunk 1
  were removed; its hash matches the pre-investigation backup.
- `options_fixed1` visibly restores the green background, Music Volume, FX
  Volume, Headphones, Brightness, Blood Color and Gibs. Controller input changes
  sliders and toggles without corrupting the layout.
- The fixed normal executable automatically discovers `test_clean`. Using only
  the probe's save directory, `options_normal_fixed` reloads Music Volume 0.7,
  Headphones Yes, altered brightness, Blood Color Hippie and Gibs Off after
  restart. The original user profile was not used for these changes.
- X opens the default-settings confirmation; choosing Yes restores the normal
  slider values, Headphones No, Blood Color Red and Gibs On, with the layout
  and background intact.
- A normal-launch smoke test reaches Hatshepsut with world, HUD and weapon
  intact. Opening Options from the gameplay pause menu also shows the complete
  green layout. This is a short rendering check, not extended combat validation.
- The optional older `nv2a_texture_cache` harness cannot link: it lacks stubs
  for `nv2a_pb_exec_batch_spans` and `xbox_FramebufferSize`. Those references
  predate this change. The new clear regression links the real runtime and
  passes; no unrelated test harness was changed.

Captures are **runtime framebuffer dumps**, not Windows window captures. They
match the user's supplied screenshot before the fix. Local evidence:
`analysis/options_before.bmp`, `analysis/options_after.bmp`,
`analysis/options_probe1`, `analysis/options_cpu2`, `analysis/options_trace1`,
`analysis/options_fixed1`, `analysis/options_normal_fixed`, and
`analysis/test_nv2a_clear*.txt`. Diagnostics enable readbacks and dumps, so
their FPS values are not a new performance baseline.

The fixed executable and matching map are installed under `build/Release`.
The previous exe is backed up at `analysis/serious_sam_before_options.exe`;
renaming it allowed updating the launch path without terminating the user's
running old session. Relaunch to use the fix. CMake's temporary diagnostic
output-directory override was removed. No Turok worktree was modified; its
shadow maps have not been visually retested with this isolated runtime fix.

## Menu performance and video presentation (2026-11-03)

The matched menu benchmark improved from **31.5 fps to 76.8 fps** at automatic
3× rendering. Measurements use `RECOMP_FPS_LOG=5` without frame dumps, batch
reports or sampling, on the animated main menu. The old build ranged from
28.8–33.4 fps with worst frames of 92–202 ms; the optimized build ranged from
72.1–89.0 fps with worst frames of 19–34 ms. These are local menu measurements,
not proof of sustained 120+ fps throughout gameplay.

Two changes account for this improvement:

- Toolkit `668b7f5` forces the common 24-bit x87 rounding path inline. Ordinary
  float-range magnitudes take one range comparison and a float conversion;
  unusual exponents keep the original exponent-preserving slow path. Precision
  and host rounding modes are unchanged. Four million binary64 patterns plus
  boundary, NaN, infinity and signed-zero cases match the original helper under
  all four host rounding modes. Math alone measured 34.6 fps.
- The engine's SwapBuffers call to BlockUntilVerticalBlank returns immediately
  when the native presenter is configured without vsync. Native stack sampling
  found 50 of 80 main-thread snapshots in that wait. The override checks the
  specific return address `0x001A95B8`; direct XMV wait sites and unknown callers
  retain the original function. Vsync intervals 1–4 retain the wait too. Toolkit
  `b83918f` exposes the existing presentation interval to the title wrapper.

The four game wrapper tests and both precision tests pass. A separate generation
into `analysis/menu_gen_verify` confirms that `--exclude-manual` retains the
original generated wait body and produces the optimized precision header:
35,103 functions, zero translation failures, ten unresolved direct-call stubs.
Runtime framebuffer captures in `analysis/menu_perf_normal_visual` show the
complete animated menu. Dump/readback runs are not performance baselines.

The user confirmed that movies showing 1000+ fps still animate at normal speed.
An older diagnostic run already showed repeated full-screen 640×480 YUY2 draws
at 1400–1900 fps; this was not introduced by the menu wait override. Video
presentation now has a separate `RECOMP_VIDEO_FPS_CAP`, default **200** in the
Serious Sam launcher. Set it to `0` to disable. The executor identifies actual
full-screen/letterboxed packed-YUV draws rather than a stale texture binding,
and also handles the PVIDEO overlay. A normal menu/world draw removes the video
limit. The decoder and audio clocks are unchanged. A lower global numeric cap
still takes precedence; explicit global `vblank` remains supported.

The initial 60 fps trial held 60.0 fps across six measured periods, but the user
reported stutter and requested 120, then 200. The 120 trial settled at
119–120 fps. Deadline tests cover 60/120/200 pacing,
late frames, changing caps, entering video and returning to unlocked rendering.
Empty flips retain the preceding surface's video classification, avoiding a
deadline reset between video draws. The installed build's logging-only attract
movie trial averaged **195.0 fps**, ranging 190.2–197.9 across five complete
five-second periods. This is presentation throughput, not decoded frame rate.
The exe SHA-256 is
`A3C7BC14AE15420B35B56B8974EA2D581DCAA1E700FC73781AEEAB6D3C33140B`;
its matching map is installed under `build/Release`. The preceding menu-optimized
exe is `analysis/serious_sam_before_video_cap.exe`. The temporary CMake output
override was removed. Toolkit video pacing is commit `94e9418`.

Evidence: `analysis/menu_perf_baseline4/measurement.json`,
`analysis/menu_fp_fast1/measurement.json`,
`analysis/menu_native_pacing1/measurement.json`,
`analysis/menu_perf_scene2/stacks_main.txt`, `analysis/video_cap_attract1`, and
`analysis/video_cap120_attract1`, and `analysis/video_cap200_final`. All probes use separate save directories and
archive-only `test_clean` assets. The speculative high-resolution vblank timer
experiment regressed performance and was discarded; it is not in this build.

### Gameplay profiling with the 200 fps video build

The normal installed exe auto-discovers archive-only `test_clean`. A fresh-save
script reaches Hatshepsut through Select Player, skill selection and the NETRICSA
Start prompt. Runtime framebuffer dumps show the world, HUD and pistol intact
(`analysis/gameplay_profile_visual1`). No video cap is applied to this scene.

A separate run without framebuffer dumps or renderer reports averages **184.9
fps** at spawn (182.1–187.0 over six complete five-second periods). After a
six-second forward-stick input, the next measurement has median 161.8 fps,
range 157.9–221.1; the changing view makes its 172.6 fps average a less useful
comparison. These are two short first-level samples, not sustained combat or
whole-game validation. Evidence is in
`analysis/gameplay_perf200_baseline1/measurement_spawn.json` and
`measurement_after_forward.json`.

Native unwound sampling is performed separately from those FPS measurements.
Of 160 title-thread samples, 18 stop in the straight-line matrix/vector helper
`0x00231BF0`, 15 in `0x00228970`, and 10 each in `0x0021DEF0` and the model
rendering routine `0x001C6840`. Most samples belong to the scene-rendering call
tree under `0x002272B0`. The executor thread is waiting in its own loop in 109
of 160 samples; DMA ownership checks and GPU ordering waits each account for
nine samples. This points first to CPU-side geometry/model work, rather than
render resolution, as the next optimization target. It does not measure GPU
hardware utilization.

In particular, the hot leaf helper repeatedly loads/stores the TLS-backed x87
stack and masks its index between arithmetic instructions. A narrowly guarded
generator optimization that keeps straight-line leaf FPU state in native
locals/registers is a promising next step. It needs bitwise differential tests
covering every initial stack position, all stack slots, guest rounding modes,
registers and guest-memory effects before enabling it. No such generator change
is included in this build, and guest C++ exceptions/call-boundary FPU state must
continue to work. Stacks: `stacks_title.txt` and `stacks_gpu.txt` beside the
measurements. No GPL engine code was copied.

## Upright stains and light halos through walls (2026-11-03)

The user supplied two gameplay screenshots: a blood stain standing vertically
above the floor, and torch halos visible through walls. Both have concrete
implementation defects, with separate fixes.

### Extended x87 operands and stain pitch

`fld xword ptr [0x00464F82]` in the original CRT asin endpoint loads an 80-bit
pi/2 constant. The lifter handled 4- and 8-byte operands, then incorrectly fell
back to `MEMF` for a 10-byte operand. Its low four bytes decode to approximately
`7.886e-19`, so asin(+/-1) returned nearly zero. The stain placement's ground
normal became nearly zero pitch instead of +/-90 degrees.

The runtime header now converts the explicit-significand x87 format to the
existing binary64 stack, with one rounding step, including directed rounding,
subnormals, signed zero, infinities and NaNs. The lifter emits extended loads
and stores for 10-byte operands. This does not change the stack to full 80-bit
arithmetic or relax the existing precision rules.

Validation:

- Twelve lifter FPU tests pass, including 10-byte operand dispatch.
- All three precision tests pass. The new extended-memory test covers the exact
  pi/2 encoding, rounding boundaries and one million finite binary64 round trips
  through extended storage across all four host rounding modes.
- `analysis/angle_probe` compiles the actual generated CRT and direction-to-angle
  functions against original disc constants. Both 24- and 53-bit guest precision
  modes return asin(+/-1)=+/-pi/2 and ground-normal pitch +/-90 degrees, with the
  expected guest stack balance. Replacing only extended loads with the previous
  float loads reproduces the near-zero result and fails the same test.
- Full generation still translates 35,103 functions with zero failures and ten
  unresolved direct calls. Only four chunks plus generated headers differ from
  the preceding generation. Original generated files are backed up in
  `analysis/gen_before_ext80`. Final generation is independently compared below.

### Real occlusion counts

`report_zpass` previously returned `clip_w * clip_h` regardless of depth tests.
`CLEAR_REPORT_VALUE` and `SET_ZPASS_PIXEL_COUNT_ENABLE` did nothing. This made
every tested light visible to the title, including lights behind walls.

The native backend now brackets counted draws with D3D11 occlusion queries,
accumulates across enable/disable intervals until reset, and normalizes scaled
and multisampled target samples to guest pixels. Reports wait for their actual
results. CPU raster survivors contribute separately. Result and timestamp are
written before the completion word is published.

The native regression draws a depth-writing wall, verifies zero samples behind
it and 1024 visible pixels in front, even with colour/depth writes disabled.
It also checks cumulative reads, disabled counting, reset while enabled and 2x
rendering. Both CPU and GPU clear/report tests pass, exercising real NV097 method
dispatch, the small RAMHT handle, report DMA resolution, unknown report types,
completion and preservation of following memory. An empty query returns zero.

Local evidence: `analysis/build_effects_query.txt`, `analysis/effects_query1`,
`analysis/gen_ext80_log.txt`, `analysis/angle_probe`, and
`analysis/build_effects_final*.txt`. Captures are runtime framebuffer dumps;
Windows Computer Use still cannot connect to its native pipe after retry/reset.
The isolated run reached gameplay and killed enemies, but the user's specific
stain/halo views have not been visually reverified yet. The user offered to test
the new build. Do not mark both screenshot issues visually verified from these
mathematical/GPU regressions alone, or use capture-run FPS as a benchmark.

Only the isolated Serious Sam runtime and project were changed. Croteam's public
source was consulted to identify effect routines; no GPL source was copied or
adapted. No game files or generated game C are staged for commit.

Toolkit commits: `9e5281d` (extended operands) and `2b25ed3` (occlusion).
Independent final regeneration under `analysis/gen_ext80_verify` is byte-for-byte
identical to every installed generated file. The Release build succeeds and is
installed with its matching map at `build/Release/serious_sam_recomp.exe`, SHA-256
`0ACD1AB409F1E2957183F945D4E0F30B35102FE371978DD59257F3F54648D646`.
The preceding executable is `analysis/serious_sam_before_effects.exe`.
CMake's temporary output-directory override was removed. The existing 200 fps
video default and unlocked menu/gameplay policies remain in place. No new
logging-only gameplay performance comparison was made for real occlusion yet;
query completion now waits for GPU results and its cost needs measurement.

## Slow-area readback and visibility stalls (2026-11-03 evening)

The user reported loading-screen presentation around 23 fps and a Hatshepsut
combat view dropping to 34–45 fps. A read-only save snapshot was taken while
the game was closed (`analysis/perf_save_snapshot`), retaining sparse files with
Robocopy `/SPARSE:Y`. Before/after instances use separate copies and the user
positioned the camera in the slow view with their Xbox controller. No scripted
input or test instance wrote to the original save folder.

Logging-only measurements, with no framebuffer captures or renderer timing:

| View | Mean fps | Five-second period range |
|---|---:|---:|
| Before, user-positioned slow view | 37.2 | 33.6–40.8 |
| After, same area/view | 142.9 | 140.2–145.5 |

Each measurement lasts 15 seconds and discards the first partial period,
leaving two complete five-second periods. This is a short live comparison,
not a deterministic replay; actors and animation continue. The user independently
reported about 140 fps in this view and about 300 elsewhere after the update.
Do not generalize that result to every level or heavy combat situation.

Before sampling, native unwound stacks were captured separately from FPS
measurement. The GPU executor had 56/80 samples under full synchronization,
including 38 publishing RGB565 pixels to CPU RAM, plus 11 in synchronous ZPASS
reads. The title had 36/80 samples in D3D fence waits. A read-only native target
cache inspection showed two views of the same 640×480 depth allocation: one
1920×1440 and one 640×480. Colour-masked visibility/model depth passes were
being classified as new unscaled shadow targets. Switching back to colour
created a conflicting alias, forcing publication of the whole dirty frame.

Toolkit `92bb1fb` records actual scene-depth ownership and reuses its scale in
later depth prepasses/queries. Separate true depth-only shadow targets remain
unscaled. ZPASS reports now queue immutable counter snapshots and complete
through nonblocking polling even if PUT stops. Counter resets cannot recycle
queries still owned by pending reports. Reports complete in submission order;
fence mirrors/events cannot retire past their pending memory writes, and GPU
semaphore releases drain reports before publishing their release values.

After profiling, no full-frame publication/layout sync path appeared in 80
executor samples; it waited for work in 33 samples. The title's fence-wait
samples dropped to 2/80. A native cache inspection showed one scaled depth
owner. Geometry/model work now dominates the title thread; the audio thread
also spends substantial time scanning contiguous allocations. These are the
next profiling targets, not changes already included here.

Loading/NETRICSA sampling found 76/100 title samples in the presenter's explicit
BlockUntilVerticalBlank at return `0x001DA528`. The wrapper now follows the
native presentation interval at that site as well as the engine swap site.
Enabled vsync and the other two direct wait sites retain their old semantics.
Decoder/audio timing is unchanged. The updated run logs about 64 fps in the
pre-gameplay loading/message stage. This is presentation throughput, not the
decoded movie frame rate or a measurement of total asset-loading time.

Validation: the native ZPASS regression checks hidden/visible pixels, scaled
scene-depth reuse both with and without query counting, a separate native-size
shadow target, pending snapshots across resets, FIFO empty reports and drain.
Both CPU/GPU clear-report regressions pass; the GPU case verifies asynchronous
real NV097 report dispatch completes after polling with submission stopped.
All four title wrapper tests pass, including the loading presenter, retained
wait sites, enabled native vsync and guest register/stack preservation.

The installed exe and matching map are under `build/Release`; SHA-256
`55A6DEDAEC0815F69DBBC890C69A1382C5AD5A1570EB95BB2F8EBA5C88DA7C5C`.
The previous executable is `analysis/serious_sam_before_async_perf.exe`.
CMake's temporary output override is removed. The user-controlled after-test
window uses `analysis/perf_user_after/saves`, not their original profile.
No Turok worktree was modified and no shared shader cache was cleared.

Evidence: `analysis/perf_user_control/measurement.json`, `stacks_gpu.txt`,
`stacks_game.txt`, `summary_gpu.txt`, `summary_game.txt`;
`analysis/perf_user_after/measurement.json` and `stacks_*.txt`/`summary_*.txt`;
`analysis/perf_slow_visual` (loading-stage diagnostic only);
`analysis/build_perf_final.txt`, `build_zpass_scene_owner_test.txt`, and
`build_report_async_test.txt`. Baseline raw stacks require the prior map at
`analysis/effects_final_build/serious_sam_recomp.map`.

## Persistent helper-shader caching (2026-11-03 evening)

At the user's request, Turok's shader-cache implementation and README were read
without modifying that worktree. Sam already has the same default-on persistent
cache and background guest-shader compilation. Its cache key includes shader
source, defines, entry point, profile and compile flags. Writes use temporary
files and atomic replacement. The default directory is
`%LOCALAPPDATA%\xboxrecomp\shadercache`; `RECOMP_SHADER_CACHE=0` disables it.

Nine native helper compile calls still bypassed persistence: scale blit (three),
MSAA depth resolve (two), presentation (two), and partial rectangle clears (two).
They now use the existing cache with the same source, entry points and flags.
No changes to shader mathematics or rendered quality are involved. Startup logs
identify the cache directory, and GPU reports now distinguish actual source
compiler calls from bytecode hits/background shader installation.

The extended native graphics fixture exercises clears, scaled targets and MSAA
depth publication as well as real visibility queries. A cold/warm regression
runs it in two separate processes using a newly generated private directory:
**12 source compiles / 0 disk hits cold; 0 source compiles / 12 disk hits warm**.
Both tests pass. Normal query tests disable persistence; the restart regression
does not touch the user's shared cache. This confirms repeated source compilation
is avoided for the exercised paths, not that all game stutter is shader-related
or that never-before-seen materials need no initial compilation. The preceding
slow-view 37.2-to-142.9 fps comparison predates this cache extension.

Evidence: `analysis/build_shader_cache_tests.txt` and
`build/nv2a_zpass/Testing/Temporary/LastTest.log`. Normal-executable verification
and final installation details follow below.

Toolkit `9cbe1ef` contains this extension. Normal game verification uses a
fresh private cache (`analysis/shader_cache_game_private`) and isolated saves:
the first run compiled 24 programs as boot progressed into the menu, then a
restart of the installed normal executable logged **23 disk hits, zero source
compiles**, with no generic-shader fallback draws or skipped vertex draws.
The cold run uses `analysis/shader_cache_game_cold`; the restarted normal launch
uses `analysis/shader_cache_game_warm`. Both are diagnostic runs with periodic
GPU reports; their FPS values are not performance benchmarks. The user-controlled
comparison window was already closed before these short boot probes.

The final executable and map are installed in `build/Release`. Exe SHA-256:
`7C89E63C609CDC844E34C6E3355C6103A04F292B953514CA46CA29FB04C25828`.
The previous performance-only executable is
`analysis/serious_sam_before_shader_cache.exe`, with its map retained under
`analysis/perf_final_build`. The temporary CMake output override is removed.
The two owned boot probes were closed, original saves remain untouched, and
normal launches use the default shared cache without additional setup.
## Keyboard/mouse controls and the PC Controls page (2026-11-03)

Sam now has its own **Options > PC Controls** page in the front end and gameplay
pause menu. It uses the title's native text-button widgets, focus rendering and
menu transitions. The nineteen scrolling rows cover sixteen bindings, mouse
speed, invert Y and restoring defaults. Keyboard/controller navigation, key and
mouse capture, wheel bindings, duplicate-key swapping, clearing a binding and a
clickable Back button are implemented. Choices live in `[pc_controls]` in the
executable's existing `serious_sam_settings.ini`; `RECOMP_PC_CONTROLS_FILE` gives
tests a private INI. No menu archives, console profiles or original saves change.

Input is deliberately Sam-specific. Its default Xbox jump/weapon buttons differ
from Turok's, so copying Turok's pad bindings would be incorrect. Keyboard actions
enter Sam's control globals immediately before the original action builder
`0x00070810` consumes them, for local player zero only. Movement merges at full
deflection rather than adding speed to a held pad stick. Button globals are
restored afterward, preventing a released keyboard button from sticking and
preserving a simultaneously held controller button. The controller's chosen
layout still works independently.

Raw mouse counts enter `ctl_fTurnRight` / `ctl_fTurnUp` as angles. The wrapper
compensates the profile sensitivity and invert multipliers which the original
builder then applies, giving the PC page independent settings without modifying
that profile. Default speed 5 is 0.1 degree/count; each speed step multiplies it
by 1.4. Neither frame time nor the maximum stick turn rate scales the counts.
The shared runtime's old direct-camera path is disabled: it contains another
title's camera addresses. `RECOMP_KBM=0` opts out of Sam's PC gameplay input.

Integration details:

- `0x00030710` builds the Options entry; `0x00030D00` reconnects its focus links
  after the original Options layout selects its visible rows.
- The extra menu is allocated once with the runtime guest heap, constructed by
  the original base-menu constructor and populated with cloned text buttons.
  Dispatch tokens `0x00030CF7/8` identify the native Open/Key callbacks, using
  unused alignment padding after the original Options initializer. No guest
  executable bytes change. Three additional `_gen` wrappers are reproducible
  with `tools/generate.ps1 -RecompOnly` (35,103 translated, zero failures, ten
  existing unresolved stubs; ten wrapped functions total).
- PC input/menu mode follows both presentation callers `0x001A95B8` and
  `0x001DA528`, the action builder, and menu renderer `0x0002B6BE`. Paused
  rendering can bypass the presentation waits, so the renderer hook is required.
  Fixed PC menu keys call the game's `0x0002B588` key-event handler on the render
  thread, with directional repeat and held-key suppression across transitions.
  The PC Pause binding pulses normal Xbox Start; it does not rely on holding a
  short keyboard tap until a USB report arrives. Xbox input keeps its original
  USB path. Escape now supplies Start at the loading prompt,
  then Pause in play and Back in menus. Held inputs and mouse deltas are discarded
  on a context change until release, so choosing a menu item cannot fire/jump.
- The only toolkit addition is `xbox_KbmInputDown`, exposing the existing binding
  reader, including mouse suppression and wheel latching. All title/menu code is
  in this repo. **No Turok project or Turok toolkit worktree was modified.**

Verification uses copied sparse save images under `analysis/kbm_*`, the untouched
disc archives, private control INIs and the existing diagnostic input/framebuffer
APIs. Five committed native tests pass. The new test covers input opt-out,
menu/player isolation, release, sensitivity/invert compensation, equal total
counts across polling rates, persistence (including numeric favourite keys),
duplicate swaps, short mouse clicks, Delete, capture timeout, controller key ABI,
and pointer hit tests in 4:3, wide and tall windows.

`analysis/make_pc_action_probe.py` also extracts the actual generated
`sub_00070810_gen` into a native harness with the constants read from the user's
disc. It verifies the resulting movement/jump/fire action, release and guest
stack/callee-saved registers. At all tested profile sensitivities (-127, 0, 127)
and both console invert states, 100 horizontal / -50 vertical counts produce
-10 / +5 degree action angles at PC speed 5. No independent replacement for the
action builder was used in that check.

Runtime captures show the page in both menu contexts, scrolling to mouse speed
and defaults, a binding changed to H, speed changed to 6, and those preferences
reloaded by a fresh process. Keyboard H then visibly moves Sam from the spawn
platform up the Hatshepsut ramp; Escape pauses. Evidence includes
`analysis/kbm_visual/game_before_move.bmp`, `game_keyboard_move.bmp`, and
`analysis/kbm_final/controls_scrolled.bmp`. These are runtime framebuffer
captures, not Windows window captures: Computer Use still fails to connect to
its native pipe (error 2 after recovery). Physical mouse feel remains for the
user to assess; raw-count/action and pointer event paths are covered by tests.
The existing 200 fps video policy and native rendering optimizations are kept;
this is not a new whole-game performance measurement or a claim about the two
historical intermittent stalls/crashes.

## PC Settings page (2026-11-03)

The user's additional request adds a separate **Options > PC Settings** entry
beside PC Controls. The original six Options rows are spaced to fit both new
entries; focus links include all visible rows. Both added pages use native Sam
text buttons and are accessible from the main and pause menus. Graphics choices
cover render resolution (automatic or five fixed sizes including the
native size), windowed/borderless fullscreen, VSync, a live frame limit,
anisotropic filtering and 2×/4×/8× MSAA, plus Restore defaults.

`src/pc_settings.c` owns these preferences in `[pc_settings]` of the same INI.
They apply through the runtime's existing render-scale, anisotropy, MSAA,
fullscreen and present-interval setters. A new `xbox_SetFrameLimit` /
`xbox_GetFrameLimit` exposes the existing flip pacer for live changes, lazily
creating its high-resolution waitable timer if a limit is enabled later. It
retains the separate movie cap and the pacer's deadline-reset behavior. No
`timeBeginPeriod` calls or Turok title-specific settings/patches were introduced.
Startup environment overrides for resolution, VSync and the global cap win over
saved values, including custom scale/rate values. Alt+Enter fullscreen changes
are reflected and saved without undoing a pending asynchronous window request.

Defaults are Auto rendering, windowed, VSync off, unlimited gameplay, 16× AF
(as in Turok's settings) and MSAA off. The view remains 4:3: widescreen needs
Sam-specific projection/culling work and is not implied by a resolution choice.
Tests can override `RECOMP_PC_SETTINGS_FILE`; otherwise the controls INI override
also isolates these preferences.

The six title tests pass, including graphics values, live setter arguments,
persistence, other INI sections, explicit/custom overrides, malformed preferences,
defaults and fullscreen synchronization. The menu/presentation regression now
checks the menu-render tick as well as both presentation callers. The actual
generated-action harness still passes. A fresh full generation into
`analysis/pc_gen_verify` translates all 35,103 functions with zero failures and
ten wrapped functions; the installed menu-render function body matches that
output (only the generated function's comment differs).

The CPU clear/cap-API regression and pure video-pacing tests pass. The existing
GPU clear/query fixture currently loses its hardware device during an occlusion
readback (`0x887A0005`, device reason `0x887A0020`). A separately archived and built
pre-settings runtime at `6c24bc2` reproduces the same failure, and an isolated cold
shader cache does too; it is not introduced by the new frame-limit API. This
fixture failure is not marked fixed. Normal-game rendering is verified separately.

Live verification of the installed build (`9558CFF7E93EA107FC21DCE59A6B9C46C26A7E3BB02409994C427AA1AC8A6D52`)
uses `analysis/pc_settings_final`. From the paused first level, keyboard navigation
opens both new entries. The graphics page switches Auto (3×) to 640×480; the
backend logs two colour and one depth targets resized to 1×. It logs MSAA changing
1×→2×→4×. AF is changed to 8× and fullscreen is enabled; the native window state
reads fullscreen=1. The live cap reads 60 and successive logging periods report
60.0 fps. Returning to Hatshepsut preserves the scene and HUD with these settings.
`options_both_entries.bmp`, `pc_settings_default.bmp`, `pc_settings_changed.bmp`,
`pc_settings_fullscreen.bmp` and `game_with_4x_msaa.bmp` preserve the framebuffer
evidence. These are functional/pacing checks, not a new uncapped performance
benchmark. Input toolkit commit: `6c24bc2`; live-cap toolkit commit: `0c20d5f`.
The fresh process in `analysis/pc_settings_restart` loads every saved value;
its backend logs MSAA 1×→4× and scale 3×→1×. Native state reads fullscreen=1
and frame_limit=60 after restart. All owned test processes were then closed.

## 2026-11-04: lightmap shadow masks repeat their first byte

The user's Xbox reference photographs show a sunlit courtyard after the storm,
with palm and fence shadows and no extra dark bands. Fresh isolated archive-only
runs reproduced the dark ramp. Disabling `RECOMP_TEX_VALIDATE_ONCE` made no visible
difference. F11 captures now also report each stage's first UV, texgen and matrix
enable state (toolkit `ed46eda`), to distinguish source images from sampling errors.

The actual Xbox directional-light mixer at `0x00294650` and masked point-light
mixers at `0x00294190` / `0x00294440` rotate an 8-bit mask then use its carry to
advance the source pointer: `rol dl,1; adc esi,0`. The lifter emitted a correct
8-bit rotation but never assigned its carry-out. The pointer stayed on the first
byte, repeating that byte across the shadowmap. This accounts for missing shadow
patterns and polygons receiving only ambient light despite being in sunlight.
Toolkit `ef79bcf` emits carry-out from the rotated result, preserves CF for a zero
masked count and captures CL before writing the destination. ZF/SF/PF stay intact.
No Croteam source code was copied or adapted; the correction follows the Xbox
instructions and the existing lifter.

Verification:

- `tests/lightmap_mask_test.c` executes the actual generated `0x294650` body,
  extracted at test configure time. Nonrepeating masks cross byte boundaries,
  start at all eight bit positions and cover padded rows. Old generated code:
  **197 failures**. Regenerated code: **456 samples pass**, padding and callee-save
  registers/stack pass. No generated game code is checked in.
- Seven title CTests pass. Toolkit rotate/carry/shift suite: 39 pass, 4 skip
  because its optional clang/gcc compiler is unavailable; 29 subtests pass.
- Full temporary generation: 35,103 functions, zero failures, 10 unresolved
  stubs. Only chunks 40/49/54/139 changed in executable code; other differences
  were the already-known menu-render wrapper declaration/comment. Installed
  only the affected chunks, preserving unchanged generated-file mtimes.
- Isolated runtime BMP captures: `analysis/lightmap_mask_before/frame.bmp`
  has a dark ramp; `analysis/lightmap_mask_after/frame.bmp` initially captured
  the same view sunlit (later overwritten by the test's game-over capture).
  `analysis/lightmap_mask_after/ramp.bmp` and `courtyard.bmp` preserve bright
  ramp/courtyard views. `lightmaps.png` shows restored palm patterns in source
  lightmaps. These are runtime framebuffer captures, not OS window screenshots.
  Thin dark streaks remain visible in some ramp views; do not claim those fixed.
- Saves are private sparse copies of `analysis/perf_save_snapshot`; game data
  is `test_clean`. Tests disabled PC mouse input so physical mouse motion did
  not steer the test camera. Controller scripts used the original input path.
  The first 80s Start dismisses NETRICSA, 85s Start pauses; B resumes. Do not
  send movement into that pause menu. Test gameplay eventually reached game
  over from enemy attacks; this was not a crash.
- Release exe SHA-256:
  `93BFFDE07847890095C39894E7E7B6AA164D29ED9E396E5A3466275FACC255C7`.

Other pending work remains: Quit Game, selectable gameplay aspect ratios with
original 4:3 cutscenes, and crash investigation. A separate earlier isolated run
`analysis/aspect_baseline` crashed during DirectSound cleanup at 22:12:16, with
`eax=FE0001B4`, guest return `003677B3`, native RIP `141CCC1B0`. This is preserved
alongside its exact pre-fix executable/map under `analysis/lightmap_baseline`.
The later coordinates probe ended at its 600-second watchdog, not that crash.
No DirectSound workaround has been added for this new crash and no general
stability claim is made. Turok projects/toolkits and original saves are untouched.

## 2026-11-04: main-menu Quit Game

Added a sixth main-menu row and native Yes/No page. No is the default; Escape/B
cancels, arrows switch and Enter/A confirms. Mouse hit boxes cover the two
choices, not their gap, and a pointer that has not moved cannot replace default
No. Callback addresses `0x30CFB..E` extend the existing unused-padding dispatch
scheme without changing Xbox code bytes. The original Main transition routines
know only their five static gadgets, so the extra Quit gadget is explicitly
hidden outside Main. The confirmation header is below the shared logo.

The first implementation called the game's unused shell Quit (`0x36000`). Its
flags stopped the main loop and it wrote PersistentSymbolsX.ini, then stalled in
engine shutdown. The final implementation instead requests the runtime's normal
Xbox dashboard return (`xbox_HalReturnToFirmware(2)`), which exits to Windows as
Turok's dashboard quit does. This is a main-menu action; it does not alter the
pause menu's existing Exit action. PC settings already persist when changed.
The initial stalled run is preserved in `analysis/quit_menu_test`; it was stopped
only after the evidence was collected.

Verification: extended PC controls tests cover No/Yes, Escape, Xbox A, guest
stack/register preservation, pointer hover and the gap between buttons. All
seven title CTests pass. Native captures under `analysis/quit_exit_test` show
the row and confirmation without leftover main-menu text; cancellation returned
to Main, and Yes exited with code 0. A final normal launch (private saves and
settings, no GAME_DIR override) used `test_clean`, then Xbox-scripted Up/A/B/A/
Left/A cancelled, reopened and quit. Its log ends `[EXIT] HalReturnToFirmware
routine=2` (`analysis/quit_controller_final`). The final exe differs from the
captured candidate only by the tested unmoved-pointer protection.

Installed exe SHA-256:
`464FD307E595337A4242C9240ED290EA6A9246608FBE2114F1D226A7B0B6A95F`.
The lightmap correction remains included. Aspect ratios with 21:9 gameplay and
original 4:3 cutscenes, the DirectSound crash, and the remaining thin streaks
remain open. No Turok tree or original save was modified.

## 2026-11-04: follow-up scope and code audit

The lighting correction is shared engine code, not a Hatshepsut patch. The
rotations in the current generated image were cross-checked against the original
Xbox instructions. Among the inspected rotate-to-carry-consumer sequences,
the three affected `rol dl,1 / adc esi,0` loops are the directional mixer and
two masked point-light mixers (`294650`, `294190`, `294440`). All are corrected
by the installed build. Visual verification remains first-level only; do not
claim every level or every kind of lighting has been validated.

No further runtime change was made in this follow-up audit. The open DirectSound
cleanup crash is distinct evidence from the historical firing/allocator crash;
no common cause has been established. Thin dark streaks, the intermittent boot
fence stall, and nine unsupported catch continuations remain open. The ten
unresolved generated stubs should not be reported as ten confirmed missing game
features: several target misdecoded bytes or unreachable code after a catch
handler's return. For example, `199950` exists as a label in its real parent,
and catch continuation `199A75` is already covered there; its duplicate helper's
unreachable tail reference alone is not evidence of a broken runtime path.
The `8B334` continuation, unlike that example, has no emitted label and warrants
further exception-CFG coverage work. Do not blindly seed the ten stubs as new
function starts, since legitimate continuation code inherits its parent's frame.
