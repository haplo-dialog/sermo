# `sermo-contract.h` — la frontière cœur ↔ backend

Cet en-tête déclare le contrat `sermo_be_*` : ce que le cœur attend d'un backend
de rendu, et rien d'autre. Les backends l'incluent au lieu de l'ancien
`sermo_backend.h`.

Il fait partie du dépôt pour qu'un simple clone compile, sans rien aller chercher
ailleurs.

## Licence — elle n'est pas celle du reste du projet

Ce fichier est sous **MIT**, pas sous GPL-2.0-or-later comme le reste de sermo.
Une frontière d'interface doit pouvoir être reprise telle quelle par un cœur ou
un backend. La MIT est compatible avec la GPL : elle s'intègre au projet à la
seule condition de conserver sa mention de copyright et son texte, reproduits
dans l'en-tête et dans `LICENSE.MIT`.
