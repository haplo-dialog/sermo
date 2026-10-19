# Histoire de gtkdialog — de László Pere à aujourd'hui

[English](HISTOIRE.en.md)

sermo descend de **gtkdialog**. Ce document retrace qui a écrit gtkdialog, qui
l'a maintenu, quand, et où en sont aujourd'hui ses différentes lignées. Chaque
fait renvoie à sa source ; ce qui n'a pas pu être établi est dit comme tel.

Les sources ont été lues le 10 octobre 2026 : dépôts GitHub (historique des
commits, fichiers `AUTHORS`, `ChangeLog`, `NEWS`), archive Google Code, suivi
Debian et Launchpad. Trois sources n'ont pas répondu ce jour-là : l'ancien forum
Puppy Linux (accès refusé), le serveur FTP de l'auteur et la Wayback Machine.

## En bref

| Période | Qui | Où | Versions |
|---|---|---|---|
| 2003/2004 – 2007 | **László Pere** (programme), Kiss Gábor (documentation) | site personnel, université de Pécs | 0.58 → **0.7.20** |
| 2005 – 2009 | empaquetage Debian et Ubuntu, puis retrait | Debian, Ubuntu | jusqu'à 0.7.20 |
| 2011 – 2013 | **Thunor** | Google Code (svn) | 0.8.0, 0.8.2, **0.8.3** |
| 2018 – 2024 | **contributeurs de woof-CE (Puppy Linux)** | `puppylinux-woof-CE/gtkdialog` | 0.8.4b → 0.8.5e |
| 2024 – 2025 | **Mick Amadio (01micko)** : gtk3dialog | `01micko/gtk3dialog` | 0.1.0 → 0.1.2 |
| 2025 | **BunsenLabs** : empaquetage de gtk3dialog | `BunsenLabs/gtk3dialog` | 0.1.2 (paquet Debian) |
| 2026 | **gtkdialog3**, publié sous le nom de László Pere | `laszlopere/gtkdialog3` | 0.9.0 annoncée |
| 2026 | **sermo** (ce projet), fork de 0.8.3 | `haplo-dialog/sermo` | 2.x puis 3.x |

```
László Pere 0.58 … 0.7.20 (2004-2007)
   └─ Thunor 0.8.0 … 0.8.3 (2011-2013, Google Code)
        ├─ woof-CE / Puppy Linux 0.8.4b … 0.8.5e (2018-2024)
        │    └─ 01micko : gtk3dialog 0.1.x (2024-2025)
        │         └─ BunsenLabs : paquet Debian de gtk3dialog (2025)
        ├─ gtkdialog3 « GtkDialog2/3 » 0.9.0 (2026, compte laszlopere)
        └─ sermo 2.x puis 3.x (2026, haplo-dialog)
```

## 1. László Pere, l'auteur (2003/2004 – 2007)

