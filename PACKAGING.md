# Politique d'empaquetage - haplo-dialog

> Comment haplo-dialog est distribué : la source est le livrable principal, un
> `.deb` accompagne chaque release, et des recettes prêtes à l'emploi couvrent
> les autres familles de distributions. Dernière mise à jour : 2026-07-23.

## Principe : la source est le livrable principal

haplo-dialog fournit un **port unique**, `gtk3dialog` (backend GTK 3), le port de
référence. Le projet maintient **un** paquet binaire, le `.deb` de `gtk3dialog` —
et fournit, dans l'arbre, des **recettes** pour les autres familles de
distributions. Héberger un binaire pour chaque cible serait ingérable : le modèle
amont/aval du logiciel libre veut que l'**amont fournisse la source + des
recettes**, et que l'**aval (distributions, utilisateurs) construise** le paquet
adapté à sa cible.

### Trois niveaux

1. **Source (tous publics).** `git clone` + build autotools. Le port embarque ses
   recettes d'empaquetage prêtes à l'emploi, voir la matrice.
2. **Un binaire fourni par le projet : le `.deb` de `gtk3dialog`.** Il est attaché
   à chaque version publiée (release du projet).
3. **Les autres cibles : recettes fournies, build à la demande.** Le projet
   n'héberge pas ces binaires ; on les construit si un besoin réel émerge.

## Ce que le projet fournit

| Cible | Fourni ? | Où |
|-------|:--:|-----|
| Source (gtk3dialog) | ✅ | dépôt Git + releases du projet |
| `.deb` gtk3dialog | ✅ | attaché à chaque release |
| Paquets pour les autres distributions | ❌ | recettes dans l'arbre ; build par l'aval |

## Recettes disponibles (matrice)

`gtk3dialog` fournit, dans `gtk3dialog/gtk3dialog_1.0.0/packaging/` :

| Port | Debian | RPM | Arch | Gentoo | Slackware |
|------|:--:|:--:|:--:|:--:|:--:|
| gtk3dialog | ✅ | ✅ | ✅ | ✅ | ✅ |

```
gtk3dialog/gtk3dialog_1.0.0/packaging/
├── debian/      # debian/ (control, rules, *.install, *.links…) → dpkg-buildpackage
├── rpm/         # *.spec                                        → rpmbuild
├── arch/        # PKGBUILD (+ .SRCINFO)                         → makepkg
├── gentoo/      # *.ebuild                                      → ebuild / emerge
└── slackware/   # *.SlackBuild (+ doinst.sh)                    → SlackBuild
```

## Construire un paquet pour votre distribution

```sh
# Debian/Ubuntu (depuis gtk3dialog/gtk3dialog_1.0.0/)
dpkg-buildpackage -us -uc -b        # utilise packaging/debian/

# Fedora/openSUSE
rpmbuild -bb packaging/rpm/gtk3dialog.spec

# Arch
( cd packaging/arch && makepkg -si )

# Gentoo  : copier packaging/gentoo/*.ebuild dans un overlay, puis `ebuild … merge`
# Slackware : sh packaging/slackware/gtk3dialog.SlackBuild
```

## Compatibilité ascendante

`gtk3dialog` installe un alias **`gtkdialog`** (binaire + page de manuel) : les
scripts gtkdialog d'époque fonctionnent sans portage.

## Build depuis les sources (sans paquet)

```sh
# autotools - gtk3dialog
cd gtk3dialog/gtk3dialog_1.0.0 && autoreconf -fi && ./configure && make -j"$(nproc)" && sudo make install
```

## Domaine du projet

- **haplo-dialog.fr** : site, documentation et source du projet.

---

*haplo-dialog, GPL-2.0-or-later*
