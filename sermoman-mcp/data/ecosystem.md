# L'écosystème d'outils autour de sermo

Ce guide s'adresse à un assistant IA qui aide à écrire des dialogues **sermo**
corrects. Il décrit les outils qui l'entourent et **dit lequel prendre
pour quoi**.

sermo est une boîte à dialogues pilotée par **XML** (racine `<window>`), de la
lignée **gtkdialog 0.8.3** (László Pere, GPL-2.0+). Un même XML, dans une
variable shell, un fichier ou stdin, devient une fenêtre ; à la fermeture, les
widgets nommés émettent leurs valeurs sur stdout sous la forme `NOM="valeur"`.

Le **même XML** rend sur **sept backends** : `gtk3`, `gtk4`, `qt6`, `fltk1`,
`efl1`, `sdl3` et `ncurses` (terminal). Ils partagent un cœur commun : l'analyse
et l'exécution durcie sont mutualisées, seul le **rendu** est propre à chaque
backend. Le binaire par défaut est choisi par `update-alternatives` sous le nom
`sermo`.

---

## Quel outil pour quoi — vue d'ensemble

| Besoin | Outil | Nature |
|---|---|---|
| **Lire la doc** (widgets, attributs, exemples) | `sermoman-mcp` | documentation seule, **lecture seule** |
| **Savoir écrire du XML correct** (actions, pièges) | la section 1 ci-dessous | connaissance de référence |
| **Vérifier et VOIR** un dialogue | le binaire `sermo` | `--print-ir`, `--render-png` |

Règle simple :

