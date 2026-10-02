# Empaquetage — sermo

[English](PACKAGING.en.md)

sermo produit des paquets Debian **séparés** : un cœur de développement commun et
un backend par toolkit. Le dossier `debian/` (debhelper) les décrit ; le script
`packaging/construire-paquets.sh` les construit et les éprouve dans une Debian
testing vierge.

## Ce qui est produit

| Paquet | Contenu | Dépend de |
|---|---|---|
| `sermo-doc` | manuel Texinfo (`info sermo-doc`) et documentation commune : copyright, changelog Debian | aucune dépendance |
| `sermo-core-dev` | `libsermocore.a`, en-têtes (`/usr/include/sermo`), `sermocore.pc` | aucune dépendance |
| `sermo-backend-gtk3` | binaire `gtk3sermo`, page de manuel, manuel Texinfo | GTK 3, VTE (`<terminal>`), gtk-layer-shell (ancrage Wayland) |
| `sermo-backend-gtk4` | binaire `gtk4sermo`, page de manuel, manuel Texinfo | GTK 4, VTE (`<terminal>`) |
| `sermo-backend-qt6` | binaire `qt6sermo`, page de manuel, manuel Texinfo, entrée de menu | Qt 6 |
| `sermo-backend-fltk1` | binaire `fltk1sermo`, page de manuel, manuel Texinfo | FLTK 1.4 |
| `sermo-backend-efl1` | binaire `efl1sermo`, page de manuel, manuel Texinfo | Enlightenment/Elementary |
| `sermo-backend-sdl3` | binaire `sdl3sermo`, page de manuel, manuel Texinfo | SDL 3 |
| `sermo-backend-ncurses` | binaire `ncursessermo`, page de manuel, manuel Texinfo | ncursesw (terminal) |
| `sermo-gtkdialog` | commande `gtkdialog` et page `gtkdialog(1)`, liens vers gtk3sermo | `sermo-backend-gtk3` |
| `gtk3sermo`, `gtk4sermo`, `qt6sermo`, `gtksermo` | paquets de transition, vides | le paquet 2.x qui les remplace, et `sermo-doc` |

Soit **14 paquets**, plus un paquet `-dbgsym` (symboles de débogage) par backend.
Les dépendances sont calculées par `dh_shlibdeps` à partir des bibliothèques
réellement liées : elles ne sont ni devinées ni recopiées à la main.

### Ce qui n'est PAS dans les paquets

**Le changelog amont.** `debhelper` l'installait dans chacun des treize
paquets : `CHANGELOG.md` pèse 51 ko en clair, 19,8 ko gzippé — et étant déjà
compressé, il traversait la compression du `.deb` sans maigrir, là où le
`copyright` (27,9 ko de texte) se tasse bien. Soit **251 ko sur l'ensemble,
13 % du poids livré**, pour un fichier identique treize fois. Il reste dans le
dépôt et sur le site.

**Une copie de la documentation commune, pour les paquets indépendants de
l'architecture.** Les cinq (`sermo-gtkdialog` et les quatre transitions)
rattachent leur `/usr/share/doc` à celui de `sermo-doc` par un lien
symbolique. Un paquet de transition ne contenait rien d'autre que ça :
`gtk3sermo` est passé de 32 ko à **1,2 ko**.

⚠️ Les huit paquets dépendants de l'architecture **gardent leur `copyright`**,
comme la politique l'exige : `--link-doc` ne franchit pas la frontière
« all » / « any », et `debhelper` refuse activement la combinaison depuis le
niveau de compatibilité 10 (CAVEAT 2 de `dh_installdocs`).

Mesuré le 2026-09-20 sur des paquets construits en Debian vierge :
**2 011 ko → 1 700 ko, soit −15 %**, lintian 0 erreur 0 avertissement.

## Pourquoi les backends ne dépendent PAS du cœur (lien statique)

Question légitime : un `sermo-backend-fltk1` ne devrait-il pas `Depends:
sermo-core-dev` pour tirer le moteur C ? **Non — et c'est voulu.**

Le cœur `libsermocore` est une bibliothèque **statique** (`.a`), **liée dans
chaque binaire backend à la compilation**. `ldd fltk1sermo` ne montre donc
**aucun** `libsermocore` : le moteur C est **embarqué** dans le binaire.
Conséquences :

- **Aucune dépendance runtime au cœur.** L'utilisateur qui installe
  `sermo-backend-fltk1` obtient un binaire **autonome** (le moteur est dedans).
  Ajouter `Depends: sermo-core-dev` serait une **erreur** : ça imposerait aux
  utilisateurs des fichiers de **compilation** (`.a` + en-têtes) inutiles à
  l'exécution.
