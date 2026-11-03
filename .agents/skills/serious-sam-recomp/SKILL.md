---
name: serious-sam-recomp
description: Start or continue the Serious Sam (Xbox, USA) static recompilation to native Windows with the xboxrecomp toolkit. Use for any work in P:\Recomp\serious_sam - code generation, building, boot bring-up, crashes, rendering, audio, input.
---

# Serious Sam (Xbox) static recomp

Goal: turn the original Xbox game's x86 code into C with the xboxrecomp toolkit and
build a native Windows `.exe` that plays the game. The toolkit provides the Xbox kernel,
GPU (NV2A) and audio (MCPX APU) at run time.

The same user already did this with Turok: Evolution, which is now fully playable. That
project is the reference for how the pipeline works and for the kinds of bugs to expect.
It is in active use by another agent, so **read it, but never modify it** (see
"Do not touch").

## Current state (2026-11-03)

- **Lightmaps (2026-11-04):** toolkit `ef79bcf` makes ROL/ROR update CF when
  consumed. The directional and two point-light shadow-mask loops use
  `rol dl,1 / adc esi,0`; stale CF repeated the first mask byte. Regenerated
  chunks 40/49/54/139 from a complete private generation; no title data/code
  committed. New seventh test extracts the actual generated `0x294650` mixer:
  old code has 197 wrong samples, fixed code passes all 456 plus row guards
  and guest ABI checks. Ramp before/after captures and restored palm-mask
  images are under `analysis/lightmap_mask_before` and `_after`. Thin dark
  streaks remain in some views and are not claimed fixed. Installed exe
  SHA-256 `93BFFDE07847890095C39894E7E7B6AA164D29ED9E396E5A3466275FACC255C7`.
  Toolkit `ed46eda` adds UV/matrix/texgen information to F11 captures only.
  Seven title tests pass; carry/rotate suite has 39 passes, 4 compiler-dependent
  skips, 29 subtests. User Xbox references show the expected sunlit courtyard,
  palm and fence shadows. Turok worktrees remain untouched.

- **PC Settings:** separate native **Options > PC Settings** in both menu
  contexts. Resolution, fullscreen, VSync, frame limit, AF and MSAA apply live
  and persist under `[pc_settings]` in the exe INI. Override
  `RECOMP_PC_SETTINGS_FILE` for tests (defaults to the controls INI path).
  Default gameplay stays unlimited, the movie cap stays 200; default AF is 16×,
  MSAA off. The view stays 4:3. The new runtime live-cap API uses the existing
  pacer; rendering/window setters were already present. Six title tests pass.
  The GPU clear/query fixture currently fails with device reason 0x887A0020;
  an archived pre-settings `6c24bc2` runtime reproduces it. See bring-up notes.

- **PC controls (2026-11-03):** keyboard/mouse defaults on. Native **Options >
  PC Controls** exists in the main and pause menus, with sixteen bindings,
  mouse speed/invert and defaults; `[pc_controls]` in the exe's settings INI
  persists choices (`RECOMP_PC_CONTROLS_FILE` isolates tests). The Sam-only
  wrappers are Options init `0x30710`, layout `0x30D00`, and action builder
  `0x70810`. Keyboard actions enter its own control globals, independently of
  console layouts, and button globals restore after building the action.
  Mouse counts become angles, compensating the console profile's multipliers.
  Keep `RECOMP_KBM_LOOK_DIRECT=0`: the toolkit's old direct-camera path has
  another title's addresses. Context switching must follow **both** presenter
  callers `0x1A95B8` and `0x1DA528` (the latter covers loading and pause menus),
  plus the action builder and menu renderer `0x2B6BE`; paused rendering can
  bypass both waits. PC menu keys call Sam's own key-event handler directly,
  and the PC Pause binding pulses Xbox Start. Entry/key callbacks use dispatch
  tokens `0x30CF7..A` in original padding. Six native tests and an actual generated-action fixture pass; runtime
  captures verify menu integration, persistence and keyboard movement in
  Hatshepsut. Physical mouse feel still needs user assessment. All work is in
  Sam / `xboxrecomp_ss`; Turok worktrees are untouched. See bring-up notes.

