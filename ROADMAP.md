# Feuille de route — sermo

**haplo-dialog**, https://haplo-dialog.fr  
*Dernière mise à jour : 2026-08-25*

---

## Version actuelle : 1.0.0 (mai 2026) ✅

Première version publique stable, **deux ports** : `gtk3sermo` (backend GTK 3,
**52 balises de widget**) et `gtk4sermo` (backend GTK 4, **56** — les 52 mêmes,
plus `flowbox`, `overlay`, `revealer`, `stack`). L'alias rétro-compatible `gtkdialog`
est fourni par un troisième paquet, `gtksermo`.

Les paquets sont téléchargeables dans les *releases* du dépôt GitLab. Les
révisions d'empaquetage avancent port par port et peuvent diverger, c'est normal
(voir `VERSIONING.md` §2).

---

## v1.1.0 - Qualité & CI (Q3 2026)

Priorité : rendre le projet **vérifiable automatiquement**.

- [x] **Build des deux ports**, chacun compile et lie un binaire fonctionnel (`autoreconf -fi && ./configure && make`) ✅
- [x] **Tests XML verts**, **55/55 par port**, headless (`--print-ir`) ✅
- [x] **Compatibilité ascendante gtkdialog**, symlink `gtkdialog` → `gtk3sermo` à l'installation ; XML/CLI/variables d'environnement d'époque vérifiés ✅
- [x] **CI fonctionnelle** ✅ — sur **GitLab** (`.gitlab-ci.yml`), pas sur Gitea/Forgejo. Elle construit les deux ports, rejoue la suite XML, les tests de comportement et **sept garde-fous**, puis **lance réellement les exemples** sous Xvfb en locale `fr_FR.UTF-8`. Ce second étage existe parce que le premier ne suffisait pas : le 2026-08-22 les deux ports plantaient à l'usage pendant que le pipeline restait vert, faute d'ouvrir la moindre fenêtre.
- [x] **Suite XML complète**, 55 cas de test ✅ (`tests/xml/` : 55 fichiers couvrant tous les widgets et actions)
- [x] **Exemples exécutés, pas seulement listés** ✅ — 55/55 pour gtk3sermo et 58/58 pour gtk4sermo ouvrent vraiment leur fenêtre, en français, à chaque poussée
- [x] **Durcissement mesuré sur le binaire** ✅ — `readelf`, pas les drapeaux annoncés : c'est ainsi qu'on a découvert que CET ne survivait pas à l'édition de liens
- [x] **haplo-dialog-xml(5) dans les paquets**, manpage installée par défaut ✅
- [x] **Paquets téléchargeables** ✅ — cinq `.deb` plus `SHA256SUMS` dans les *releases* GitLab, re-téléchargés anonymement pour vérification
- [ ] **`.deb` signé**, la signature du dépôt APT n'existe pas encore (les sommes SHA256 sont publiées, elles attestent l'intégrité, pas l'origine)
- [ ] **Valgrind propre**, zéro fuite sur les 55 XML de référence, dans les deux ports
- [ ] **cppcheck en CI**, analyse statique automatique, zéro erreur

---

## v1.2.0 - Fonctionnalités (Q4 2026)

Priorité : enrichir sans casser la compatibilité.

- [ ] **`--validate` mode**, parse le XML et retourne les erreurs sans afficher
- [ ] **Thème CSS externe** gtk3sermo, `--style=fichier.css`
- [ ] **`<filewatcher>`**, widget spécifique haplo-dialog, surveille un fichier et rafraîchit
- [ ] **Internationalisation**, support `gettext` pour les labels fixes

---

## v2.0.0 - Architecture (2027)

Priorité : rendre le projet maintenable à long terme.

- [ ] **Bibliothèque partagée** `libhaplodialog.so`, API C pour intégration dans d'autres outils
- [ ] **`<script>` Lua**, actions scriptables en Lua (alternative à shell)
- [ ] **Accessibilité (AT-SPI)**, support lecteur d'écran pour gtk3sermo
- [ ] **Documentation PDF**, `makeinfo --pdf` généré et distribué
- [ ] **Snap / Flatpak**, format d'empaquetage universel pour gtk3sermo

---

## Port de référence - niveau de garantie

| Port | Garantie | Mainteneur | CI |
|------|----------|-----------|-----|
| `gtk3sermo` | ✅ Production | haplo-dialog | oui — build, XML, comportement, 7 garde-fous, exemples réels |
| `gtk4sermo` | ✅ Production | haplo-dialog | oui — même pipeline, mêmes bancs |

`gtk3sermo` reste le **port de référence** : c'est celui qui a le plus d'usage
réel derrière lui, et l'alias rétro-compatible `gtkdialog` pointe vers lui.
`gtk4sermo` a reçu les mêmes passes de sûreté et passe les mêmes bancs, mais il
est plus jeune. Ce qui reste ouvert est listé port par port dans les
`TODO-SECURITY.md`.

### Ancrage Wayland — une seule différence assumée

`layer`, `edge`, `dist` et `reserve` ne sont implémentés **que** côté GTK 3
(via `gtk-layer-shell`). Sur le port GTK 4, ces quatre attributs sont ignorés et
une fenêtre ordinaire s'ouvre. Ce n'est pas une régression à corriger sous peu :
le protocole wlr-layer-shell n'a pas d'équivalent GTK 4 aussi établi.

---

## Ce qui ne sera PAS fait

Pour éviter la dispersion :

- Pas de port Windows (hors scope du projet)
- Pas de port macOS
- Pas de binding Python/Ruby (utiliser directement le binaire depuis le shell)
- Pas de support GTK2 (abandonné)
- Pas de remplacement de Zenity pour les cas simples, sermo cible les dialogues complexes
- Pas de `clang-format` imposé sur le code hérité : le réimprimer réécrirait au
  moins un tiers de chaque fichier et rendrait les correctifs illisibles
  (mesuré, voir `CONTRIBUTING.md` §5)

---

## Comment contribuer à la roadmap

Ouvrir un ticket avec le label `roadmap` ou écrire à `devel@haplo-dialog.fr`.  
Les priorités sont discutées ouvertement, toute suggestion est bienvenue.

---

*sermo, "Le shell mérite une vraie interface graphique."*

---

