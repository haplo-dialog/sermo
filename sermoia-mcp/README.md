<img src="icon/sermoia-mark.png" alt="sermoia" width="88" align="right">

# sermoia-mcp — serveur MCP pour écrire du sermo avec une IA

[English](README.en.md)

sermoia-mcp donne à un assistant IA (Claude, ou tout client **MCP**) les outils pour
écrire un dialogue sermo, le faire **vérifier** par l'analyseur réel, le **voir** en
image et le **corriger**, avant de le montrer à l'utilisateur.

C'est le second serveur MCP de sermo, à côté de [sermoman-mcp](../sermoman-mcp/) :
sermoman **explique** le langage (le manuel), sermoia le **fait pratiquer**. Écrit en C
(GLib, GIO, json-glib). Il parle JSON-RPC 2.0 sur son entrée et sa sortie standard,
un message par ligne. GPL-2.0-or-later.

## La boucle conseillée

1. `sermo_schema` une fois : la liste des balises et des actions.
2. `sermo_examples` avec ce que le dialogue doit faire (« formulaire de contact ») :
   des dialogues déjà validés à imiter.
3. Écrire le XML.
4. `sermo_check` : corriger jusqu'à « OK ».
5. `sermo_render` : regarder la mise en page.
6. `sermo_run`, **seulement si l'utilisateur veut l'essayer** : c'est le seul outil qui
   exécute les commandes du dialogue.
7. `sermo_vote` : dire si l'exemple suivi a aidé.

Le serveur envoie ce résumé au client dès l'ouverture (champ `instructions` de
`initialize`).

## Les six outils

| Outil | Rôle | Lance-t-il les commandes du XML ? |
|---|---|---|
| `sermo_schema` | décrit le langage | non |
| `sermo_check` | fait analyser un XML | non |
| `sermo_render` | dessine le dialogue en PNG | **non, elles sont bloquées** |
| `sermo_run` | montre le dialogue à l'utilisateur, rend ses valeurs | **oui** |
| `sermo_examples` | sert des exemples validés | non |
| `sermo_vote` | note un exemple | non (écrit une ligne dans le registre) |

Chaque outil porte des indications MCP (`readOnlyHint`, `destructiveHint`,
`openWorldHint`) : un client peut ainsi demander confirmation avant `sermo_run`, le
seul marqué « destructif ».

### `sermo_schema`

Sans argument. Rend un objet JSON :
- les widgets, rangés par famille ; la liste est vérifiée par le banc contre les balises
  que l'analyseur du cœur reconnaît ;
- les balises de contenu, les attributs communs, les actions et les signaux ;
- un exemple minimal et la forme de la sortie.

Les attributs propres à chaque widget sont dans le manuel (sermoman-mcp).

### `sermo_check`

| Argument | Type | |
|---|---|---|
| `xml` | chaîne, **requis** | le dialogue complet, racine `<window>` |
| `ir` | booléen | `true` : joint la représentation interne lue par l'analyseur |

Lance `sermo --print-ir`, qui analyse sans rien afficher ni exécuter. Rend :
- `OK — le XML s'analyse (…)` si l'analyseur l'accepte ;
- sinon `ERREUR — l'analyseur refuse ce XML.`, suivi du message de l'analyseur et d'un
  conseil, avec `isError`.

Exemple de message : `sermocore: Error in line 1, near token '<bouton/>': Unknown tag.`

Les messages des bibliothèques graphiques qui ne disent rien du XML (« Gtk-Message »…)
sont retirés.

### `sermo_render`

| Argument | Type | |
|---|---|---|
| `xml` | chaîne, **requis** | le dialogue complet |

Dessine le dialogue en PNG sans l'afficher, et rend l'image suivie d'une légende (port
utilisé, taille). **Toutes les commandes du XML sont bloquées** pendant le rendu
(`<input>`, `<action>`, y compris par `sh -c`) : demander un aperçu ne lance jamais
rien. Revers : un widget rempli par une commande paraît vide sur l'image.

Selon le port choisi (`SERMO_RENDER_BIN`) :

| Port | Comment |
|---|---|
| qt6, sdl3, efl1 | plateforme hors écran du toolkit, sans affichage |
| gtk3, gtk4, fltk1, lvgl | `--render-png` sous `xvfb-run` (paquet `xvfb`) |
| ncurses, html | pas de fenêtre : aperçu **structurel** en texte (`--print-ir`) |

Si aucune image ne sort (rendu raté, délai dépassé, `xvfb-run` absent), l'outil rend
l'aperçu structurel et dit pourquoi. Une image de moins de 16 × 16 pixels est refusée :
qt6 a pu rendre la liste déroulante seule, en 2 × 2.

### `sermo_run`

