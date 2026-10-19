# Roadmap — sermo

[Français](ROADMAP.md)

## Phase 1 — Modularisation ✅ (done)

- [x] Extract the core `libsermocore` (parsing + hardened execution, no toolkit).
- [x] Unify the core source (9/10 files) on the gtk3sermo reference.
- [x] Move the GObject bridge out of the core into the backends.
- [x] Two build variants (neutral + GLib) from one source; then the GTK4 variant.
- [x] `sermocore.pc`: backends consume the core via pkg-config.
- [x] Port all seven backends onto `libsermocore` (qt6, gtk3, fltk1, efl1, sdl3, gtk4, ncurses).
      lvgl and html came later (2.8.0): nine today.
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
- [ ] Additional backends via the frontier contract. *(2.8.0: lvgl and html, on the
      neutral core.)*
- [x] Website (`haplo-dialog.fr`) and packages attached to each release, with
      signed checksums (`SHA256SUMS.asc`, since 2.8.2).
- [ ] apt repository (there is none today).

## Sources and targets — read, then write on a click

- [x] **3.0.0** (released on 2026-10-15): read a PLC, read-only —
      `<input modbus>`, `<input opcua>`.
- [x] **3.1.0** (2026-10-19): MQTT (`<input mqtt>`, `<action mqtt>`),
      MAVLink (`<input mavlink>`; `<action mavlink>`: ground settings, safety
      orders, sending a mission without starting it), sound and video
      (`<input media>`, `<action media>`, mpv in a bubblewrap cage).
- [x] **3.1.1** (2026-10-19): video inside the dialog window (gtk3 under
      X11, `dans=$ZONE`), separate-window fallback elsewhere.
- [x] **3.1.2** (2026-10-20): player security, mpv's cage no longer
      reaches the session sockets (D-Bus bus…).
- [ ] **3.2.0**: pilot a drone (arm, take off, change mode, move); video in
      the dialog's window on the other ports and under Wayland (already done
      in 3.1.1 for gtk3 under X11); the player tried under Wayland.

> Progress is **guarded by the three benches** (XML 55 · behaviour 78 · guards):
> no phase advances while a bench is red.
