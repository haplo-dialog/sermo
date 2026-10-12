#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Administrateur système
# Besoin   : lire le journal système filtré par service, gravité et période
# Enrobe   : journalctl
# Ports    : gtk3sermo conseillé ; ncursessermo par SSH
# Terminal : rien de perdu (le calendrier devient une saisie de date)
# Usage    : ./4-journaux.sh [binaire sermo]
#            Lecture seule : rien n'est modifié, SERMO_A_BLANC est sans effet ici.

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

# Arguments : date du jour (AAAA-MM-JJ), avertissement éventuel sur les droits.
dialogue_principal() {
	local jour=${1:-2026-10-11} avertissement=${2:-}
	cat <<XML
<window title="Journal système" default-width="520">
  <vbox>
    <frame label="Filtres">
      <vbox>
        <hbox><text><label>Service (vide = tous) :</label></text>
          <entry space-expand="true" space-fill="true"><variable>UNITE</variable><default>ssh.service</default></entry></hbox>
        <hbox><text><label>Gravité au moins :</label></text>
          <comboboxtext space-expand="true" space-fill="true">
            <variable>PRIORITE</variable>
            <default>warning</default>
            <item>emerg</item>
            <item>alert</item>
            <item>crit</item>
            <item>err</item>
            <item>warning</item>
            <item>notice</item>
            <item>info</item>
            <item>debug</item>
          </comboboxtext></hbox>
        <checkbox><label>Messages du noyau seulement (-k)</label><variable>NOYAU</variable></checkbox>
      </vbox>
    </frame>
    <frame label="Période">
      <vbox>
        <radiobutton><label>Dernière heure</label><variable>P_HEURE</variable></radiobutton>
        <radiobutton><label>Depuis le dernier démarrage</label><default>true</default><variable>P_DEMARRAGE</variable></radiobutton>
        <radiobutton><label>Depuis la date choisie ci-dessous</label><variable>P_DATE</variable></radiobutton>
        <calendar><variable>DEPUIS</variable><default>$(xml "$jour")</default></calendar>
      </vbox>
    </frame>
    <hbox><text><label>Nombre de lignes au plus :</label></text>
      <spinbutton range-min="10" range-max="5000" range-step="10"><variable>LIGNES</variable><default>300</default></spinbutton>
      <checkbox><label>Les plus récentes d'abord</label><default>true</default><variable>INVERSE</variable></checkbox></hbox>
XML
	[ -n "$avertissement" ] && printf '    <text><label>%s</label></text>\n' "$(xml "$avertissement")"
	cat <<XML
    <hbox>
      <button cancel></button>
      <button><label>Afficher</label><action>exit:afficher</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Arguments : fichier de résultat, commande lancée (pour mémoire).
dialogue_resultat() {
	local fichier=${1:-} commande=${2:-journalctl --no-pager -p warning -b -u ssh.service}
	cat <<XML
<window title="Journal" default-width="760" default-height="520">
  <vbox>
    <text><label>$(xml "$commande")</label></text>
    <edit editable="false" space-expand="true" space-fill="true">
XML
	if [ -n "$fichier" ]; then
		printf '      <input file>%s</input>\n' "$(xml "$fichier")"
	else
		printf '      <default>%s</default>\n' "oct. 11 09:12:03 poste sshd[812]: error: kex_exchange_identification (démonstration)"
	fi
	cat <<XML
    </edit>
    <hbox>
      <button><label>Fermer</label><action>exit:fermer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

suite() {
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = afficher ] || exit 0
		local args=(journalctl --no-pager --quiet)
		case "$PRIORITE" in
		emerg|alert|crit|err|warning|notice|info|debug) args+=(--priority="$PRIORITE") ;;
		*) echo "Gravité inconnue : $PRIORITE" >&2; exit 1 ;;
		esac
		if [ -n "$UNITE" ]; then
			[[ "$UNITE" =~ ^[A-Za-z0-9@._:\\-]+$ && "$UNITE" != -* ]] ||
				{ echo "Nom de service refusé : $UNITE" >&2; exit 1; }
			args+=(--unit="$UNITE")
		fi
		[ "$NOYAU" = true ] && args+=(--dmesg)
		if [ "$P_HEURE" = true ]; then
			args+=(--since=-1h)
		elif [ "$P_DATE" = true ]; then
			[[ "$DEPUIS" =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}$ ]] || { echo "Date illisible : $DEPUIS" >&2; exit 1; }
			args+=(--since="$DEPUIS")
		else
			args+=(--boot)
		fi
		[[ "$LIGNES" =~ ^[0-9]+$ ]] || LIGNES=300
		args+=(--lines="$LIGNES")
		[ "$INVERSE" = true ] && args+=(--reverse)
		local f; f=$(mktemp "${XDG_RUNTIME_DIR:-$HOME}/sermo-journal-XXXXXX") || exit 1
		# Lecture seule : journalctl ne modifie rien.
		"${args[@]}" > "$f" 2>&1
		[ -s "$f" ] || echo "(aucune ligne ne correspond à ces filtres)" > "$f"
		local commande; commande=$(printf '%q ' "${args[@]}")
		echo "$commande"
		export DIALOGUE; DIALOGUE=$(dialogue_resultat "$f" "$commande")
		"$SERMO" --program=DIALOGUE >/dev/null
		rm -f -- "$f"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v journalctl >/dev/null || { echo "journalctl est absent : cette machine n'utilise pas systemd." >&2; exit 1; }
	local avertissement=""
	if [ "$EUID" != 0 ] && ! id -nG | grep -qwE 'adm|systemd-journal'; then
		avertissement="Hors des groupes adm et systemd-journal : seul votre propre journal est visible."
	fi
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$(date +%F)" "$avertissement")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
