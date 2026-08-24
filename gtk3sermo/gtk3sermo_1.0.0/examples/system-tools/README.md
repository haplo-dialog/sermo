<div align="right"><sub><code>maj :     2026-08-18 12:40:07 · par claude</code></sub></div>
<div align="right"><sub><code>réindex : —</code></sub></div>

# Outils Système, l'exemple qui va au bout

Les cinq scripts de `../showcase/` montrent les widgets un par un. Celui-ci
montre ce qu'on obtient quand on les assemble : un outil d'administration à six
onglets, qui affiche de vraies données et fait un vrai travail.

```sh
./system-tools.sh
```

## Debian seulement, et c'est dit au lancement

Cet exemple appelle `apt`, `dpkg`, `journalctl` et `deborphan`. Ailleurs, il
afficherait des onglets vides. Il vérifie donc `/etc/debian_version` au
démarrage et refuse proprement, avec le nom du système détecté, plutôt que de
faire semblant.

Le bureau, lui, n'a pas d'importance : le terminal et l'éditeur sont détectés
parmi ceux installés (`x-terminal-emulator`, `xfce4-terminal`, `gnome-terminal`,
`konsole`… et `mousepad`, `gedit`, `kate`, `pluma`…).

## Ce que l'exemple enseigne

**Le motif `--do`.** C'est le point important. Une `<action>` ne peut pas
appeler une fonction bash du script parent : gtk3sermo lance un sous-shell.
La solution est que le script se rappelle lui-même :

```xml
<action>bash "$SCRIPT_PATH" --do terminal</action>
```

Les valeurs des widgets arrivent alors par l'environnement, sans fichier
intermédiaire ni `export -f`. C'est la façon propre d'écrire un programme
gtk3sermo qui dépasse la boîte de dialogue.

**Des données réelles.** L'en-tête relève la distribution, le noyau, l'uptime,
le processeur, l'interface réseau active, la mémoire et le disque au lancement.
La barre de progression est liée au pourcentage de mémoire réellement utilisée.

**Une sortie de commande dans une fenêtre.** Les cadres `<edit>` sont remplis
par la sortie d'une commande, y compris par le script lui-même
(`--print noyau 20`).

## Ce qui est réellement fait côté sécurité

- **Toute élévation passe par `pkexec`** (PolicyKit). Aucun `sudo` dans une
  interface graphique, y compris pour la mise à jour des paquets.
- **L'aide en ligne** n'accepte qu'une commande figurant dans une liste
  blanche, comparée à l'identique, jamais par motif.
- **L'onglet Fichiers** ne travaille que sur une liste **fermée** de chemins.
  La liste déroulante n'est pas éditable, et la garde est **revérifiée en
  shell** avant toute ouverture, y compris en root. Écrite deux fois exprès :
  le jour où quelqu'un remplacera le widget par un champ libre, la garde
  tiendra encore.
- **Rien n'est déposé dans `/tmp` sous un nom prévisible.**
- `set -e` et `trap ERR` sont volontairement absents : gtk3sermo renvoie un
  code non nul à la fermeture normale de la fenêtre.

## Un détail que Debian vous réserve

`dmesg` est protégé par défaut (`kernel.dmesg_restrict = 1`) : pour un
utilisateur ordinaire, il échoue. L'exemple essaie `dmesg`, puis `journalctl -k`
(qui marche pour les membres du groupe `adm` ou `systemd-journal`), et à défaut
explique pourquoi le cadre est vide au lieu de le laisser vide.

## Licence

Domaine public (**CC0-1.0**) : ce script est écrit par le projet. Le reste
d'`examples/` vient de gtkdialog et suit la GPL. Copiez, découpez,
republiez sans condition et sans mention.