| Argument | Type | |
|---|---|---|
| `xml` | chaîne, **requis** | le dialogue complet |
| `timeout` | entier | secondes laissées à l'utilisateur, de 5 à 3600 (défaut 300) |

Ouvre le dialogue dans une vraie fenêtre, attend que l'utilisateur le ferme, et rend ce
que sermo écrit : une ligne `NOM="valeur"` par widget nommé, puis `EXIT="…"`. Les
commandes du XML **s'exécutent**, comme dans un script : ne l'appeler qu'avec l'accord
de l'utilisateur, sur un XML vérifié.

Il faut un affichage (`DISPLAY` ou `WAYLAND_DISPLAY`). Sans affichage, l'outil répond
une erreur claire au lieu de rendre « aucune valeur ». Passé le délai, le dialogue est
fermé de force, sans valeurs.

### `sermo_examples`

| Argument | Type | |
|---|---|---|
| `task` | chaîne | ce que le dialogue doit faire : un mot ou une phrase |
| `limit` | entier | nombre d'exemples rendus, de 1 à 10 (défaut 4) |

Sans `task` : la carte du catalogue (sujets, nombre d'exemples, vote).

Avec `task`, la phrase est découpée en mots ; les mots vides (« de », « pour »,
« dialogue »…) sont retirés. Chaque sujet marque :
- 3 points par mot trouvé dans son nom ;
- 1 point par mot trouvé dans ses fichiers.

Les sujets sont classés par points, puis par vote, puis par nom, et chaque exemple est
rendu en entier. Si rien ne correspond, l'outil rend la liste des sujets.

### `sermo_vote`

| Argument | Type | |
|---|---|---|
| `example` | chaîne, **requis** | `sujet` ou `sujet/fichier.xml`, tel que `sermo_examples` l'a donné |
| `helpful` | booléen, **requis** | `true` : a aidé (+1) ; `false` : a trompé (−1) |
| `note` | chaîne | pourquoi, en une phrase |

Ajoute une ligne au registre (`sujet<TAB>±1<TAB>note`) : on n'y efface jamais rien. Un
sujet absent du catalogue est refusé.

## Sécurité

- **Ce qui exécute** : `sermo_run`, et lui seul. Un dialogue sermo sert à lancer des
  commandes : `sermo_run` les lance, comme le ferait le script. Pour borner ce qu'il peut
  lancer, poser `SERMO_ALLOWED_CMDS` dans l'environnement du serveur (voir
  `SECURITY.md` à la racine) : le cœur refuse alors toute commande hors liste, et le
  repli `/bin/sh -c`.
- **Ce qui n'exécute pas** : `sermo_check` (analyse seule) et `sermo_render`. Le rendu
  pose `SERMO_ALLOWED_CMDS` sur une commande qui n'existe pas, ce qui interdit tout. Le
  banc le vérifie : un `<input>` qui crée un fichier témoin ne le crée pas.
- **Le canal MCP est protégé** : un processus lancé lit `/dev/null`, jamais le canal, et
  ses sorties sont captées. Un port qui écrit ses `VAR="…"` même en rendu (html) ne
  casse donc pas le protocole.
- **Fichiers temporaires** : le XML et l'image passent par un dossier privé (0700),
  effacé après chaque appel.
- **Délais** : à l'échéance, tout le groupe de processus est tué (`xvfb-run`, Xvfb et
  le port), et le serveur répond encore après.
- **Messages hors normes** : un message de plus de 8 Mio est refusé.

## Variables d'environnement

| Variable | Défaut | |
|---|---|---|
| `SERMO_BIN` | `sermo` | binaire pour `check`, `run` et l'aperçu texte |
| `SERMO_RENDER_BIN` | `SERMO_BIN` | binaire pour `render` (un port à fenêtre pour avoir une image) |
| `SERMO_EXAMPLES_DIR` | `examples` | catalogue d'exemples |
| `SERMO_VOTE_FILE` | `$XDG_STATE_HOME/sermoia/votes.tsv` | registre des votes |
| `SERMOIA_CHECK_DELAI` | 20 s (≤ 120) | délai de l'analyse |
| `SERMOIA_RENDER_DELAI` | 30 s (≤ 300) | délai de chaque tentative de rendu |
| `SERMO_ALLOWED_CMDS` | — | liste des commandes que `sermo_run` peut lancer (voir Sécurité) |

## Construire, lancer, vérifier

```sh
cmake -S sermoia-mcp -B sermoia-mcp/_build && cmake --build sermoia-mcp/_build
SERMO_BIN=/usr/bin/gtk3sermo SERMO_RENDER_BIN=/usr/bin/qt6sermo \
SERMO_EXAMPLES_DIR=sermoia-mcp/examples sermoia-mcp/_build/sermoia-mcp
bash sermoia-mcp/tests/verifie.sh
```

