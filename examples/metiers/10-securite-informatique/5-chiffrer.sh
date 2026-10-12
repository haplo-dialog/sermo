#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Sécurité informatique
# Besoin   : chiffrer un fichier avec une phrase secrète avant de l'envoyer ou de le stocker, et effacer l'original si on le veut
# Enrobe   : gpg --symmetric ; shred pour effacer l'original
# Ports    : gtk3sermo conseillé ; ncursessermo par SSH
# Terminal : rien de perdu
# Usage    : ./5-chiffrer.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            La phrase secrète passe à gpg par un descripteur, jamais sur la ligne de commande,
#            et n'est transmise à aucun autre processus que gpg.

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"

# Échappe une valeur avant de l'insérer dans le XML.
xml() { local s=${1//&/"&amp;"}; s=${s//</"&lt;"}; s=${s//>/"&gt;"}; printf '%s' "${s//\"/"&quot;"}"; }

# Lance une commande, ou l'affiche seulement en mode à blanc.
agir() {
	if [ "$SERMO_A_BLANC" = 1 ]; then
		printf 'à blanc :'; printf ' %q' "$@"; printf '\n'
	else
		"$@"
	fi
}

dialogue_principal() {
	local fichier=${1:-$HOME/Documents/contrats-2026.tar}
	cat <<XML
<window title="Chiffrer un fichier" default-width="540">
  <vbox>
    <frame label="Fichier">
      <hbox><text><label>Fichier à chiffrer :</label></text>
        <entry space-expand="true" space-fill="true"><variable>FICHIER</variable><default>$(xml "$fichier")</default></entry></hbox>
    </frame>
    <frame label="Phrase secrète">
      <vbox>
        <hbox><text><label>Phrase secrète :</label></text>
          <password space-expand="true" space-fill="true"><variable>PHRASE</variable></password></hbox>
        <hbox><text><label>Une seconde fois :</label></text>
          <password space-expand="true" space-fill="true"><variable>PHRASE2</variable></password></hbox>
      </vbox>
    </frame>
    <frame label="Options">
      <vbox>
        <hbox><text><label>Algorithme :</label></text>
          <comboboxtext><variable>ALGO</variable><default>AES256</default>
            <item>AES256</item><item>CAMELLIA256</item><item>TWOFISH</item>
          </comboboxtext></hbox>
        <hbox><text><label>Compression (0 aucune, 9 maximale) :</label></text>
          <hscale range-min="0" range-max="9" range-step="1" draw-value="true" space-expand="true" space-fill="true"><variable>COMPRESSION</variable><default>6</default></hscale></hbox>
        <checkbox><label>Sortie en texte (ASCII, --armor) pour un courriel</label><variable>ARMURE</variable></checkbox>
        <checkbox><label>Effacer l'original après chiffrement (shred)</label><variable>EFFACER</variable></checkbox>
      </vbox>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Chiffrer</label><action>exit:chiffrer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_confirmation() {
	local fichier=${1:-$HOME/Documents/contrats-2026.tar} sortie=${2:-$HOME/Documents/contrats-2026.tar.gpg}
	cat <<XML
<window title="Effacer l'original ?">
  <vbox>
    <text><label>Le fichier chiffré sera : $(xml "$sortie")</label></text>
    <text><label>L'original $(xml "$fichier") sera ensuite écrasé puis supprimé.</label></text>
    <text><label>Sans la phrase secrète, le contenu sera perdu pour de bon.</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Chiffrer et effacer</label><action>exit:effacer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# La commande gpg, un argument par ligne.
commande_gpg() {
	echo gpg; echo --batch; echo --yes; echo --pinentry-mode; echo loopback
	echo --passphrase-fd; echo 3
	echo --symmetric; echo --cipher-algo; echo "$ALGO"
	echo --compress-level; echo "$COMPRESSION"
	[ "$ARMURE" = true ] && echo --armor
	echo --output; echo "$SORTIE"; echo --; echo "$FICHIER"
}

chiffrer() {
	mapfile -t cmd < <(commande_gpg)
	# La phrase arrive sur le descripteur 3 : elle n'apparaît ni dans ps ni à l'écran.
	agir "${cmd[@]}" 3<<<"$PHRASE" || exit 1
	if [ "$1" = effacer ]; then agir shred --remove --zero -- "$FICHIER"; fi
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$ALGO" in AES256|CAMELLIA256|TWOFISH) ;; *) ALGO=AES256 ;; esac
	[[ $COMPRESSION =~ ^[0-9]$ ]] || COMPRESSION=6
	SORTIE=$FICHIER.gpg; [ "$ARMURE" = true ] && SORTIE=$FICHIER.asc
	case "$1" in
	principal)
		[ "$EXIT" = chiffrer ] || exit 0
		[ -f "$FICHIER" ] || { echo "Fichier introuvable : $FICHIER" >&2; exit 1; }
		[ -n "$PHRASE" ] || { echo "La phrase secrète est vide." >&2; exit 1; }
		[ "$PHRASE" = "$PHRASE2" ] || { echo "Les deux phrases secrètes diffèrent." >&2; exit 1; }
		[ ${#PHRASE} -ge 12 ] || echo "Attention : phrase de moins de 12 caractères."
		if [ "$EFFACER" = true ]; then
			# La confirmation se lit ici, sur la sortie de sermo : la phrase
			# reste dans CE processus, le seul qui chiffre. Le second sermo est
			# lancé sans elle dans son environnement (ni lui ni ses enfants).
			export DIALOGUE; DIALOGUE=$(dialogue_confirmation "$FICHIER" "$SORTIE")
			reponse=$(env -u PHRASE -u PHRASE2 "$SERMO" --program=DIALOGUE | grep '^EXIT=')
			[ "$reponse" = 'EXIT="effacer"' ] || exit 0
			chiffrer effacer
		else
			chiffrer garder
		fi
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v gpg >/dev/null || { echo "gpg est absent : installez le paquet gnupg." >&2; exit 1; }
	command -v shred >/dev/null || { echo "shred est absent : installez le paquet coreutils." >&2; exit 1; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
