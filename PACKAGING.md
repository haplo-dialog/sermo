# Politique d'empaquetage - haplo-dialog

> Comment haplo-dialog est distribué : la source est le livrable principal, un
> `.deb` construit et vérifié accompagne chaque release, et des recettes non
> construites par nos soins sont fournies pour les autres familles de
> distributions. Dernière mise à jour : 2026-08-17.

## Principe : la source est le livrable principal

haplo-dialog fournit un **port unique**, `gtk3sermo` (backend GTK 3), le port de
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
2. **Un binaire fourni par le projet : le `.deb` de `haplo-dialog`.** Il est attaché
   à chaque version publiée (release du projet).
3. **Les autres cibles : recettes fournies, build à la demande.** Le projet
   n'héberge pas ces binaires ; on les construit si un besoin réel émerge.

## Ce que le projet fournit

| Cible | Fourni ? | Où |
|-------|:--:|-----|
| Source (gtk3sermo) | ✅ | dépôt Git + releases du projet |
| `.deb` haplo-dialog | ✅ | attaché à chaque release |
| Paquets pour les autres distributions | ❌ | recettes dans l'arbre ; build par l'aval |

## Recettes disponibles, et ce qu'elles valent

Une recette fournie n'est pas un paquet testé. Le tableau dit lequel des deux,
pour éviter la lecture optimiste d'une colonne de coches identiques.

`gtk3sermo` fournit, dans `gtk3sermo/gtk3sermo_1.0.0/packaging/` :

| Cible | Recette fournie | Construite et vérifiée par le projet |
|---|:--:|---|
| Debian et dérivées | ✅ | ✅ à chaque version : `dpkg-buildpackage`, lintian sans erreur ni avertissement, `.deb` joint à la release |
| Fedora, openSUSE (RPM) | ✅ | ❌ jamais construite par nos soins |
| Arch (PKGBUILD) | ✅ | ❌ jamais construite par nos soins |
| Gentoo (ebuild) | ✅ | ❌ jamais construite par nos soins |
| Slackware (SlackBuild) | ✅ | ❌ jamais construite par nos soins |

Les quatre dernières sont un **point de départ pour un empaqueteur**, pas une
garantie. Si vous en faites tourner une, dites-le nous : elle passera dans la
colonne de droite, avec la version et la distribution où elle a marché.

```
gtk3sermo/gtk3sermo_1.0.0/packaging/
├── debian/      # debian/ (control, rules, *.install, *.links…) → dpkg-buildpackage
├── rpm/         # *.spec                                        → rpmbuild
├── arch/        # PKGBUILD (+ .SRCINFO)                         → makepkg
├── gentoo/      # *.ebuild                                      → ebuild / emerge
└── slackware/   # *.SlackBuild (+ doinst.sh)                    → SlackBuild
```

## Construire un paquet pour votre distribution

```sh
# Debian/Ubuntu (depuis gtk3sermo/gtk3sermo_1.0.0/)
dpkg-buildpackage -us -uc -b        # utilise packaging/debian/

# Fedora/openSUSE
rpmbuild -bb packaging/rpm/gtk3sermo.spec

# Arch
( cd packaging/arch && makepkg -si )

# Gentoo  : copier packaging/gentoo/*.ebuild dans un overlay, puis `ebuild … merge`
# Slackware : sh packaging/slackware/gtk3sermo.SlackBuild
```

## Compatibilité ascendante

`gtk3sermo` installe un alias **`gtkdialog`** (binaire + page de manuel) : les
scripts gtkdialog d'époque fonctionnent sans portage.

## Build depuis les sources (sans paquet)

```sh
# autotools - gtk3sermo
cd gtk3sermo/gtk3sermo_1.0.0 && autoreconf -fi && ./configure && make -j"$(nproc)" && sudo make install
```

## Domaine du projet

- **haplo-dialog.fr** : site, documentation et source du projet.

---

*haplo-dialog, GPL-2.0-or-later*
