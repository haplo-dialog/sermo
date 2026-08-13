# Politique de Sécurité — haplo-dialog

**Mainteneur :** haplo-dialog — devel@haplo-dialog.fr
**Date :** 2026-05-29

---

## Versions supportées

| Port | Version | Support sécurité |
|------|---------|-----------------|
| gtk3dialog | 1.0.0 | ✅ Actif |

gtk3dialog est le port unique et de référence (backend GTK 3, 43 widgets) ; il fournit
l'alias rétro-compatible `gtkdialog`.

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
| PIE | `-fPIE -pie` | ASLR — adresses aléatoires |
| Full RELRO | `-Wl,-z,relro -Wl,-z,now` | GOT en lecture seule |
| NX Stack | `-Wl,-z,noexecstack` | Pile non exécutable |
| Format security | `-Wformat=2 -Werror=format-security` | Blocage format strings dangereux |
| Overflow format | `-Wformat-overflow=2` | Détection débordements printf |

### Protection à l'exécution

| Mécanisme | Implémentation |
|-----------|---------------|
| `safe_exec()` / `safe_popen()` | Ni `system()` ni `popen()`. Commande **sans** métacaractère shell → `exec()` direct (argv, sans shell). Commande **avec** métacaractère → repli `/bin/sh -c`, **journalisé**. |
| Refus « fail-closed » | `HAPLO_NO_SHELL_FALLBACK=1` refuse tout repli `/bin/sh -c` (échoue au lieu d'exécuter). |
| Longueur de commande | Bornée dans `safe_exec()`/`safe_popen()`. |
| Environnement enfant | Filtré : le bloc `DIALOG` (plusieurs Kio de XML) n'est pas hérité par les processus lancés. |
| Sûreté mémoire | `g_strlcpy` (noms de variables), copie de widgets bornée, `argv` de spawn toujours NUL-terminé ; aucune fonction interdite (`strcpy`/`strcat`/`sprintf`/`system`/`popen`). |
| Parser XML | Rejet propre de l'XML malformé (message + code de sortie non nul, jamais d'`abort`) ; parser soumis au **fuzzing** (`tests/fuzz/`). |

### Modèle de confiance

gtk3dialog exécute l'interface décrite par l'**auteur du script XML** — comme un
script shell exécute ce que son auteur écrit. Les balises `<action>`/`<input>`
peuvent lancer des commandes : **c'est voulu et documenté**. La frontière de
confiance est donc l'auteur **local** du script, pas un tiers distant. Le
durcissement ci-dessus protège contre l'**entrée malformée** et les **bugs de
sûreté mémoire** — pas contre un auteur de script hostile, qui peut de toute
façon lancer des commandes. Pour interpréter du XML issu d'une source moins
fiable, positionnez `HAPLO_NO_SHELL_FALLBACK=1`.

### Warnings traités comme erreurs

```
-Wall -Wextra -Wshadow -Wnull-dereference
-Wstrict-prototypes -Wimplicit-fallthrough=3
-Werror=format-security
```

---

## Historique des correctifs

| Date | Port | Description | Sévérité |
|------|------|-------------|----------|
| 2026-08-11 | gtk3dialog | Passe sûreté mémoire : `g_strlcpy` (noms de variables), copie de widgets bornée à `MAXWIDGETS`, `argv` de spawn NUL-terminé, filtrage de l'environnement enfant | Moyen |
| 2026-05-29 | gtk3dialog | Upgrade FORTIFY_SOURCE 2→3, ajout stack-clash, noexecstack | Moyen |
| 2026-05-22 | gtk3dialog | Renommage binaires, correction symlink gtkdialog | Faible |
| 2026-05-01 | gtk3dialog | Port initial — safe_exec, PIE, RELRO | Élevé |

---

## Vérification post-installation

```bash
# Vérifier les protections du binaire installé :
hardening-check /usr/bin/gtk3dialog
checksec --file=/usr/bin/gtk3dialog

# Vérifier ASLR actif sur le système :
cat /proc/sys/kernel/randomize_va_space   # doit être 2
```

---

*haplo-dialog est un fork de gtkdialog (László Pere, GPL-2.0-or-later).*
*haplo-dialog — devel@haplo-dialog.fr — 2026*