- **`sermo-core-dev` est un paquet de DÉVELOPPEMENT uniquement** : il ne contient
  que `libsermocore.a` + en-têtes + `sermocore.pc` (aucun `.so`). Il ne sert
  **qu'à compiler un nouveau backend**. La bibliothèque et `sermocore.pc` vivent
  dans le répertoire multiarch (`/usr/lib/x86_64-linux-gnu/…`).
- **Pas besoin d'un paquet runtime épuré** (`sermo-core.deb`) : le « runtime »
  d'un utilisateur non-développeur, c'est le paquet **backend lui-même**,
  autonome. On installe `sermo-backend-<toolkit>` (+ éventuellement
  `sermo-gtkdialog`), rien d'autre.

**Compromis assumé** : chaque backend embarque sa copie du cœur (environ 170 Ko
de code). Le modèle Debian idiomatique — un `.so` partagé `libsermocore1` (runtime)
+ `libsermocore-dev` (en-têtes), les backends dépendant du runtime — n'est PAS
possible aujourd'hui : le cœur rappelle des symboles du backend
(`widget_*_create`, ponts `sermo_be_*`), donc une `.so` autonome aurait des symboles
non résolus. C'est la cible de la **Part 3** (IR + inversion de la
boucle d'évènements) ; elle rendra ce découpage `.so` + runtime possible et
supprimera la duplication.

Trois variantes du cœur servent à la construction : neutre (qt6, fltk1, efl1,
sdl3, ncurses), GLib (gtk3) et GTK 4 (gtk4). Seule la variante neutre est livrée
dans `sermo-core-dev`.

## Alias `sermo`

Chaque backend s'enregistre dans `update-alternatives` pour `/usr/bin/sermo`
(`debian/sermo-backend-<t>.alternatives`, posé par `dh_installalternatives`).
Priorités : gtk3=50, gtk4=45, qt6=40, fltk1=30, efl1=20, sdl3=10, ncurses=5 ; le
backend GTK 3 est donc le défaut quand plusieurs sont installés. La page
`sermo(1)` et le manuel Texinfo `sermo` suivent le même choix.

```sh
update-alternatives --display sermo          # qui répond à « sermo »
sudo update-alternatives --config sermo      # en choisir un autre
```

## Compatibilité gtkdialog — un paquet séparé

La **commande** `gtkdialog` (pour les scripts historiques) vit dans un paquet
**à part**, `sermo-gtkdialog` :

- il ne contient que les liens `/usr/bin/gtkdialog → gtk3sermo` et
  `gtkdialog.1.gz → gtk3sermo.1.gz` ;
- `Depends: sermo-backend-gtk3` (il a besoin du vrai binaire) ;
- `Provides: gtkdialog`, `Conflicts: gtk3dialog, gtkdialog, haplo-dialog`,
  `Replaces: gtk3dialog, gtkdialog, haplo-dialog`.

**Pourquoi séparé** : d'autres projets possèdent aussi `/usr/bin/gtkdialog` — le
`gtkdialog` original et surtout le **`gtk3dialog` de BunsenLabs**. Fournir la
commande depuis le backend lui-même entrerait en **conflit de fichier** avec eux.
En paquet séparé et mutuellement exclusif (`Conflicts`/`Replaces`), l'utilisateur
installe `sermo-gtkdialog` **ou** l'un des autres, jamais les deux — sans casser
l'installation du backend GTK 3 pour ceux qui ont déjà un `gtkdialog`.

## Passage depuis la 1.x

La 1.x était livrée sous les noms `gtk3sermo`, `gtk4sermo`, `qt6sermo` et
`gtksermo`, en fichiers `.deb` joints à la publication. Installer les paquets 2.x
voulus depuis leurs fichiers retire les paquets 1.x qu'ils remplacent :

```sh
sudo apt install ./sermo-backend-gtk3_2.7.5-1_amd64.deb ./sermo-gtkdialog_2.7.5-1_all.deb
```

Éprouvé le 2026-09-17 dans un conteneur Debian testing vierge, sans réseau, depuis
`gtk3sermo`, `gtk4sermo`, `gtksermo` 1.1.4-2 et `qt6sermo` 1.0.2-2 (hors de
`construire-paquets.sh`). Le guide complet est [MIGRATION.md](MIGRATION.md).

Pour qui suit un dépôt apt, ces noms sont aussi des **paquets de transition** :
vides, ils dépendent du paquet qui les remplace (`sermo-backend-gtk3`,
`sermo-backend-gtk4`, `sermo-backend-qt6`, `sermo-gtkdialog`). Les nouveaux
paquets déclarent `Breaks` et `Replaces` sur les anciens `(<< 2.0~)`. Un simple
`apt upgrade` fait donc venir les backends 2.x.

