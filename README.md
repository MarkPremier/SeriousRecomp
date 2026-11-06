# SeriousRecomp

A static recompilation of **Serious Sam** (Xbox, 2002, USA) to native Windows,
built with [sp00nznet/xboxrecomp](https://github.com/sp00nznet/xboxrecomp).

The game's x86 code is translated to C and compiled into a Windows `.exe`;
the Xbox kernel, GPU (NV2A) and audio (MCPX APU) are provided by the
xboxrecomp runtime. **No game code or data is in this repository.** You need
your own disc image, and the C code is generated from it on your machine.

## Status

- Mouse menus: the Select Player footer's Back, New Player, Delete Player and
  Select actions can be clicked on either their labels or controller icons.
  Moving between menu items now plays the game's normal selection sound,
  including the PC pages, once per changed item.
  Hovered footer labels use the same animated lettering as selected menu text.

- Audio buffer correction (2026-11-04): normal launches now keep sample DMA
  provenance, as Turok does. Sam's ordinary-RAM PCM stream collided numerically
  with a contiguous graphics allocation; redirecting it played graphics data
  as a repeating buzz. Private recordings confirm the repeating signal is gone
  through first-level movement and firing. Audio pacing still needs work:
  diagnostic runs can drain the output queue. See [bring-up notes](docs/bringup.md).

- Lightmap correction (2026-11-04): rotate instructions now publish their carry
  flag, allowing Sam's shadow-mask mixers to advance between bytes. The prior
  code repeated the first mask byte, losing patterned shadows and leaving
  sunlit surfaces dark. A native test of the actual generated mixer fails with
  197 wrong samples before the fix and passes all 456 after it. Isolated gameplay
  captures show the ramp brightening and patterned palm shadows returning.
  Thin streaks still visible in some views remain under investigation.

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
  stream prefetch also applies to normal launches. A pending-command
  submission fix addresses the GPU fence stall exposed by Explorer launching.
- Fixed occlusion-query completion and registered the title's D3D software-method
  handler, which signals completion events and performs pushbuffer fixups.
  Sustained menu animation is verified. Sam rising into view is his original
  entrance animation. See
  [bring-up notes](docs/bringup.md) for evidence and the next investigation.
- Xbox/XInput controller discovery and the guest USB bus are enabled. D-pad,
  left-stick menu movement, A select, B back, and first-level movement are
  verified through USB input reports. The connected Xbox controller is detected.
- Keyboard/mouse controls are enabled by default. **Options > PC Controls** is
  available from the main menu and the gameplay pause menu, with rebinding,
  mouse speed, invert Y and remembered settings. Xbox input remains available.
- **Options > PC Settings** controls render resolution, aspect ratio
  (widescreen and ultrawide, Hor+), fullscreen, VSync, frame limit,
  anisotropic filtering and MSAA, with live changes and saved choices.
- Escape, Enter, Space or a left click skips the intro and other movies.
  Options rows, including the PC entries, retain the original alternating
  left/right slide-in animation.
- The main menu includes **Quit Game**, with a confirmation that defaults to No.
- Fixed buffered stream writes and update opens so settings and signed profiles
  persist. The first Hatshepsut level now loads with its HUD and weapon.
- Release configuration checks that C optimization flags are present.
  Hatshepsut's spawn scene improved from 48–52 fps to roughly 150–186 fps at
  automatic 3× rendering; moving onto the ramp reached about 195–205 fps.
  A normal launch also exceeds 120 fps. These are first-level measurements;
  extended play and audio quality remain unverified.
- **Runs from the untouched disc folder** (2026-11-02). The toolkit now
  dispatches the game's C++ exceptions to its own catch blocks, so the engine's
  "file not found, use the default" paths work as on the console. A fresh
  install with an empty save folder goes from Player Name through the intro to
  Hatshepsut gameplay (~185 fps) with no placeholder files, loose assets or
  settings wrappers. Known issue: an intermittent GPU fence stall at the boot
  movie (seen in 4 of 15 clean runs; see [bring-up notes](docs/bringup.md)).
- Corrected guest thread identities in toolkit commit `6c87b8c`: the game's
  recursive locks previously saw every thread as ID zero. A regression verifies
  distinct IDs and lock exclusion. This addresses a real synchronization bug;
  its effect on the intermittent intro freeze is still being evaluated.
- Fixed the cyan Options screen and missing rows in toolkit `0938eda`.
  Slider fills now honour their clear rectangles and packed RGB565 colours.
  The green background and all six rows render correctly; controller changes
  and settings reload are verified with isolated saves and framebuffer captures.
- Menu optimization: matched, logging-only measurements improved from 31.5 to
  76.8 fps at automatic 3× rendering. The engine swap path now follows native
  vsync configuration, and common x87 rounding avoids an out-of-line helper
  without changing precision. These are menu results, not a general 120+ fps
  gameplay guarantee. See [bring-up notes](docs/bringup.md) for validation.
- Rendering corrections: extended x87 constants now load as 80-bit operands,
  fixing the zero-pitch result for ground-facing stains. Native occlusion
  reports use actual passing pixels instead of always reporting full visibility,
  addressing light halos through walls. Mathematical and GPU regressions pass;
  user gameplay verification of these two effects is pending.
- Slow-area optimization: a user-positioned Hatshepsut view improved from
  **37.2 to 142.9 fps** at the existing automatic 3× scale. Scene depth now stays
  scaled through colour-masked queries and model depth passes, and visibility
  reports complete asynchronously with ordered fences. The loading/video
  presenter also honours native vsync-off. See [bring-up notes](docs/bringup.md)
  for the short live comparison and its limits.
- Shader bytecode persists across launches, including presentation, scaling,
  rectangle-clear and depth-resolve helpers. A two-process cold/warm graphics
  test compiles 12 shaders initially, then reloads all 12 with zero source
  compiles. This reduces repeat compilation stalls; steady scene frame rate
  still depends on the scene's work.

- Frame limit (2026-11-06): with a limit set, Tomb of Ramses dropped from 120
  to 64 fps while uncapped play held. Each fence wait gave the PC GPU up to
  2 ms to return visibility counts, and a lightly loaded GPU used it every
  time; the grace is now 0.25 ms and the spot holds 120.

## Playing

You need your own copy of Serious Sam for the original Xbox, **USA** release.
Keep `serious_sam_recomp.exe` and `d3dcompiler_ms.dll` together in a folder and
start the exe:

- Put your disc image (`.iso`, a full image of the disc, or a trimmed `.xiso`)
  beside the program, and it is extracted once into a `game` folder there
  (about 1.8 GB).
- Or, with no image beside it, the program asks for the image or for the
  `default.xbe` in a folder of already extracted files (`default.xbe`,
  `Main.gro`, `Movies0.gro` to `Movies5.gro`).

The folder is remembered in `serious_sam_settings.ini` beside the program, and
the game's `default.xbe` is checked to be the USA release's (SHA-1
`5463f81814860649ce6fae754e34d84b02ed93cb`): the recompiled code is that
executable's, so other releases are refused. Saves go in the game folder's
`saves`. Compiled shaders are cached in `%LOCALAPPDATA%\xboxrecomp\shadercache`,
so the first sight of a new effect may hitch once.

An Xbox/XInput controller works as on the console; keyboard and mouse work too
(see [PC controls](#pc-controls)). **Options > PC Settings** has resolution,
aspect ratio (widescreen/ultrawide), display mode, VSync, frame limit,
anisotropic filtering and MSAA. Escape, Enter, Space or a click skips movies.
The main menu has no music; that is how the original plays.

If the game crashes or freezes, the details are in `serious_sam_log.txt`
beside the program (the run before is kept as `serious_sam_log.prev.txt`).
Please include it with any report.

Known issues in this beta: only the first two levels have been played
through; a rare freeze during the boot movies; one crash once seen after
firing in Hatshepsut (not reproduced since); one native stack overflow when
loading Sand Canyon (not reproduced; the log now records where it happens).

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

The runtime is branch `serious-sam` of
[MarkPremier/xboxrecomp](https://github.com/MarkPremier/xboxrecomp); the
disassembler is upstream at the commit the Turok project uses:

```powershell
git clone -b serious-sam https://github.com/MarkPremier/xboxrecomp.git xboxrecomp_ss
git clone https://github.com/sp00nznet/xboxrecomp.git xboxrecomp
git -C xboxrecomp checkout f0f8a461496a4f23d9b0aeaa284b8d70ba7ab82b
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
function identification, ABI analysis and C generation in order. After changing
only the generator or `src/recomp_manual.c`, `-RecompOnly` reuses the existing
analysis and regenerates the C in about two minutes. It uses explicit
per-game analysis directories, excludes manual overrides, and backs up existing
generated C under `analysis/` before regeneration. Analysis, game data and generated
C are ignored by Git. The Turok runtime is never used as a shared output directory.

## Running and diagnostics

For this development checkout, double-click
`build/Release/serious_sam_recomp.exe`. The local build falls back to
`test_clean/` (below). No probe script or environment setup is required.

The launcher checks an explicit `RECOMP_GAME_DIR`, a remembered folder in
`serious_sam_settings.ini`, `game/` beside the executable, an adjacent ISO/XISO,
then the developer fallback. Otherwise it offers a file picker. It verifies the
USA `default.xbe` SHA-1 before loading. The game needs only what is on the disc:
`default.xbe`, `Main.gro` and `Movies0-5.gro`.

Explorer launches write `serious_sam_log.txt` beside the executable and retain
the preceding run as `serious_sam_log.prev.txt`. Redirected stderr remains usable
for diagnostic probes. Saves default to `saves/` within the selected game folder;
`RECOMP_SAVE_DIR` overrides this. Use a separate game/saves copy for testing.

For testing, hard-link the extracted disc into `test_clean/`. It takes no extra
space, and the game's saves and cache files stay out of the original folder:

```powershell
New-Item -ItemType Directory test_clean | Out-Null
Get-ChildItem '..\SeriousSam\Serious Sam (USA)' -File | ForEach-Object {
    New-Item -ItemType HardLink -Path "test_clean\$($_.Name)" -Target $_.FullName }
```

`test_game/` is the older bring-up folder with every `Main.gro` entry unpacked
and placeholder files added; it is no longer needed.

The launcher defaults `RECOMP_PB_EXEC`, `RECOMP_AC97_READY`, `RECOMP_FB_WINDOW`,
`RECOMP_TITLE_KEVENTS`, `RECOMP_HEAP_RECLAIM`, `RECOMP_NV2A_NATIVE_FENCES`,
`RECOMP_VBLANK` and `RECOMP_STREAM_PREFETCH` to `1` if unset. It also defaults `RECOMP_USB=1`, `RECOMP_USB_NDP=4`,
`RECOMP_USB_PORT=2` (Xbox controller 1), and `RECOMP_XINPUT_SLOT=auto`.
Connect an Xbox/XInput pad before or during play. Use the D-pad or left stick
to navigate, A to select, B to return, and Start to skip movies or pause.

Use the isolated bring-up probe to capture bounded logs:

```powershell
pwsh -File tools/boot-test.ps1 -Seconds 30 -Label native -Visible
# from a PowerShell session, to pass extra settings:
& .\tools\boot-test.ps1 -Seconds 30 -Label fresh -Visible `
    -ExtraEnv @{RECOMP_SAVE_DIR = "$PWD\test_clean\saves_fresh"}
```

`-GameDir` selects another game folder (default `test_clean/`).
`RECOMP_EH_DISPATCH=0` turns off C++ exception dispatch (throws then return to
the throw site, as before) for comparison.

`-DspPassthrough` is an optional diagnostic that disables DSP execution and
acknowledges a fixed observed mailbox. It is no longer required to pass audio
initialization and does not verify working sound.

The original XBE is loaded from the selected game folder. Use
`RECOMP_FPS_LOG=5` for periodic performance logging.
`RECOMP_CPU_RGB565=1` restores the slower CPU colour fallback for diagnosis.
Shader caching and background guest-shader compilation are enabled by default,
as in Turok. The disk cache is `%LOCALAPPDATA%\xboxrecomp\shadercache`;
`RECOMP_SHADER_CACHE=<folder>` selects another directory (`0` disables it).
`RECOMP_SHADER_ASYNC=0` selects synchronous compilation for diagnosis.
The log records the cache directory; GPU diagnostic reports distinguish disk
hits from actual source compiler calls. New uncached shaders compile on first
encounter, and changed shader sources automatically get new cache keys.
`RECOMP_RENDER_SCALE=1` selects original resolution; the runtime otherwise uses
automatic scaling. Lowering it gave little menu-speed improvement in this test.
`RECOMP_VIDEO_FPS_CAP` defaults to `200` for full-screen packed-YUV movies;
set it to `0` to disable the video limit. Movie animation already runs at normal
speed; the limit avoids repeatedly presenting the same decoded frames. Menus
and gameplay remain unlocked unless native vsync or `RECOMP_FPS_CAP` is set.
Release builds emit a linker map with a fixed image base for crash symbolization:

```powershell
.\.venv\Scripts\python.exe ..\xboxrecomp_ss\tools\symbolize.py build\Release\serious_sam_recomp.map <RIP>
```

## PC controls

Open **Options > PC Controls** from the main menu or the gameplay pause menu.
Use the arrows or controller to select a row, then Enter/A to bind a key, mouse
button or wheel direction. A duplicate binding swaps with its previous action;
Delete or right-click clears a binding. Escape/B cancels or returns. The page
scrolls with the selection or mouse wheel; its rows and Back button accept clicks.
Mouse speed (1–10), invert Y and Restore defaults are on the same page.

| Action | Default |
|---|---|
| Move | WASD |
| Look | Mouse |
| Fire | Left mouse |
| Use / sniper zoom | Right mouse |
| Jump / swim up | Space |
| Crouch / swim down | Ctrl |
| NETRICSA | Tab |
| Next / previous weapon | Wheel up / down |
| Reload | R |
| Favourite weapon 1 / 2 | 1 / 2 |
| Third-person view | H |
| Pause / Start | Escape |

Escape, Enter, Space or a left click skips a movie, as Start does on a pad.
Menus always keep arrows/WASD, Enter/Space to select and Escape/Backspace to
return, independently of gameplay bindings. Escape also supplies Start at the
loading screen's “press START” prompt. In NETRICSA and the level briefing, the
arrows or W/S scroll the text (the mouse wheel too) and A/D or left/right
change message. Click the game window to capture the
mouse for looking; Alt+Escape releases it. Opening a menu releases it too.

Settings are saved under `[pc_controls]` in `serious_sam_settings.ini` beside the
executable. They do not change the console player's controls or disc archives.
Raw mouse counts enter Sam's normal action builder, independently of frame time
and console sensitivity/invert. `RECOMP_KBM=0` disables PC gameplay input.
Tests can set `RECOMP_PC_CONTROLS_FILE=<private ini>` to isolate preferences.

### Mouse in menus

In any menu (the front end and the pause menu) the mouse works as a pointer:

| Mouse | In a menu |
|-------|-----------|
| Move over an entry | focuses it |
| Left click | chooses it; on an option row (slider or choice), the next value |
| Right click | on an option row, the previous value; elsewhere Back |
| Wheel | over an option row, its value; elsewhere up / down |

Entries are found and focused the way Sam's own menus do it: each button's
rectangle comes from its own size and alignment code (`sub_0001B237`, vtable
`+0x20`) plus its page's offset, and focus moves through its KillFocus and
SetFocus. `RECOMP_MENU_MOUSE_TRACE=1` logs every click with the boxes it
tested, and every change of the active menu. On the PC Controls and PC
Settings pages, BACK is the stop after the last row for keys and pads too.

## PC settings

**Options > PC Settings** is available from both the main and pause menus.
Left/right, Enter/A or a mouse click changes a value. Changes apply immediately
and are saved in `[pc_settings]` in `serious_sam_settings.ini` beside the exe.

| Setting | Choices |
|---|---|
| Render resolution | Auto, 640×480, 1280×960, 1920×1440, 2560×1920, 2880×2160 |
| Aspect ratio | 4:3, 16:10, 16:9, 21:9, Auto (the monitor's shape) |
| Display | Windowed, borderless fullscreen; Alt+Enter also updates the page |
| VSync | Off, On |
| Frame limit | Unlimited, 30, 60, 120, 144, 200, 240, VBlank |
| Anisotropic filtering | Game default, 2×, 4×, 8×, 16× |
| Anti-aliasing | Off, 2×, 4×, 8× MSAA |

Defaults retain automatic resolution and aspect, windowed display, VSync off
and unlimited gameplay, with 16× AF and MSAA off. The separate 200 fps movie
limit remains. Resolution sets the internal rendering height; the width follows
the aspect ratio.

Widescreen is Hor+: the vertical view is the console's
and more is shown at the sides, with the world culled for the wider view, so
nothing is missing at the edges. The HUD, menus, text and movies keep their 4:3
shape in the middle. The front end (main menu and its pages) stays 4:3; a level
and its pause menu use the chosen aspect.
Explicit `RECOMP_RENDER_SCALE`, `RECOMP_VSYNC` and `RECOMP_FPS_CAP` overrides win
at launch and appear on the page. `RECOMP_PC_SETTINGS_FILE` selects a private
graphics INI for tests; otherwise the controls INI path is shared.

## Reference

Croteam released the Serious Engine 1 source (GPL-2.0) in 2016:
<https://github.com/Croteam-official/Serious-Engine>. It is useful for
identifying functions in the XBE. If code from it is adapted here, this
repository must be licensed GPL-2.0 compatibly.
