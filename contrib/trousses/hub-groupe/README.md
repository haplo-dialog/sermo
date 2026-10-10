# hub — boîte à outils groupée (esprit PowerToys)

> **État mesuré le 2026-10-04** (voir `contrib/README.md`). La fenêtre s'ouvre sur
> les six ports graphiques. **En terminal (ncurses), ce hub ne marche pas encore** :
> il sort aussitôt, sans rien afficher.

Un seul conteneur, les outils rangés **par groupe** (`système / admin /
user / autres`), avec une **pastille de santé** par outil et une
**politique d'entreprise** optionnelle. Forme A : chaque module se lance
dans son propre processus.

```
hub/
  hub.sh            le conteneur groupé
  groupes.conf      politique (ordre des sections + modules exposés)
  modules/
    servys.sh       groupe admin, BESOIN_ROOT=oui
    info.sh         groupe user,  BESOIN_ROOT=non
```

## Ce que ça fait de plus que le hub simple

- **Sections par groupe.** L'axe est le **privilège/public**, pas le thème :
  `user` = sans root, `admin` = élévation, `système` = profond/secours.
- **Santé avant lancement.** Chaque module répond à `--check` ; le hub montre
  `●  prêt` ou `✕  <raison>`. Sur un parc hétérogène, on voit immédiatement
  ce qui est utilisable **sur la machine courante** (ex. `servys` → `✕
  systemd requis` sur une machine non-systemd).
- **Avertissement de privilège.** Un module `BESOIN_ROOT=oui` affiche `(root)`
  et, si l'opérateur n'a pas les droits, un `⚠` non bloquant (l'élévation se
  fera au lancement via `run_as_root`).
- **Politique `groupes.conf`** — contrôlée par l'entreprise, déployée par
  classe de machine ou par rôle :
  - `ORDRE: système admin user` — ordre des sections ;
  - `ACTIFS: servys info` — si présent, **seuls** ces modules sont montrés
    (sinon, tous ceux trouvés). Mêmes modules, exposition différente.

## Lancer

```sh
./hub.sh              # backend auto ; ncurses si pas d'affichage
./hub.sh gtk3sermo    # forcer un port
```

## Sécurité (inchangée)

Le hub ne fait jamais confiance au texte : il résout un choix en prenant le
**premier mot** de la ligne (le `NOM`), le valide sur l'**ensemble fermé**
re-scanné, et ne lance qu'un **chemin trouvé sur le disque**. Les lignes
d'en-tête (`──  admin  ──`) et « (aucun module) » ne résolvent vers rien.
Chaque module garde ses propres gardes.

## Vérifié ici

- `bash -n` OK ; manifests + `--check` des deux modules OK.
- Rendu groupé correct (sections, pastilles, tag `(root)`).
- Résolution par premier mot robuste **même quand la ligne porte la
  pastille** ; en-tête de section refusé.
- Politique `ACTIFS` testée (retirer `info` le fait disparaître du groupe
  user).
- XML du dialogue : **valide, zéro erreur**.

## À confirmer sur binaire réel (inchangé)

Le hub capture le stdout du sélecteur pour relire `MODULE`/`EXIT`. Vérifier
que `ncursessermo` dessine bien son UI sur le terminal quand stdout est
capturé ; sinon, router le dump via `<output file>`. À cocher au smoke test
ncurses.

## Passer en « hub de hubs » plus tard

Trivial : un hub racine dont chaque « module » est en fait un `hub.sh`
pointé sur un sous-dossier (`modules-système/`, `modules-admin/`…). Même
forme A, récursée.
