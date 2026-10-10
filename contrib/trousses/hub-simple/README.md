# hub-admin — conteneur forme A (lanceur) + modules

> **État mesuré le 2026-10-04** (voir `contrib/README.md`). La fenêtre s'ouvre sur
> les six ports graphiques. **En terminal (ncurses), ce hub ne marche pas encore** :
> il sort aussitôt, sans rien afficher.

Un conteneur qui **découvre** les outils du dossier `modules/` et lance
celui qu'on choisit, **chacun dans son propre processus**. C'est la « panel
par public » dont on parlait : ici le public `admin-devops`.

```
hub-admin/
  hub-admin.sh      le conteneur (ne fait que lister + lancer, zéro privilège)
  modules/
    servys.sh       gestion des services systemd (déjà vu)
    info.sh         infos système en lecture seule
```

## Lancer

```sh
./hub-admin.sh              # choisit le backend ; ncurses si pas d'affichage
./hub-admin.sh gtk3sermo    # forcer un port
```

Le hub liste les modules (par leur `--manifest`), montre la fiche du module
sélectionné, et « Lancer » l'ouvre. On revient au hub quand le module se
ferme.

## Comment un module est reconnu

Tout script de `modules/` qui répond à `--manifest` (charte §8bis) apparaît
automatiquement — rien à déclarer ailleurs. Dépose un `.sh` conforme, il est
là au lancement suivant.

```
$ bash modules/servys.sh --manifest
NOM: servys
DESCRIPTION: Gestion des services systemd (start / stop / restart / état)
PUBLIC: admin-devops
BACKENDS: tous
```

## Sécurité

Le hub **ne fait jamais confiance au texte** d'un manifest : il ne lance
qu'un **chemin trouvé sur le disque** dont le `NOM` correspond exactement au
choix, sur un ensemble **re-scanné** au moment du lancement. Une chaîne
fabriquée ne mène nulle part (testé : `rm -rf /` → refusé). Chaque module
garde par ailleurs ses propres gardes : le hub n'en est pas responsable.

## Un TTY = un seul TUI (charte §9)

- **Graphique** : le module s'ouvre en **fenêtre à part**, le hub se rouvre.
- **Terminal** : le hub **cède le TTY** au module au premier plan, puis
  revient à la liste quand il se ferme (deux UIs ncurses ne partagent pas un
  terminal).

## Vérifié ici / à confirmer chez toi

Vérifié : `bash -n` des trois scripts, manifests lisibles, découverte des
deux modules, résolution NOM→chemin (et refus d'une entrée malveillante),
XML des dialogues bien formé (hors faux positif `<button cancel>`, idiome
gtkdialog normal).

**À confirmer sur binaire réel** — un point que je ne peux pas tester sans
sermo installé : le hub **capture le stdout** du sélecteur pour relire le
module choisi (`MODULE="..."`, `EXIT="..."`). C'est l'usage classique de
gtkdialog, mais vérifie que **`ncursessermo` dessine bien son UI sur le
terminal quand stdout est capturé** (`sortie=$(choisir)`). Si un backend
écrit son UI sur stdout, il faudra router le dump des variables via
`<output file>` plutôt que stdout. À valider dans le smoke test ncurses.

## Forme B (onglets) — pour plus tard

Si tu veux un vrai tableau de bord tout-en-un, c'est la forme B : chaque
module expose aussi `--xml` (variables **préfixées**), et le hub concatène
les fragments dans un `<notebook>`. Attention : les onglets se rendent mal
en ncurses — à réserver au desktop, pas au tronc commun portable.
