# sermoman-mcp

**Un compagnon de documentation pour développer avec sermo, à côté de votre
assistant IA.**

Ce petit serveur [MCP](https://modelcontextprotocol.io) donne à un assistant IA
(Claude, ou tout autre client compatible) une connaissance précise et à jour de
**sermo** : la syntaxe XML des dialogues, l'architecture modulaire (le cœur
`libsermocore` et les sept backends de rendu), l'histoire du projet et son
écosystème d'outils. Votre assistant peut alors vous aider à écrire des
dialogues corrects, sans inventer de balise.

> **Optionnel.** Il vit dans l'arbre des sources de sermo. sermo fonctionne
> parfaitement sans ; installez ce compagnon si vous développez avec l'aide
> d'une IA et souhaitez qu'elle connaisse le format sur le bout des doigts.

## Protection de l'utilisateur d'abord

Ce serveur est conçu pour être **inoffensif** :

- **Strictement local** : il communique par l'entrée/sortie standard (`stdio`).
  Il **n'ouvre aucun port**, ne fait **aucun accès réseau** (ni entrant ni
  sortant). Rien ne quitte votre machine.
- **Lecture seule** : il n'exécute rien, n'écrit aucun fichier, ne publie rien.
- **Contenu embarqué** : la documentation qu'il sert est incluse dans le dépôt,
  dérivée de la documentation publique du projet.

Le pire qu'il puisse faire, c'est vous donner une réponse de documentation.

## Prérequis

- Python 3 (aucune dépendance externe).

## Installation

Ce serveur est fourni dans l'arbre des sources de sermo, à
`sermoman-mcp/server.py`. Il suffit de le déclarer dans la configuration MCP de
votre client, en pointant le chemin **absolu** vers `server.py` (voir
`mcp.example.json`) :

```json
{
  "mcpServers": {
    "sermoman": {
      "command": "python3",
      "args": ["/chemin/absolu/vers/sermo/sermoman-mcp/server.py"]
    }
  }
}
```

Redémarrez votre client : l'assistant dispose alors des outils ci-dessous.

## Outils fournis

| Outil | Ce qu'il donne |
|-------|----------------|
| `sermo_reference` | La référence complète de la syntaxe XML (widgets, attributs, sous-éléments), commune aux sept backends. |
| `sermo_guide` | Le guide de prise en main (le cœur et les backends, installation, compatibilité gtkdialog, sécurité, licences). |
| `sermo_architecture` | L'architecture modulaire : `libsermocore`, les sept backends, le contrat cœur↔backend, les variantes de build, le packaging, les bancs. |
| `sermo_heritage` | La vie de sermo : lignée gtkdialog → haplo-dialog → sermo, historique des versions, feuille de route, bilan de santé. |
| `sermo_ecosystem` | Les outils autour de sermo (sermoman-mcp, le binaire et ses options de vérification) : quel outil pour quoi. |
| `sermo_search` | Recherche plein texte dans toute la documentation embarquée. |
| `sermo_example` | Un exemple complet de dialogue ; sans argument, la liste des exemples curés disponibles. |
| `sermo_how_to_report` | Comment signaler un bug ou proposer une amélioration. |

## Vérifier que la documentation servie est juste

Deux bancs **locaux** contrôlent la documentation contre le code réel du dépôt
modulaire (les binaires des backends dans `../sermo-backend-*/_build/` et les
sources de `../libsermocore/`).

```sh
./tests/verifie-exemples.sh
```

Ce banc extrait **chaque** bout de XML embarqué dans `data/` et le rejoue contre
les binaires réels des backends avec `--print-ir`. Il échoue si un seul exemple
n'est pas analysable.

Il existe parce que le 2026-08-30, plusieurs formes documentées étaient des
erreurs de syntaxe — `<frame><label>`, `<notebook><label>`,
`<expander><label>`, `<pixmap><filename>`, `<table><column-header>` — et
l'exemple phare du guide ne démarrait pas. Rien ne rejouait la documentation,
donc personne ne l'avait vu.

Codes de retour : `0` tout passe · `1` un exemple casse, **ou** rien n'a été
extrait · `77` aucun binaire de backend n'est disponible, rien n'a été vérifié.
Un banc muet ne rend jamais 0.

```sh
./tests/verifie-verite.sh
```

Le banc précédent prouve que la syntaxe documentée s'analyse. Il ne prouve rien
sur le **fond** : l'affirmation « `safe_system()` impose une liste blanche de
commandes autorisées » serait passée au travers — et elle est passée au travers
pendant des mois, alors que le code laisse tout passer par défaut quand
`SERMO_ALLOWED_CMDS` est absente.

Celui-ci relie chaque affirmation forte de `data/` à un fait du code de
`libsermocore` : le défaut de `safe_system`, l'absence de la vieille formule
fausse, l'existence des deux verrous d'environnement, et la couverture des
balises du lexer par la référence.

Codes de retour : `0` · `1` · `77` si les sources de `libsermocore` sont
introuvables (poser `SERMO_SRC`).

## Signaler un bug, proposer une idée

Ce compagnon ne publie rien lui-même. Pour remonter un bug ou une idée
d'amélioration sur sermo, indiquez la version, le backend utilisé, un script XML
minimal qui reproduit le cas, et le comportement attendu vs observé.

## Licence

GPL-2.0-or-later — voir [COPYING](COPYING). Documentation embarquée dérivée de
la documentation publique de sermo (elle-même héritée de gtkdialog 0.8.3,
László Pere).