- **Shader cache:** Sam already inherited Turok's persistent DXBC cache and
  background guest-shader compilation, enabled by default. Native helper shaders
  for presentation, scaling, partial clears and MSAA depth resolves now use it
  too (nine formerly direct compile calls). The log names the active directory;
  diagnostic GPU reports count actual source compiler calls. The new two-process
  graphics regression compiles 12 programs cold and loads 12 with zero source
  compiles on restart in an isolated test cache. Default cache directory is
  `%LOCALAPPDATA%\xboxrecomp\shadercache`; never clear that shared directory for
  tests. Use `RECOMP_SHADER_CACHE=<private folder>` or `0`. Cold unseen guest
  variants still use the existing background-compilation path.
  Toolkit commit `9cbe1ef`; an actual normal-executable warm restart also logged
  23 disk hits with zero source compiles. Final exe SHA-256 starts `7C89E63C`;
  the performance-only preceding build starts `55A6DEDA`. See bring-up notes
  for full hashes, private cache evidence and backups.
- **Slow-area performance (2026-11-03 evening):** toolkit `92bb1fb` preserves
  scene-depth ownership through colour-masked queries/depth prepasses, avoiding
  alternating 3×/1× depth aliases and full RGB565 readbacks. ZPASS reports are
  asynchronous, retain snapshots through resets, and gate fence/event retirement
  until their writes complete; semaphore releases drain pending reports. The
  loading/video presenter at caller `0x001DA528` now honours vsync-off as does
  the engine swap. Decoder clocks and the 200 fps video cap remain. With the
  user positioning the camera, short logging-only samples improved 37.2 to
  142.9 fps in the slow view (user reports ~300 elsewhere). This is not a
  whole-game or deterministic benchmark. Original saves are untouched; testing
  uses sparse copies under `analysis/perf_*`. Runtime and four wrapper tests
  pass. See the latest bring-up section for evidence and installed exe hash.
- **Effect corrections (2026-11-03):** x87 `fld m80` was emitted as `MEMF`,
  reading only four of the ten bytes in the CRT asin endpoint's pi/2 constant.
  The actual generated ground-normal function returned near-zero pitch for
  `(0,+/-1,0)`; the corrected lifter returns +/-90 degrees. The runtime template
  now decodes/stores extended operands while retaining its binary64 arithmetic
  stack. Native ZPASS reports also replace unconditional full-surface visibility
  with real D3D11 occlusion samples normalized to guest pixels. This addresses
  the user's upright blood stains and halos through walls. Automated regressions
  pass; user gameplay verification of these effects is pending. See bring-up
  notes. Keep Turok worktrees untouched.
- **Performance update (2026-11-03):** toolkit `668b7f5` inlines common x87
  24-bit rounding with unchanged numerical results; `b83918f` exposes the native
  present interval. The game wrapper bypasses only the engine SwapBuffers
  vblank wait at caller `0x001A95B8` when native vsync is off. Direct XMV waits
  and enabled vsync retain original semantics. Matched logging-only main-menu
  measurements improved 31.5 to 76.8 fps at automatic 3× scale. Four wrapper
  tests and two precision tests pass. The user confirmed 1000+ fps movie
  animation is normally timed. Full-screen packed-YUV video presentation has
  a separate `RECOMP_VIDEO_FPS_CAP`, default 200 at the user's request after
  trials of 60 and 120. It does not cap menus/gameplay or change decoder clocks.
  See the latest bring-up section for verification and remaining work. Do not
  claim sustained gameplay at 120+ fps or that historical crashes are solved.
- Repo: `P:\Recomp\serious_sam`, branch `main`, remote
  `https://github.com/MarkPremier/SeriousRecomp` (private). It contains the
  xboxrecomp `templates/new-game` scaffold, renamed for this game.
