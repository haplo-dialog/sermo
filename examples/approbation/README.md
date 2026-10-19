<!-- SPDX-License-Identifier: CC0-1.0 -->
# approbation — une porte d'approbation humaine en une fenêtre sermo

Un agent IA (ou n'importe quel programme) veut faire quelque chose de risqué. Avant, une **vraie fenêtre**
s'ouvre : *Refuser* / *Autoriser*. Seul un clic sur *Autoriser* vaut accord.

| Fichier | Rôle |
|---|---|
| `approbation.sh` | La fenêtre. Le code de retour dit la réponse (voir plus bas). |
| `approbation-hook.sh` | Hook `PreToolUse` de Claude Code, qui appelle `approbation.sh` pour chaque action risquée. |
| `../../tests/garde_approbation.sh` | Le banc : faux sermo, aucun affichage requis. |

## Utiliser la fenêtre

```sh
./approbation.sh --detail "rm -rf build/"
./approbation.sh --titre "Mise à jour" --question "Le script veut redémarrer :" --delai 60 \
                 --detail-fichier /chemin/du/detail.txt
```

| Option | Rôle |
|---|---|
| `--detail TEXTE` | Le texte à montrer (ce que le demandeur veut faire). |
| `--detail-fichier FICHIER` | Le même, lu dans un fichier (64 Kio au plus). Il faut `--detail` ou `--detail-fichier`. |
| `--titre T` | Titre de la fenêtre (défaut « Approbation requise »). |
| `--question Q` | Phrase au-dessus du détail. |
| `--delai S` | Secondes d'attente, nombre entier ≥ 1 (défaut 120). |

Codes de retour : `0` autorisé · `1` refusé ou fenêtre fermée · `2` indisponible (pas d'affichage, pas de port
à fenêtre, sermo sans réponse) · `3` délai écoulé · `64` usage (option inconnue, délai invalide, fichier
illisible). Seul `0` vaut accord.

Le binaire est le premier trouvé parmi `gtk3sermo gtk4sermo qt6sermo fltk1sermo efl1sermo sdl3sermo sermo`,
s'il y a un affichage (`DISPLAY` ou `WAYLAND_DISPLAY`). `SERMO_APPROBATION_BIN` force un binaire (essais).

`--titre` et `--question` entrent dans le XML (échappés) : ils doivent venir de l'appelant, pas de l'agent
qu'on surveille.

Dépendances : `bash`, `coreutils` (`timeout`, `mktemp`, `head`), `sed`, un port sermo à fenêtre ; `python3`
pour le hook.

## Brancher le hook

Dans le fichier `settings.json` des réglages de Claude Code (celui du projet, ou
le vôtre pour tous les projets) :

```json
{
  "hooks": {
    "PreToolUse": [
      { "matcher": "Bash|Write|Edit",
        "hooks": [ { "type": "command",
                     "command": "/chemin/vers/sermo/examples/approbation/approbation-hook.sh" } ] }
    ]
  }
}
```

Réponses du hook : *Autoriser* → `allow` · *Refuser* ou fenêtre fermée → `deny` (l'agent est prévenu) ·
pas d'affichage, sermo en erreur, délai de 120 s (`APPROBATION_DELAI`) → `ask`, c'est-à-dire l'invite normale
de Claude Code. Jamais d'accord par défaut. Les commandes banales sans caractère de shell
(`ls`, `pwd`, `date`, `whoami`, `git status|diff|log`) ne demandent rien : les réglages habituels s'appliquent.

Variables du hook : `APPROBATION_DELAI` (secondes, défaut 120) ; `APPROBATION_SCRIPT` (chemin d'un autre
`approbation.sh` ; défaut : celui du même dossier).

## Ce que ça garantit, et ce que ça ne garantit pas

- **Garanti (testé)** : seule la réponse exacte `EXIT="autoriser"` donne 0 ; le texte venu de l'agent n'entre jamais
  dans le XML (il est affiché depuis un fichier temporaire en mode 600), donc pas d'injection de bouton ni de
  `$(…)` ; la réponse n'est jamais passée à `eval`.
- **Non garanti** : un agent qui a un shell **sous la même session graphique** peut, en X11, envoyer de faux clics
  (`xdotool`) à la fenêtre. Sous Wayland c'est beaucoup plus difficile, mais pas à exclure partout. Ce n'est donc
  pas un bac à sable : c'est une porte de confirmation, qui vaut ce que vaut la séparation entre l'agent et
  l'affichage. Pour une vraie frontière : agent dans un autre compte/conteneur, fenêtre sur la session de l'humain.
- Un hook qui ne porte que `Bash` ne voit pas une écriture de script suivie de son exécution : mettre `Write|Edit`
  au matcher, comme ci-dessus.

## Le banc

```sh
bash tests/garde_approbation.sh                          # faux sermo, sans affichage
bash tests/garde_approbation.sh /usr/bin/gtk3sermo       # + vraie fenêtre sous xvfb-run
```

Avec le faux sermo, le banc vérifie : seul `EXIT="autoriser"` donne 0 ; refus, fenêtre fermée
(`EXIT="abort"`), silence et délai ne donnent jamais 0 ; le détail arrive intact dans le fichier affiché et
n'entre pas dans le XML ; le hook rend `allow`, `deny` ou `ask`, et rien pour une commande banale ; une
commande banale suivie d'une autre (`ls; rm …`) est quand même demandée ; une écriture (`Write`) est demandée
et son chemin montré. Avec un vrai binaire en argument, il vérifie aussi que la fenêtre s'ouvre et attend
(le délai rend 3). `ci/bancs.sh` lance la version sans binaire.

Non vérifié par le banc : l'ordre de focus clavier. *Refuser* vient en premier dans le XML ; qu'un Entrée
distrait ne tombe pas sur *Autoriser* dépend du port.
