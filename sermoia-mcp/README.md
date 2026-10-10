<img src="icon/sermoia-mark.png" alt="sermoia" width="88" align="right">

# sermoia-mcp — serveur MCP pour écrire du sermo avec une IA

Donne à une IA la boucle **apprendre → générer → vérifier → voir → corriger** sur
le langage XML de sermo, sous forme d'outils **MCP** (JSON-RPC 2.0 sur stdio).

C'est le second serveur MCP de sermo, à côté de [sermoman-mcp](../sermoman-mcp/)
qui sert le manuel : sermoman **explique** le langage, sermoia **fait écrire,
vérifier et voir** des dialogues. Écrit en C (GLib, json-glib). GPL-2.0-or-later.

## Outils

| Outil | Rôle |
|---|---|
| `sermo_schema` | décrit le langage en JSON : conteneurs, widgets, balises de contenu, attributs, actions, signaux |
| `sermo_check` | valide un XML (`--print-ir`) et renvoie `{ok:…}` avec le diagnostic de l'analyseur |
| `sermo_run` | affiche le dialogue et renvoie les valeurs saisies (`VAR="…"`) — demande un affichage, ou le port ncurses dans un terminal |
| `sermo_render` | dessine la fenêtre en **PNG** (`--render-png`) pour que l'IA la voie ; sans image possible, renvoie un aperçu structurel texte |
| `sermo_examples` | sert des exemples **validés** du catalogue, filtrés par sujet (`task`) et **triés par vote** |
| `sermo_vote` | enregistre qu'un exemple a aidé (+1) ou trompé (−1) ; registre en ajout seul |

Variables d'environnement :

- `SERMO_BIN` — binaire sermo utilisé pour vérifier et exécuter (défaut : `sermo`) ;
- `SERMO_RENDER_BIN` — binaire utilisé pour le rendu (défaut : `SERMO_BIN`) ;
- `SERMO_EXAMPLES_DIR` — catalogue d'exemples (défaut : `examples`) ;
- `SERMO_VOTE_FILE` — registre de vote (défaut : `$XDG_STATE_HOME/sermoia/votes.tsv`).

## Construire, lancer, vérifier

```sh
cmake -S sermoia-mcp -B sermoia-mcp/_build && make -C sermoia-mcp/_build
SERMO_BIN=/usr/bin/gtk3sermo SERMO_EXAMPLES_DIR=sermoia-mcp/examples \
    sermoia-mcp/_build/sermoia-mcp          # parle MCP sur stdio
bash sermoia-mcp/tests/verifie.sh         # schéma, exemples, protocole
```

Dépendances : `libglib2.0-dev`, `libjson-glib-dev`. La version annoncée aux
clients est celle du fichier `VERSION` du dépôt.

`tests/verifie.sh` contrôle trois choses sur le code réel : le schéma nomme
**exactement** les balises de widget que reconnaît l'analyseur du cœur ; chaque
exemple du catalogue s'analyse ; le serveur répond au protocole (version, six
outils, XML juste et XML faux distingués, exemples, vote).

## Un seul serveur pour tous les ports

Le langage et l'analyseur sont ceux du cœur, communs aux neuf ports : `schema`,
`check`, `run`, `examples` et `vote` ne dépendent pas du port choisi. Seul le
rendu en dépend :

- **ports à fenêtre** (gtk3, gtk4, qt6, fltk1, efl1, sdl3, lvgl) → image PNG ;
- **ncurses** (terminal) et **html** (exportateur) → aperçu structurel texte
  (`--print-ir`), toujours exploitable par l'IA.

## Catalogue d'exemples (`examples/`)

Chaque sous-dossier est un **sujet**, chaque `.xml` un dialogue complet que
`tests/verifie.sh` fait analyser par un binaire réel. Deux familles :

- **patrons de dialogue**, utiles à la génération : `formulaire`,
  `saisie-simple`, `confirmation`, `information`, `mot-de-passe`,
  `cases-a-cocher`, `choix-radio`, `zone-texte`, `liste-selection`,
  `liste-deroulante`, `onglets`, `tableau`, `arbre` ;
- **widgets**, un dialogue minimal par widget : `button`, `checkbox`, `entry`,
  `combobox`, `spinbutton`, `calendar`, `list`, `table`, `tree`, `notebook`,
  `menu`, `timer`…

Pointer `SERMO_EXAMPLES_DIR` ailleurs pour servir un autre catalogue.

## Déclarer le serveur dans un client MCP

```json
{
  "mcpServers": {
    "sermoia": {
      "command": "/chemin/vers/sermoia-mcp/_build/sermoia-mcp",
      "env": {
        "SERMO_BIN": "/usr/bin/gtk3sermo",
        "SERMO_EXAMPLES_DIR": "/chemin/vers/sermoia-mcp/examples"
      }
    }
  }
}
```
