# Sécurité — sermo

[English](SECURITY.en.md)

sermo exécute des commandes shell décrites dans un script XML. Sa sécurité tient
à **où** cette exécution se fait et **comment** elle est bornée. L'essentiel se
fait dans le **cœur** `libsermocore`, commun aux sept backends : `safe_exec.c`
existe en deux copies, `src/` (six ports) et `src-gtk4/` (gtk4), qui ne diffèrent
que par trois lignes de commentaire ; les tests unitaires jouent les deux.

Deux chemins échappent au cœur, et c'est là que sont les limites : le
`<terminal>` de gtk3 et gtk4, et le `<linkbutton>` — voir
[Ce que ces deux variables n'atteignent pas](#ce-que-ces-deux-variables-natteignent-pas).

## Modèle de menace

La **frontière de confiance est l'auteur local du script XML**. sermo n'est pas
un bac à sable qui exécuterait du XML hostile : quiconque écrit le script peut
déjà lancer des commandes. Le rôle de sermo est de **ne pas transformer une
saisie utilisateur en exécution de code** que l'auteur n'a pas voulue.

## Exécution durcie (dans le cœur)

- **`safe_system` / `safe_popen`** remplacent `system`/`popen`. `safe_system`
  n'ouvre un shell **que** si la commande contient des métacaractères ; sinon
  elle exécute directement par `argv[]`, sans shell. Le repli `/bin/sh -c` est
  **journalisé**.
- **Aucun** `strcpy`/`strcat`/`sprintf` : bornage systématique.
- **Échappement de la sortie** : les valeurs exportées (`VAR="…"`) échappent les
  quatre caractères que le shell développe entre guillemets doubles
  (`\`, `"`, `$`, accent grave), pour que `eval "$(sermo …)"` ne réexécute pas ce
  qu'un utilisateur a tapé dans un champ. Vérifié par
  `tests/garde_echappement_sortie.sh` (banc « garde », 4 cas).
- **`--do`** est la voie recommandée quand le dialogue peut être utilisé par
  quelqu'un d'autre que l'auteur : les valeurs arrivent par l'environnement et ne
  sont **jamais** relues comme du code.
- **Un fichier Glade est du code** (`--glade-xml`, gtk3sermo et gtk4sermo) : ses
  gestionnaires de signaux sont des commandes. Ils passent par le **même chemin**
  que les `<action>` (`safe_system`, `SERMO_ALLOWED_CMDS`,
  `SERMO_NO_SHELL_FALLBACK`), et un fichier d'interface mérite la même confiance
  qu'un script de dialogue — pas davantage.

## Taille de ce que lit un `<input>`

Depuis la 2.7.3, chaque `<input>` — commande ou fichier — lit **au plus
16 Mio**. Au-delà, la lecture s'arrête comme sur une fin de fichier, et **un**
avertissement part sur la sortie d'erreur, jamais sur la sortie (qu'un `eval`
lit) :

    <input> « yes » : lecture arrêtée à 16777216 octets (limite SERMO_INPUT_MAX)

Une commande coupée ainsi reçoit `SIGPIPE` à sa prochaine écriture : `yes`
s'arrête de lui-même. La limite est posée dans le cœur
(`libsermocore/src/sermo_input.c`) et vaut pour les sept backends.

- **`SERMO_INPUT_MAX`** — la limite, en octets. `0` la retire. Une valeur qui
  n'est pas un nombre d'octets (`16M`, `-1`) est signalée, et la limite par
  défaut s'applique.
- **La barre de progression n'est pas plafonnée.** Elle ne garde qu'une ligne à
  la fois, et doit pouvoir suivre une longue commande jusqu'au bout : une copie
  qui affiche chaque nom de fichier dépasse vite 16 Mio. Sa mémoire reste stable,
  même sur `yes`.

Jusqu'à la 2.7.2, rien ne bornait cette lecture. Branché sur `yes`, gtk3sermo
dépassait 700 Mo en 7 secondes, et ncurses finissait sur une erreur de
segmentation (mesuré le 2026-09-17). Vérifié par
`tests/garde_input_sans_fin.sh` (sept ports, avec un témoin sans limite) et
`tests/unit/test_sermo_input.c`.

## Bornage optionnel de l'exécution (deux variables d'environnement)

Deux garde-fous, **éteints par défaut**, permettent de déployer un dialogue dans
un contexte moins fiable (kiosque, session invité) sans changer le script. Ils
sont lus dans le cœur (`safe_exec.c`, ses deux copies) et valent donc pour les
sept backends :

- **`SERMO_NO_SHELL_FALLBACK`** — quand elle est posée (à n'importe quelle
  valeur), le repli `/bin/sh -c` (utilisé pour les commandes contenant des
  métacaractères shell) est **refusé** au lieu d'être seulement journalisé : le
  programme **échoue proprement** (*fail-closed*) plutôt que d'ouvrir un shell.
  Les commandes sans métacaractère continuent de s'exécuter en direct par
  `exec()`. Vérifié par `tests/garde_option_do.sh`.
- **`SERMO_ALLOWED_CMDS`** — liste des commandes autorisées, séparées par des
  virgules (ex. `SERMO_ALLOWED_CMDS=ls,cat,/usr/local/bin/outil`). Tant qu'elle
  est posée, seules ces commandes peuvent être lancées, **et** le repli
  `/bin/sh -c` est refusé (sinon `sh -c '…'` contournerait la liste). La
  comparaison porte sur la commande **telle qu'elle sera lancée** :
  - une entrée sans `/` (`ls`) autorise ce nom, et le chemin exact auquel le
    `PATH` du programme le résout (`/usr/bin/ls`) ;
  - une entrée avec `/` autorise exactement ce chemin, et le nom seul si le
    `PATH` le résout vers ce chemin ;
  - un programme qui porte un nom autorisé mais se trouve ailleurs
    (`/tmp/x/ls`) est **refusé**.

  Une valeur vide équivaut à une variable absente. Vérifié par
  `tests/garde_allowed_cmds.sh`.

Les noms de la 1.x, **`HAPLO_NO_SHELL_FALLBACK`** et **`HAPLO_ALLOWED_CMDS`**,
sont encore lus quand le nom `SERMO_…` n'est pas posé, avec un avertissement :
un déploiement durci en 1.x le reste en 2.x.

### Ce que ces deux variables n'atteignent pas

Trois limites connues. Les deux premières ouvrent un chemin d'exécution qui ne
passe pas par `safe_exec.c` : la liste et le refus de repli y sont sans effet.

- **`<terminal>` ouvre un shell hors des deux garde-fous** (gtk3sermo et
  gtk4sermo, les seuls qui aient ce widget). Le widget lance `/bin/sh` par VTE
  (`sermo-backend-gtk3/src/widget_terminal.c`), et la sortie de son `<input>`
  y est **tapée** comme au clavier (`vte_terminal_feed_child`). Une commande
  autorisée qui imprime une ligne fait donc exécuter cette ligne par le shell
  du terminal. Mesuré le 2026-09-19 sur le binaire du paquet 2.7.1, avec
  `SERMO_ALLOWED_CMDS` **et** `SERMO_NO_SHELL_FALLBACK` posés : une `<action>`
  ordinaire est refusée au même moment où le `<terminal>` écrit son fichier
  témoin. **Un dialogue qui contient un `<terminal>` donne un shell** : ne le
  déployez pas dans un contexte que vous croyez borné par ces variables.
- **Les boutons-liens (`<linkbutton>`)** ouvrent leur adresse par le navigateur
  du bureau (`xdg-open` ou l'équivalent du toolkit), sans passer par cette
  liste. Ils ne passent pas par un shell pour autant : l'appel est un
  `execlp("xdg-open", …)` direct.
- **`--include` annule la voie directe, et s'oppose à la liste.** Avec cette
  option, chaque commande est réécrite en `. 'fichier'; commande` avant d'être
  lancée : la chaîne porte toujours `;` et `'`, donc **tout** part par
  `/bin/sh -c`, même une commande sans le moindre métacaractère. Et comme le
  repli est refusé dès que `SERMO_ALLOWED_CMDS` est posée, `--include` avec une
  liste blanche n'exécute plus rien du tout. Les deux mesures ont été faites le
  2026-09-19 sur le binaire du paquet 2.7.1.

## Durcissement du binaire

Cœur et backends compilent et lient avec : `-D_FORTIFY_SOURCE=3`,
`-fstack-protector-strong`, `-fstack-clash-protection`, `-fcf-protection=full`
(CET IBT/SHSTK), PIE, RELRO complet, BIND_NOW, NX. À conserver sur **chaque**
cible — vérifié par `tests/garde_durcissement.sh`.

`_FORTIFY_SOURCE` n'agit qu'avec l'optimisation. Chaque `CMakeLists.txt` choisit
donc le type de build `Release` quand aucun n'est donné ; un build `Debug` perd
cette protection. Le cœur est contrôlé à part, sur sa bibliothèque, par
`tests/garde_fortify_coeur.sh` : sur qt6, sdl3 et ncurses, le backend optimisé
apporte ses propres appels fortifiés et masquerait un cœur qui ne l'est pas.

⚠️ **CET est posé, pas actif.** Les notes IBT et SHSTK sont forcées à l'édition
de liens (`-Wl,-z,ibt -Wl,-z,shstk`), et le banc les trouve. Mais sur Debian testing,
ni les objets de démarrage de la glibc, ni la glibc, ni les bibliothèques des
toolkits n'en portent (mesuré au `readelf` le 2026-09-16). Sur ce système, CET ne
peut donc pas agir à l'exécution — déduit de l'absence des notes, pas mesuré en
exécution.

## Limites connues

- **Pas de bac à sable.** Une commande lancée par un dialogue a les droits de
  l'utilisateur qui l'a ouvert. sermo borne *comment* elle est lancée, pas *ce
  qu'elle fait*.
- **Pas d'élévation de privilèges** : pas de `pkexec`, rien qui demande un mot de
  passe administrateur.
- **Pas de liste de commandes imposée par défaut** : la confiance va à l'auteur du
  script. La liste optionnelle `SERMO_ALLOWED_CMDS` reste disponible (voir plus
  haut).
- **Le plafond vaut pour chaque `<input>`, pas pour tout le dialogue.** Un
  dialogue qui lit beaucoup d'`<input>`, ou qui les relit par `refresh:`, peut
  garder plusieurs fois 16 Mio (voir
  [Taille de ce que lit un `<input>`](#taille-de-ce-que-lit-un-input)).
- **La mémoire grossit dans un dialogue qui se rafraîchit sans fin** : chaque
  export des variables perd quelques octets, et `<list>` ou `<table>`
  rafraîchies sans `clear:` accumulent leurs rangées.
- **`<terminal>` donne un shell** (gtk3sermo, gtk4sermo) : il lance `/bin/sh`
  par VTE, hors de `SERMO_ALLOWED_CMDS` et de `SERMO_NO_SHELL_FALLBACK`, et la
  sortie de son `<input>` y est tapée comme au clavier (mesuré le 2026-09-19 ;
  voir plus haut).
- **`--include` fait tout passer par `/bin/sh -c`**, et n'exécute plus rien si
  une liste de commandes est posée (voir plus haut).
- **Un fichier Glade est du code**, et **`<linkbutton>`** ouvre son adresse hors
  de la liste des commandes (voir plus haut).
- **Pas d'audit externe.** Ce qui est vérifié l'est par les bancs du dépôt
  (`ci/bancs.sh`) et par des relectures du mainteneur. Les écarts ouverts sont
  dans [TODO.md](TODO.md).

## Signaler une faille

- Écrivez à **`devel@haplo-dialog.fr`**. N'ouvrez pas de ticket public tant que la
  faille n'est pas corrigée.
- Donnez : la commande lancée (`gtk3sermo`, `qt6sermo`…) et sa sortie `--version`,
  la version du paquet, un script XML minimal et les étapes qui mènent au
  problème.
- **Délais** — sermo a un seul mainteneur : accusé de réception sous **14 jours** ;
  correctif, ou avis public avec contournement, au plus tard **90 jours** après
  le signalement, plus tôt si la faille est déjà publique.
- **Versions suivies** : seule la dernière version publiée (2.7.x aujourd'hui)
  reçoit des correctifs. La ligne 1.x n'en reçoit plus : passez à la 2.x
  ([MIGRATION.md](MIGRATION.md)).
- Si vous le souhaitez, votre nom figure dans le [CHANGELOG.md](CHANGELOG.md) avec
  le correctif.
