# Sécurité — durcissement : état et pistes

La **posture de sécurité en place** — durcissement compile + exécution, modèle de
confiance, signalement de vulnérabilité — fait foi dans le fichier **SECURITY.md**
(à la racine du dépôt). Ce document ne liste que les pistes de durcissement
**complémentaire** envisagées, sans rien affirmer que le code ne fasse pas.

Ce port suit le port GTK 3 point pour point : même modèle d'exécution, mêmes
variables d'environnement, mêmes drapeaux. Là où il en diffère, c'est dit.

## En place (résumé — détail dans SECURITY.md)

- Exécution sans `system()`/`popen()` (`safe_exec`/`safe_popen`) ; repli
  `/bin/sh -c` **journalisé** et désactivable (`HAPLO_NO_SHELL_FALLBACK`) ;
  liste blanche facultative `HAPLO_ALLOWED_CMDS`, éteinte par défaut.
- Durcissement compile : `_FORTIFY_SOURCE=3`, `-fstack-protector-strong`,
  `-fstack-clash-protection`, `-fcf-protection=full` **plus** `-Wl,-z,ibt`
  et `-Wl,-z,shstk` — sans ces deux-là, CET ne survit pas à l'édition de liens
  et le binaire produit ne le porte pas, quoi que dise le Makefile. Mesuré sur
  le binaire par `tests/garde_durcissement.sh`, pas déduit des drapeaux.
  Également : PIE, Full RELRO, NX, durcissement des chaînes de format.
- Sûreté mémoire : `g_strlcpy`, copies bornées, `argv` de spawn NUL-terminé ;
  **aucune** fonction interdite (`strcpy`/`strcat`/`sprintf`/`system`/`popen`)
  ailleurs que dans des commentaires ; longueur de commande bornée ; recopie des
  enfants d'un conteneur bornée à `MAXWIDGETS` avec refus nommé.
- Aucun appel `gtk_*`/`gdk_*` hors du thread principal : le thread de la barre de
  progression délègue par `g_idle_add`. Vérifié par
  `tests/garde_progressbar_thread.sh`.
- Parser XML soumis au **fuzzing** (`tests/fuzz/`) ; rejet propre de l'XML
  malformé (message + code de sortie non nul, jamais d'`abort`).

## Modèle de confiance (rappel)

La frontière de confiance est l'**auteur local du script XML** : ses
`<action>`/`<input>` peuvent lancer des commandes, c'est voulu. Le durcissement
vise l'entrée malformée et les bugs mémoire, pas un auteur de script hostile.

## Corrigé le 2026-08-24 — écarts propres à ce port

Trois défauts que le port GTK 3 n'avait pas, trouvés en le comparant ligne à ligne :

- **`variables.c`** — `strncpy(new->Name, name, NAMELEN)` dans un tampon
  `g_malloc` non mis à zéro : un nom de 512 caractères ou plus n'était pas
  terminé (CWE-170, puis CWE-125 à la première lecture). Remplacé par
  `g_strlcpy(new->Name, name, sizeof(new->Name))`.
- **`automaton.c`** — la recopie des widgets d'un conteneur écrivait dans
  `widgets[]`/`widgettypes[]`, tableaux fixes de `MAXWIDGETS`, sans vérifier la
  place (CWE-787). Mesuré avant correctif : 300 enfants directs acceptés en
  silence. Désormais refus net, même message que le port GTK 3.
- **`actions.c`** — `action_append()` recopiait le premier paramètre depuis le
  DÉBUT de la chaîne au lieu de la position de la correspondance
  (`string` au lieu de `string + matched[1].rm_so`).

## Pistes de durcissement complémentaire (optionnel)

- [ ] Bac à sable **seccomp** (filtre d'appels système) en option de compilation.
- [ ] **AddressSanitizer** en option de `configure` — il se pose aujourd'hui à la
      main (`CFLAGS="-fsanitize=address -g -O1"`), ce qui marche mais n'est
      documenté nulle part dans le `--help`.

*Ces pistes sont additives : elles renforceraient une base déjà durcie, elles ne
comblent pas un trou connu.*

## Ce qui a quitté cette liste, et pourquoi

- *« Intégrer `hardening-check` au pipeline CI »* — **fait**, autrement :
  `tests/garde_durcissement.sh` lit le binaire produit avec `readelf` à chaque
  poussée. Plus strict que `hardening-check`, qui ne regarde pas CET.
- *« Ajouter du fuzzing sur le parser XML »* — **fait** : `tests/fuzz/`, avec
  repli interne quand `afl` est absent.
- *« Vérifier que pkexec est utilisé pour toute élévation »* — **sans objet** :
  le projet n'appelle `pkexec` nulle part et n'élève aucun privilège. Un
  programme qui n'élève rien n'a pas de chemin d'élévation à sécuriser.
- *« Auditer `tag_attributes.c` — snprintf systématique »* et *« CMD_INPUT :
  liste blanche stricte »* — remplacés par des mesures : aucune fonction
  interdite dans `src/` (banc `tests/garde_fonctions_interdites.sh`), et la
  liste blanche existe désormais pour de bon sous `HAPLO_ALLOWED_CMDS`
  (banc `tests/garde_allowed_cmds.sh`).
- *« Valider les chemins de fichiers passés via XML (traversal path) »* —
  **hors modèle de confiance** : l'auteur du script XML est déjà autorisé à
  lancer des commandes ; lui interdire un chemin ne protège de rien.
