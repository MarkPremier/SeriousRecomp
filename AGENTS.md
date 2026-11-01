# Agent instructions

This repo is a static recompilation of Serious Sam (Xbox, USA) to native Windows.

Before you start any task, read `.agents/skills/serious-sam-recomp/SKILL.md`. It covers
the current state, the paths, which toolkit to use, the full pipeline, lessons from the
earlier Turok: Evolution recomp, and the folders you must not modify.

The user explicitly asked to follow the Turok project's approach (2026-11-02):
usable standalone launches, automatic/remembered game-file discovery, useful logs,
and progress verified in the actual game window through movies, menus and gameplay.
GPU rendering and playable performance are the target; CPU bring-up fallbacks are
temporary. Verify the normal executable launch separately from diagnostic probes.
