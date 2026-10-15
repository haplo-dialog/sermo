#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Atelier, industrie, laboratoire
# Besoin   : voir en direct quelques variables d'un serveur OPC UA, sans rien y écrire
# Enrobe   : opcua-lire (petit client C de ce dossier, open62541 ; lecture seule, certificats en option)
# Ports    : gtk3sermo conseillé ; lvglsermo sur un écran d'atelier ; ncursessermo par SSH
# Terminal : rien de perdu (la jauge devient une barre de caractères)
# Usage    : make && ./opcua-lecture.sh [binaire sermo]
#            OPCUA_ADRESSE (défaut opc.tcp://127.0.0.1:4841, le faux serveur)
#            OPCUA_INTERVALLE en ms (défaut 1000) · OPCUA_LIRE : chemin du client
#            Serveur à certificats : OPCUA_MODE=sign ou chiffre (défaut none),
#            OPCUA_CERTIFICAT et OPCUA_CLE (ceux de ce poste, DER ou PEM),
#            OPCUA_CONFIANCE (certificat du serveur ; plusieurs : séparés par « : »),
#            OPCUA_URI (ApplicationUri du certificat ; défaut urn:sermo:opcua-lire)
#            Essai sans équipement : ./faux-serveur-opcua & puis ./opcua-lecture.sh
#
# Les variables lues sont décrites dans VARIABLES, plus bas : à adapter au serveur.
# Une seule connexion par tour (toutes les variables d'un coup) ; chaque widget
# lit ensuite sa valeur dans un petit fichier, sans retourner sur le réseau.

export SERMO="${SERMO:-sermo}"
# Le script lui-même, pour les <input> : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"
export OPCUA_ADRESSE="${OPCUA_ADRESSE:-opc.tcp://127.0.0.1:4841}"
export OPCUA_INTERVALLE="${OPCUA_INTERVALLE:-1000}"
export OPCUA_MODE="${OPCUA_MODE:-none}" OPCUA_CERTIFICAT="${OPCUA_CERTIFICAT:-}" OPCUA_CLE="${OPCUA_CLE:-}"
export OPCUA_CONFIANCE="${OPCUA_CONFIANCE:-}" OPCUA_URI="${OPCUA_URI:-urn:sermo:opcua-lire}"
ICI=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
if [ -z "${OPCUA_LIRE:-}" ]; then
	if [ -x "$ICI/opcua-lire" ]; then OPCUA_LIRE=$ICI/opcua-lire; else OPCUA_LIRE=$(command -v opcua-lire); fi
fi
export OPCUA_LIRE

# Une ligne par variable : nœud|libellé|unité|jauge min|jauge max
# (nœud en notation OPC UA ; jauge vide : pas de jauge).
VARIABLES='ns=1;s=temperature|Température|°C|0|100
ns=1;s=pression|Pression|mbar|900|1100
ns=1;s=vitesse|Vitesse|tr/min|0|2000
ns=1;s=pieces|Pièces produites|pièces||'

