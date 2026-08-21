<div align="right"><sub><code>maj :     2026-08-18 12:40:07 · par claude</code></sub></div>
<div align="right"><sub><code>réindex : —</code></sub></div>

# Sécurité — durcissement : état et pistes

La **posture de sécurité en place** — durcissement compile + exécution, modèle de
confiance, signalement de vulnérabilité — fait foi dans le fichier **SECURITY.md**
(à la racine du dépôt). Ce document ne liste que les pistes de durcissement
**complémentaire** envisagées, sans rien affirmer que le code ne fasse pas.

## En place (résumé — détail dans SECURITY.md)

- Exécution sans `system()`/`popen()` (`safe_exec`/`safe_popen`) ; repli
  `/bin/sh -c` **journalisé** et désactivable (`HAPLO_NO_SHELL_FALLBACK`).
- Durcissement compile : `_FORTIFY_SOURCE=3`, `-fstack-protector-strong`,
  `-fstack-clash-protection`, `-fcf-protection=full`, PIE, Full RELRO, NX,
  durcissement des chaînes de format.
- Sûreté mémoire : `g_strlcpy`, copies bornées, `argv` de spawn NUL-terminé ;
  **aucune** fonction interdite (`strcpy`/`strcat`/`sprintf`/`system`/`popen`) ;
  longueur de commande bornée.
- Parser XML soumis au **fuzzing** (`tests/fuzz/`) ; rejet propre de l'XML
  malformé (message + code de sortie non nul, jamais d'`abort`).

## Modèle de confiance (rappel)

La frontière de confiance est l'**auteur local du script XML** : ses
`<action>`/`<input>` peuvent lancer des commandes, c'est voulu. Le durcissement
vise l'entrée malformée et les bugs mémoire, pas un auteur de script hostile.

## Pistes de durcissement complémentaire (optionnel)

- [ ] Bac à sable **seccomp** (filtre d'appels système) en option de compilation.
- [ ] **AddressSanitizer** dans les builds de debug (`--enable-debug`).
- [ ] `hardening-check` intégré au pipeline CI de packaging.

*Ces pistes sont additives : elles renforceraient une base déjà durcie, elles ne
comblent pas un trou connu.*
