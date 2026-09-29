# Roadmap — sermo

## Phase 1 — Modularisation ✅ (done)

- [x] Extract the core `libsermocore` (parsing + hardened execution, no toolkit).
- [x] Unify the core source (9/10 files) on the gtk3sermo reference.
- [x] Move the GObject bridge out of the core into the backends.
- [x] Two build variants (neutral + GLib) from one source; then the GTK4 variant.
- [x] `sermocore.pc`: backends consume the core via pkg-config.
- [x] Port all seven backends onto `libsermocore` (qt6, gtk3, fltk1, efl1, sdl3, gtk4, ncurses).
- [x] Separate packaging (`sermo-core-dev` + `sermo-backend-*`), disjoint deps,
      `sermo` alias, `gtkdialog` bridge.

## Phase 2 — Consolidation ✅ (done)

- [x] Resolve gtk4's 3 value discrepancies (`05-filechooser`, `11-infobar`,
      `13-password`) — a two-enum mix in the `SERMOCORE_GTK4` variant, fixed
      2026-09-09. **gtk4 = 24/24.**
- [x] Full documentation + man page + Texinfo.
- [x] Integrate gtk4 into `packaging/build-debs.sh` — 9 `.deb` (core + 7 backends + `sermo-gtkdialog`).

## Phase 3 — Clean frontier

- [x] Rename the `qt6_*` bridges to `sermo_be_*` (neutral core + 5 neutral
      backends): the core no longer names a port. Loop = `sermo_be_run_loop()`.
- [x] **Drop the dependency on the former monolithic edition**: the GTK4 core (variant
      `SERMOCORE_GTK4`) is now **vendored** in `libsermocore/src-gtk4/`; the repo
      builds on its own (gtk4: XML 55/55 · behaviour 24/24).
- [ ] **Single source** for gtk3+gtk4 (drop `SERMOCORE_GTK4`): GTK 4 removed
      `gtk_main`/`gtk_socket`/the event model, so five core files diverge;
      reconciling them into one source remains the real work.
- [ ] Formalise an IR consumed by the backend → clean shared `.so` (later step).

## Phase 4 — Ecosystem

- [ ] Process isolation (IPC): the shell confined to the core.
- [ ] Additional backends via the frontier contract.
- [ ] Product publication (apt repository, website).

> Progress is **guarded by the three benches** (XML 55 · behaviour 53 · guards):
> no phase advances while a bench is red.
