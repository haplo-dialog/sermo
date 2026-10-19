<!-- SPDX-License-Identifier: CC0-1.0 -->
# dns-testeur — résoudre un nom, comparer des résolveurs, en une fenêtre sermo

Une fenêtre pour les questions DNS courantes : *ce nom existe-t-il ? que répond mon résolveur ? un autre
dit-il pareil ?*

- **Question** : un nom, un type (`A AAAA MX TXT NS CNAME SOA CAA SRV PTR`), un délai.
- **Résolveurs** : jusqu'à 6, séparés par des espaces. `système` (celui de la machine), une adresse IPv4 ou
  IPv6, ou `adresse#port`.
- **Tableau** : pour chaque résolveur, le statut (`NOERROR`, `NXDOMAIN`, `SERVFAIL`, ou pas de réponse),
  DNSSEC (`validé` quand le résolveur pose le drapeau AD), le nombre de réponses, le temps, et **si sa réponse
  est la même que celle du premier**. Une différence se voit tout de suite : résolveur filtrant, menteur, ou
  simplement plusieurs adresses servies tour à tour.
- **Détail** : les enregistrements reçus, par résolveur.

```sh
./dns-testeur.sh                 # binaire « sermo » par défaut
./dns-testeur.sh --interroger debian.org AAAA "système 9.9.9.9 1.1.1.1" 3
./dns-testeur.sh --xml           # affiche le XML du dialogue, sans l'ouvrir
```

`--interroger NOM TYPE "RÉSOLVEURS" [DÉLAI]` : délai en secondes, 3 par défaut. `DT_DIR` : dossier de
session (état, résultats, détail). Sans lui, le script en crée un dans `$XDG_RUNTIME_DIR` (ou `$TMPDIR`, ou
`/tmp`) et l'efface en sortant.

Codes de sortie : `0` fait · `2` `dig` absent (ou `awk`, `iconv`, `md5sum`, `timeout`), ou sermo introuvable ·
`64` entrée invalide ou option inconnue.

## Installation

```sh
sudo apt install bind9-dnsutils
```

## Confidentialité

Interroger un résolveur public (9.9.9.9, 1.1.1.1…) **lui apprend le nom demandé**. Par défaut, seul le
résolveur de la machine est interrogé ; en ajouter un autre est un choix, écrit dans le champ.

## Ce que l'exemple garantit (testé par `tests/garde_dns_testeur.sh`)

- **Entrées vérifiées avant tout usage** :
  - nom : étiquettes DNS, 253 caractères au plus, jamais commençant par `-` ;
  - type : liste fermée ;
  - résolveurs : `système`, une IP ou `IP#port`, 6 au plus ;
  - délai : de 1 à 30 s.

  Toute entrée refusée donne le code 64, et **`dig` n'est pas lancé**.
- **Arguments séparés** : le nom passe par `-q`, le type par `-t`, jamais dans une chaîne de commande.
- **Les réponses sont des données.** Seules les lignes au format d'un enregistrement sont gardées (50 au plus par résolveur, 400 caractères chacune), sans caractères de contrôle, en UTF-8 valide, et jamais insérées dans le XML.
- **La comparaison ignore l'ordre et le TTL** : deux résolveurs qui servent les mêmes adresses dans un autre ordre donnent « oui ».

## Limites

- `validé` veut dire que **le résolveur** dit avoir validé DNSSEC (drapeau AD) : `dig` ne refait pas la validation lui-même.
- Pas de DNS chiffré (DoT, DoH), pas de transfert de zone, pas de requête récursive pas à pas (`+trace`).
- La fenêtre attend la fin des requêtes, une par résolveur : au plus 6 × (délai + 2 s).
