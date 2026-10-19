#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Administration système, support, journalisation, sécurité (TLS, Kerberos)
# Besoin   : voir d'un coup d'œil si l'horloge est juste : heure locale et UTC, fuseau,
#            horloge matérielle, NTP actif et synchronisé, serveur, écart mesuré
# Enrobe   : timedatectl show / timesync-status (systemd-timesyncd) ; lecture seule
# Ports    : tout port à fenêtre (tree, text)
# Terminal : ncursessermo par SSH : rien de perdu
# Usage    : ./heure-ntp.sh [binaire sermo]
#            ./heure-ntp.sh --etat        (élément|valeur, puis la ligne de bilan)
#            HN_ECART_MAX : écart toléré en millisecondes (défaut 500)
#
# Codes de sortie : 0 fait · 2 timedatectl absent · 64 usage.
# Rien n'est modifié : régler l'heure, le fuseau ou NTP reste « timedatectl set-… »,
# qui demande l'autorisation d'un administrateur. Chaque valeur lue est contrôlée
# (format attendu) ; une valeur inattendue s'affiche « ? », jamais telle quelle.

export SERMO="${SERMO:-sermo}"
export SERMO_SCRIPT="$0"
export HN_ECART_MAX="${HN_ECART_MAX:-500}"

# Valeur d'une clé dans « Clé=valeur » (show) ou « Clé: valeur » (timesync-status).
val() { sed -n "s/^ *$2[=:] *//p" <<<"$1" | head -n 1 | LC_ALL=C tr -d '\000-\037\177' | cut -c1-80; }
# La valeur si elle a la forme attendue, « ? » sinon.
forme() { [[ "$1" =~ $2 ]] && printf '%s' "$1" || printf '?'; }
oui_non() { case "$1" in yes) echo oui ;; no) echo non ;; *) echo '?' ;; esac; }
# « -27.881ms », « 512us », « 1.2s » → millisecondes entières (valeur absolue) ; vide sinon.
en_ms() {
	[[ "$1" =~ ^[+-]?([0-9]{1,9})(\.[0-9]{1,6})?(us|ms|s)$ ]] || return 1
	awk -v n="${1%[a-z]*}" -v u="${BASH_REMATCH[3]}" 'BEGIN { if (n < 0) n = -n; f = (u == "s") ? 1000 : (u == "ms") ? 1 : 0.001; printf "%d", n * f }'
}

etat() {
	local show sync tz ntp synchro rtc locrtc srv adr strate ecart delai gigue inter ms bilan
	command -v timedatectl >/dev/null || { echo "timedatectl est absent (paquet systemd)" >&2; exit 2; }
	show=$(LC_ALL=C timeout 3 timedatectl show 2>/dev/null)
	sync=$(LC_ALL=C timeout 3 timedatectl timesync-status 2>/dev/null)
	tz=$(forme "$(val "$show" Timezone)" '^[A-Za-z0-9_+/-]{1,64}$')
	ntp=$(oui_non "$(val "$show" NTP)"); synchro=$(oui_non "$(val "$show" NTPSynchronized)")
	locrtc=$(val "$show" LocalRTC)
	rtc=$(forme "$(val "$show" RTCTimeUSec)" '^[A-Z][a-z]{2} [0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2} [A-Z]{2,5}$')
	srv=$(val "$sync" Server)
	adr=$(forme "${srv%% *}" '^[0-9A-Fa-f.:]{2,39}$'); [ "$adr" = '?' ] && adr=""
	srv=$(sed -n 's/^[^ ]* (\(.*\))$/\1/p' <<<"$srv"); srv=$(forme "$srv" '^[A-Za-z0-9.-]{1,253}$')
	strate=$(forme "$(val "$sync" Stratum)" '^[0-9]{1,2}$')
	ecart=$(forme "$(val "$sync" Offset)" '^[+-]?[0-9]{1,9}(\.[0-9]{1,6})?(us|ms|s)$')
	delai=$(forme "$(val "$sync" Delay)" '^[0-9]{1,9}(\.[0-9]{1,6})?(us|ms|s)$')
	gigue=$(forme "$(val "$sync" Jitter)" '^[0-9]{1,9}(\.[0-9]{1,6})?(us|ms|s)$')
	inter=$(forme "$(val "$sync" 'Poll interval')" '^[0-9a-z ]{1,20}( \(min: [0-9a-z ]{1,20}; max [0-9a-z ]{1,20}\))?$')
	[[ "$HN_ECART_MAX" =~ ^[0-9]{1,6}$ ]] || { echo "HN_ECART_MAX invalide : $HN_ECART_MAX" >&2; exit 64; }
	printf '%s|%s\n' \
		"Heure locale" "$(date '+%A %-d %B %Y, %H:%M:%S %Z')" \
		"Heure UTC" "$(date -u '+%Y-%m-%d %H:%M:%S')" \
		"Fuseau" "$tz" \
		"Horloge matérielle" "$rtc$( [ "$locrtc" = yes ] && echo ' — en heure LOCALE (déconseillé)')" \
		"NTP activé" "$ntp" \
		"Synchronisée" "$synchro" \
		"Serveur" "${srv/#\?/—}${adr:+ ($adr)}" \
		"Strate" "${strate/#\?/—}" \
		"Écart mesuré" "${ecart/#\?/—}" \
		"Délai réseau" "${delai/#\?/—}" \
		"Gigue" "${gigue/#\?/—}" \
		"Intervalle d'interrogation" "${inter/#\?/—}"
	if [ "$ntp" != oui ]; then bilan="ATTENTION : NTP n'est pas activé (timedatectl set-ntp true, en administrateur)"
	elif [ "$synchro" != oui ]; then bilan="ATTENTION : l'horloge n'est pas synchronisée"
	elif ms=$(en_ms "$ecart") && [ "$ms" -gt "$HN_ECART_MAX" ]; then bilan="ATTENTION : écart de $ms ms (toléré : $HN_ECART_MAX ms)"
	elif [ -z "$sync" ]; then bilan="Synchronisée (détails : seulement avec systemd-timesyncd ; avec chrony, voir chronyc tracking)"
	else bilan="OK : horloge synchronisée"; fi
	printf 'Bilan|%s\n' "$bilan"
}

dialogue_principal() {
	cat <<XML
<window title="Heure et NTP" default-width="620" default-height="460">
  <vbox spacing="6">
    <tree space-expand="true" space-fill="true">
      <label>Élément|Valeur</label>
      <variable>ETAT</variable>
      <height>360</height>
      <input>exec "\$SERMO_SCRIPT" --etat</input>
    </tree>
    <text><label>Lecture seule. Régler l'heure, le fuseau ou NTP : timedatectl, en administrateur.</label></text>
    <timer interval="2" visible="false">
      <variable>HORLOGE</variable>
      <action>refresh:ETAT</action>
    </timer>
    <hbox><button><label>Fermer</label><action>exit:fermer</action></button></hbox>
  </vbox>
</window>
XML
}

usage() { sed -n '/^# Usage/,/^# Codes/p' "$0" | sed '$d;s/^# \{0,1\}//' >&2; exit 64; }

main() {
	case "${1:-}" in
	--etat) etat; exit ;;
	--xml)  dialogue_principal; exit 0 ;;
	-*)     usage ;;
	esac
	[ -n "${1:-}" ] && SERMO=$1
	command -v "$SERMO" >/dev/null || { echo "sermo introuvable : $SERMO" >&2; exit 2; }
	command -v timedatectl >/dev/null || { echo "timedatectl est absent (paquet systemd)" >&2; exit 2; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE >/dev/null
	exit 0
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
