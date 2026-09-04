# Politique d'empaquetage - haplo-dialog

> Comment haplo-dialog est distribué : la source est le livrable principal, le
> `.deb` se construit depuis l'arbre, et des recettes non construites par nos
> soins sont fournies pour les autres familles de distributions.
>
> **Les binaires sont publiés** depuis le 2026-08-24, joints à la release
> [v1.1.4](https://gitlab.com/haplo-dialog/sermo/-/releases/v1.1.4) avec leurs sommes de contrôle : `gtk3sermo`, `gtk4sermo`,
> `gtksermo` et les deux paquets de symboles. Le troisième port, `qt6sermo`
> (1.0.1-1), n'a **pas encore** de `.deb` construit : sa source et ses recettes
> sont dans l'arbre, rien n'est publié pour lui. Il n'y a **pas de dépôt APT** :
> on télécharge, on vérifie, on installe. Les anciens paquets `gtk3dialog` ont
> été retirés au renommage, et ceux de la `v1.0.0` l'ont été le 2026-08-27
> parce qu'ils portaient trois défauts — voir le `README`.
> Dernière mise à jour : 2026-08-27.

## Principe : la source est le livrable principal

haplo-dialog fournit **trois ports** : `gtk3sermo` (backend GTK 3), `gtk4sermo`
(backend GTK 4) et `qt6sermo` (backend Qt 6, construit par CMake et versionné à
part). `gtk3sermo` est le port de
référence. Le projet maintient **un** paquet binaire, le `.deb` de `haplo-dialog`,
et fournit, dans l'arbre, des **recettes** pour les autres familles de
distributions. Héberger un binaire pour chaque cible serait ingérable : le modèle
amont/aval du logiciel libre veut que l'**amont fournisse la source + des
recettes**, et que l'**aval (distributions, utilisateurs) construise** le paquet
adapté à sa cible.

### Trois niveaux

1. **Source (tous publics).** `git clone` + build autotools. Le port embarque ses
   recettes d'empaquetage ; ce qu'elles valent est dit dans la matrice, colonne
   par colonne.
2. **Le `.deb` se construit depuis l'arbre**, en une commande
   (`dpkg-buildpackage -us -uc -b`), et donne `gtk3sermo`, `gtksermo` et
   `gtk4sermo`. Les binaires obtenus sont publiés dans la release v1.1.4, avec leurs sommes.
   `qt6sermo` a lui aussi sa recette `debian/`, mais son `.deb` n'a **pas encore
   été construit** ni publié.
3. **Les autres cibles : recettes fournies, build à la demande.** Le projet
   n'héberge pas ces binaires ; on les construit si un besoin réel émerge.

## Ce que le projet fournit

| Cible | Fourni ? | Où |
|-------|:--:|-----|
| Source (les trois ports) | ✅ | dépôt Git |
| `.deb` (`gtk3sermo`, `gtksermo`, `gtk4sermo`) | ✅ | recette dans l'arbre, **binaires publiés** dans la release, avec leurs sommes |
| `.deb` (`qt6sermo`) | ⏳ | recette dans l'arbre ; **pas encore construit ni publié** |
| Paquets pour les autres distributions | ❌ | recettes dans l'arbre ; build par l'aval |

## Recettes disponibles, et ce qu'elles valent

Une recette fournie n'est pas un paquet testé. Le tableau dit lequel des deux,
pour éviter la lecture optimiste d'une colonne de coches identiques.

`gtk3sermo` fournit, dans `gtk3sermo/gtk3sermo_1.1.4/packaging/` :

| Cible | Recette fournie | Construite et vérifiée par le projet |
|---|:--:|---|
| Debian et dérivées | ✅ | ✅ construite et vérifiée à chaque version : `dpkg-buildpackage`, lintian sans erreur ni avertissement. Le `.deb` obtenu n'est pas hébergé par le projet |
| Fedora, openSUSE (RPM) | ✅ | ❌ jamais construite par nos soins |
| Arch (PKGBUILD) | ✅ | ❌ jamais construite par nos soins |
| Gentoo (ebuild) | ✅ | ❌ jamais construite par nos soins |
| Slackware (SlackBuild) | ✅ | ❌ jamais construite par nos soins |

Les quatre dernières sont un **point de départ pour un empaqueteur**, pas une
garantie : le projet ne les fait tourner sur aucune de ces distributions. Leur
URL de source, elle, est bonne — l'archive du tag répond **HTTP 200** (vérifié le
2026-08-27). Ce qui reste non vérifié, ce sont les noms de dépendances propres à
chaque distribution et le déroulé de la recette elle-même.

Si vous en faites tourner une, dites-le nous : elle passera dans la colonne de
droite, avec la version et la distribution où elle a marché.

```
gtk3sermo/gtk3sermo_1.1.4/packaging/
├── debian/      # debian/ (control, rules, *.install, *.links…) → dpkg-buildpackage
├── rpm/         # *.spec                                        → rpmbuild
├── arch/        # PKGBUILD (+ .SRCINFO)                         → makepkg
├── gentoo/      # *.ebuild                                      → ebuild / emerge
└── slackware/   # *.SlackBuild (+ doinst.sh)                    → SlackBuild
```

`qt6sermo` fournit les **mêmes cinq formats** sous
`qt6sermo/qt6sermo_1.0.1/packaging/`, calqués sur CMake. Aucun n'a été construit
ni vérifié par nos soins, `debian/` compris : la colonne de droite du tableau
ci-dessus ne vaut donc **pas** pour lui.

## Construire un paquet pour votre distribution

```sh
# Debian/Ubuntu (depuis gtk3sermo/gtk3sermo_1.1.4/)
dpkg-buildpackage -us -uc -b        # utilise packaging/debian/

# Fedora/openSUSE
rpmbuild -bb packaging/rpm/gtk3sermo.spec

# Arch
( cd packaging/arch && makepkg -si )

# Gentoo  : copier packaging/gentoo/*.ebuild dans un overlay, puis `ebuild … merge`
# Slackware : sh packaging/slackware/gtk3sermo.SlackBuild
```

## Compatibilité ascendante

L'alias **`gtkdialog`** est fourni par un paquet SÉPARÉ, **`gtksermo`** (binaire
et lien de page de manuel), volontairement en conflit avec `gtkdialog` et
`gtk3dialog`. Le paquet `gtk3sermo` seul ne l'installe PAS. Les scripts d'époque
fonctionnent sans portage dès que `gtksermo` est installé — ou en appelant
directement `gtk3sermo`. Depuis les sources, `make install` pose bien le lien.

## Build depuis les sources (sans paquet)

```sh
# autotools - gtk3sermo
cd gtk3sermo/gtk3sermo_1.1.4 && autoreconf -fi && ./configure && make -j"$(nproc)" && sudo make install

# CMake - qt6sermo
cd qt6sermo/qt6sermo_1.0.1 && cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j"$(nproc)" && sudo cmake --install build
```

## Domaine du projet

- **haplo-dialog.fr** : site, documentation et source du projet.

---

*haplo-dialog, GPL-2.0-or-later*
