# sermo — recettes hors Debian

Six recettes pour les neuf ports modulaires (`gtk3 gtk4 qt6 fltk1 efl1 sdl3 lvgl
ncurses html`), plus le cœur (`sermo-core-dev`) et le manuel (`sermo-doc`).
Elles remplacent l'ancien kit `sermo-backend-qt6/packaging/`, écrit pour un
`qt6sermo 1.0.0` autonome qui n'existe plus (retiré ; l'historique git le garde).

| Système | Fichier | Paquets produits | Commande `sermo` |
|---|---|---|---|
| Arch Linux | `arch/PKGBUILD` | 11 (paquets scindés d'un seul PKGBUILD) | non : `<port>sermo` |
| Fedora / RPM | `rpm/sermo.spec` | 11 sous-paquets | oui : `alternatives` |
| Gentoo | `gentoo/dev-util/sermo/sermo-2.8.2.ebuild` | 1, un drapeau USE par port | non : `<port>sermo` |
| Slackware | `slackware/sermo.SlackBuild` | un `.txz` par port demandé | non : `<port>sermo` |
| Alpine | `alpine/APKBUILD` | 8 sous-paquets (sans efl1 ni fltk1 : EFL absent, FLTK en 1.3), `provider_priority` | non : `<port>sermo` |
| NixOS | `nix/default.nix` | une dérivation par port + `all` | non : `<port>sermo` |

Debian (`debian/`) reste la référence : il a `update-alternatives`, le paquet de
transition `sermo-gtkdialog`, et c'est lui que `packaging/construire-paquets.sh`
éprouve dans une Debian vierge.

## Une seule construction

Les six recettes appellent le même script, `construire-os.sh` : le cœur dans ses
variantes (neutre, GLib, GTK 4), chaque port contre la bonne variante, puis
`cmake --install` — ce sont les CMakeLists des ports qui savent quels fichiers
ils livrent. C'est la suite de gestes de `debian/rules`, écrite une fois.

```sh
sh packaging/autres-os/construire-os.sh construire  . _os gtk3 ncurses html
sh packaging/autres-os/construire-os.sh installer   . _os html /tmp/dest /usr
sh packaging/autres-os/construire-os.sh coeur-dev   . _os /tmp/dest /usr lib
sh packaging/autres-os/construire-os.sh manuel      . _os /tmp/dest /usr
```

## Ce qui a été vérifié, et ce qui ne l'a pas été

**Reconstruit dans un conteneur de chaque système** (12/10/2026, sur l'arbre exact
de la 2.8.2, archive identique à celle de l'étiquette, recettes **sans aucune
correction** ; journaux gardés hors dépôt). Dans chaque conteneur, les binaires ont
été lancés : `--version` pour chaque port, et un vrai dialogue sans écran pour
`html` et `ncurses` (`NOM` et `EXIT` rendus) :
- **Arch** et **Fedora 44** : les neuf ports construits, **installés** (`pacman -U`,
  `dnf install` ; `alternatives` pointe `sermo` vers gtk3sermo) et lancés ;
- **NixOS** (nixpkgs-unstable) : les neuf ports, du premier coup ;
- **Gentoo** : les neuf ports avec le profil `default/linux/amd64/23.0/desktop`
  (environ 20 min, sans autounmask) ; avec le profil de base, la demande d'EFL
  bloque sur `media-libs/freetype[harfbuzz]` (réglage du système, pas de la
  recette) ; `html` et `ncurses` passent avec n'importe quel profil ;
- **Alpine** (edge) : les sept ports prévus (ni EFL, absent d'Alpine, ni FLTK, en
  1.3 sans `Fl_Flex`), dépôt local signé par `abuild-keygen -a -i`, paquets
  installés par `apk add` ;
- **Slackware 15.0** : `html`, `ncurses` et `coeur-dev`, construits, installés par
  `installpkg` et lancés. **Ne passent pas** : `gtk3` (gtk-layer-shell n'existe pas
  dans Slackware 15.0 ; `-DSERMO_LAYER_SHELL=OFF` dans `CMAKE_EXTRA` le contourne,
  non essayé) et `lvgl` (wayland-protocols 1.25 n'a `linux-dmabuf` qu'en
  `unstable/`). Pas de Qt 6, d'EFL ni de SDL 3 ; FLTK en 1.3.

Les essais du 10/10/2026 (version de travail 2.8.0) avaient trouvé et fait
corriger : le lien en PIE (Slackware), la glibc antérieure à 2.34 (Slackware),
musl (Alpine), FLTK 1.3 (Alpine).

Avant ces essais, à l'écriture des recettes, aucun de ces six systèmes n'était
disponible ; les rejouages ci-dessous datent de ce moment.

**Rejoué sur une Debian testing** (10/10/2026) :
- les neuf ports se construisent et s'installent par `construire-os.sh`
  (59 s) ; la liste des fichiers livrés par port est celle que déclarent les
  sections `%files` du RPM et les `amove` d'Alpine ;
- `PKGBUILD` : `build()` puis `package_sermo-core-dev`, `package_sermo-doc` et
  quatre `package_sermo-backend-*` rejoués avec un `srcdir`/`pkgdir` factices ;
- `APKBUILD` : `core_dev`, `backend_html`, `backend_gtk3` rejoués avec un
  `amove` factice qui vérifie l'existence des fichiers ;
- `sermo.SlackBuild` : `html`, `ncurses` et `coeur-dev` rejoués avec un
  `makepkg` factice ; il n'efface rien (répertoires `mktemp`) ;
- `sh -n` / `bash -n` sur toutes les recettes.

**Non vérifié, ou à reprendre** :
- Slackware : `gtk3` avec `-DSERMO_LAYER_SHELL=OFF` ; et le SlackBuild **copie**
  les pages de manuel dans `/usr/man` sans retirer `/usr/share/man` (doublon,
  vu par `makepkg` le 10/10) ;
- une installation sur un système réel, hors conteneur ;
- les sommes de contrôle (`SKIP`) : elles ne se posent qu'à la publication de
  l'étiquette — geste de S. Cage.
