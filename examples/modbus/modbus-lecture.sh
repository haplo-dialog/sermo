#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Atelier, industrie, laboratoire
# Besoin   : voir en direct quelques registres d'un automate Modbus/TCP, sans rien y écrire
# Enrobe   : mbpoll (lecture seule : jamais de valeur à écrire sur sa ligne de commande)
# Ports    : gtk3sermo conseillé ; lvglsermo sur un écran d'atelier ; ncursessermo par SSH
# Terminal : rien de perdu (la jauge devient une barre de caractères)
# Usage    : ./modbus-lecture.sh [binaire sermo]
#            MODBUS_HOTE (défaut 127.0.0.1) · MODBUS_PORT (défaut 5020, le faux automate ;
#            502 pour un vrai) · MODBUS_ESCLAVE (défaut 1) · MODBUS_INTERVALLE en ms (défaut 1000)
#            Essai sans automate : ./faux-automate.py & puis ./modbus-lecture.sh
#
# Les registres lus sont décrits dans REGISTRES, plus bas : à adapter à l'automate.
# Une seule requête Modbus par tour (tous les registres d'un coup) ; chaque widget
# lit ensuite sa valeur dans un petit fichier, sans retourner sur le réseau.

export SERMO="${SERMO:-sermo}"
# Le script lui-même, pour les <input> : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"
export MODBUS_HOTE="${MODBUS_HOTE:-127.0.0.1}"
export MODBUS_PORT="${MODBUS_PORT:-5020}"
export MODBUS_ESCLAVE="${MODBUS_ESCLAVE:-1}"
export MODBUS_INTERVALLE="${MODBUS_INTERVALLE:-1000}"

# Registres de maintien (holding, mbpoll -t 4), numérotés à partir de 1 comme mbpoll.
# Une ligne par registre : numéro|libellé|diviseur|unité|jauge min|jauge max
# (diviseur 10 : l'automate envoie 565 pour 56.5 ; jauge vide : pas de jauge).
REGISTRES='1|Température|10|°C|0|100
2|Pression|1|mbar|900|1100
3|Vitesse|1|tr/min|0|2000
4|Pièces produites|1|pièces||'