- Disc extracted to `P:\Recomp\SeriousSam\Serious Sam (USA)\`: `default.xbe`,
  `Main.gro`, `Movies0.gro`-`Movies5.gro`. The ISO is next to it.
- `src/main.c`: entry point `0x0003D755`. Turok-style game-file discovery checks
  environment, remembered/portable files, image and developer fallback. The local
  fallback is `serious_sam/test_clean`, the disc hard-linked (xbe + .gro only).
  Normal launches enable the required stream prefetch and keep a crash log.
- **Options rendering fixed (toolkit `0938eda`):** the cyan screen/missing rows
  were caused by ignoring NV097 clear rectangles and interpreting packed
  RGB565 clear colours as ARGB. Both software and native clears now preserve
  pixels outside the requested rectangle; GPU partial clears use a scissored
  draw. The normal exe is updated. Isolated-save tests verify the green Options
  background, all six rows, changing controls, and persistence across restart.
  Evidence uses runtime flip dumps because the Windows capture helper remains
  unavailable. New `tests/nv2a_clear` passes on CPU and GPU and fails with the
  old executor. See the latest bring-up section. Turok files were untouched.
- **Latest (Claude Code takeover, 2026-11-02):** the toolkit dispatches guest C++
  exceptions to the title's catch blocks (toolkit `3f45f57`), so the game runs
  from the untouched disc data: a fresh install reaches Hatshepsut gameplay at
  ~185 fps with no placeholder files or settings wrappers (`RECOMP_OPTIONAL_DEFAULTS`
  is gone). `test_game/` (unpacked Main.gro + placeholders) is obsolete. Open
  issues, in order: an intermittent BlockOnTime fence stall during the boot movie
  (4 of 15 clean runs), and a heap-allocator crash after firing in Hatshepsut.
  Toolkit `6c87b8c` now gives the main thread and workers nonzero, distinct XAPI
  thread IDs (`fs:[0x28] + 0x12C`). Previously every thread returned zero, so
  Serious Engine's recursive locks bypassed exclusion. The new synthetic-XBE
  regression fails against the prior runtime and passes with the fix. Do not
  call the intro freeze or firing crash fixed from that result alone. This
  session's baseline startup probes did not reproduce the intermittent freeze.
  Twenty post-fix 30-second archive-only probes also completed without a
  detected stall/crash; startup FPS was 31.4–32.3, not a new gameplay result.
  A separate 75-second normal launch auto-discovered `test_clean` and kept
  presenting frames (later 26.7–29.2 fps); phase/visual correctness is pending.
  Windows Computer Use currently fails to connect to its native pipe, so new
  actual-window verification is pending; do not equate fence progress with
  visual confirmation. See the last section of `docs/bringup.md` for results.
  The paragraph below is Codex's
  earlier history.
- Update: first native x64 D3D11 Release executable built successfully on
  2026-11-01 using isolated worktree `P:\Recomp\xboxrecomp_ss` at `1d9e929`.
  Current generation contains 35,103 functions, zero failures and 10 unresolved
  direct-call stubs. The boot movie visibly plays using native DSP. RGB565 colour
  now renders on D3D11, with a CPU fallback available for diagnosis. ZPASS report
  decoding/RAMHT lookup and the title's D3D software-method callback are fixed;
  full animated menus run about 43–49 fps, versus about 11 fps on the CPU path.
  Sam's rising entrance animation is correct (confirmed by the user). USB/XInput
  is initialized; menu direction/select/back and first-level movement are verified.
  Stream close flushes used pages, and update opens prefetch existing contents
  before appending signatures. Worker stream contexts are initialized lazily.
  Hatshepsut loads with HUD/weapon. An empty cached CMAKE_C_FLAGS_RELEASE left the
  initial Release C code unoptimized. A fresh cache supplies /O2 /Ob2 and
  NDEBUG; CMake checks the Release optimization flags. The spawn scene improved
  from 48–52 fps to
  150–186 fps at automatic 3× scale; the ramp reached 195–205 fps. A normal launch
  also exceeds 120 fps. The user targets 120+ fps across gameplay, so broaden
  validation beyond this first scene before claiming that target generally.
  Read `docs/bringup.md` for evidence. Fresh
  archive-only installation was subsequently verified by Claude as described
  above. `tools/generate.ps1` is the reproducible pipeline.
  User chose D3D11 and explicitly requested performance work.

## XBE facts

| | |
|---|---|
| Title ID | `0x54540004`, North America, built 2002-10-02 |
| XDK | 4928 for all libraries (Turok was 4627, so XDK-specific toolkit fixes may not carry over) |
| Base / entry | `0x00010000` / `0x0003D755` |
| Sections | `.text` (0x2F2D30 bytes) D3D XMV D3DX XGRPH DSOUND WMADEC XNET XPP .rdata .data DOLBY $$XTIMAGE $$XSIMAGE |
| Kernel imports | 124 |
| Debug path | `D:\work\Sources\SeriousSamX\ReleaseX\SeriousSamX.exe` |

The game is built on Croteam's Serious Engine 1. Croteam released that source in 2016
under GPL-2.0: https://github.com/Croteam-official/Serious-Engine. Use it as a reference
to identify functions, structures, vtables and file formats (`.gro` files are Serious
Engine's zip-based archives). The repo has no license yet. If you copy or adapt code
from that source, stop and tell the user first, because it would make this repo
GPL-2.0.

## Machine and tools

- Windows 11, Git Bash and PowerShell 7. Paths are on the `P:` drive.
- Visual Studio Community 2026. CMake is not on PATH:
  `C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe`.
  Build from a Developer PowerShell, or call that path directly.
- Python: `py -3`. capstone 5.0.7 is installed.
- No `gh` CLI. Git pushes work through the stored git credential.
- The toolkit can unpack disc images itself: `py -3 -m tools.xiso ls|get|unpack`.
  You do not need xdvdfs.

## Toolkits

There are two clones next to this repo:

- `P:\Recomp\xboxrecomp`: upstream sp00nznet/xboxrecomp at `f0f8a46` (v0.16.0). It has
  uncommitted Turok fixes in its working tree. **Use only its disassembler**
  (`tools.disasm`), which finds about a thousand more functions than the fork's.
- `P:\Recomp\xboxrecomp_fork`: `MarkPremier/xboxrecomp` (remote `mark`), branch
  `turok-evolution`. This is aknavj's fork, which adds a Direct3D 11 GPU backend, plus
  every runtime fix the Turok work produced (pushbuffer walker, vblank-locked flips,
  anti-aliased surfaces, APU and DirectSound locking, input, Proton). Upstream's runtime
  rendered Turok at about 9 fps on a CPU rasterizer; the fork made it playable.
  **Use this runtime.**

Turok is still being developed in `xboxrecomp_fork`, so give Serious Sam its own
worktree and branch of the fork rather than building against that one:

```
git -C P:\Recomp\xboxrecomp_fork worktree add P:\Recomp\xboxrecomp_ss -b serious-sam turok-evolution
```

Then configure with `-DXBOXRECOMP_DIR=../xboxrecomp_ss`. Commit toolkit fixes on
`serious-sam` as one commit per fix, with messages that say which game needed them.
Check with the user before pushing that branch to the `mark` remote.

The template's `CMakeLists.txt` defaults to `../xboxrecomp` (upstream). Compare it with
`P:\Recomp\turok_evolution_fork\CMakeLists.txt`, which builds against the fork (map file,
`/DYNAMICBASE:NO`, LTCG, the D3D11 sources). Copy what applies, but nothing
Turok-specific.

## Pipeline

Run these from PowerShell. They follow the Turok fork's README, step 3.

```powershell
$xbe = "P:\Recomp\SeriousSam\Serious Sam (USA)\default.xbe"
$out = "P:\Recomp\serious_sam\disasm_out"   # gitignore this

