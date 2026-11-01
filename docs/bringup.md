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
