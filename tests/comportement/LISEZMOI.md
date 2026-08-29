# Banc de comportement

## Pourquoi il existe

`tests/xml/run_tests.sh` lance `--print-ir` : il **parse** le XML, imprime la
représentation interne, et sort. Il ne construit aucun widget et n'ouvre aucune
fenêtre. Ses 55 PASS attestent que le XML est **compris** — rien d'autre.

Le 2026-08-29, cinq fonctions du port GTK 4 se sont révélées mortes ou menteuses
alors que la suite XML était verte depuis des mois :

| | Ce que ça rendait |
|---|---|
| `<checkbox><default>true</default>` | `CB="false"` |
| `<radiobutton><default>true</default>` | `RB="false"` |
| `border-width` | aucun effet, fenêtre inchangée |
| `<menuitem checkbox=>` | aucune variable |
| `<filechooser>` | `FC=""`, et `<output file:>` **tronquait** le fichier |

Aucune n'était détectable par `--print-ir` : le XML était parfaitement compris.
C'est ce que le programme en **faisait** qui était faux.

## Ce que ce banc mesure

`run.sh` lance le dialogue **pour de vrai** sous Xvfb et compare les variables
shell qu'il rend. Chaque cas se ferme tout seul par un
`<timer><action>exit:fin</action></timer>`.

`geometrie.sh` mesure ce qui ne produit **aucune** variable : la taille. C'est
le seul moyen de prouver `border-width`.

Un `Gtk-CRITICAL` ou un `WARNING **` fait **échouer** le cas. Les cinq défauts en
émettaient, et personne ne les lisait.

## Les deux modes de `run.sh`

```sh
./run.sh /chemin/absolu/vers/gtk4sermo                    # contre les .attendu
./run.sh --diff /chemin/gtk3sermo /chemin/gtk4sermo       # les deux ports doivent coïncider
```

- **attendu** : chaque cas a un `.attendu` qui fait foi. Marche avec **un seul**
  port, donc utilisable en intégration continue. C'est le mode branché dans les
  trois CI.
- **`--diff`** : le port GTK 3 sert d'**oracle**, parce que c'est lui qui marche.
  Toute divergence est un défaut du port GTK 4, sans avoir à écrire l'attendu.
  **C'est le mode qui aurait attrapé les cinq défauts sans qu'on ait rien prévu.**

## Ajouter un cas

1. Écrire `cas/NN-nom.xml`, avec une ligne `<!-- DESC: … -->` et un
   `<timer visible="false"><variable>_T</variable><action>exit:fin</action></timer>`
   pour que le dialogue se ferme seul.
2. Vérifier qu'il parse : `gtk3sermo --program=MAIN_DIALOG --print-ir`.
3. Engendrer l'attendu depuis l'oracle :

```sh
MAIN_DIALOG="$(cat cas/NN-nom.xml)" xvfb-run -a \
  /chemin/gtk3sermo --program=MAIN_DIALOG 2>/dev/null \
  | grep -E '^[A-Za-z_][A-Za-z0-9_]*=' | grep -v '^EXIT=' | LC_ALL=C sort > cas/NN-nom.attendu
```

4. Relire l'attendu **à la main**. S'il est vide ou trivial, le cas ne prouve
   rien : c'est arrivé avec `<combobox>`, qui rend `""` sur les **deux** ports.
   On l'a remplacé par `<comboboxtext>`, qui rend une vraie valeur.

## Deux règles

- ⛔ **Donner un CHEMIN, jamais un nom.** `run_tests.sh` cherche le binaire
  d'abord dans le `PATH` : depuis que les paquets sont installés,
  `./run_tests.sh gtk4sermo` valide `/usr/bin/gtk4sermo` et non la construction
  en cours. Ce banc-ci **refuse** un nom sans barre oblique.
- ⛔ **Il ne se tait jamais.** Pas de Xvfb, pas de binaire, aucun cas trouvé : il
  sort en **erreur**, jamais en succès vide. Un banc muet qui répond « tout va
  bien » est pire que pas de banc.

## Ce qu'il ne couvre pas

Les couleurs, les polices, le placement fin, et tout ce qui n'est ni une valeur
rendue ni une taille de fenêtre. Pour ces cas-là, il reste
`tests/garde_clic_widgets.sh` et l'œil.