cd P:\Recomp\xboxrecomp
py -3 -m tools.xbe_parser $xbe --json "P:\Recomp\SeriousSam\ss_analysis.json"
py -3 -m tools.disasm $xbe --force -o $out --seed-functions P:\Recomp\serious_sam\seeds\manual_seeds.json
Copy-Item "$out\*.json" P:\Recomp\xboxrecomp_ss\tools\disasm\output\ -Force

cd P:\Recomp\xboxrecomp_ss
py -3 -m tools.func_id $xbe
py -3 -m tools.abi_analysis $xbe
py -3 -m tools.recomp $xbe --all --split 250 `
    --gen-dir P:\Recomp\serious_sam\src\recomp\gen `
    --exclude-manual P:\Recomp\serious_sam\src\recomp_manual.c

cd P:\Recomp\serious_sam
cmake -S . -B build -DXBOXRECOMP_DIR=../xboxrecomp_ss
cmake --build build --config Release --parallel
build\Release\serious_sam_recomp.exe 2> stderr.txt
```

- Create `seeds/manual_seeds.json` before the first run. Copy the format from
  `P:\Recomp\turok_evolution_fork\seeds\manual_seeds.json`, but not its addresses.
- The upstream disassembler takes several minutes. Back up `src/recomp/gen` before you
  regenerate it.
- Check the toolkit docs in `xboxrecomp*/docs/` (`GETTING_STARTED.md`, `pipeline/`,
  `runtime/`) for flags before guessing.

## Lessons from Turok

Apply these from the start.

1. **Never pass `--text-only` to `tools.disasm`.** Without it, the XDK library sections
   (D3D, DSOUND, XPP, XMV) are barely swept.
2. **`--exclude-manual` is required.** Every override in `src/recomp_manual.c` must be
   declared exactly `extern void sub_XXXXXXXX_gen(void);`. With any other form, the
   generator silently drops the generated body.
3. **Missing functions fail silently.** Functions reached only through pointers or
   runtime-built tables (vtables, thunks, the code after a switch jump table) are not
   found. The symptom is a feature that does nothing, or a crash. Grep `stderr` for
   `[ICALL] Failed to resolve VA 0x...`, seed every such address and regenerate.
   Helpers you can copy into this repo:
   - `py -3 -m tools.seed_from_log <stderr> <xbe> --functions ... --seeds ...` (toolkit)
   - `P:\Recomp\turok_evolution_fork\tools\scan_missed_functions.py <xbe> <gen dir>`,
     which finds code after jump tables and gaps in `.text`.
4. **Hangs during start-up are usually a wait on hardware that never answers**, such as
   D3D fence or semaphore waits or audio init spins. Turok's `main.c` defaults
   `RECOMP_PB_EXEC=1`, `RECOMP_AC97_READY=1` and `RECOMP_FB_WINDOW=1` for that reason.
   Check whether the fork already handles each one before adding a game-side wrapper.
   `RECOMP_WATCHDOG_SECS=N` dumps state when the game hangs.
5. **Crashes:** run `py -3 xboxrecomp_ss/tools/symbolize.py build/Release/<exe>.map <RIP>`
   on a `[CRASH]` line. This needs the map file and `/DYNAMICBASE:NO`.
6. **Measure performance with `RECOMP_FPS_LOG=5` only.** Per-frame logging to unbuffered
   stderr halves the frame rate and gives misleading results.
7. **Never call `timeBeginPeriod(1)`.** It slowed XMV movies to 1-5 fps in Turok.
8. **Protect the user's saves.** Run test instances against a copy of the game folder,
   not the real one. Turok used `RECOMP_GAME_DIR=<copy>`, a hard-linked copy with its own
   save data. Scripted menu input once picked New Game and overwrote a real save. Back
   up saves before any automated play.
9. **Some visual oddities are in the original game.** The user has xemu and a real Xbox.
   When you are unsure whether behaviour is a bug, ask the user for a reference
   screenshot rather than chasing it.
   The user confirmed Sam's head rising into the menu is his normal entrance
   animation; no position/transform fix is needed for that sequence.
10. **Before wrapping a missing-file or settings path, check for a C++ throw.**
    `RECOMP_EXCEPTION_TRACE=1` prints each throw's message; `[EH] ... not caught`
    or `[EH] continuation ... is not a label` means the exception support needs
    work, not that the game needs a placeholder file. Verify against `test_clean/`
    with an empty `RECOMP_SAVE_DIR`, never only against prepared data.
11. **Check numeric env switches by value, not presence.** `getenv("X")` is true
    for `X=0`; that bug kept two wrappers on in runs meant to test without them.

## Do not touch

These belong to the Turok project, which another agent is working on:

- `P:\Recomp\turok*` (all of them)
- `P:\Recomp\xboxrecomp_fork`, including its `turok-evolution` branch
- `P:\Recomp\xboxrecomp`'s working tree (run its tools, but don't edit, reset or check
  out anything there)
- `P:\Recomp\save_backup_*`

You may read all of them.

## Working conventions

- Commit to `serious_sam` in small, descriptive commits. Before you push, check that
  no game data is staged (`*.xbe`, `*.gro`, `*.iso`, generated `src/recomp/gen/`, extracted
  assets). `.gitignore` covers these, but check any new file types you add.
- Keep `README.md` current, with the Status section, the exact pipeline commands and
  the runtime switches, in the same structure as the Turok README. A new person should
  be able to rebuild from it.
- Ask the user before releases, force-pushes and anything visible on GitHub other than
  normal pushes to `main`.
- Report what you verified by running it separately from what you only changed.
