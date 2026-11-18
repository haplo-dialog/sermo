# Publier une version de sermo — liste à cocher

Rien ne se publie tant que chaque case n'est pas cochée **avec sa preuve** (sortie de
commande, date, constat). Les preuves vont dans le fichier de mesures du dossier de publication. Règles : `VERSIONING.md`,
section « Rythme ».

## 1. Le moment

- [ ] **14 jours au moins** depuis la dernière version mineure de sermo (date de la
      dernière entrée publiée du CHANGELOG ou de `debian/changelog`).
      Sinon : **exception** (faille de sécurité ou régression grave), raison écrite dans
      l'entrée de version.
- [ ] Une **majeure** a été annoncée au cahier de route au moins un mois avant.

## 2. Ce qui est vérifié par la machine

- [ ] Tests verts : `ci/bancs.sh` puis `ci/bilan-md.sh` (et la CI en ligne de la racine publiée, verte).
- [ ] Sanitizers verts : `SERMO_BANC_SANITIZERS=1 bash ci/bancs.sh` (ASan et UBSan sur les neuf ports), ou le job `sanitizers` de la CI.
- [ ] Garde de version verte : `bash tests/garde_version.sh`.
- [ ] Paquets : paquets construits depuis un clone propre, `lintian` sans erreur, installés dans une Debian testing vierge puis purgés sans reste ; bancs rejoués sur les binaires des paquets.

## 3. Ce que la doc doit dire (selon ce qui change pour l'utilisateur)

| Changement | À jour avant de publier |
|---|---|
| interne (code, tests) | CHANGELOG |
| visible (option, comportement) | CHANGELOG, NEWS FR/EN, `debian/changelog`, manuel FR/EN, pages de manuel et Texinfo, README |
| mis en avant (nouveauté, vitrine) | les précédents, plus le site |

- [ ] Chaque changement de la version est rangé dans une ligne de ce tableau, et ses
      documents sont à jour. Rien n'y dit le contraire du code (chercher les anciennes
      phrases **sans filtre d'extension**).

## 4. Les relectures

- [ ] Le code de la version a été relu par une IA qui ne l'a pas écrit, avec une
      **liste écrite** de ce qu'il faut vérifier ; ses constats sont vérifiables et traités.
- [ ] Zones à risque touchées (`<action mavlink>`, `<action media>`, `--do`, `safe_system`, programmes d'aide réseau (MQTT, Modbus, OPC UA), serveurs MCP) : **essai réel ou banc** joué, pas
      seulement une lecture.
- [ ] Le mainteneur a lu **ce qu'on promet** : README et notes de version FR/EN, en mots simples
      (≈ 15 min). Date de cette lecture : ……

## 5. Avant et après

- [ ] `INVENTAIRE.md` relu (statut, copie de vérité, version) — fichier du mainteneur, hors de ce dépôt.
- [ ] Retour arrière prévu : une version publiée ne se dépublie pas. Si elle est mauvaise,
      on publie un correctif qui annule le problème, et le CHANGELOG le dit.
- [ ] Après publication : version suivante ouverte (non publiée), cahier de route coché.
