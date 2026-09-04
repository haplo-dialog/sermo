# Banc de comportement qt6 (étalonné sur gtk3sermo)

Compiler ne prouve rien sur ce qu'un widget **restitue** : le port qt6 passe par
le shim `qt6-compat.h`, qui portait des leurres (`g_strsplit`→`NULL`, etc.,
corrigés en M1). Ce banc joue chaque cas et compare la valeur exportée au shell
à celle du port de **référence gtk3sermo**. Même syntaxe → même valeur : c'est
la promesse « écrit une fois, tourne sur GTK ou Qt ».

## Lancer

```sh
QT6_BIN=/chemin/vers/qt6sermo ./run.sh
# ou, si qt6sermo est dans le PATH : ./run.sh
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

## État : 22 cas, PARITÉ atteinte (2026-09-01)

Le banc couvre 22 cas et qt6 rend **les mêmes valeurs que gtk3sermo sur les 22**.
Corrigés en M3 (du plus fort levier au plus dur) : `timer` (true/false),
`infobar`, `filechooser`, `entry`+`<input>`, `menuitem` cochable, `fontbutton`
(« Famille Taille »), `combobox` (pas de sélection sans défaut), `list`, `tree`,
`table` (en-tête depuis `<label>`, colonne exportée). Détail et méthode :
`haplo-kb/reference/port-qt6-shim-creux.md`.

⚠️ **22 cas ≠ 47 widgets.** Sont couverts les widgets qui RENDENT une valeur.
Les conteneurs et widgets d'affichage (window, vbox, hbox, frame, notebook,
expander, aspectframe, eventbox, scrolledw, text, statusbar, pixmap, image,
séparateurs, drawingarea, spinner, pulse, progressbar, levelbar, linkbutton,
menubar, button) n'ont pas de cas — ils n'exportent pas de valeur lisible.
`comboboxentry` reste à caser.

⚠️ **À back-porter** : ces cas vivent dans l'arbre qt6 ; les rapatrier dans le
banc partagé gtk3sermo (public) pour qu'ils soient une spec commune aux ports.

Câblage CI : la parité étant atteinte sur ces 22 cas, le banc peut désormais
entrer dans la CI (jalon M4) comme garde anti-régression — à faire une fois le
banc élargi aux widgets restants et back-porté dans le banc partagé.
