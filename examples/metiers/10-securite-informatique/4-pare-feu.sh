#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Sécurité informatique
# Besoin   : relire les règles du pare-feu et ouvrir ou fermer un port en entrée, sans retenir la syntaxe
# Enrobe   : ufw (status, allow, deny) s'il est installé, sinon nft (list ruleset, add rule)
# Ports    : gtk3sermo conseillé ; ncursessermo par SSH
# Terminal : rien de perdu
# Usage    : ./4-pare-feu.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            Lire et modifier les règles demandent les droits administrateur.

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

# Le pare-feu présent : ufw d'abord, sinon nft (souvent dans /usr/sbin, hors du PATH).
outil() {
	command -v ufw 2>/dev/null && return
	command -v nft 2>/dev/null && return
	[ -x /usr/sbin/ufw ] && { echo /usr/sbin/ufw; return; }
	[ -x /usr/sbin/nft ] && echo /usr/sbin/nft
}

dialogue_principal() {
	local outil=${1:-nft}
	# Les règles (plusieurs lignes) entrent par <default>, échappées par xml :
	# jamais collées dans une commande <input>, où elles deviendraient du shell.
	cat <<XML
<window title="Pare-feu ($(xml "${outil##*/}"))" default-width="600">
  <vbox>
    <frame label="Règles en place">
      <edit editable="false" height-request="180"><variable>REGLES</variable><default>$(xml "${TEXTE_REGLES:-}")</default></edit>
    </frame>
    <frame label="Nouvelle règle en entrée">
      <vbox>
        <hbox><text><label>Protocole :</label></text>
          <comboboxtext><variable>PROTO</variable><default>tcp</default><item>tcp</item><item>udp</item></comboboxtext>
          <text><label>Port :</label></text>
          <spinbutton range-min="1" range-max="65535" range-step="1"><variable>PORT</variable><default>443</default></spinbutton></hbox>
        <hbox><text><label>Depuis (vide = partout) :</label></text>
          <entry space-expand="true" space-fill="true"><variable>SOURCE</variable><default>192.168.1.0/24</default></entry></hbox>
        <hbox>
          <radiobutton><label>Autoriser</label><variable>AUTORISER</variable><default>true</default></radiobutton>
          <radiobutton><label>Refuser</label><variable>REFUSER</variable></radiobutton>
        </hbox>
      </vbox>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Ajouter la règle</label><action>exit:ajouter</action></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_confirmation() {
	local commande=${1:-nft add rule inet filter input ip saddr 192.168.1.0/24 tcp dport 443 accept}
	cat <<XML
<window title="Confirmer la règle">
  <vbox>
    <text><label>Cette commande modifie le pare-feu en service :</label></text>
    <entry editable="false" width-request="520"><variable>COMMANDE</variable><default>$(xml "$commande")</default></entry>
    <text><label>Par SSH, une erreur peut couper votre propre connexion. Continuer ?</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Appliquer</label><action>exit:appliquer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# La commande de la règle, un argument par ligne.
regle() {
	local verdict=accept
	[ "$REFUSER" = true ] && verdict=drop
	if [ "${OUTIL##*/}" = ufw ]; then
		echo "$OUTIL"; [ "$verdict" = accept ] && echo allow || echo deny
		echo proto; echo "$PROTO"
		echo from; echo "${SOURCE:-any}"
		echo to; echo any; echo port; echo "$PORT"
	else
		echo "$OUTIL"; echo add; echo rule; echo inet; echo filter; echo input
		if [ -n "$SOURCE" ]; then
			[[ $SOURCE = *:* ]] && echo ip6 || echo ip
			echo saddr; echo "$SOURCE"
		fi
		echo "$PROTO"; echo dport; echo "$PORT"; echo "$verdict"
	fi
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	OUTIL=$(outil)
	case "$1" in
	principal)
		[ "$EXIT" = ajouter ] || exit 0
		[[ $PROTO = tcp || $PROTO = udp ]] || { echo "Protocole refusé : $PROTO" >&2; exit 1; }
		[[ $PORT =~ ^[0-9]+$ ]] && [ "$PORT" -ge 1 ] && [ "$PORT" -le 65535 ] || { echo "Port refusé : $PORT" >&2; exit 1; }
		[ -z "$SOURCE" ] || [[ $SOURCE =~ ^[0-9A-Fa-f.:/]+$ ]] || { echo "Adresse source refusée : $SOURCE" >&2; exit 1; }
		export PROTO PORT SOURCE AUTORISER REFUSER
		mapfile -t cmd < <(regle)
		export DIALOGUE; DIALOGUE=$(dialogue_confirmation "$(printf '%q ' "${cmd[@]}")")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite confirmation' >/dev/null
		;;
	confirmation)
		[ "$EXIT" = appliquer ] || exit 0
		[ "$(id -u)" -eq 0 ] || echo "Droits administrateur nécessaires : lancez la commande ci-dessous en root."
		mapfile -t cmd < <(regle)
		agir "${cmd[@]}"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	OUTIL=$(outil)
	[ -n "$OUTIL" ] || { echo "Ni ufw ni nft : installez le paquet nftables (ou ufw)." >&2; exit 1; }
	# Lecture seule. Sans droits, le message d'erreur s'affiche à la place des règles.
	if [ "${OUTIL##*/}" = ufw ]; then regles=$("$OUTIL" status verbose 2>&1)
	else regles=$("$OUTIL" list ruleset 2>&1); fi
	export TEXTE_REGLES=${regles:-(aucune règle chargée)}
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$OUTIL")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
