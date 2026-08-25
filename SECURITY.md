# Politique de sécurité — sermo

**Mainteneur :** haplo-dialog, devel@haplo-dialog.fr
**Date :** 2026-05-29

---

## Versions supportées

| Port | Version | Support sécurité |
|------|---------|-----------------|
| gtk3sermo | 1.0.0 | ✅ Actif |
| gtk4sermo | 1.0.0 | ✅ Actif |

Les deux ports partagent le même cœur C et la même grammaire. Depuis le
2026-08-24 ils ont la **même posture mémoire** : `g_strlcpy` pour les copies de
noms, borne explicite sur la recopie des widgets d'un conteneur, et aucune
fonction de la famille `strcpy`/`strcat`/`sprintf`/`gets`, ce qu'un banc
rejoue à chaque poussée sur les deux `src/`. L'alias
rétro-compatible `gtkdialog` est fourni par un paquet séparé, `gtksermo`.

⚠️ Le port GTK 4 est le plus jeune : il a reçu la même passe, mais il a moins
d'usage réel derrière lui. Ce qui reste ouvert est listé, port par port, dans
les fichiers `TODO-SECURITY.md`.

---

## Signaler une vulnérabilité

Envoyez un email à **devel@haplo-dialog.fr** avec :

- Description du problème
- Étapes de reproduction
- Impact estimé (CVE si applicable)
- Vos coordonnées (pour vous créditer)

**Délai de réponse :** 48h maximum.
**Divulgation responsable :** Nous demandons 90 jours avant publication publique.

---

## Mécanismes de sécurité en place

### Hardening à la compilation

| Mécanisme | Flag | Effet |
|-----------|------|-------|
| FORTIFY_SOURCE=3 | `-D_FORTIFY_SOURCE=3` | Détection buffer overflow à compile+runtime |
| Stack canary | `-fstack-protector-strong` | Détection de stack smashing |
| Stack clash | `-fstack-clash-protection` | Protection contre stack clash attacks |
| Control Flow | `-fcf-protection=full` | CFI hardware (x86 CET) |
| PIE | `-fPIE -pie` | ASLR, adresses aléatoires |
| Full RELRO | `-Wl,-z,relro -Wl,-z,now` | GOT en lecture seule |
| NX Stack | `-Wl,-z,noexecstack` | Pile non exécutable |
| Format security | `-Wformat=2 -Werror=format-security` | Blocage format strings dangereux |
| Overflow format | `-Wformat-overflow=2` | Détection débordements printf |

### Protection à l'exécution

| Mécanisme | Implémentation |
|-----------|---------------|
| `safe_exec()` / `safe_popen()` | Ni `system()` ni `popen()`. Commande **sans** métacaractère shell → `exec()` direct (argv, sans shell). Commande **avec** métacaractère → repli `/bin/sh -c`, **journalisé**. |
| Refus « fail-closed » | `HAPLO_NO_SHELL_FALLBACK=1` refuse tout repli `/bin/sh -c` (échoue au lieu d'exécuter). |
| Liste de commandes, facultative | `HAPLO_ALLOWED_CMDS=ls,cat,date` borne les commandes lançables. **Éteinte par défaut** : le langage sert à lancer des commandes, et 14 des exemples livrés appellent `bash` ou `sh` — une liste active par défaut casserait le produit sans protéger personne, la commande venant du script que l'appelant a écrit. Elle vise celui qui **déploie** un dialogue dans un contexte moins fiable. Le nom comparé est le nom de base (`/bin/ls` = `ls`), et tant qu'elle est posée le repli `/bin/sh -c` est refusé — sinon `sh -c` la contournerait. |
| Longueur de commande | Bornée dans `safe_exec()`/`safe_popen()`. |
| Environnement enfant | Filtré : le bloc `DIALOG` (plusieurs Kio de XML) n'est pas hérité par les processus lancés. |
| Sûreté mémoire | `g_strlcpy` (noms de variables), copie de widgets bornée à `MAXWIDGETS` avec refus nommé, `argv` de spawn toujours NUL-terminé ; aucune fonction interdite (`strcpy`/`strcat`/`sprintf`/`gets`/`system`/`popen`) appelée dans `src/` — **vérifié à chaque poussée** par `tests/garde_fonctions_interdites.sh`, qui ignore les commentaires et exige une frontière de mot à gauche (`safe_system` ne compte pas pour `system`). |
| Sûreté des threads | Aucun appel `gtk_*`/`gdk_*` hors du thread principal : le thread de la barre de progression délègue par `g_idle_add`. `gdk_threads_enter()` ne verrouille plus rien depuis GTK 3.6 — du code hérité qui paraît protégé ne l'est pas. Vérifié par `tests/garde_progressbar_thread.sh`. |
| Sortie rendue au shell | Les lignes `NOM="valeur"` et `EXIT="valeur"` échappent les **quatre** caractères que le shell développe entre guillemets doubles : `\`, `"`, `$` et l'accent grave. Sans cela, une valeur **tapée par l'utilisateur du dialogue** — qui n'est pas forcément l'auteur du script — devient du code dès que l'appelant fait `eval`. Mesuré le 2026-08-25 avant correctif : saisir `$(touch /tmp/preuve)` dans un champ, puis évaluer la ligne, créait le fichier. Vérifié à chaque poussée par `tests/garde_echappement_sortie.sh`, qui ouvre une vraie fenêtre, clique, et surveille un fichier témoin. |
| Parser XML | Rejet propre de l'XML malformé (message + code de sortie non nul, jamais d'`abort`) ; parser soumis au **fuzzing** (`tests/fuzz/`). |