# Échappe une valeur avant de l'insérer dans le XML.
xml() { local s=${1//&/"&amp;"}; s=${s//</"&lt;"}; s=${s//>/"&gt;"}; printf '%s' "${s//\"/"&quot;"}"; }

# Argument : la table REGISTRES ; l'intervalle en ms.
dialogue_principal() {
	local registres=${1:-$REGISTRES} intervalle=${2:-1000}
	local n lib div unite min max
	cat <<XML
<window title="Automate Modbus (lecture seule)" default-width="560">
  <vbox>
    <frame label="Registres de maintien">
      <grid columns="3" row-spacing="6" column-spacing="10">
$(while IFS='|' read -r n lib div unite min max; do
	[ -n "$n" ] || continue
	printf '        <text><label>%s</label></text>\n' "$(xml "$lib")"
	if [ -n "$min" ]; then
		printf '        <levelbar range-min="%s" range-max="%s" digits="1" space-expand="true" space-fill="true"><variable>JAUGE_%s</variable><input>exec "$SERMO_SCRIPT" --valeur %s</input></levelbar>\n' \
			"$(xml "$min")" "$(xml "$max")" "$n" "$n"
	else
		printf '        <text><label></label></text>\n'
	fi
	printf '        <text><variable>VALEUR_%s</variable><input>exec "$SERMO_SCRIPT" --valeur %s texte</input></text>\n' "$n" "$n"
done <<< "$registres")
      </grid>
    </frame>
    <text><variable>ETAT</variable><input>exec "\$SERMO_SCRIPT" --etat</input></text>
    <timer milliseconds="true" interval="$(xml "$intervalle")" visible="false">
      <variable>HORLOGE</variable>
      <action>exec "\$SERMO_SCRIPT" --lire-en-fond</action>
$(while IFS='|' read -r n lib div unite min max; do
	[ -n "$n" ] || continue
	[ -n "$min" ] && printf '      <action>refresh:JAUGE_%s</action>\n' "$n"
	printf '      <action>refresh:VALEUR_%s</action>\n' "$n"
done <<< "$registres")
      <action>refresh:ETAT</action>
    </timer>
    <hbox>
      <button><label>Fermer</label><action>exit:fermer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Lecture seule : une requête mbpoll pour tous les registres de REGISTRES, rangés
# « numéro valeur brute » dans MODBUS_CACHE. En cas d'échec, le cache est vidé et
# la raison est gardée pour la ligne d'état.
lire() {
	local refs="" n reste sortie
	while IFS='|' read -r n reste; do [ -n "$n" ] && refs+=${refs:+,}$n; done <<< "$REGISTRES"
	if sortie=$(LC_ALL=C timeout 5 mbpoll -1 -q -t 4 -a "$MODBUS_ESCLAVE" -r "$refs" \
		-p "$MODBUS_PORT" -o 2 -- "$MODBUS_HOTE" 2>&1); then
		printf '%s\n' "$sortie" | sed -n 's/^\[\([0-9]*\)\]:[[:space:]]*\(-\{0,1\}[0-9]*\)$/\1 \2/p' \
			> "$MODBUS_CACHE.nouveau"
		printf 'lu à %s\n' "$(date +%T)" >> "$MODBUS_CACHE.nouveau"
	else
		printf 'pas de réponse (%s)\n' "$(printf '%s\n' "$sortie" | sed -n 's/^mbpoll: //p' | tail -n 1)" > "$MODBUS_CACHE.nouveau"
	fi
	# Fenêtre fermée pendant la lecture : le cache a été effacé, ne pas le recréer.
	if [ -e "$MODBUS_CACHE" ]; then mv -f -- "$MODBUS_CACHE.nouveau" "$MODBUS_CACHE"; else rm -f -- "$MODBUS_CACHE.nouveau"; fi
}

# Le minuteur de sermo attend la fin de son action : une lecture lente (équipement
# injoignable, plusieurs secondes de délai) figerait la fenêtre. On lance donc la
# lecture en fond, une seule à la fois ; les widgets montrent la dernière valeur lue.
lire_en_fond() {
	mkdir -- "$MODBUS_CACHE.verrou" 2>/dev/null || return 0
	( trap 'rmdir -- "$MODBUS_CACHE.verrou"' EXIT; lire ) </dev/null >/dev/null 2>&1 &
}

# Valeur d'un registre, divisée par son diviseur ; « texte » ajoute l'unité.
# Sans valeur lue : 0 pour la jauge, « — » pour le texte.
valeur() {
	local n=$1 forme=${2:-} r brute="" lib div unite min max
	[ -r "$MODBUS_CACHE" ] && while read -r r brute; do [ "$r" = "$n" ] && break; brute=""; done < "$MODBUS_CACHE"
	while IFS='|' read -r r lib div unite min max; do [ "$r" = "$n" ] && break; done <<< "$REGISTRES"
	if ! [[ "$brute" =~ ^-?[0-9]+$ ]]; then
		[ "$forme" = texte ] && echo "—" || echo 0
		return
	fi
	if [ "$forme" = texte ]; then
		awk -v b="$brute" -v d="${div:-1}" -v u="$unite" 'BEGIN { printf (d == 1 ? "%d %s\n" : "%.1f %s\n"), b / d, u }'
	else
		awk -v b="$brute" -v d="${div:-1}" 'BEGIN { printf "%.1f\n", b / d }'
	fi
}

etat() {
	local derniere
	derniere=$( [ -r "$MODBUS_CACHE" ] && tail -n 1 "$MODBUS_CACHE")
	printf '%s:%s esclave %s — %s\n' "$MODBUS_HOTE" "$MODBUS_PORT" "$MODBUS_ESCLAVE" "${derniere:-pas encore lu}"
}

main() {
	case "${1:-}" in
	--lire)   lire; exit ;;
	--lire-en-fond) lire_en_fond; exit ;;
	--valeur) valeur "$2" "${3:-}"; exit ;;
	--etat)   etat; exit ;;
	esac
	[ -n "${1:-}" ] && SERMO=$1
	command -v mbpoll >/dev/null || { echo "mbpoll est absent : sudo apt install mbpoll" >&2; exit 1; }
	for c in awk sed timeout; do
		command -v "$c" >/dev/null || { echo "$c est absent : installez coreutils, sed et gawk ou mawk." >&2; exit 1; }
	done
	# Ces valeurs partent sur la ligne de commande de mbpoll : on les contrôle d'abord.
	[[ "$MODBUS_HOTE" =~ ^[A-Za-z0-9][A-Za-z0-9.:-]*$ ]] || { echo "MODBUS_HOTE invalide : $MODBUS_HOTE" >&2; exit 1; }
	[[ "$MODBUS_PORT" =~ ^[0-9]{1,5}$ ]] && [ "$MODBUS_PORT" -ge 1 ] && [ "$MODBUS_PORT" -le 65535 ] ||
		{ echo "MODBUS_PORT invalide : $MODBUS_PORT" >&2; exit 1; }
	[[ "$MODBUS_ESCLAVE" =~ ^[0-9]{1,3}$ ]] && [ "$MODBUS_ESCLAVE" -le 255 ] ||
		{ echo "MODBUS_ESCLAVE invalide : $MODBUS_ESCLAVE" >&2; exit 1; }
	[[ "$MODBUS_INTERVALLE" =~ ^[0-9]{3,6}$ ]] || { echo "MODBUS_INTERVALLE invalide (en ms, 100 au moins) : $MODBUS_INTERVALLE" >&2; exit 1; }

	# Le cache vit dans le dossier privé de la session, et disparaît avec la fenêtre.
	local dossier=${XDG_RUNTIME_DIR:-$HOME/.cache}
	[ -d "$dossier" ] || dossier=$HOME
	export MODBUS_CACHE
	MODBUS_CACHE=$(mktemp "$dossier/sermo-modbus.XXXXXX") || exit 1
	trap 'rm -f -- "$MODBUS_CACHE" "$MODBUS_CACHE.nouveau"; rmdir -- "$MODBUS_CACHE.verrou" 2>/dev/null' EXIT
	trap 'exit 129' HUP; trap 'exit 130' INT; trap 'exit 143' TERM
	lire

	export DIALOGUE; DIALOGUE=$(dialogue_principal "$REGISTRES" "$MODBUS_INTERVALLE")
	"$SERMO" --program=DIALOGUE >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
