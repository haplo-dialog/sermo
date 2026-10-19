# Banc de repères visuels

Cinq scripts shell, cinq fenêtres. Ensemble, ils passent en revue l'essentiel
de ce que `gtk3sermo` sait afficher, et servent à vérifier d'un coup d'œil
qu'une version se comporte comme la précédente.

Chacun est autonome : rien à installer, rien à configurer.

```sh
./01-formulaire.sh
```

| Script | Ce qu'il montre |
|---|---|
| `01-formulaire.sh` | `entry`, `password`, `spinbutton`, `checkbox`, `comboboxtext`, boutons, imbrication `frame` > `vbox` > `hbox` |
| `02-conteneurs.sh` | colonnes, zone extensible, bandeau pleine largeur, répartition de l'espace |
| `03-onglets.sh` | `notebook` à trois onglets placés en haut (`tab-pos="top"`), changement de page, contenu multiligne |
| `04-indicateurs.sh` | `progressbar`, `hscale`, `vscale`, jauge de niveau, compteur, `spinner` |
| `05-boutons-icones.sh` | icônes du thème par nom, boutons à icône, alignements |

Les cinq acceptent un nom de binaire en argument (défaut `gtk3sermo`) :
`./01-formulaire.sh qt6sermo`. `./01-formulaire.sh gtkdialog` fonctionne aussi
quand le paquet `sermo-gtkdialog` est installé : il pose `gtkdialog` comme
alias de `gtk3sermo`.

## Licence

Domaine public (**CC0-1.0**) : ces cinq scripts sont écrits pour sermo.
Copiez, modifiez, republiez sans condition et sans mention.

Une partie d'`examples/` vient de gtkdialog (GPL) ; chaque fichier dit sa
licence.