### Modèle de confiance

Les deux ports exécutent l'interface décrite par l'**auteur du script XML**,
comme un script shell exécute ce que son auteur écrit. Les balises `<action>`/`<input>`
peuvent lancer des commandes : **c'est voulu et documenté**. La frontière de
confiance est donc l'auteur **local** du script, pas un tiers distant. Le
durcissement ci-dessus protège contre l'**entrée malformée** et les **bugs de
sûreté mémoire**, pas contre un auteur de script hostile, qui peut de toute
façon lancer des commandes. Pour interpréter du XML issu d'une source moins
fiable, positionnez `HAPLO_NO_SHELL_FALLBACK=1`.

### Une frontière de plus : celui qui SE SERT du dialogue

L'auteur du script et la personne devant l'écran ne sont pas toujours la même.
Un script qui affiche une fenêtre à quelqu'un d'autre, puis fait `eval` de la
sortie, lui donne l'exécution de commandes si la sortie n'est pas échappée.
C'était le cas jusqu'au 2026-08-25.

Deux conseils, dans l'ordre :

1. **Préférez `--do`.** Les valeurs arrivent par l'environnement et ne sont
   jamais relues comme du code. C'est la seule voie qui ne dépend pas de la
   qualité d'un échappement.
2. Si vous tenez à `eval`, sachez que la ligne est désormais échappée pour un
   contexte entre guillemets doubles — mais `eval` reste `eval` : ne l'employez
   pas sur une sortie que vous n'avez pas produite vous-même.

### Avertissements du compilateur

Un seul est traité comme une **erreur**, celui qui arrête la construction :

```
-Werror=format-security
```

Les autres sont activés mais **restent des avertissements** : la construction
réussit avec eux. Il en subsiste 24 côté GTK 3 et 36 côté GTK 4, hérités de
l'amont gtkdialog.

```
-Wall -Wextra -Wformat=2 -Wformat-overflow=2 -Wshadow -Wnull-dereference
-Wstrict-prototypes -Wimplicit-fallthrough=3
```

Les purger et passer `-Werror` global est une piste ouverte, pas un état de fait :
l'annoncer avant de l'avoir fait serait une garantie que le compilateur dément.

---

## Historique des correctifs

| Date | Port | Description | Sévérité |
|------|------|-------------|----------|
| 2026-08-24 | gtk4sermo | Passe sûreté mémoire : `g_strlcpy` pour le nom de variable (un nom de 512 caractères ou plus n'était pas terminé — CWE-170 puis CWE-125), borne `MAXWIDGETS` sur la recopie des widgets (CWE-787 : 300 enfants directs étaient acceptés en silence), et `action_append()` qui recopiait son premier paramètre depuis le début de la chaîne. Test de non-régression : `tests/garde_maxwidgets.sh` | Moyen |
| 2026-08-11 | gtk3sermo | Passe sûreté mémoire : `g_strlcpy` (noms de variables), copie de widgets bornée à `MAXWIDGETS`, `argv` de spawn NUL-terminé, filtrage de l'environnement enfant | Moyen |
| 2026-05-29 | gtk3sermo | Upgrade FORTIFY_SOURCE 2→3, ajout stack-clash, noexecstack | Moyen |
| 2026-05-22 | gtk3sermo | Renommage binaires, correction symlink gtkdialog | Faible |
| 2026-05-01 | gtk3sermo | Port initial, safe_exec, PIE, RELRO | Élevé |

---

## Vérification post-installation

```bash
# Vérifier les protections du binaire installé :
hardening-check /usr/bin/gtk3sermo
checksec --file=/usr/bin/gtk3sermo

# Vérifier ASLR actif sur le système :
cat /proc/sys/kernel/randomize_va_space   # doit être 2
```

---

*haplo-dialog est un fork de gtkdialog (László Pere, GPL-2.0-or-later).*
*haplo-dialog, devel@haplo-dialog.fr, 2026*
