# Feuille de route — sermo

## Phase 1 — Modularisation ✅ (faite)

- [x] Extraire le cœur `libsermocore` (analyse + exécution durcie, sans toolkit).
- [x] Unifier la source du cœur (9/10 fichiers) sur l'étalon gtk3sermo.
- [x] Sortir le bridge GObject du cœur vers les backends.
- [x] Deux variantes de build (neutre + GLib) depuis une source ; puis la
      variante GTK4.
- [x] `sermocore.pc` : les backends consomment le cœur par pkg-config.
- [x] Porter les sept backends sur `libsermocore` (qt6, gtk3, fltk1, efl1, sdl3,
      gtk4, ncurses).
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
- [ ] Backends supplémentaires via le contrat de frontière.
- [ ] Publication produit (dépôt apt, site).

> La progression est **gardée par les trois bancs** (XML 55 · comportement 53 ·
> gardes) : aucune phase n'avance en laissant un banc rouge.
