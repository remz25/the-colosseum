# COLISEUM: Changelog

## 2026-09-25 (later)
- Str/Mag split on. `build.py --debug` (debug menu, separate ROM) and `--test` (on-target tests).
- C pipeline: `src/**/*.c` -> arm-none-eabi-gcc -> lyn -> `build/Coliseum.lyn.event`, installed by
  `src/Coliseum.event` (one include line added to `ROMBuildfile.event`).
- Run state (`src/core/run_state.c`): encounter schedule (3 normal wins -> reward + Elite; 9 ->
  Boss), gold, Recover charges; saved in the game save and suspend (two chunks added to upstream
  `ExModularSave.event`: game `$11F0`, suspend `$290E`, 0x40 bytes each).
- `tests/run_tests.py` + `tests/emu/gdb.py`: on-target unit tests and save integration in mGBA.

## 2026-09-25
- Project created from the FE8 Skill System (upstream `65b959d`), own git history on `main`.
- `scripts/build.py`: checked build (clean-ROM CRC, each tool from its folder, assembler must say
  "No errors", output header/size validated). `MAKE_HACK_full.cmd` fixed to call `ColorzCore.exe`.
- Master spec saved as `docs/COLISEUM_SPEC.md`; Phase 1 analysis in `ARCHITECTURE.md`; decisions
  in `GAME_DESIGN.md`.
