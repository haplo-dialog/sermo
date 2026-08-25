# Roadmap — gtk3sermo-haplo1

**Mainteneur :** haplo-dialog <devel@haplo-dialog.fr>  
**Dépôt :** https://haplo-dialog.fr  
**Mise à jour :** 2026-06-06 (audit #5 — vérification finale)

---

## Version actuelle : 1.0.0

Migration GTK2→GTK3 complète, sécurité renforcée, 13 nouveaux widgets natifs GTK3.
Voir `BILAN_SANTE.md` pour le détail des corrections (score 100/100 patch 22).

**Vérification finale (2026-06-06) :** build `make` → rc=0, 0 erreur ; tests XML
`tests/xml/` 55/55 ; compatibilité ascendante gtkdialog confirmée (symlink
`gtkdialog`→`gtk3sermo`, alias man `gtkdialog.1`, dialogues d'époque XML/CLI/--program).

---

## v1.0.0 stable — Actions bloquantes avant publication

| ID  | Action                                        | Priorité | Statut |
|-----|-----------------------------------------------|----------|--------|
| ~~A1~~ | ~~Compiler sur Debian 12 / Haplo-Linux~~ | ~~CRITIQUE~~ | ✅ **Résolu patch 17** |
| ~~A2~~ | ~~Corriger les erreurs de compilation résiduelles~~ | ~~CRITIQUE~~ | ✅ **Résolu patch 17** |
| A3  | Valider `checksec` : RELRO=Full, PIE=Enabled, Canary=Found | HAUTE | En attente |
| A4  | Valider `valgrind --leak-check=full` sur exemples de base | HAUTE | En attente |
| ~~A5~~ | ~~Construire et tester le paquet `.deb`~~ | ~~HAUTE~~ | ✅ **Résolu patch 22** — `gtk3sermo_1.0.0-6_amd64.deb` |
| A6  | Publier le paquet dans un dépôt APT | NORMALE | En attente |

---

## v1.0.0 — Bugs runtime corrigés (patches 14-22)

| ID   | Description | Patch |
|------|-------------|-------|
| T9   | Fenêtres de même nom → comportement imprévu | patch 14 |
| T11  | Ordre d'initialisation widgets / dépendances | patch 15 |
| T13  | Guillemet non appairé imprimé sur stdout | patch 16 |
| T14  | Commentaires XML `<!-- -->` plantaient le parser | patch 18 |
| T15  | `<frame label="...">` affichait `label="..."` littéralement | patch 18 |
| T16  | `<window>` ignorait tous ses attributs de taille | patch 18 |
| T17  | `<edit>` avec `<input>` command : warning "not implemented" | patch 18 |
| T20  | Lexer : espaces entre attributs de tag → syntax error | patch 20 |
| T21  | Parser : 1310 conflits r/r bloquant les widgets Lot 1 en séquence | patch 20 |
| T22  | `pulse:/start:/stop:` → `safe_system()` "No such file" | patch 21 |
| T23  | `condition="active_is_true"` bare → "Unknown expression" | patch 21 |

---

## v0.9.1 — Correctifs et polissage (Q3 2026)

### Nouveaux widgets — Lot 1 (2026-05-24)

- [x] `<vseparator>` — GtkSeparator vertical (délégation widget_hseparator, tokens propres)
- [x] `<vscale>` — GtkScale vertical (délégation widget_hscale, tokens propres)
- [x] `<spinner>` — GtkSpinner, actions start/stop, variable 0/1
- [x] `<image>` — GtkImage natif GTK3, icon-name + file, tous icon-size
- [x] `<pulse>` — GtkProgressBar mode pulse, compatible `<timer>` pour auto-animation
- [x] `<password>` — GtkEntry masqué, icône œil toggle, placeholder, max-length
- [x] `<aspectframe>` — GtkAspectFrame, ratio/xalign/yalign/obey-child/label

### Améliorations restantes

- [ ] Tester `create_gvim()` de bout en bout et retirer le marqueur EXPERIMENTAL (R9)
- [ ] Ajouter `--version` machine-readable (format `major.minor.patch`)
- [ ] Valider `checksec` + `valgrind` (A3, A4)
- [x] ~~Construire le paquet `.deb` (A5)~~ — `gtk3sermo_1.0.0-2_amd64.deb` produit
- [ ] Publier le paquet dans un dépôt APT (A6)
- [ ] Lot 2 : `<paned>`, `<scrolledwindow>`, `<grid>`, `<levelbar>`, `<toolbar>`, `<aboutdialog>`, `<notification>`, `<completion>`

---

## v1.0 — Migration GTK4 (2027)

### Objectif

Produire un binaire `gtk3sermo` fonctionnel sur GTK4, compatible avec les scripts
gtk3sermo 0.9.x, sans dépendances GTK3.

### Pré-requis

- GTK4 >= 4.12 disponible sur Haplo-Linux ou sa version suivante
- Base de code 0.9.x stable avec suite de tests automatisés

### API GTK3 à remplacer pour GTK4

Ces API sont intentionnellement conservées en 0.9.x (valides GTK3) mais devront
être migrées pour GTK4 :

| API GTK3 conservée              | Occurrences | Remplacement GTK4          | Fichier(s) |
|---------------------------------|-------------|----------------------------|------------|
| `gtk_box_pack_start/end()`      | 9           | `gtk_box_append/prepend()`  | widget_*.c |
| `gtk_container_get_children()`  | 2           | `gtk_widget_get_first_child()` + iteration | widget_list.c |
| `gtk_container_add()`           | 6           | `gtk_box_append()` ou widget-specific | automaton.c, widget_*.c |

### Changements architecturaux prévus pour GTK4

- Remplacer `GtkSocket`/`WIDGET_GVIM` par une solution moderne (Wayland-compatible)
- Migrer `GtkFileChooserButton` (supprimé GTK4) vers `GtkFileDialog`
- Remplacer `GtkInfoBar` (deprecated GTK4.10) par une solution custom
- Adapter le système de signaux (`GdkEventMask` supprimé en GTK4)
- Parser Bison/Flex : inchangé — pas de dépendance à GTK

### Critères d'acceptation v1.0

1. Compile sans erreur avec GTK4 et `-Wall -Wextra`
2. 100% des exemples fonctionnent sans modification
3. `checksec` retourne tous les flags de sécurité
4. Suite de tests `make check` passe à 100%
5. Paquet `.deb` publié dans un dépôt APT

---

## Versionnage

| Version | GTK | Statut |
|---------|-----|--------|
| 0.8.3   | GTK2 | Abandonné (2012) |
| 1.0.0 | GTK3 >= 3.22 | En cours de stabilisation |
| 0.9.1   | GTK3 >= 3.22 | Planifié Q3 2026 |
| 1.0     | GTK4 >= 4.12 | Planifié 2027 |

---

## Contribution

Patches, rapports de bugs et demandes de fonctionnalités : **devel@haplo-dialog.fr**  
Format des commits : `widget_name: description courte (#issue)` ou `core: description`  
Licence : GPL-2.0-or-later — voir `COPYING`

---

*Haplo-Linux — 2026 — GPL-2.0-or-later*

---

*Document horodaté — dernière mise à jour : 2026-06-07 (audit #5 — vérification finale : build rc=0, tests XML 55/55).*