# Échappe une valeur avant de l'insérer dans le XML.
xml() { local s=${1//&/"&amp;"}; s=${s//</"&lt;"}; s=${s//>/"&gt;"}; printf '%s' "${s//\"/"&quot;"}"; }

# Argument : la table VARIABLES ; l'intervalle en ms. Les widgets sont
# numérotés dans l'ordre de la table (1, 2, …) : le nœud n'entre pas dans le XML.
dialogue_principal() {
	local variables=${1:-$VARIABLES} intervalle=${2:-1000}
	local n=0 noeud lib unite min max
	cat <<XML
<window title="Serveur OPC UA (lecture seule)" default-width="560">
  <vbox>
    <frame label="Variables">
      <grid columns="3" row-spacing="6" column-spacing="10">
$(while IFS='|' read -r noeud lib unite min max; do
	[ -n "$noeud" ] || continue
	n=$((n + 1))
	printf '        <text><label>%s</label></text>\n' "$(xml "$lib")"
	if [ -n "$min" ]; then
		printf '        <levelbar range-min="%s" range-max="%s" digits="1" space-expand="true" space-fill="true"><variable>JAUGE_%s</variable><input>exec "$SERMO_SCRIPT" --valeur %s</input></levelbar>\n' \
			"$(xml "$min")" "$(xml "$max")" "$n" "$n"
	else
		printf '        <text><label></label></text>\n'
	fi
	printf '        <text><variable>VALEUR_%s</variable><input>exec "$SERMO_SCRIPT" --valeur %s texte</input></text>\n' "$n" "$n"
done <<< "$variables")
      </grid>
    </frame>
    <text><variable>ETAT</variable><input>exec "\$SERMO_SCRIPT" --etat</input></text>
    <timer milliseconds="true" interval="$(xml "$intervalle")" visible="false">
      <variable>HORLOGE</variable>
      <action>exec "\$SERMO_SCRIPT" --lire-en-fond</action>
$(n=0; while IFS='|' read -r noeud lib unite min max; do
	[ -n "$noeud" ] || continue
	n=$((n + 1))
	[ -n "$min" ] && printf '      <action>refresh:JAUGE_%s</action>\n' "$n"
	printf '      <action>refresh:VALEUR_%s</action>\n' "$n"
done <<< "$variables")
      <action>refresh:ETAT</action>
    </timer>
    <hbox>
      <button><label>Fermer</label><action>exit:fermer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Lecture seule : un appel à opcua-lire pour toutes les variables de VARIABLES.
# Le cache garde une valeur par ligne, dans l'ordre de la table (vide si la
# variable est illisible), puis une ligne d'état.
lire() {
	local noeuds=() noeud reste sortie code ligne valeurs="" erreurs=0
	while IFS='|' read -r noeud reste; do [ -n "$noeud" ] && noeuds+=("$noeud"); done <<< "$VARIABLES"
	# L'adresse a été contrôlée (opc.tcp://…) : elle ne peut pas passer pour une option.
	local securite=() t
	if [ "$OPCUA_MODE" != none ]; then
		securite=(-m "$OPCUA_MODE" -c "$OPCUA_CERTIFICAT" -k "$OPCUA_CLE" -u "$OPCUA_URI")
		local IFS=:
		for t in $OPCUA_CONFIANCE; do securite+=(-T "$t"); done
		unset IFS
	fi
	sortie=$(timeout 10 "$OPCUA_LIRE" -t 2000 "${securite[@]}" "$OPCUA_ADRESSE" "${noeuds[@]}" 2>"$OPCUA_CACHE.erreur")
	code=$?
	# 0 : tout lu ; 2 : au moins une variable illisible, les autres sont bonnes.
	if [ $code = 0 ] || [ $code = 2 ]; then
		for noeud in "${noeuds[@]}"; do
			# La ligne commence par le nœud et une espace ; la valeur est le reste.
			ligne=$(printf '%s\n' "$sortie" | awk -v n="$noeud" 'index($0, n " ") == 1 { print substr($0, length(n) + 2); exit }')
			[[ "$ligne" == ERREUR* ]] && { ligne=""; erreurs=$((erreurs + 1)); }
			valeurs+=$ligne$'\n'
		done
		printf '%slu à %s%s\n' "$valeurs" "$(date +%T)" \
			"$( [ $erreurs -gt 0 ] && printf ' (%d variable(s) illisible(s))' $erreurs)" > "$OPCUA_CACHE.nouveau"
	else
		for noeud in "${noeuds[@]}"; do valeurs+=$'\n'; done
		printf '%spas de réponse (%s)\n' "$valeurs" \
			"$( [ $code = 124 ] && echo "délai dépassé" || sed -n 's/^opcua-lire: //p' "$OPCUA_CACHE.erreur" | tail -n 1)" > "$OPCUA_CACHE.nouveau"
	fi
	# Fenêtre fermée pendant la lecture : le cache a été effacé, ne pas le recréer.
	if [ -e "$OPCUA_CACHE" ]; then mv -f -- "$OPCUA_CACHE.nouveau" "$OPCUA_CACHE"; else rm -f -- "$OPCUA_CACHE.nouveau"; fi
}

# Le minuteur de sermo attend la fin de son action : une lecture lente (équipement
# injoignable, plusieurs secondes de délai) figerait la fenêtre. On lance donc la
# lecture en fond, une seule à la fois ; les widgets montrent la dernière valeur lue.
lire_en_fond() {
	mkdir -- "$OPCUA_CACHE.verrou" 2>/dev/null || return 0
	( trap 'rmdir -- "$OPCUA_CACHE.verrou"' EXIT; lire ) </dev/null >/dev/null 2>&1 &
}

# Valeur de la variable n° N (ordre de la table) ; « texte » ajoute l'unité.
# Sans valeur lue : 0 pour la jauge, « — » pour le texte.
valeur() {
	local n=$1 forme=${2:-} v="" noeud lib unite min max i=0
	[[ "$n" =~ ^[0-9]+$ ]] || return
	[ -r "$OPCUA_CACHE" ] && v=$(sed -n "${n}p" "$OPCUA_CACHE")
	while IFS='|' read -r noeud lib unite min max; do
		[ -n "$noeud" ] || continue
		i=$((i + 1)); [ $i = "$n" ] && break
	done <<< "$VARIABLES"
	if [ "$forme" = texte ]; then
		[ -n "$v" ] && printf '%s %s\n' "$v" "$unite" || echo "—"
	elif [[ "$v" =~ ^-?[0-9]+(\.[0-9]+)?([eE][-+]?[0-9]+)?$ ]]; then
		awk -v b="$v" 'BEGIN { printf "%.1f\n", b }'
	else
		echo 0
	fi
}

etat() {
	local derniere
	derniere=$( [ -r "$OPCUA_CACHE" ] && tail -n 1 "$OPCUA_CACHE")
	printf '%s — %s\n' "$OPCUA_ADRESSE" "${derniere:-pas encore lu}"
}

main() {
	case "${1:-}" in
	--lire)   lire; exit ;;
	--lire-en-fond) lire_en_fond; exit ;;
	--valeur) valeur "$2" "${3:-}"; exit ;;
	--etat)   etat; exit ;;
	esac
	[ -n "${1:-}" ] && SERMO=$1
	[ -n "$OPCUA_LIRE" ] && [ -x "$OPCUA_LIRE" ] ||
		{ echo "opcua-lire est introuvable : lancez make dans $ICI (paquet libopen62541-1.4-dev)" >&2; exit 1; }
	for c in awk sed timeout; do
		command -v "$c" >/dev/null || { echo "$c est absent : installez coreutils, sed et gawk ou mawk." >&2; exit 1; }
	done
	# L'adresse part sur la ligne de commande du client : on la contrôle d'abord.
	# (hôte, IPv4 ou [IPv6], port, chemin ; « ] » en tête de la classe pour y être littéral)
	local motif='^opc\.tcp://[][A-Za-z0-9.:-]+(/[A-Za-z0-9._/-]*)?$'
	[[ "$OPCUA_ADRESSE" =~ $motif ]] ||
		{ echo "OPCUA_ADRESSE invalide : $OPCUA_ADRESSE" >&2; exit 1; }
	case "$OPCUA_MODE" in
	none) ;;
	sign|chiffre)
		local f
		for f in "$OPCUA_CERTIFICAT" "$OPCUA_CLE"; do
			[ -n "$f" ] && [ -r "$f" ] || { echo "OPCUA_MODE=$OPCUA_MODE : OPCUA_CERTIFICAT et OPCUA_CLE doivent nommer des fichiers lisibles" >&2; exit 1; }
		done
		[ -n "$OPCUA_CONFIANCE" ] || { echo "OPCUA_MODE=$OPCUA_MODE : OPCUA_CONFIANCE (certificat du serveur) est obligatoire" >&2; exit 1; }
		local IFS=:
		for f in $OPCUA_CONFIANCE; do
			[ -r "$f" ] || { echo "OPCUA_CONFIANCE : $f est illisible" >&2; exit 1; }
		done
		unset IFS
		[[ "$OPCUA_URI" =~ ^[A-Za-z][A-Za-z0-9+.-]*:[^[:space:]]+$ ]] || { echo "OPCUA_URI invalide : $OPCUA_URI" >&2; exit 1; }
		;;
	*) echo "OPCUA_MODE invalide (none, sign ou chiffre) : $OPCUA_MODE" >&2; exit 1 ;;
	esac
	[[ "$OPCUA_INTERVALLE" =~ ^[0-9]{3,6}$ ]] || { echo "OPCUA_INTERVALLE invalide (en ms, 100 au moins) : $OPCUA_INTERVALLE" >&2; exit 1; }

	# Le cache vit dans le dossier privé de la session, et disparaît avec la fenêtre.
	local dossier=${XDG_RUNTIME_DIR:-$HOME/.cache}
	[ -d "$dossier" ] || dossier=$HOME
	export OPCUA_CACHE
	OPCUA_CACHE=$(mktemp "$dossier/sermo-opcua.XXXXXX") || exit 1
	trap 'rm -f -- "$OPCUA_CACHE" "$OPCUA_CACHE.nouveau" "$OPCUA_CACHE.erreur"; rmdir -- "$OPCUA_CACHE.verrou" 2>/dev/null' EXIT
	trap 'exit 129' HUP; trap 'exit 130' INT; trap 'exit 143' TERM
	lire

	export DIALOGUE; DIALOGUE=$(dialogue_principal "$VARIABLES" "$OPCUA_INTERVALLE")
	"$SERMO" --program=DIALOGUE >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
