# `sermo-contract.h` — la frontière cœur ↔ backend

Cet en-tête déclare le contrat `sermo_be_*` : ce que le cœur attend d'un backend
de rendu, et le peu que le cœur lui fournit en retour. Les backends l'incluent
au lieu de l'ancien `sermo_backend.h`.

Onze fonctions sont obligatoires. Neuf sont **facultatives** : le cœur les
déclare faibles, un port qui ne les fournit pas se lie quand même et l'action
correspondante n'est pas servie. Six datent de la 3.3.0 :
`sermo_be_button_click`, `sermo_be_widget_append`, `sermo_be_widget_rows`,
`sermo_be_window_close`, `sermo_be_window_present`, `sermo_be_window_new`.
Chacune est décrite dans l'en-tête, au-dessus de sa déclaration.

Il fait partie du dépôt pour qu'un simple clone compile, sans rien aller chercher
ailleurs.

## Licence — elle n'est pas celle du reste du projet

Ce fichier est sous **MIT**, pas sous GPL-2.0-or-later comme le reste de sermo.
Une frontière d'interface doit pouvoir être reprise telle quelle par un cœur ou
un backend. La MIT est compatible avec la GPL : elle s'intègre au projet à la
seule condition de conserver sa mention de copyright et son texte, reproduits
dans l'en-tête et dans `LICENSE.MIT`.
