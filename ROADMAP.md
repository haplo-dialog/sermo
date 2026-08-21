# Roadmap - haplo-dialog

**haplo-dialog**, https://haplo-dialog.fr  
*Dernière mise à jour : 2026-06-07*

---

## Version actuelle : 1.0.0 (mai 2026) ✅

Première version publique stable. Port gtk3sermo (backend GTK 3), 43 widgets, sécurité renforcée.

---

## v1.1.0 - Qualité & CI (Q3 2026)

Priorité : rendre le projet **vérifiable automatiquement**.

- [x] **Build gtk3sermo**, le port de référence compile et lie un binaire fonctionnel (autoreconf -fi && ./configure && make) ✅
- [x] **Tests XML verts**, 52/52 (headless, `--print-ir`) ✅
- [x] **Compatibilité ascendante gtkdialog**, symlink `gtkdialog` → `gtk3sermo` à l'installation ; XML/CLI/variables d'environnement d'époque vérifiés ✅
- [ ] **CI fonctionnel**, gtk3sermo compile sur Gitea/Forgejo à chaque push
- [x] **Suite XML complète**, 52 cas de test ✅ (tests/xml/ : 52 fichiers couvrant tous les widgets et actions)
- [ ] **Valgrind propre**, zéro leak sur les 52 XML de référence avec gtk3sermo
- [ ] **cppcheck CI**, analyse statique automatique, zéro erreur
- [x] **haplo-dialog-xml(5) dans les paquets**, manpage installée par défaut ✅
- [ ] **`.deb` officiel signé**, paquet binaire de release signé
- [x] **Exemples**, `examples/` couvre tous les widgets (53 répertoires, scripts `#!/bin/sh`)

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
| `gtk3sermo` | ✅ Production | haplo-dialog | Oui (v1.1) |

`gtk3sermo` est le port de référence de haplo-dialog (backend GTK 3, 43 widgets). Il fournit l'alias rétro-compatible `gtkdialog`.

---

## Ce qui ne sera PAS fait

Pour éviter la dispersion :

- Pas de port Windows (hors scope du projet)
- Pas de port macOS
- Pas de binding Python/Ruby (utiliser directement le binaire depuis le shell)
- Pas de support GTK2 (abandonné)
- Pas de remplacement de Zenity pour les cas simples, haplo-dialog cible les dialogues complexes

---

## Comment contribuer à la roadmap

Ouvrir un ticket avec le label `roadmap` ou écrire à `devel@haplo-dialog.fr`.  
Les priorités sont discutées ouvertement, toute suggestion est bienvenue.

---

*haplo-dialog, "Le shell mérite une vraie interface graphique."*

---

