# SeriousRecomp

A static recompilation of **Serious Sam** (Xbox, 2002, USA) to native Windows,
built with [sp00nznet/xboxrecomp](https://github.com/sp00nznet/xboxrecomp).

The game's x86 code is translated to C and compiled into a Windows `.exe`;
the Xbox kernel, GPU (NV2A) and audio (MCPX APU) are provided by the
xboxrecomp runtime. **No game code or data is in this repository.** You need
your own disc image, and the C code is generated from it on your machine.

## Status

- Bring-up started on 2026-11-01. XBE metadata verified: USA title
  `0x54540004`, XDK 4928, entry point `0x0003D755`.
- Separate runtime worktree: `../xboxrecomp_ss`, branch `serious-sam`, based on
  fork commit `1d9e929`. Turok working trees are not build inputs.
- Native x64 Release configuration and runtime library build verified.
- Regenerated with runtime feedback: 35,103 functions translated with zero
  failures; 10 unresolved direct-call stubs remain. Native D3D11 x64 Release
  builds at `build/Release/serious_sam_recomp.exe`.
- Startup now loads fonts, scripts, menu textures and player models. Guest-object
  waits, lazy file streams and missing first-run settings have bring-up fixes.
- Vertical blank interrupts release D3D presentation waits. The APU uses the
  runtime's physical-memory mapper, allowing startup with the DSP interpreter
  enabled and without the diagnostic mailbox bypass.
- Boot movie visibly plays and reaches an animated, fully drawn main menu.
  RGB565 colour targets now render on D3D11 and present directly from GPU
  surfaces. Menu measurements improved from roughly 11 fps to 43–49 fps,
  including automatic 3× rendering on this monitor.
- The launcher follows Turok's file discovery and crash-log workflow. Required
  stream/settings defaults also apply to normal launches. A pending-command
  submission fix addresses the GPU fence stall exposed by Explorer launching.
- Fixed occlusion-query completion and registered the title's D3D software-method
  handler, which signals completion events and performs pushbuffer fixups.
  Sustained menu animation is verified. Sam rising into view is his original
  entrance animation. See
  [bring-up notes](docs/bringup.md) for evidence and the next investigation.
- Xbox/XInput controller discovery and the guest USB bus are enabled. D-pad,
  left-stick menu movement, A select, B back, and first-level movement are
  verified through USB input reports. The connected Xbox controller is detected.
- Fixed buffered stream writes and update opens so settings and signed profiles
  persist. The first Hatshepsut level now loads with its HUD and weapon.
- Release builds explicitly enable optimization for the game and C runtime.
  Hatshepsut's spawn scene improved from 48–52 fps to roughly 150–186 fps at
  automatic 3× rendering; moving onto the ramp reached about 195–205 fps.
  A normal launch also exceeds 120 fps. These are first-level measurements;
  extended play and audio quality remain unverified.

## What you need

- An original Xbox disc image of Serious Sam (USA), XISO format
- Windows 10/11 with Visual Studio 2022 or later ("Desktop development with C++",
  which includes CMake)
- Python 3.10+ and Capstone 5.0.7 (use the local environment below)
- The upstream disassembler and the isolated fork runtime described below

## Setup

The layout this project expects, side by side:

```
Recomp/
  SeriousSam/Serious Sam (USA)/   extracted disc (default.xbe, data)
  xboxrecomp/                     upstream disassembler (read-only tools)
  xboxrecomp_ss/                  isolated fork runtime and C generator
  serious_sam/                    this repository
```

The local disc is already extracted. From this project directory in PowerShell:

```powershell
py -3 -m venv .venv
.\.venv\Scripts\python.exe -m pip install capstone==5.0.7
.\tools\generate.ps1
$cmake = 'C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
& $cmake -S . -B build -G 'Visual Studio 18 2026' -A x64 -DXBOXRECOMP_DIR=../xboxrecomp_ss
& $cmake --build build --config Release --parallel 6
```

`generate.ps1` runs XBE parsing, full disassembly (including Xbox libraries),
function identification, ABI analysis and C generation in order. It uses explicit
per-game analysis directories, excludes manual overrides, and backs up existing
generated C under `analysis/` before regeneration. Analysis, game data and generated
C are ignored by Git. The Turok runtime is never used as a shared output directory.

## Running and diagnostics

For this development checkout, double-click
`build/Release/serious_sam_recomp.exe`. The local build falls back to the prepared
`test_game/` data used for bring-up. No probe script or environment setup is
required for movie playback.

The launcher checks an explicit `RECOMP_GAME_DIR`, a remembered folder in
`serious_sam_settings.ini`, `game/` beside the executable, an adjacent ISO/XISO,
then the developer fallback. Otherwise it offers a file picker. It verifies the
USA `default.xbe` SHA-1 before loading. Discovery and disc extraction were adapted
from Turok; a fresh install using only the disc archives has not yet been
validated. Current verification uses prepared loose assets and first-run files
in `test_game/` (see [bring-up notes](docs/bringup.md)).

Explorer launches write `serious_sam_log.txt` beside the executable and retain
the preceding run as `serious_sam_log.prev.txt`. Redirected stderr remains usable
for diagnostic probes. Saves default to `saves/` within the selected game folder;
`RECOMP_SAVE_DIR` overrides this. Use a separate game/saves copy for testing.

Copy the extracted game folder to `test_game/` before boot testing. Keep the copy
and saves local; do not commit either. Then run:

```powershell
$env:RECOMP_GAME_DIR = "$PWD\test_game"
$env:RECOMP_SAVE_DIR = "$PWD\test_game\saves"
$env:RECOMP_WATCHDOG_SECS = '20'
& .\build\Release\serious_sam_recomp.exe 1>stdout.txt 2>stderr.txt
```

The launcher defaults `RECOMP_PB_EXEC`, `RECOMP_AC97_READY`, `RECOMP_FB_WINDOW`,
`RECOMP_TITLE_KEVENTS`, `RECOMP_HEAP_RECLAIM`, `RECOMP_NV2A_NATIVE_FENCES`, and
`RECOMP_VBLANK`, `RECOMP_STREAM_PREFETCH`, and `RECOMP_OPTIONAL_DEFAULTS` to `1`
if unset. It also defaults `RECOMP_USB=1`, `RECOMP_USB_NDP=4`,
`RECOMP_USB_PORT=2` (Xbox controller 1), and `RECOMP_XINPUT_SLOT=auto`.
Connect an Xbox/XInput pad before or during play. Use the D-pad or left stick
to navigate, A to select, B to return, and Start to skip movies or pause.

Use the isolated bring-up probe to capture bounded logs:

```powershell
pwsh -File tools/boot-test.ps1 -Seconds 30 -Label native -Visible
```

`-DspPassthrough` is an optional diagnostic that disables DSP execution and
acknowledges a fixed observed mailbox. It is no longer required to pass audio
initialization and does not verify working sound.

The original XBE is loaded from the selected game folder. Use
`RECOMP_FPS_LOG=5` for periodic performance logging.
`RECOMP_CPU_RGB565=1` restores the slower CPU colour fallback for diagnosis.
`RECOMP_RENDER_SCALE=1` selects original resolution; the runtime otherwise uses
automatic scaling. Lowering it gave little menu-speed improvement in this test.
Release builds emit a linker map with a fixed image base for crash symbolization:

```powershell
.\.venv\Scripts\python.exe ..\xboxrecomp_ss\tools\symbolize.py build\Release\serious_sam_recomp.map <RIP>
```

## Reference

Croteam released the Serious Engine 1 source (GPL-2.0) in 2016:
<https://github.com/Croteam-official/Serious-Engine>. It is useful for
identifying functions in the XBE. If code from it is adapted here, this
repository must be licensed GPL-2.0 compatibly.
