# Boîtes à outils

[English](README.en.md)

Quatre boîtes de réglages et d'outils, chacune avec un **lanceur** et **dix outils**.
Le lanceur présente les outils dans une seule fenêtre ; un clic en ouvre un, puis on
revient au lanceur.

Comme les [scripts métier](../metiers/README.md), elles servent aussi de **test de
réussite des ports** : `examples/metiers/verifie.sh` joue chacun de leurs dialogues
sur les neuf ports et compare les valeurs à gtk3sermo.

## Les quatre boîtes

| Dossier | Pour qui | Port conseillé |
|---|---|---|
| `systeme` | la personne devant son poste, qui le règle | gtk3sermo, qt6sermo |
| `serveur` | la personne qui tient un serveur, souvent par SSH | ncursessermo |
| `administrateur` | la personne qui administre une machine Debian | gtk3sermo ; ncursessermo par SSH |
| `utilisateur` | tout le monde, sans connaissances techniques | gtk3sermo, qt6sermo, htmlsermo |

**Système** : écran, son, Wi-Fi, date et heure, clavier et langue, souris et pavé
tactile, énergie, applications au démarrage, applications par défaut, Bluetooth.

**Serveur** : site nginx, certificats TLS, ports à l'écoute, durcissement SSH,
fail2ban, base de données, rotation des journaux, partages NFS et Samba, alertes
par courriel, minuteries systemd.

**Administrateur** : groupes, droits sur un dossier, paquets, dépôts apt, montages
(fstab), formatage, modules du noyau, règles sudo, nom de la machine, redémarrage
programmé.

**Utilisateur** : rechercher des fichiers, archives, mot de passe, rappel, imprimer,
convertir un document en PDF, ranger un dossier, copier sur une clé USB, prêter un
fichier sur le réseau local, place sur le disque.

## Les lancer sans risque

```sh
examples/boites-a-outils/utilisateur/lanceur.sh gtk3sermo
```

Le premier argument est le port (défaut : `sermo`). **Par défaut, rien n'est
modifié** : la commande que l'outil lancerait est affichée (`à blanc : …`). Pour agir
pour de vrai, après avoir lu cette commande :

```sh
SERMO_A_BLANC=0 examples/boites-a-outils/utilisateur/lanceur.sh gtk3sermo
```

- Une action destructive demande une confirmation dans un second dialogue (pour un
  formatage, il faut retaper le nom du disque).
- Aucun outil n'appelle `sudo` ni `pkexec`. Quand il faut les droits root, l'outil le
  dit et affiche la commande à lancer en root.
- Les saisies sont vérifiées avant d'agir : listes de valeurs permises, chemins
  normalisés, dossiers système refusés.

## Comment elles sont écrites

Les règles sont celles des scripts métier : [CONVENTION.md](../metiers/CONVENTION.md).
Domaine public (CC0-1.0) : copiez-les et adaptez-les.

Ces dialogues sont aussi servis par `sermoia-mcp` (`sermo_examples`), sous un sujet
par boîte.
