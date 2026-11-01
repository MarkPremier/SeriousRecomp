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
- Full disassembly recovered 34,814 function entries from 1,292,133 instructions;
  ABI analysis completed (2,342 entries detected as `thiscall`).
- First native D3D11 x64 Release executable built successfully at
  `build/Release/serious_sam_recomp.exe`. All 34,814 entries translated with zero
  translation failures (2,544,138 lines of C); 12 unresolved direct-call stubs remain.
- First isolated boot reached guest startup and kernel/thread initialization,
  then stalled during CRT initialization/cleanup. The 10-second watchdog ended
  the run with exit code 3. No menu, graphics or gameplay has been verified.
- Collected 97 runtime-observed seed candidates in `seeds/runtime_feedback.json`
  using the toolkit's executable-section/body checks. The next generation pass
  includes them; the current executable predates that feedback. See
  [bring-up notes](docs/bringup.md).

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

The launcher defaults `RECOMP_PB_EXEC`, `RECOMP_AC97_READY` and `RECOMP_FB_WINDOW`
to `1` if unset. `RECOMP_GAME_DIR` and `RECOMP_SAVE_DIR` override runtime file
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
