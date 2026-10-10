# servys — gestionnaire de services (front-end systemctl)

> **État mesuré le 2026-10-04** (voir `contrib/README.md`). La fenêtre s'ouvre sur
> les six ports graphiques. Hors des ports GTK, choisir un service dans la liste
> ne met pas encore à jour le panneau d'état : passer par « Rafraîchir ».
> Démarrer, arrêter et redémarrer n'ont pas été essayés.

Premier outil de la trousse, écrit à la lettre selon
`contrib/trousses/CHARTE.md`. Voir/piloter les services systemd **en fenêtre** en
local, et depuis la **même interface en ncurses** via SSH.

## Ce qu'il fait

- Liste les unités `.service` (noms propres, un par ligne).
- Panneau d'état à droite : `systemctl status` du service sélectionné, avec
  un en-tête `État` en **symbole texte** (`●` actif · `○` inactif · `✕`
  échec), jamais une icône.
- Boutons **Démarrer / Arrêter / Redémarrer / Rafraîchir**.

## Pourquoi c'est un vrai outil, pas une démo

Il **enrobe `systemctl`**, ne réimplémente rien. Sa seule valeur unique :
le *même* front-end en GTK et dans un terminal. `systemctl` seul oblige à
retenir les sous-commandes ; une GUI GTK meurt en SSH ; `servys` fait les
deux avec un seul XML.

## Fallback — comment il bascule tout seul

Au lancement, sans `$DISPLAY` ni `$WAYLAND_DISPLAY`, il prend
`ncursessermo` s'il est présent. On peut forcer un port :
`./servys.sh ncursessermo` ou `./servys.sh gtk3sermo`.

**Élévation adaptative** (charte §5) : `pkexec` quand il y a un affichage,
sinon `doas`/`sudo` — l'invite de mot de passe se fait alors dans le TTY,
ce qui est sûr (le tabou « sudo-en-GUI » ne vaut que pour la fenêtre).

## Sécurité (charte §4)

Le nom d'unité issu de la liste est validé **deux fois** avant d'atteindre
`systemctl` : par motif (`^[a-zA-Z0-9@._-]+\.service$`) **et** par
appartenance à l'ensemble réel des unités (`grep -qxF`). La liste n'est pas
éditable ; la garde tiendrait quand même si un jour on la rendait libre.

## Ce que j'ai vérifié / ce qui reste à toi

Vérifié ici : syntaxe bash (`bash -n`), XML du dialogue bien formé
(`xmllint` après expansion), compteur du test déterministe (1,2,3).

**Non vérifié ici** (pas de systemd ni de binaire sermo dans
l'environnement) : le comportement réel de `systemctl` et la sélection de
liste. À faire de ton côté — voir ci-dessous.

### Test de parité automatique (mécanisme)

`tests/cas/31-servys-refresh.*` prouve, de façon déterministe, que
`action → Refresh:STATUT → re-exécution de l'input` rend la **même** valeur
en GTK et en ncurses (étalon `STATUT="2"`). À poser dans le corpus
`comportement/cas/` et à passer sur `gtk3sermo` puis `ncursessermo`.

Ce cas ne couvre pas la **sélection dans la liste** : le timer invisible
déclenche des actions, il ne clique pas une ligne. Cette partie passe par
le smoke test manuel.

### Smoke test manuel (les deux backends)

1. `./servys.sh gtk3sermo` : sélectionner un service → l'état s'affiche ;
   Rafraîchir recharge la liste ; Démarrer/Arrêter sur un service de test
   (p. ex. `cups.service`) change l'état après le clic.
2. En SSH, sans `$DISPLAY` : `./servys.sh` doit ouvrir en **ncurses** et se
   comporter à l'identique (états en symboles, mêmes boutons, même effet).
3. Vérifier que l'élévation bascule bien : pkexec en session graphique,
   invite `sudo`/`doas` dans le terminal.

Tant que le point 2 n'est pas propre, l'outil reste « GTK avec un README
optimiste » — la règle de la charte (§7).