Dépendances : `libglib2.0-dev` (GLib et GIO), `libjson-glib-dev` ; `xvfb` pour le rendu
en image avec gtk3, gtk4, fltk1 ou lvgl. La version annoncée aux clients est celle du
fichier `VERSION` du dépôt.

`tests/verifie.sh` fait cinq contrôles sur le code et les binaires réels :

1. **Schéma** : il nomme exactement les balises de widget de l'analyseur du cœur.
2. **Catalogue** : chaque exemple s'analyse.
3. **Protocole et outils** :
   - initialisation et négociation de version ;
   - six outils annotés ;
   - check juste et faux, sans bruit GTK ;
   - exemples cherchés par mot et par phrase ;
   - vote, avec sujet hors catalogue refusé et note assainie ;
   - `ping` et codes d'erreur JSON-RPC ;
   - un rendu qui ne laisse rien sur le canal et n'exécute pas un `<input>` témoin.
4. **Image** : une vraie image PNG avec un port à fenêtre, toujours sans exécuter.
5. **Délai** : un dialogue jamais fermé est coupé à 5 s, sans processus survivant.

Quatre mutations du code (commandes non bloquées au rendu, petites images refusées,
délai ignoré, `ping` refusé) font chacune échouer le banc.

## Déclarer le serveur dans un client MCP

Claude Code :

```sh
claude mcp add sermoia --env SERMO_BIN=/usr/bin/gtk3sermo --env SERMO_RENDER_BIN=/usr/bin/qt6sermo --env SERMO_EXAMPLES_DIR=/chemin/vers/sermoia-mcp/examples -- /chemin/vers/sermoia-mcp/_build/sermoia-mcp
```

Autres clients (fichier de configuration JSON) :

```json
{
  "mcpServers": {
    "sermoia": {
      "command": "/chemin/vers/sermoia-mcp/_build/sermoia-mcp",
      "env": {
        "SERMO_BIN": "/usr/bin/gtk3sermo",
        "SERMO_RENDER_BIN": "/usr/bin/qt6sermo",
        "SERMO_EXAMPLES_DIR": "/chemin/vers/sermoia-mcp/examples"
      }
    }
  }
}
```

`qt6sermo` comme `SERMO_RENDER_BIN` rend sans affichage ni `xvfb`.

## Le protocole, à la main

```sh
printf '%s\n' \
  '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-06-18"}}' \
  '{"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"sermo_check","arguments":{"xml":"<window><vbox><bouton/></vbox></window>"}}}' \
  | SERMO_BIN=/usr/bin/gtk3sermo sermoia-mcp/_build/sermoia-mcp
```

La seconde réponse porte `isError: true` et le texte
`ERREUR — l'analyseur refuse ce XML. sermocore: Error in line 1, near token '<bouton/>': Unknown tag. …`.

Points du protocole :
- versions acceptées : 2025-06-18, 2025-03-26 et 2024-11-05 ; le serveur reprend celle du
  client s'il la connaît ;
- `ping` est pris en charge ;
- les notifications sont lues sans réponse ;
- codes d'erreur : -32700 (JSON illisible), -32600 (requête invalide), -32601 (méthode
  inconnue), -32602 (outil inconnu) ;
- les lots JSON-RPC ne sont pas pris en charge.

## Catalogue d'exemples (`examples/`)

Chaque sous-dossier est un **sujet**, chaque `.xml` un dialogue complet que le banc fait
analyser. Deux familles :

- **patrons de dialogue**, utiles à la génération : `formulaire`, `saisie-simple`,
  `confirmation`, `information`, `mot-de-passe`, `cases-a-cocher`, `choix-radio`,
  `zone-texte`, `liste-selection`, `liste-deroulante`, `onglets`, `tableau`, `arbre` ;
- **widgets**, un dialogue minimal par widget : `button`, `checkbox`, `entry`,
  `combobox`, `spinbutton`, `calendar`, `list`, `table`, `tree`, `notebook`, `menu`,
  `timer`…

Les noms de sujets et de fichiers se limitent aux lettres, chiffres, `-`, `_` et `.`.
Pointer `SERMO_EXAMPLES_DIR` ailleurs pour servir un autre catalogue.

## Limites connues

- **efl1** : sans `width-request`, un champ de saisie vide sort sans largeur sur
  l'image (défaut du port, mesuré le 2026-10-11).
- **html et ncurses** n'ont pas d'image : l'aperçu est le texte de `--print-ir`.
- **`sermo_run` avec ncurses** : le serveur n'a pas de terminal à lui donner, le dialogue
  sort aussitôt (`EXIT="abort"`).
- **Pas d'appels en parallèle** : les requêtes sont traitées une par une, et un
  `sermo_run` attend la fermeture du dialogue.
