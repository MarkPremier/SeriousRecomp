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
- Boot movie now visibly plays and finishes at the main menu without input.
  RGB565 colour draws use the CPU rasterizer because the D3D11 backend's
  16-bit path only writes depth. Movie probes report roughly 15–28 fps.
- Menu rendering still has layout/clipping issues and a subsequent stall;
  gameplay and audio quality are unverified. See
  [bring-up notes](docs/bringup.md) for evidence and the next investigation.

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
`RECOMP_VBLANK` to `1` if unset. Use the isolated bring-up probe to enable the
current stream/settings workarounds and capture bounded logs:

```powershell
pwsh -File tools/boot-test.ps1 -Seconds 30 -Label native -Visible
```

`-DspPassthrough` is an optional diagnostic that disables DSP execution and
acknowledges a fixed observed mailbox. It is no longer required to pass audio
initialization and does not verify working sound.

`RECOMP_GAME_DIR` and `RECOMP_SAVE_DIR` override runtime file
and save roots. Saves default to this project's `saves/` directory, separate from
the runtime's shared default save location. The original XBE is loaded from the extracted disc path in
`src/main.c`. Use `RECOMP_FPS_LOG=5` for periodic performance logging.
Release builds emit a linker map with a fixed image base for crash symbolization:

```powershell
.\.venv\Scripts\python.exe ..\xboxrecomp_ss\tools\symbolize.py build\Release\serious_sam_recomp.map <RIP>
```

## Reference

Croteam released the Serious Engine 1 source (GPL-2.0) in 2016:
<https://github.com/Croteam-official/Serious-Engine>. It is useful for
identifying functions in the XBE. If code from it is adapted here, this
repository must be licensed GPL-2.0 compatibly.
