# sermo-backend-html — `htmlsermo`

Le port HTML de sermo. Il **n'ouvre pas de fenêtre** : il écrit le dialogue dans
une page HTML autonome — CSS embarqué, aucun fichier externe. Même cœur neutre
que les ports qt6, fltk1, efl1, sdl3, lvgl et ncurses : le cœur lit le dialogue, tient
les variables et joue les actions ; ce port écrit la page.

Aucune bibliothèque graphique : il s'installe sur une machine sans affichage.
Dans l'alternative `sermo`, il a la priorité la plus basse (1) : il ne répond
que si aucun port à fenêtre n'est installé.

## Trois modes (`SERMO_HTML_MODE`)

| Mode | Ce que fait la page | Réseau | JavaScript |
|---|---|---|---|
| `export` (défaut) | page morte : les champs sont verrouillés ; les valeurs sont imprimées comme le ferait une fenêtre fermée | aucun | aucun |
| `interactive` | les champs se saisissent ; « Composer le résultat » écrit le bloc `VAR="valeur"` à copier ou télécharger | aucun | un court script, dans la page |
| `serve` | la page est servie sur la boucle locale ; le formulaire renvoyé fait imprimer `VAR="valeur"` sur la sortie standard, comme les autres ports | 127.0.0.1 seulement | aucun |

Le mode `serve` est le seul écouteur réseau du projet. Ses gardes : écoute sur
`127.0.0.1` uniquement, port tiré par le noyau, jeton de 128 bits dans le chemin
(comparé à temps constant), une seule page servie, requête bornée, une seule
réponse puis sortie, délai d'expiration (`SERMO_HTML_TIMEOUT`, 300 s par défaut).

## Variables d'environnement

- `SERMO_HTML_OUT` — fichier écrit (défaut `sermo-output.html` ; `-` pour la
  sortie standard) ;
- `SERMO_HTML_MODE` — `export`, `interactive` ou `serve` ;
- `SERMO_HTML_TIMEOUT` — délai du mode `serve`, en secondes ;
- `SERMO_HTML_CSS` — feuille de style ajoutée **dans** la page (refusée si elle
  contient `</style`, ou au-delà de 256 Kio) ; `SERMO_HTML_CSS_ONLY=1` la met à
  la place du style interne ;
- `SERMO_HTML_IMG_MAX` — taille maximale d'une image embarquée.

## Ce que ce port ne fait pas

- Pas de fenêtre, donc pas de `--render-png` : les bancs qui cliquent ou mesurent
  une fenêtre (taille, clic, exemples sous xdotool) ne s'appliquent pas à lui.
- Une barre de progression dont la commande ne finit pas bloque l'export : la
  commande est lue jusqu'au bout avant les minuteries, comme sur ncurses.

## Construire

Dépendances : celles du cœur seulement.

```sh
ci/construire.sh html          # → sermo-backend-html/_build/htmlsermo
```

Bancs : `ci/bancs.sh html` (XML 55/55, comportement 61/61, mesurés le 2026-10-10).
