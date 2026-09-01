# ROADMAP — qt6sermo (port Qt6)

**haplo-dialog** | **devel@haplo-dialog.fr** | **Dépôt :** https://haplo-dialog.fr | Mis à jour : 2026-06-10

---

## Version actuelle : 1.0.0

| Composant | État |
|-----------|------|
| Build system (CMake 3.20, Qt6 ≥ 6.2) | ✅ Opérationnel |
| qt6-compat.h (pont C/C++ + extern "C") | ✅ Complet |
| Widgets (`src/widget_*.cpp`) | ✅ 47 fichiers (49 widgets fonctionnels) |
| `<table>` (QTreeWidget multi-colonnes) | ✅ Intégré |
| `<menubar>`/`<menu>` (QMenuBar/QMenu) | ✅ Intégré |
| `<terminal>` (QTermWidget6 optionnel) | ✅ Fallback QPlainTextEdit |
| Durcissement (FORTIFY=3, PIE, RELRO, CFI, LTO) | ✅ Complet |
| **Bilan de santé** | ✅ **90/100** |
| **Build (`cmake --build`)** | ✅ **rc=0, 0 erreur — qt6sermo 240 Ko** |
| **Tests XML `tests/xml/` (52 cas, `--print-ir`)** | ✅ **52/52** (était 50/52 ; levelbar/drawingarea câblés) |
| **Tests du cœur (`tests/run_unit_tests.sh`)** | ✅ **9/9** |
| **Tests de comportement widgets (`tests/behaviour/`, 12 cas)** | ✅ **12/12** (2026-06-10, après correctifs timer + hscale) |
| CI (`.gitlab-ci.yml` — étages docs / build-test xvfb-run / qa) | ✅ Opérationnelle |
| Intégration bureau (`data/qt6sermo.desktop` + `.metainfo.xml`) | ✅ Fournie |

---

## Jalons

### 🔵 v1.0.0 stable — « Release Haplo » (cible : 2026-Q4)

- [x] Build CMake reproductible (Release + LTO) — vérifié rc=0, qt6sermo 240 Ko
- [x] Pont C/C++ propre (qt6-compat.h, extern "C")
- [x] Widgets P1 + table + menubar + terminal
- [x] Tests XML `tests/xml/` 52/52 (`--print-ir`) — levelbar/drawingarea câblés (était 50/52)
- [x] Tests de comportement par widget (`tests/behaviour/`, 12/12 sous xvfb-run)
- [ ] Tests fonctionnels sur les exemples (`examples/*/`)
- [x] `checksec` → Full RELRO, PIE, Stack Canary, CFI
- [ ] Paquet `.deb` / `.rpm` dans le dépôt Haplo — publication apt **en préparation** (`tools/make-apt-repo.sh` à la racine)

### 🟢 v1.0.0 — « Qt6 complet » (cible : 2027)

- [ ] `g_signal_connect` : pont réel signaux GTK→Qt (actuellement no-op)
- [ ] Remplacer les statement-expressions GCC (`g_slist_last`/`g_list_last`) par du C ISO
- [ ] Sélection multi-lignes dans `<table>`
- [ ] Thème Qt6 aligné sur Haplo-Dark

### 🔮 v1.1.0 — « Multi-toolkit » (long terme)

- [ ] Sélection du toolkit via `HAPLO_DIALOG_TOOLKIT=gtk3|gtk4|qt6|fltk|efl|sdl3`
- [ ] API shell commune entre les six ports

---

## Contacts et ressources

- **Mainteneur :** S.Cage, D.Marques — devel@haplo-dialog.fr
- **Dépôt :** https://haplo-dialog.fr
- **Qt6 API :** https://doc.qt.io/qt-6/
- **Licence :** GPL-2.0-or-later

---

*Document horodaté — dernière mise à jour : 2026-06-10 (audit #6 — haplo-dialog v1.0.0).*