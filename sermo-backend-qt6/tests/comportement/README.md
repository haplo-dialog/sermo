# Banc de comportement qt6 (étalonné sur gtk3sermo)

Compiler ne prouve rien sur ce qu'un widget **restitue** : le port qt6 passe par
le shim `qt6-compat.h`, qui portait des leurres (`g_strsplit`→`NULL`, etc.,
corrigés en M1). Ce banc joue chaque cas et compare la valeur exportée au shell
à celle du port de **référence gtk3sermo**. Même syntaxe → même valeur : c'est
la promesse « écrit une fois, tourne sur GTK ou Qt ».

## Lancer

```sh
# le lanceur générique joue ce corpus sur N'IMPORTE QUEL backend :
bash tests/comportement/run.sh sermo-backend-qt6/_build/qt6sermo
bash tests/comportement/run_all.sh          # les sept backends
# ce lanceur-ci, historique, reste utilisable : QT6_BIN=<binaire> ./run.sh
```

Codes : `0` parité atteinte · `1` au moins un écart/blocage · `77` binaire
qt6sermo ou xvfb absent (rien vérifié — jamais un faux succès).

## ⚠️ Ce banc est VOLONTAIREMENT ROUGE jusqu'à la parité (jalon M3)

Les fichiers `cas/*.attendu` sont l'**étalon gtk3sermo**, pas des valeurs
adaptées à qt6. Chaque widget réparé fait passer son cas du rouge au vert. Vert
partout = parité.

⛔ **Ne jamais** « corriger » un `.attendu` vers ce que qt6 rend aujourd'hui pour
faire taire un échec : ce serait consacrer le bug. On répare le port, pas
l'étalon.

## État au 2026-09-17 : 53 cas, parité sur les sept backends

Le corpus compte **53 cas** ; les sept backends rendent les mêmes valeurs que
gtk3sermo sur les 53 (`tests/comportement/run_all.sh`, rejoué le 2026-09-17).

**`<chooser>` (cas 53, 2.7.1).** Les cinq ports sans GTK **avortaient** dès
l'ouverture d'un dialogue qui en contient un (« ASSERT FAILED: widget != NULL »,
code 134) : le cœur y construisait le sélecteur embarqué par un appel GTK qui rend
NULL. Le cas vérifie que le dialogue s'ouvre, se ferme et n'exporte rien tant que
rien n'est choisi. Il a aussi révélé que le `<filechooser>` de fltk1 exportait son
libellé d'invite « (Aucun) » au lieu d'une valeur vide.

**Dossier de travail.** Chaque cas joue dans un dossier NEUF, garni d'une copie de
`cas/donnees/` : un cas lit un fichier par un chemin relatif (`<input
file>donnees/…`) ou compte ce qu'une commande a écrit, sans dépendre du dossier
d'où l'on lance le banc ni des cas précédents.

**`<input>` (cas 43 à 52, 2.7.1).** Commande et fichier, pour entry, text, edit,
comboboxtext, combobox, list, tree, table et les widgets numériques ; chaque
commande ne s'exécute qu'une fois (les cas exportent un compteur). ⚠️ Chez
l'étalon, la 1re rangée d'une list, d'un tree ou d'une table ne ressort que si le
widget a le **focus** (premier widget focalisable) : ces cas les placent SEULS
dans leur fenêtre, sinon la valeur attendue dépendrait de l'ordre des widgets.

Historique — au 2026-09-01, 22 cas et la parité qt6. Corrigés en M3 (du plus fort levier au plus dur) : `timer` (true/false),
`infobar`, `filechooser`, `entry`+`<input>`, `menuitem` cochable, `fontbutton`
(« Famille Taille »), `combobox` (pas de sélection sans défaut), `list`, `tree`,
`table` (en-tête depuis `<label>`, colonne exportée).

⚠️ **Un cas ≠ un widget.** Sont couverts les widgets qui RENDENT une valeur.
Les conteneurs et widgets d'affichage (window, vbox, hbox, frame, notebook,
expander, aspectframe, eventbox, scrolledw, statusbar, pixmap, image,
séparateurs, drawingarea, spinner, pulse, linkbutton, menubar, button) n'ont pas
de cas — ils n'exportent pas de valeur lisible. Exceptions, parce qu'un
comportement s'y mesure autrement : `progressbar` (cas 41, sa commande ferme le
dialogue arrivée à 100), `frame` (cas 38 et 42, titre exporté, enfants tous
vivants), `levelbar` (cas 31 et 51, sa valeur) et `text` (cas 43, le texte lu
par `<input>`).
`comboboxentry` reste à caser.

⚠️ **À back-porter** : ces cas vivent dans l'arbre qt6 ; les rapatrier dans le
banc partagé gtk3sermo (public) pour qu'ils soient une spec commune aux ports.

Câblage CI : le banc tourne dans la CI du dépôt (`ci/bancs.sh`), sur les sept
backends.
