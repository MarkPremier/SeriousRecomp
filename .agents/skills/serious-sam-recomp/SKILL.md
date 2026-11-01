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

## Current state (2026-11-02)

- Repo: `P:\Recomp\serious_sam`, branch `main`, remote
  `https://github.com/MarkPremier/SeriousRecomp` (private). It contains the
  xboxrecomp `templates/new-game` scaffold, renamed for this game.
- Disc extracted to `P:\Recomp\SeriousSam\Serious Sam (USA)\`: `default.xbe`,
  `Main.gro`, `Movies0.gro`-`Movies5.gro`. The ISO is next to it.
- `src/main.c`: entry point `0x0003D755`. Turok-style game-file discovery checks
  environment, remembered/portable files, image and developer fallback. The local
  fallback is the isolated, prepared `serious_sam/test_game` folder. Normal launches
  enable the required stream prefetch and first-run defaults and keep a crash log.
- Update: first native x64 D3D11 Release executable built successfully on
  2026-11-01 using isolated worktree `P:\Recomp\xboxrecomp_ss` at `1d9e929`.
  Current generation contains 35,103 functions, zero failures and 10 unresolved
  direct-call stubs. The boot movie visibly plays using native DSP. RGB565 colour
  now renders on D3D11, with a CPU fallback available for diagnosis. ZPASS report
  decoding/RAMHT lookup and the title's D3D software-method callback are fixed;
  full animated menus run about 43–49 fps, versus about 11 fps on the CPU path.
  Sam's rising entrance animation is correct (confirmed by the user). Menu input
  and gameplay remain unverified. Read `docs/bringup.md` for evidence. Fresh
  archive-only installation is unverified; do not equate the prepared data test
  with a portable release. `tools/generate.ps1` is the reproducible pipeline.
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
