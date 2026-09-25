# CLAUDE.md: COLISEUM (FE8 roguelike on the FE8 Skill System)

**Read first each session:** `TODO.md` (where we are), `GAME_DESIGN.md` (decisions),
`docs/COLISEUM_SPEC.md` (the master spec, source of truth), `ARCHITECTURE.md`.

## Rules
- Follow the spec. Never change a core rule (spec §88) or anything in §89's "must ask" list without
  asking the developer. Record every gameplay-affecting decision in `GAME_DESIGN.md`.
- Build incrementally; a feature is done only when it meets spec §91 (works in game, tested,
  documented). Keep `TEST_STATUS.md` honest: say what was verified in mGBA and what wasn't.
- Update `TODO.md` and `CHANGELOG.md` with every change; commit with a clear message.
- Never commit ROMs or saves (`*.gba`, `*.sav` are git-ignored). Never modify `FE8_clean.gba`.
- Upstream Skill System code: prefer config switches and new files over editing upstream files, so
  upstream fixes stay mergeable (`git remote upstream`). When an upstream file must change, note it
  in `CHANGELOG.md`.

## Commands
- Build: `py -3 scripts/build.py` (full) or `--quick`. Output `Coliseum.gba`. Close mGBA first.
- Do not use `MAKE_HACK_full.cmd` for verification (it can fail silently); `scripts/build.py`
  checks every step.

## Testing without the developer
- Emulator: copy the ROM (and a save) to a scratch folder, start `mGBA.exe --gdb <rom>`, drive it
  over GDB (key injection at FE8U's key read `0x080013FC`, VRAM screenshots). mGBA exits when the
  debugger disconnects, so keep one persistent driver connection per session.
- Reference: FE8 decomp `github.com/FireEmblemUniverse/fireemblem8u`; function addresses from
  FEBuilder's `asmmap_FE8.en.txt` and disassembly.
