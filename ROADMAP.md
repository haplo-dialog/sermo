# Feuille de route — sermo

[English](ROADMAP.en.md)

## Phase 1 — Modularisation ✅ (faite)

- [x] Extraire le cœur `libsermocore` (analyse + exécution durcie, sans toolkit).
- [x] Unifier la source du cœur (9/10 fichiers) sur l'étalon gtk3sermo.
- [x] Sortir le bridge GObject du cœur vers les backends.
- [x] Deux variantes de build (neutre + GLib) depuis une source ; puis la
      variante GTK4.
- [x] `sermocore.pc` : les backends consomment le cœur par pkg-config.
- [x] Porter les sept backends sur `libsermocore` (qt6, gtk3, fltk1, efl1, sdl3,
      gtk4, ncurses). lvgl et html sont venus ensuite (2.8.0) : neuf aujourd'hui.
- [x] Packaging séparé (`sermo-core-dev` + `sermo-backend-*`), dépendances
      disjointes, alias `sermo`, pont `gtkdialog`.

## Phase 2 — Consolidation ✅ (faite)

- [x] Résorber les 3 écarts de valeur de gtk4 (`05-filechooser`, `11-infobar`,
      `13-password`) — mélange de deux énums `WIDGET_*` dans la variante
      `SERMOCORE_GTK4`, corrigé le 2026-09-09. **gtk4 = 24/24.**
- [x] Documentation complète + page de manuel + Texinfo.
- [x] Intégrer gtk4 dans `packaging/build-debs.sh` — 9 `.deb` (cœur + 7 backends
      + `sermo-gtkdialog`).

## Phase 3 — Frontière propre

- [x] Renommer les ponts `qt6_*` → `sermo_be_*` (cœur neutre + 5 backends neutres) :
      le cœur ne nomme plus un port. Boucle = `sermo_be_run_loop()`.
- [x] **Retirer la dépendance à l'ancienne édition monolithique** : le cœur GTK4 (variante
      `SERMOCORE_GTK4`) est désormais **vendoré** dans `libsermocore/src-gtk4/` ;
      le dépôt se construit seul (gtk4 : XML 55/55 · comportement 24/24).
- [ ] **Source unique** gtk3+gtk4 (retirer `SERMOCORE_GTK4`) : GTK 4 ayant retiré
      `gtk_main`/`gtk_socket`/le modèle d'évènements, cinq fichiers cœur divergent ;
      les réconcilier en une seule source reste le vrai travail.
- [ ] Formaliser une IR consommée par le backend → `.so` partagée propre (étape ultérieure).

## Phase 4 — Écosystème

- [ ] Cloisonnement de processus (IPC) : shell confiné au cœur.
- [ ] Backends supplémentaires via le contrat de frontière. *(2.8.0 : lvgl et html, sur le
      cœur neutre.)*
- [x] Site (`haplo-dialog.fr`) et paquets joints à chaque publication, avec
      sommes de contrôle signées (`SHA256SUMS.asc`, depuis la 2.8.2).
- [ ] Dépôt apt (il n'y en a pas aujourd'hui).

## Sources et cibles — lire, puis écrire sur un clic

- [x] **3.0.0** (publiée le 2026-10-15) : lire un automate, en lecture seule —
      `<input modbus>`, `<input opcua>`.
- [x] **3.1.0** (2026-10-19) : MQTT (`<input mqtt>`, `<action mqtt>`),
      MAVLink (`<input mavlink>` ; `<action mavlink>` : réglage au sol, ordres
      de sécurité, envoi d'une mission sans la lancer), son et vidéo
      (`<input media>`, `<action media>`, mpv en cage bubblewrap).
- [x] **3.1.1** (2026-10-19) : la vidéo dans la fenêtre du dialogue (gtk3
      sous X11, `dans=$ZONE`), repli sur une fenêtre séparée ailleurs.
- [x] **3.1.2** (2026-10-20) : sécurité du lecteur, la cage de mpv ne
      laisse plus joindre les sockets de session (bus D-Bus…).
- [ ] **3.2.0** : piloter un drone (armer, décoller, changer de mode, se
      déplacer) ; la vidéo dans la fenêtre du dialogue sur les autres ports et
      sous Wayland (déjà faite en 3.1.1 pour gtk3 sous X11) ; le lecteur
      essayé sous Wayland.

> La progression est **gardée par les trois bancs** (XML 55 · comportement 78 ·
> gardes) : aucune phase n'avance en laissant un banc rouge.
