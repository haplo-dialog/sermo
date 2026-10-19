<!-- SPDX-License-Identifier: CC0-1.0 -->
# bluetooth-scan — les appareils Bluetooth à portée, en une fenêtre sermo

Une fenêtre pour voir **ce qui émet en Bluetooth autour de soi** et **ce que le poste connaît déjà**, en
lecture seule :

- **Balayer 10 s** : `bluetoothctl --timeout 10 scan on`. Chaque appareil vu s'affiche avec son adresse, son
  nom (s'il en annonce un) et son signal (dBm, du plus fort au plus faible).
- **Connu du poste** : `appairé` ou `connecté`, d'après `bluetoothctl devices Paired` / `Connected`.
- Rien n'est appairé, connecté ni approuvé.

```sh
./bluetooth-scan.sh                 # binaire « sermo » par défaut
./bluetooth-scan.sh --etat          # adaptateur prêt ? (code 3 sinon)
./bluetooth-scan.sh --balayer 15 && ./bluetooth-scan.sh --liste   # avec BS_DIR posé
./bluetooth-scan.sh --xml           # affiche le XML du dialogue, sans l'ouvrir
```

- `--balayer [SECONDES]` : de 5 à 30 s (défaut 10). `--liste` rend `adresse|nom|signal|connu`.
- `BS_DIR` : dossier où vit le dernier balayage. Sans lui, chaque appel en crée un et l'efface en sortant :
  posez-le (un dossier existant) pour que `--liste` lise ce que `--balayer` a trouvé.
- `BT_SYSFS` : autre racine que `/sys/class/bluetooth`, pour les essais (le banc y met un faux adaptateur).

Codes de sortie : `0` fait · `2` `bluetoothctl` ou `python3` absent, ou sermo introuvable · `3` pas
d'adaptateur ou service arrêté · `64` usage (option inconnue, durée hors de 5 à 30 s).

## Installation

```sh
sudo apt install bluez
sudo systemctl enable --now bluetooth
```

## Pas d'adaptateur, pas d'attente

Sans adaptateur (ou sans le service `bluetooth`), `bluetoothctl` **attend sans fin**. L'exemple regarde
d'abord `/sys/class/bluetooth` et l'état du service, et dit tout de suite ce qui manque, sans lancer
`bluetoothctl`.

## Confidentialité

Les noms des appareils voisins (« Téléphone de … », « Montre de … ») sont des données personnelles. Ils sont
affichés, **jamais envoyés**. Le dernier balayage vit dans un dossier privé (mode 700) de `$XDG_RUNTIME_DIR`,
effacé à la fermeture.

## Ce que l'exemple garantit (testé par `tests/garde_bluetooth_scan.sh`)

- **Lecture seule**, et un **délai sur chaque appel** : le balayage dure au plus sa durée plus 5 s, de 5 à 30 s.
- **Seules les lignes au format attendu sont lues** : `[NEW] Device <adresse> <nom>`, `[CHG] Device <adresse> RSSI: …`, `Device <adresse>`. Les codes de couleur sont retirés. Signal entre -127 et +20 dBm, sinon ignoré.
- **Les noms sont des données** : contrôles et `|` neutralisés, 40 caractères, UTF-8 réparé, jamais insérés dans le XML.

## Limites — et ce qui n'a pas été vérifié

- **Non vérifié : aucun vrai adaptateur n'a été essayé.** Les formats lus sont ceux de `bluetoothctl` (BlueZ), avec le signal écrit `-60` **ou** `0xffffffc4 (-60)` selon les versions ; ils sont testés sur une sortie fabriquée, pas sur une vraie capture.
- Un appareil qui n'annonce pas de nom s'affiche « (sans nom) ». Le Bluetooth basse consommation change souvent d'adresse (adresses aléatoires) : un même objet peut apparaître plusieurs fois.
- Le balayage gèle la fenêtre pendant sa durée (10 s par le bouton).