Les paquets de transition peuvent ensuite partir. **D'abord** marquer comme voulus
les paquets 2.x qu'on utilise : venus comme dépendances, ils sont notés
« installés automatiquement », et `apt autoremove` les retirerait avec les paquets
de transition.

```sh
sudo apt install sermo-backend-gtk3 sermo-gtkdialog      # ceux qu'on utilise
sudo apt purge gtk3sermo gtk4sermo qt6sermo gtksermo
```

## Construire les paquets

### Dans une Debian testing vierge (méthode de référence)

```sh
bash packaging/construire-paquets.sh
SERMO_PAQUETS_1X=/chemin/des/paquets-1.x bash packaging/construire-paquets.sh   # + passage depuis la 1.x
```

Le script archive le **commit courant** (`git archive` : rien de non commité
n'entre dans les paquets), puis, dans des conteneurs `debian:testing` jetables :

1. installe les dépendances de construction et construit (`dpkg-buildpackage`)
   en utilisateur ordinaire ;
2. passe `lintian` (aucune erreur admise ; les exceptions sont justifiées dans
   `debian/*.lintian-overrides`) ;
3. installe tous les paquets dans un système vierge : chaque binaire trouve ses
   bibliothèques et répond à `--version`, l'alternative `sermo` et la commande
   `gtkdialog` mènent à gtk3sermo, un dialogue tourne en terminal ; puis purge,
   et vérifie qu'il ne reste ni fichier ni alternative ;
4. si `SERMO_PAQUETS_1X` est posé : installe la 1.x, sert les paquets 2.x par un
   dépôt apt local, vérifie que `apt upgrade` prévoit la transition, la joue, puis
   retire les paquets de transition comme indiqué plus haut et vérifie que les
   backends 2.x restent.

Résultats et journaux dans `packaging/sortie/`. Les conteneurs téléchargent depuis
les miroirs Debian (de l'ordre de 550 Mo la première fois) ; le cache
`packaging/sortie/cache-apt` et le cache apt du poste réduisent les fois
suivantes. Il faut Docker.

### Rejouer les bancs sur les binaires des paquets

Le binaire d'un paquet n'est pas celui de la CI : les drapeaux de
`dpkg-buildflags` s'y ajoutent, les symboles de débogage partent dans le paquet
`-dbgsym`. C'est lui qui est livré, c'est donc lui qu'on mesure :

```sh
SERMO_TASKSET=0,1 bash packaging/bancs-sur-paquets.sh packaging/sortie/paquets <commit>
```

Le script clone le commit, construit l'arbre (les bancs du cœur en ont besoin),
remplace les sept binaires de l'arbre par ceux extraits des `.deb` (sommes SHA-256
comparées), vérifie que chaque binaire trouve ses symboles dans son paquet
`-dbgsym`, puis joue `ci/bancs.sh` tel quel. `SERMO_TASKSET` limite la mesure à des
cœurs donnés, comme une CI partagée : n'y lancer rien d'autre pendant ce temps, les
bancs graphiques attendent leurs fenêtres un temps borné (10 s pour les exemples).

### Directement, sur une Debian testing

```sh
sudo apt build-dep ./
dpkg-buildpackage -us -uc -b
```

`debian/rules` construit les trois variantes du cœur, les sept backends, et
rejoue pendant la construction les tests unitaires et le banc XML sur chaque
binaire (`DEB_BUILD_OPTIONS=nocheck` les saute). Les paquets arrivent dans le
dossier parent.

### Licences : `debian/copyright`

`debian/copyright` (format DEP-5) est **produit** par
`packaging/copyright-dep5.py` à partir des fichiers suivis par git : chaque liste
de fichiers est exacte. Le relancer quand des fichiers entrent ou sortent (cas de
test CC0, sources reprises de l'amont…). La provenance, port par port, est
détaillée dans [LICENCES.md](LICENCES.md).

## Installer et désinstaller

```sh
sudo apt install ./sermo-backend-gtk3_2.7.5-1_amd64.deb      # GTK 3
sudo apt install ./sermo-gtkdialog_2.7.5-1_all.deb           # + la commande gtkdialog
sudo apt install ./sermo-core-dev_2.7.5-1_amd64.deb          # pour compiler un backend

sudo apt purge sermo-gtkdialog sermo-backend-gtk3            # tout retirer
```

Il n'y a pas de dépôt apt : les paquets sont joints à chaque publication, avec
leurs sommes de contrôle (voir le [README](README.md#installer)).
