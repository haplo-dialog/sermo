<div align="right"><sub><code>maj :     2026-08-22 16:30:41 · par claude</code></sub></div>
<div align="right"><sub><code>réindex : —</code></sub></div>

# ROADMAP — gtk4sermo (port GTK4)

**Haplo-Linux** | **devel@haplo-dialog.fr** | Mis à jour : 2026-06-07

---

## Version actuelle : 1.0.0-gtk4 (dev)

| Composant | État |
|-----------|------|
| Build system (autotools) | ✅ Opérationnel |
| gtk4-compat.h (65+ shims) | ✅ Complet |
| Widgets originaux portés | ✅ 27 widgets |
| Widgets GTK3 portés en GTK4 | ✅ 11 widgets |
| Widgets GTK4-natifs | ✅ 9 widgets |
| **Total widgets** | ✅ **50 widgets** |
| Parser (lexer/bison) — nouveaux tokens | ✅ Intégré |
| automaton.c — routing nouveaux widgets | ✅ Intégré |
| **Compilation complète (0 erreur)** | ✅ **gtk4sermo ~1,45 Mo** |
| Warnings de compilation | ✅ 1549 → 77 |
| **Tests XML `tests/xml/` (52 cas, `--print-ir`)** | ✅ **52/52** (était 0/52 avant grammaire réparée) |

---

## Jalons

### 🔵 v1.0.0-gtk4-beta1 — « Parser complet » (cible : 2026-Q3)

**Objectif :** premier binaire `gtk4sermo` compilé et fonctionnel avec les 50 widgets.

- [x] Intégrer les 20 nouveaux tokens dans `gtkdialog_lexer.l`
- [x] Intégrer les 20 nouvelles règles dans `gtkdialog_parser.y`
- [x] Ajouter les 20 `create_widget()` dans `automaton.c`
- [x] Compilation sans erreur sur Debian Testing / Haplo-Linux (0 erreur ; warnings 1549 → 77)
- [x] Grammaire réparée (commentaires `<!-- -->`, espaces entre attributs, 8 widgets câblés) — **tests XML `tests/xml/` 52/52** (était 0/52)
- [ ] Test des 50 widgets (scripts `examples/*/`)
- [ ] `checksec` → Full RELRO, PIE, Stack Canary

### 🟢 v1.0.0-gtk4 stable — « Release Haplo » (cible : 2026-Q4)

**Objectif :** paquet `.deb` stable dans le dépôt Haplo.

- [ ] Zéro régression vs gtk3d GTK3 (scripts 0.8.3 compatibles)
- [ ] `valgrind` : zéro leak sur fenêtre simple
- [ ] Paquet `gtk4sermo_1.0.0_amd64.deb` créé
- [ ] Coexistence `gtk3d` (GTK3) + `gtk4sermo` (GTK4) validée
- [ ] Documentation complète (texi regeneré, manuel à jour)
- [ ] Soumission dépôt https://haplo-dialog.fr

### 🟡 v1.0.0-gtk4 — « GTK4 complet » (cible : 2027)

**Objectif :** supprimer les stubs restants, migration GTK4 complète.

- [ ] `<menubar>/<menu>/<menuitem>` : migration GMenuModel + GtkPopoverMenu
- [ ] `<table>` : migration GtkColumnView + GListModel
- [ ] `gtk_dialog_run` : réécriture asynchrone
- [ ] CSS theming : suppression de tous les `override_*` restants
- [ ] GtkDropDown : remplacer GtkComboBoxText (déprécié GTK4.10)
- [ ] GtkColumnView : remplacer GtkTreeView (déprécié GTK4.10)
- [ ] GtkFileDialog : remplacer GtkFileChooserDialog (déprécié GTK4.10)

### 🔮 v1.1.0 — « Multi-toolkit » (long terme)

Intégration dans le meta-projet haplo-dialog :

- [ ] API shell commune entre les 5 ports
- [ ] Outil de conversion automatique scripts GTK3 → GTK4

---

## Matrice des widgets par version

| Widget | GTK3 (1.0.0) | GTK4 (1.0.0) | v1.0 |
|--------|:---:|:---:|:---:|
| button, checkbox, entry, edit… (27 originaux) | ✅ | ✅ | ✅ |
| switch | ✅ | ✅ | ✅ |
| calendar | ✅ | ✅ | ✅ |
| filechooser | ✅ | ✅ | ✅ |
| image | ✅ | ✅ | ✅ |
| infobar | ✅ | ⚠️ shim | ✅ CSS |
| linkbutton | ✅ | ✅ | ✅ |
| password | — | ✅ GTK4 | ✅ |
| pulse | ✅ | ✅ | ✅ |
| searchentry | ✅ | ✅ | ✅ |
| spinner | ✅ | ✅ | ✅ |
| aspectframe | ✅ | ✅ shim | ✅ |
| levelbar | — | ✅ GTK4 | ✅ |
| revealer | — | ✅ GTK4 | ✅ |
| stack | — | ✅ GTK4 | ✅ |
| flowbox | — | ✅ GTK4 | ✅ |
| overlay | — | ✅ GTK4 | ✅ |
| drawingarea | — | ✅ GTK4 | ✅ |
| menubar/menu/menuitem | ✅ | ⛔ stub | ✅ GMenuModel |
| table (CList) | ✅ | ⛔ stub | ✅ ColumnView |
| gvim (socket) | ✅ | ⛔ stub | ❓ Wayland |

**Légende :** ✅ Fonctionnel | ⚠️ Shim partiel | ⛔ Stub | — Non applicable | ❓ Incertain

---

## Contacts et ressources

- **Mainteneur :** haplo-dialog — devel@haplo-dialog.fr
- **Dépôt :** https://haplo-dialog.fr
- **GTK4 API :** https://docs.gtk.org/gtk4/
- **Projet original :** https://code.google.com/archive/p/gtk3d/
- **Licence :** GPL-2.0-or-later

---

*Document horodaté — dernière mise à jour : 2026-06-07 (audit #5 — grammaire réparée, tests XML 0/52 → 52/52).*
