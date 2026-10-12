#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Sécurité informatique
# Besoin   : passer en revue les comptes qui peuvent ouvrir une session (dernière connexion, état du mot de passe) et verrouiller un compte dormant
# Enrobe   : getent, lastlog (ou lastlog2), passwd -S en lecture ; passwd -l pour verrouiller
# Ports    : gtk3sermo conseillé ; ncursessermo par SSH
# Terminal : rien de perdu
# Usage    : ./3-audit-comptes.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            « passwd -S » et « passwd -l » sur un autre compte demandent les droits administrateur.

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

# Arguments : lignes « compte|uid|shell|dernière connexion ».
dialogue_principal() {
	local lignes=("$@")
	[ ${#lignes[@]} -gt 0 ] || lignes=("alice|1000|/bin/bash|2026-10-09 08:12" "bob|1001|/bin/bash|jamais" "stagiaire|1002|/bin/sh|2025-12-18 17:40")
	cat <<XML
<window title="Audit des comptes" default-width="600">
  <vbox>
    <text><label>Comptes pouvant ouvrir une session (shell de connexion valide) :</label></text>
    <tree scrollable="true" height-request="200">
      <label>Compte|UID|Shell|Dernière connexion</label>
      <variable>COMPTE</variable>
$(for l in "${lignes[@]}"; do printf '      <item>%s</item>\n' "$(xml "$l")"; done)
    </tree>
    <frame label="Compte sélectionné">
      <vbox>
        <radiobutton><label>Afficher l'état du mot de passe (passwd -S)</label><variable>VOIR_ETAT</variable><default>true</default></radiobutton>
        <radiobutton><label>Verrouiller le mot de passe (passwd -l)</label><variable>VERROUILLER</variable></radiobutton>
      </vbox>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Appliquer au compte</label><action>exit:compte</action></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_etat() {
	local compte=${1:-bob} etat=${2:-bob P 2026-03-02 0 99999 7 -1}
	cat <<XML
<window title="État du compte">
  <vbox>
    <text><label>Compte : $(xml "$compte")</label></text>
    <entry editable="false"><variable>ETAT</variable><default>$(xml "$etat")</default></entry>
    <text><label>Deuxième champ : P mot de passe utilisable, L verrouillé, NP sans mot de passe.</label></text>
    <hbox><button ok></button></hbox>
  </vbox>
</window>
XML
}

dialogue_verrouiller() {
	local compte=${1:-stagiaire}
	cat <<XML
<window title="Verrouiller le compte ?">
  <vbox>
    <text><label>Verrouiller le mot de passe du compte $(xml "$compte") ?</label></text>
    <text><label>La personne ne pourra plus se connecter par mot de passe (une clé SSH reste valable).</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Verrouiller</label><action>exit:verrouiller</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Dernière connexion d'un compte, en lecture seule.
derniere_connexion() {
	local d
	if command -v lastlog2 >/dev/null; then
		d=$(lastlog2 -u "$1" 2>/dev/null | awk 'NR==2 { $1=""; $2=""; $3=""; sub(/^ +/, ""); print }')
	elif command -v lastlog >/dev/null; then
		d=$(lastlog -u "$1" 2>/dev/null | awk 'NR==2 { print $(NF-5), $(NF-4), $(NF-3), $(NF-2) }')
	fi
	case "$d" in ""|*"Never"*|*"never"*|*"Jamais"*|*"**"*) echo "jamais ou inconnue" ;; *) echo "$d" ;; esac
}

# Lignes « compte|uid|shell|dernière » des comptes à shell de connexion.
comptes() {
	local nom uid shell
	while IFS=: read -r nom _ uid _ _ _ shell; do
		case "$shell" in */nologin|*/false|/bin/sync|"") continue ;; esac
		[ "$uid" -ge 1000 ] || [ "$uid" -eq 0 ] || continue
		printf '%s|%s|%s|%s\n' "$nom" "$uid" "$shell" "$(derniere_connexion "$nom" | tr '|' '/')"
	done < <(getent passwd)
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = compte ] || exit 0
		COMPTE=${COMPTE%%|*}
		getent passwd "$COMPTE" >/dev/null || { echo "Choisissez un compte dans la liste." >&2; exit 1; }
		export COMPTE
		export DIALOGUE
		if [ "$VERROUILLER" = true ]; then
			DIALOGUE=$(dialogue_verrouiller "$COMPTE")
			"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite verrouiller' >/dev/null
		else
			# Lecture seule : toujours lancée. Sans droits, passwd le dit.
			etat=$(LC_ALL=C passwd -S -- "$COMPTE" 2>&1 | head -1)
			DIALOGUE=$(dialogue_etat "$COMPTE" "${etat:-illisible}")
			"$SERMO" --program=DIALOGUE >/dev/null
		fi
		;;
	verrouiller)
		[ "$EXIT" = verrouiller ] || exit 0
		[ "$(id -u)" -eq 0 ] || echo "Droits administrateur nécessaires : lancez la commande ci-dessous en root."
		agir passwd -l -- "$COMPTE"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	for c in getent passwd; do
		command -v "$c" >/dev/null || { echo "$c est absent : installez le paquet libc-bin / passwd." >&2; exit 1; }
	done
	mapfile -t lignes < <(comptes)
	export DIALOGUE; DIALOGUE=$(dialogue_principal "${lignes[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
