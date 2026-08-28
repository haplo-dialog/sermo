# Feuille de route — gtk4sermo (port GTK 4)

**haplo-dialog** | **devel@haplo-dialog.fr** | Mis à jour : 2026-08-25

---

## Version actuelle : 1.0.0-12 — **publiée**

Ce n'est plus un port de développement. Le paquet est téléchargeable dans la
release [v1.1.0](https://gitlab.com/haplo-dialog/sermo/-/releases/v1.1.0), la CI
le construit et l'éprouve à chaque poussée comme le port GTK 3, et il a reçu la
même passe de sûreté mémoire (voir `TODO-SECURITY.md`).

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
| **Tests XML `tests/xml/` (55 cas, `--print-ir`)** | ✅ **55/55** (était 0/52 avant grammaire réparée) |
| **Exemples réellement ouverts** (Xvfb, `fr_FR.UTF-8`) | ✅ **58/58** — 0 plantage, 0 erreur de syntaxe, 0 sans fenêtre |
| **Tests de comportement `safe_exec`** | ✅ 0 échec |
| **Durcissement mesuré au `readelf`** | ✅ PIE, Full RELRO, BIND_NOW, NX, canari, FORTIFY, CET (IBT + SHSTK) |
| **Passe de sûreté mémoire** | ✅ alignée sur le port GTK 3 (2026-08-24) |
| **Paquet publié** | ✅ `.deb` + dbgsym dans la release v1.1.0 |

---

## Jalons

### 🔵 v1.0.0-gtk4-beta1 — « Parser complet » (cible : 2026-Q3)

**Objectif :** premier binaire `gtk4sermo` compilé et fonctionnel avec les 50 widgets.

- [x] Intégrer les 20 nouveaux tokens dans `gtkdialog_lexer.l`
- [x] Intégrer les 20 nouvelles règles dans `gtkdialog_parser.y`
- [x] Ajouter les 20 `create_widget()` dans `automaton.c`
- [x] Compilation sans erreur sur Debian Testing (0 erreur ; warnings 1549 → 77)
- [x] Grammaire réparée (commentaires `<!-- -->`, espaces entre attributs, 8 widgets câblés) — **tests XML `tests/xml/` 55/55** (était 0/52)
- [x] Test des widgets par les exemples livrés — **58/58 ouvrent leur fenêtre**, sous Xvfb, en locale française, à chaque poussée (`tests/run_examples.sh`)
- [x] Durcissement vérifié — mieux que `checksec` : `tests/garde_durcissement.sh` lit le **binaire produit** au `readelf` et exige aussi CET (IBT + SHSTK), que `checksec` ne regarde pas. C'est ce banc qui a révélé que `-fcf-protection=full` ne survivait pas à l'édition de liens sans `-Wl,-z,ibt -Wl,-z,shstk`.

### 🟢 v1.0.0-gtk4 stable (cible : 2026-Q4)

**Objectif :** paquet `.deb` stable, publié et téléchargeable.

- [x] Zéro régression vs le port GTK 3 sur la suite XML — **55/55 des deux côtés**, même corpus
- [x] Paquet `gtk4sermo` construit **et publié** (`.deb` + dbgsym, release v1.1.0)
- [x] Coexistence `gtk3sermo` + `gtk4sermo` : binaires et pages de manuel distincts, aucun conflit déclaré ; seul `gtksermo` fournit l'alias `gtkdialog`
- [ ] `valgrind` : zéro fuite sur fenêtre simple
- [ ] Dépôt APT signé (aujourd'hui : téléchargement direct + sommes SHA256, qui attestent l'intégrité, pas l'origine)

### 🟡 « GTK 4 complet » (cible : 2027)

**Objectif :** retirer ce qui reste d'emprunté à GTK 3.

- [x] `<menubar>`/`<menu>`/`<menuitem>` : **fait** — `GMenuModel` + `GtkPopoverMenu`. Avant ça, `<menuitem>` produisait un `GtkPopoverMenu` autonome empilé comme un widget ordinaire ; GTK le réalisait en surface popup sans parent et le programme mourait sur un SIGSEGV.
- [x] `<table>` : **fait** — `GtkColumnView`.
- [x] CSS : **fait** — plus aucun `override_color`/`override_background`/`override_font` dans `src/`.
- [ ] `gtk_dialog_run` : deux appels restants (`actions.c`, `printing.c`). Ce n'est pas un bouchon vide — `gtk4-compat.h` en donne une émulation synchrone réelle par `GMainLoop` local et le signal `response`, qui ne gèle pas la boucle principale. Reste à passer aux API asynchrones que GTK 4 encourage.
- [ ] `GtkDropDown` : remplacer `GtkComboBoxText` (déprécié depuis GTK 4.10)
- [ ] `GtkColumnView` : remplacer les derniers `GtkTreeView` (déprécié depuis GTK 4.10)
- [ ] `GtkFileDialog` : remplacer `GtkFileChooserDialog` (déprécié depuis GTK 4.10)
- [ ] `<gvim>` : bouchon assumé — l'embarquement par socket X11 n'a pas d'équivalent Wayland

### 🔮 Long terme

- [ ] Outil de conversion automatique de scripts GTK 3 → GTK 4

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
| menubar/menu/menuitem | ✅ | ✅ GMenuModel | ✅ |
| table (CList) | ✅ | ✅ GtkColumnView | ✅ |
| gvim (socket) | ✅ | ⛔ bouchon | ❓ sans équivalent Wayland |

**Légende :** ✅ Fonctionnel | ⚠️ Shim partiel | ⛔ Stub | — Non applicable | ❓ Incertain

---

## Contacts et ressources

- **Mainteneur :** haplo-dialog — devel@haplo-dialog.fr
- **Dépôt :** https://gitlab.com/haplo-dialog/sermo
- **Site :** https://haplo-dialog.fr
- **API GTK 4 :** https://docs.gtk.org/gtk4/
- **Projet d'origine :** gtkdialog (László Pere, 2003-2007 ; Thunor, 2011-2012)
- **Licence :** GPL-2.0-or-later

---

*Document horodaté — dernière mise à jour : 2026-08-25 (port publié, exemples 58/58, menus et `<table>` sortis du bouchon).*
