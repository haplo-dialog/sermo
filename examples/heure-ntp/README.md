<!-- SPDX-License-Identifier: CC0-1.0 -->
# heure-ntp — l'horloge est-elle juste ? En une fenêtre sermo

Une fenêtre, rafraîchie toutes les 2 s, qui dit si l'heure de la machine est fiable. Une horloge fausse casse
TLS (certificats « pas encore valides »), Kerberos, la lecture des journaux et les sauvegardes.

| Élément | Source |
|---|---|
| Heure locale, heure UTC | `date` |
| Fuseau, horloge matérielle, NTP activé, synchronisée | `timedatectl show` |
| Serveur, strate, écart mesuré, délai, gigue, intervalle | `timedatectl timesync-status` (systemd-timesyncd) |
| **Bilan** | OK, ou **ATTENTION** : NTP désactivé, non synchronisé, ou écart au-delà de `HN_ECART_MAX` ms (500 par défaut) |

```sh
./heure-ntp.sh                 # binaire « sermo » par défaut
./heure-ntp.sh --etat          # sans fenêtre : élément|valeur, puis le bilan
./heure-ntp.sh --xml           # affiche le XML du dialogue, sans l'ouvrir
```

Codes de sortie : `0` fait · `2` `timedatectl` absent, ou sermo introuvable · `64` option inconnue, ou
`HN_ECART_MAX` invalide (avec `--etat` : un nombre de 1 à 6 chiffres).

## Lecture seule

L'exemple **ne règle rien**. Pour activer NTP ou changer de fuseau : `timedatectl set-ntp true`,
`timedatectl set-timezone Europe/Paris`. Ces commandes demandent l'autorisation d'un administrateur, et
c'est voulu.

## Ce que l'exemple garantit (testé par `tests/garde_heure_ntp.sh`)

- **Chaque valeur lue est contrôlée** (fuseau, date, adresse, nom du serveur, strate, durées) : une valeur qui n'a pas la forme attendue s'affiche « ? » ou « — », **jamais telle quelle**. Le tableau garde ses deux colonnes même si une réponse contient `|`.
- Le bilan suit l'ordre de gravité : NTP désactivé, puis non synchronisé, puis écart trop grand.
- Délai de 3 s par appel à `timedatectl`.

## Limites

- Les détails (serveur, écart…) viennent de **systemd-timesyncd**. Avec `chrony` ou `ntpsec`, seuls « activé » et « synchronisé » s'affichent, et le bilan le dit (`chronyc tracking` pour chrony).
- L'écart est celui que mesure le démon local : l'exemple n'interroge aucun serveur lui-même.
