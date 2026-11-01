# SeriousRecomp

A static recompilation of **Serious Sam** (Xbox, 2002, USA) to native Windows,
built with [sp00nznet/xboxrecomp](https://github.com/sp00nznet/xboxrecomp).

The game's x86 code is translated to C and compiled into a Windows `.exe`;
the Xbox kernel, GPU (NV2A) and audio (MCPX APU) are provided by the
xboxrecomp runtime. **No game code or data is in this repository.** You need
your own disc image, and the C code is generated from it on your machine.

## Status

- Project scaffolded from the xboxrecomp `new-game` template. Not yet building.

## What you need

- An original Xbox disc image of Serious Sam (USA), XISO format
- Windows 10/11 with Visual Studio 2022 or later ("Desktop development with C++",
  which includes CMake)
- Python 3.10+ with `pip install capstone`
- [xdvdfs](https://github.com/antangelo/xdvdfs) or extract-xiso

## Setup

The layout this project expects, side by side:

```
Recomp/
  SeriousSam/Serious Sam (USA)/   extracted disc (default.xbe, data)
  xboxrecomp/                     the toolkit
  serious_sam/                    this repository
```

1. **Extract the disc**

   ```
   xdvdfs unpack "Serious Sam (USA).iso" "SeriousSam/Serious Sam (USA)"
   ```

2. **Parse the XBE** and set `YOUR_GAME_ENTRY_POINT` in `src/main.c`

   ```
   cd xboxrecomp
   py -3 -m tools.xbe_parser "../SeriousSam/Serious Sam (USA)/default.xbe"
   ```

3. **Disassemble and recompile** into `src/recomp/gen/` (see the xboxrecomp docs),
   then configure and build with CMake.

## Reference

Croteam released the Serious Engine 1 source (GPL-2.0) in 2016:
<https://github.com/Croteam-official/Serious-Engine>. It is useful for
identifying functions in the XBE. If code from it is adapted here, this
repository must be licensed GPL-2.0 compatibly.