- Une question de syntaxe → `sermoman-mcp` (il **ne fait qu'informer**).
- Comment marchent les actions/signaux et où sont les pièges → la section 1
  ci-dessous.
- Contrôler ce qu'on vient d'écrire → la **ligne de commande** `sermo`, pas un
  serveur MCP.

> **sermo ne fournit qu'un seul serveur MCP : `sermoman-mcp`, celui du manuel.**
> Il informe, il n'agit pas. La génération de fenêtres par une IA — catalogue
> d'exemples notés, vote, boucle de correction automatique — **a été retirée de
> sermo** et ne fait plus partie de ce dépôt.

---

## 1. Écrire du XML sermo correct — actions et pièges

Cette section rassemble les **actions** courantes et les **pièges** connus.
Elle reprend la structure de la documentation « for-claude » de **gtkdialog3**
(© László Pere, GPL-2), adaptée aux divergences durcies de sermo.

### La boucle recommandée

1. **Apprendre** le langage (`sermo_reference`, ou le manuel utilisateur).
2. **Générer** le XML (racine `<window>`, widgets dans des conteneurs).
3. **Vérifier** sans affichage : `sermo --file=X --print-ir` (erreur → corriger).
4. **Voir** (facultatif) : `sermo --render-png out.png --file=X`.
5. **Exécuter** : `sermo --file=X` (ou `--program=VAR`, `--stdin`).

Les étapes 3 et 4 sont des **options du binaire**, pas des outils MCP : elles
marchent sans aucun serveur.

### Vérifier et voir sans écran (`--print-ir`, `--render-png`)

`sermo --file=X --print-ir` analyse le XML et décrit la fenêtre en texte, sans
rien afficher : code de retour 0 = la syntaxe passe. C'est le contrôle le moins
cher, à faire systématiquement.

`sermo --render-png out.png --file=X` rend le dialogue en **image PNG** sans
écran réel — de quoi **regarder** ce qui a été produit. Le rendu existe pour les
**six** backends graphiques (gtk3, gtk4, qt6, fltk1, efl1, sdl3) : offscreen pour
qt6/efl1/sdl3, sous xvfb pour gtk3/gtk4/fltk1. La taille demandée par
`default-width`/`default-height` est respectée sur les six. Le backend terminal
`ncurses` n'a pas d'image ; pour lui, `--print-ir` tient lieu d'aperçu.

### Deux principes

- **Variables = connecteurs.** Un widget dont on veut la valeur porte
  `<variable>NOM</variable>`. **Sans nom, il n'exporte rien.**
- **Actions = logique.** `<action>préfixe:cible</action>`. Un préfixe reconnu
  (`exit:`, `refresh:`…) déclenche un comportement ; **tout le reste part au
  shell**. Avant chaque action, toutes les valeurs de widgets sont exportées
  dans l'environnement.

### Actions reconnues (21 préfixes, plus le shell implicite)

Les plus utiles : `exit:VALEUR`, `launch:NOM`, `closewindow:NOM`,
`presentwindow:NOM`, `enable:` / `disable:`, `show:` / `hide:`, `activate:`,
`grabfocus:`, `refresh:`, `save:`, `fileselect:`, `clear:`,
`removeselected:` (un seul mot, **sans tiret**), `break`.

⚠️ sermo n'a **pas** les `set:Widget.prop` ni le presse-papiers
(`cut/copy/paste/selectall`) de gtkdialog3. `append:` / `insert:` sont
incomplets — à éviter.

### Les pièges à connaître

- **Ce n'est PAS du XML complet.** Pas de commentaires `<!-- -->`, ni CDATA, ni
  DTD, ni namespaces, ni entités `&amp;` : tout cela **fait échouer l'analyse**.
  Mettez les commentaires dans le script shell, autour.
- **Échappement dans le balisage.** Échappez `<` `>` `"` `\` par antislash
  (`\<`, `\>`, `\"`, `\\`). Vérifiez toujours avec `--print-ir`.
- **Échappement de sortie durci (propre à sermo).** La sortie `NOM="…"` échappe
  les quatre caractères que le shell développe entre guillemets (`\ " $` et
  l'accent grave), donc `eval` ne réexécute pas une saisie. **Mais** si le
  dialogue est utilisé par autrui, préférez **`--do`** : les valeurs arrivent
  par l'environnement, jamais relues comme du code.
- **Exécution durcie via `safe_system` (propre à sermo).** Pas de shell si la
  commande n'a **aucun** métacaractère (exec direct) ; sinon repli `/bin/sh -c`
  **journalisé et refusable** (`SERMO_NO_SHELL_FALLBACK=1`). Liste blanche
  optionnelle `SERMO_ALLOWED_CMDS=ls,cat,…`. Les fonctions shell d'une action se
  fournissent via `--include=FICHIER`.
- **Multi-fenêtres.** Pour `launch:` / `closewindow:`, le nom exporté, la cible
  `--program` et le `<variable>` de la fenêtre doivent être **identiques**.
- **progressbar en thread de fond.** L'`<input>` tourne dans un thread ; il ne
  doit **pas toucher le toolkit directement** (garde de sûreté GTK). Lignes
  numériques 0–100 → barre, non numériques → texte, 100 → déclenche les actions.
- **tree / table.** Colonnes séparées par `|` ; un `|` littéral dans les données
  n'est pas géré directement.
- **timer.** Ne tourne que s'il est *sensitive* ; `disable:` / `enable:` le
  pilotent ; `visible="false"` le cache.
- **Disponibilité selon le backend.** `terminal` exige VTE et n'existe que sur
  les backends GTK. Les widgets graphiques (image, colorbutton, drawingarea…)
  n'ont pas d'équivalent sur `ncurses`.

#### Les cinq formes qui ont l'air justes et qui arrêtent le programme

Elles paraissent naturelles et sortent sur une erreur de syntaxe. Le parseur est
dans le cœur : ces cinq cas valent donc **sur tous les ports**, pas seulement gtk3.
Rejouées le 2026-09-14 sur sermo 2.5.0 — les formes fautives ET les corrections.

| On écrit spontanément | Message | La forme qui marche |
|---|---|---|
| `<frame><label>Titre</label>` | `near token '<label>'` | `<frame Titre>` ou `<frame label="Titre">` |
| `<notebook><label>a</label>` | `near token '<label>'` | `<notebook tab-labels="a\|b">` |
| `<expander><label>Titre</label>` | `near token '<label>'` | `<expander label="Titre">` |
| `<pixmap><filename>x.png</filename>` | `near token '<filename>'` | `<pixmap><input file>x.png</input></pixmap>` |
| `<table><column-header>c</column-header>` | `near token '<column-header>'` | `<table><label>c1\|c2</label>` |

⚠️ **`<expander Titre>` en positionnel échoue aussi**, alors que `<frame Titre>`
passe. Les deux conteneurs ne se comportent pas pareil — vérifié, pas déduit.

Dans `<pixmap>`, `file` est un attribut **nu**, sans valeur : le chemin est le
contenu de l'élément. Pour une icône du thème :
`<pixmap><input file stock="gtk-info"></input></pixmap>`.

**Vérifier une syntaxe sans ouvrir de fenêtre** (n'importe quel backend) :

```sh
MAIN_DIALOG="$(cat mon-dialogue.xml)" sermo --program=MAIN_DIALOG --print-ir
```

`--print-ir` analyse, imprime la représentation interne et sort, sans construire
de widget ni ouvrir de fenêtre. Code de retour 0 = la syntaxe passe.

---

## 2. sermoman-mcp — la documentation (lecture seule)

Le serveur où vit **ce fichier**. C'est un compagnon de documentation embarquée,
à côté de l'assistant IA. Il donne une connaissance précise et à jour de la
syntaxe XML de sermo, pour aider à écrire des dialogues corrects **sans inventer
de balises**.

### Il est inoffensif par construction

- **Strictement local** : il parle par l'entrée/sortie standard (`stdio`).
  Aucun port ouvert, aucun accès réseau. Rien ne quitte la machine.
- **Lecture seule** : il n'exécute rien, n'écrit aucun fichier, ne publie rien.
- **Contenu embarqué** : la doc servie est incluse dans le dépôt et dérivée de
  la documentation publique du projet.

Le pire qu'il puisse faire, c'est donner une réponse de documentation.

### Ses outils

| Outil | Ce qu'il donne |
|---|---|
| `sermo_reference` | la référence complète de la syntaxe XML (widgets, attributs, sous-éléments) |
| `sermo_guide` | le guide de prise en main (installation, compatibilité gtkdialog, sécurité, licences) |
| `sermo_architecture` | l'architecture modulaire : le cœur, les sept backends, le contrat cœur↔backend, le packaging, les bancs |
| `sermo_heritage` | la vie de sermo : lignée, historique des versions, feuille de route, bilan de santé |
| `sermo_ecosystem` | ce fichier : quel outil pour quoi |
| `sermo_search` | recherche plein texte dans la documentation |
| `sermo_example` | un exemple complet de dialogue ; sans argument, la liste des exemples curés |
| `sermo_how_to_report` | comment signaler un bug ou proposer une amélioration |

Prérequis : Python 3, aucune dépendance externe.

---

## Récapitulatif — le bon réflexe

1. **Écrire** un dialogue : demander la syntaxe à `sermoman-mcp`, s'appuyer sur
   les principes et pièges de la section 1.
2. **Contrôler** : `sermo --file=X --print-ir` d'abord, puis
   `sermo --render-png out.png --file=X` pour **voir** avant d'exécuter.
3. **Toujours** vérifier une syntaxe douteuse avec `--print-ir` (code de retour
   0 = ça passe) plutôt que de deviner.

---

## Licences et lignée

Code sermo : GPL-2.0-or-later. Hérite de **gtkdialog 0.8.3** (László Pere,
GPL-2.0+). La section 1 reprend la structure de la documentation « for-claude »
de gtkdialog3 (© László Pere, GPL-2). Documentation sous GPL-2.0-or-later, comme
le code ; la plupart des exemples viennent de gtkdialog et suivent la même licence.

Contact : `devel@haplo-dialog.fr` · https://haplo-dialog.fr
