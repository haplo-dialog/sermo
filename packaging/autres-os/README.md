# sermo — recettes hors Debian

Six recettes pour les neuf ports modulaires (`gtk3 gtk4 qt6 fltk1 efl1 sdl3 lvgl
ncurses html`), plus le cœur (`sermo-core-dev`) et le manuel (`sermo-doc`).
Elles remplacent l'ancien kit `sermo-backend-qt6/packaging/`, écrit pour un
`qt6sermo 1.0.0` autonome qui n'existe plus (retiré ; l'historique git le garde).

| Système | Fichier | Paquets produits | Commande `sermo` |
|---|---|---|---|
| Arch Linux | `arch/PKGBUILD` | 11 (paquets scindés d'un seul PKGBUILD) | non : `<port>sermo` |
| Fedora / RPM | `rpm/sermo.spec` | 11 sous-paquets | oui : `alternatives` |
| Gentoo | `gentoo/dev-util/sermo/sermo-2.8.1.ebuild` | 1, un drapeau USE par port | non : `<port>sermo` |
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

Aucun de ces six systèmes n'était disponible à l'écriture.

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

**Non vérifié** — à faire sur le système cible :
- les noms de paquets de dépendances (`vte3`, `vte291-gtk4-devel`,
  `gui-libs/vte:2.91-gtk4`, `vte3-gtk4`, `sdl3`, `fltk14`…) ;
- `rpmbuild`, `makepkg`, `ebuild … manifest`, `abuild`, `nix-build` eux-mêmes ;
- le comportement de `abuild` sur les pages de manuel hors paquet `-doc` ;
- le build sous musl (Alpine) ;
- les sommes de contrôle (`SKIP`) : elles ne se posent qu'à la publication de
  l'étiquette — geste de S. Cage.