- **Auteur** : le fichier `AUTHORS` de l'amont nomme « Program: Pere László »
  et « Documentation: Kiss Gábor ».
  [woof-CE `AUTHORS`](https://github.com/puppylinux-woof-CE/gtkdialog/blob/master/AUTHORS)
- **Dates** : les en-têtes des sources portent « Copyright (C) 2003-2007 László
  Pere ». La plus ancienne entrée du `ChangeLog` est la **0.58.6, du 22 avril
  2004**. La toute première version et sa date n'ont pas été retrouvées.
- **Dernière version** : **0.7.20, du 9 avril 2007** (`ChangeLog`).
- **Licence** : GPL version 2 « ou (à votre choix) toute version ultérieure »,
  soit **GPL-2.0-or-later**.
- **Hébergement** : le site personnel de l'auteur à l'université de Pécs
  (`linux.pte.hu/~pipas/gtkdialog/`). La page existe encore mais ne contient plus
  qu'un lien vers un serveur FTP aujourd'hui injoignable.
- **Numérotation** : la série 0.59.x (2004-2005) était la branche « instable » ;
  numériquement plus haute que 0.6 et 0.7, elle a obligé Debian à ajouter des
  *epochs* (`1:`, puis `2:`) à ses numéros de paquet.

**Debian** a empaqueté gtkdialog de 2005 (0.59.4, Fabio Tranchitella) à 2008
(2:0.7.20-4), puis l'a **retiré en 2009** faute d'amont maintenu (bogue
[#543945](https://bugs.debian.org/543945) ; Debian 5 « lenny » est la dernière
version qui le contient). Ubuntu l'a gardé jusqu'à 10.04.
[tracker.debian.org/pkg/gtkdialog](https://tracker.debian.org/pkg/gtkdialog)

## 2. Thunor et la série 0.8 (2011 – 2013)

- Le **16 juin 2011**, Thunor crée le projet `gtkdialog` sur Google Code et y
  importe l'archive **0.7.20** de László Pere, avec les correctifs de Debian. La
  page du projet le présente comme la continuation du projet de László Pere,
  qui n'avait plus le temps de le maintenir.
  [archive Google Code](https://code.google.com/archive/p/gtkdialog)
- Versions publiées : **0.8.0** (26 septembre 2011), **0.8.2** (21 septembre
  2012), **0.8.3** (**6 janvier 2013**, dernière archive publiée). La 0.8.1 n'a
  jamais été publiée.
- En **mars-avril 2013**, Thunor ajoute dans svn une prise en charge **GTK 3
  expérimentale** (`--enable-gtk3`). Elle n'a jamais été publiée en archive.
  Son dernier commit date du **9 avril 2013**.
- Les notices de copyright de Thunor portent **2011-2012** (code) et 2011-2013
  (exemples).

C'est la série 0.8, de loin la plus diffusée (Puppy Linux et ses dérivés), qui
fait parfois passer Thunor pour l'auteur de gtkdialog. L'auteur est László Pere ;
Thunor en est le second mainteneur.

## 3. woof-CE / Puppy Linux (2015 – 2024)

- Le dépôt [`puppylinux-woof-CE/gtkdialog`](https://github.com/puppylinux-woof-CE/gtkdialog)
  est créé le **14 mars 2015** et reprend **tout l'historique svn de Thunor**
  (mêmes commits, depuis l'import de la 0.7.20).
- Aucun commit entre avril 2013 et **mars 2018**. Le fichier `AUTHORS` note
  alors : « Continued by the woof-CE gtkdialog contributors 2018-03-19 ». D'où
  les deux dates : 2015 (titre) est la création du dépôt, 2018 (tableau) le
  début des contributions de woof-CE.
- Contributeurs principaux : woodenshoe-wi (GTK 3 et VTE 2.91, 2018),
  **Mick Amadio (01micko)**, **Dima Krasner** (construction Meson et
  **ancrage Wayland layer-shell**, novembre 2021), step- (widget `chooser`,
  commentaires XML, terminal GTK 3, 2022), peabee (fusions).
- GTK 2 et GTK 3 y cohabitent (option Meson `gtkver`).
- Versions : 0.8.4b (mars 2021, première version publiée sur GitHub), 0.8.5
  (mars 2022), … **0.8.5e (13 septembre 2024)**, qui est aussi le dernier commit.
- **État au 10 octobre 2026** : non archivé, 35 tickets ouverts, aucun commit
  depuis septembre 2024. C'est la **continuation directe** de la lignée
  Pere → Thunor.

## 4. gtk3dialog : 01micko, puis BunsenLabs (2024 – 2025)

- [`01micko/gtk3dialog`](https://github.com/01micko/gtk3dialog) est un fork
  GitHub de woof-CE (créé en 2020). Le **25 novembre 2024**, Mick Amadio y
  renomme tout en **gtk3dialog**, à partir du dernier commit de woof-CE. Il
  retire GTK 2 et autotools, et ajoute pages de manuel et exemples Wayland.
  Versions **0.1.x** ; la **0.1.2** date du **11 mai 2025**.
- [`BunsenLabs/gtk3dialog`](https://github.com/BunsenLabs/gtk3dialog) est un
  fork de `01micko/gtk3dialog`, créé le **4 juin 2025**. Sa branche principale
  est **identique** à celle de 01micko : BunsenLabs n'y ajoute que
  l'**empaquetage Debian** (branche `carbon`, John Crawley, juin-juillet 2025),
  pour sa distribution BunsenLabs « Carbon ».
- **Le support layer-shell n'a donc pas été écrit chez BunsenLabs** : il a été
  écrit dans woof-CE en 2021 (Dima Krasner, puis Mick Amadio), et il est arrivé
  chez BunsenLabs par gtk3dialog.
- **État au 10 octobre 2026** : aucun commit depuis mai 2025 chez 01micko,
  juillet 2025 chez BunsenLabs.

## 5. gtkdialog3, publié sous le nom de László Pere (2026)

- [`laszlopere/gtkdialog3`](https://github.com/laszlopere/gtkdialog3) est un
  dépôt **neuf, sans historique antérieur** : son premier commit (14 mars 2026)
  ajoute d'un coup les 300 fichiers du projet, portés de GTK 2 vers GTK 3. Le
  dépôt lui-même a été créé sur GitHub le 22 mai 2026.
- Il repart de **gtkdialog 0.8.3** (Thunor), pas de woof-CE ni de gtk3dialog :
  son `ChangeLog` passe directement de mars 2026 à la 0.8.3 de janvier 2013.
- Le projet s'est d'abord appelé « GtkDialog2 », puis « GtkDialog3 » ; il
  annonce une version **0.9.0**, mais aucune version n'est publiée (ni étiquette,
  ni archive) au 10 octobre 2026.
- Ses fichiers `AUTHORS` et `NEWS` le présentent comme repris « par l'auteur
  original ». Des indices vont dans ce sens (adresse de courriel au domaine de
  l'université de Pécs dans deux commits) ; ce document ne le tient pas pour
  prouvé et écrit donc « publié sous le nom de László Pere ».
- Licence : GPL-2.0-or-later (en-têtes SPDX).
- Dernier commit : 14 juin 2026.

## 6. Et sermo

- sermo part de **gtkdialog 0.8.3** — l'archive publiée par Thunor, qui ne
  connaît que GTK 2. Il garde le langage XML et l'analyseur, et réécrit le reste
  en un cœur commun et plusieurs couches de rendu.
- L'**ancrage Wayland** de `gtk3sermo` (attributs `layer`, `edge`, `dist`,
  `reserve`) a été porté depuis gtk3dialog (dépôt BunsenLabs) ; il a été écrit
  dans woof-CE par **Dima Krasner** (2021) et étendu par **Mick Amadio**
  (2021-2024). Leurs notices sont conservées.
- La documentation du serveur MCP de sermo reprend la structure de la
  documentation `for-claude` de gtkdialog3.
- Le détail, fichier par fichier, de ce qui vient de l'amont est dans
  [LICENCES.md](LICENCES.md).

## Ce qui reste incertain

- La date et le numéro de la toute première version de gtkdialog.
- Le contenu de l'ancien forum Puppy Linux et du FTP de l'auteur (inaccessibles
  le 10 octobre 2026).
- L'identité de la personne derrière le compte `laszlopere` : plausible, non
  prouvée.
- La date réelle d'écriture des commits de gtkdialog3 (datés de mars 2026, mais
  publiés en mai 2026).

## Sources

- <https://github.com/puppylinux-woof-CE/gtkdialog> (`AUTHORS`, `ChangeLog`, `NEWS`, commits, versions)
- <https://github.com/01micko/gtk3dialog> (commits, étiquettes)
- <https://github.com/BunsenLabs/gtk3dialog> (`README.md`, branche `carbon`, `debian/changelog`)
- <https://github.com/laszlopere/gtkdialog3> (`AUTHORS`, `NEWS`, `ChangeLog`, commits)
- <https://code.google.com/archive/p/gtkdialog> (projet et téléchargements de Thunor)
- <https://tracker.debian.org/pkg/gtkdialog>, <https://snapshot.debian.org/package/gtkdialog/>, <https://bugs.debian.org/543945>
- <https://launchpad.net/ubuntu/+source/gtkdialog>
- <https://aur.archlinux.org/packages/gtkdialog>
