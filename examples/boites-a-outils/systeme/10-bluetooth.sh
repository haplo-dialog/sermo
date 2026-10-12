#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Système
# Besoin   : voir ses appareils Bluetooth, en chercher de nouveaux, les appairer, leur faire confiance, les connecter ou les oublier
# Enrobe   : bluetoothctl (BlueZ)
# Ports    : gtk3sermo ou qt6sermo conseillés ; ncursessermo possible
# Terminal : rien de perdu
# Usage    : ./10-bluetooth.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée ;
#            la recherche d'appareils aussi, car elle fait émettre la radio)
#            Un appareil qui demande de taper un code (certains claviers) s'appaire
#            depuis un terminal, dans « bluetoothctl » en mode interactif.

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

# Arguments : adaptateur allumé (true/false), résumé, puis des lignes
# « adresse|nom|appairé|confiance|connecté ».
dialogue_principal() {
	local allume=${1:-true} resume=${2:-Adaptateur hci0 allumé, visible : non} l
	shift 2 2>/dev/null || set --
	local lignes=("$@")
	[ ${#lignes[@]} -gt 0 ] || lignes=(
		"00:1A:7D:DA:71:13|Casque JBL Tune 510BT|oui|oui|oui"
		"E4:17:D8:2C:09:5B|Souris MX Anywhere 3|oui|oui|non"
		"5C:F3:70:8B:44:A1|Enceinte Salon|non|non|non")
	cat <<XML
<window title="Bluetooth" default-width="640" default-height="460">
  <vbox>
    <hbox><text><label>Bluetooth allumé :</label></text>
      <switch><variable>ALLUME</variable><default>$(xml "$allume")</default></switch>
      <button><label>Chercher des appareils (10 s)</label><action>exit:chercher</action></button></hbox>
    <tree scrollable="true" space-expand="true" space-fill="true">
      <label>Adresse|Nom|Appairé|Confiance|Connecté</label>
      <variable>APPAREIL</variable>
XML
	for l in "${lignes[@]}"; do printf '      <item>%s</item>\n' "$(xml "$l")"; done
	cat <<XML
    </tree>
    <frame label="Sur l'appareil choisi">
      <vbox>
        <hbox>
          <radiobutton><label>Connecter</label><default>true</default><variable>A_CONNECTER</variable></radiobutton>
          <radiobutton><label>Déconnecter</label><variable>A_DECONNECTER</variable></radiobutton>
          <radiobutton><label>Appairer</label><variable>A_APPAIRER</variable></radiobutton>
          <radiobutton><label>Faire confiance</label><variable>A_CONFIANCE</variable></radiobutton>
          <radiobutton><label>Retirer la confiance</label><variable>A_DEFIANCE</variable></radiobutton>
          <radiobutton><label>Oublier</label><variable>A_OUBLIER</variable></radiobutton>
        </hbox>
        <text><label>Confiance : l'appareil se reconnecte seul, sans rien demander.</label></text>
      </vbox>
    </frame>
    <statusbar><variable>RESUME</variable><default>$(xml "$resume")</default></statusbar>
    <hbox>
      <button cancel></button>
      <button><label>Appliquer</label><action>exit:appliquer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_confirmation() {
	local adresse=${1:-5C:F3:70:8B:44:A1} nom=${2:-Enceinte Salon}
	cat <<XML
<window title="Oublier un appareil ?">
  <vbox>
    <text><label>« $(xml "$nom") » ($(xml "$adresse")) sera oublié : appairage et confiance supprimés.</label></text>
    <text><label>Pour le réutiliser, il faudra le remettre en mode appairage et recommencer.</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Oublier</label><action>exit:oublier</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# bluetoothctl attend un démon : on borne chaque lecture.
bt() { timeout 8 bluetoothctl "$@" </dev/null 2>/dev/null; }
oui_non() { case "$1" in yes) echo oui ;; *) echo non ;; esac; }

# « Device AA:BB:… Nom » → « adresse|nom|appairé|confiance|connecté ».
appareils() {
	local mot adr nom info
	while read -r mot adr nom; do
		[ "$mot" = Device ] || continue
		info=$(bt info "$adr")
		printf '%s|%s|%s|%s|%s\n' "$adr" "${nom//|/ }" \
			"$(oui_non "$(awk '$1 == "Paired:" { print $2 }' <<<"$info")")" \
			"$(oui_non "$(awk '$1 == "Trusted:" { print $2 }' <<<"$info")")" \
			"$(oui_non "$(awk '$1 == "Connected:" { print $2 }' <<<"$info")")"
	done < <(bt devices)
}

adresse_valide() { [[ $1 =~ ^([0-9A-F]{2}:){5}[0-9A-F]{2}$ ]] && bt devices | grep -q "^Device $1 "; }

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		case "$EXIT" in
		chercher)
			# La recherche fait émettre la radio : elle passe par agir (à blanc, seulement
			# affichée), puis la liste revient.
			[ "$SERMO_A_BLANC" = 1 ] || echo "Recherche pendant 10 secondes…"
			agir timeout 15 bluetoothctl --timeout 10 scan on </dev/null
			exec "$0" "$SERMO"
			;;
		appliquer) ;;
		*) exit 0 ;;
		esac
		local allume=false
		bt show | grep -q 'Powered: yes' && allume=true
		if [ "$ALLUME" != "$allume" ]; then
			if [ "$ALLUME" = true ]; then agir bluetoothctl power on; else agir bluetoothctl power off; fi
		fi
		[ -n "$APPAREIL" ] || exit 0
		local adr=${APPAREIL%%|*}
		adresse_valide "$adr" || { echo "Appareil inconnu : $adr" >&2; exit 1; }
		if [ "$A_CONNECTER" = true ]; then agir bluetoothctl connect "$adr"
		elif [ "$A_DECONNECTER" = true ]; then agir bluetoothctl disconnect "$adr"
		elif [ "$A_APPAIRER" = true ]; then agir bluetoothctl pair "$adr"
		elif [ "$A_CONFIANCE" = true ]; then agir bluetoothctl trust "$adr"
		elif [ "$A_DEFIANCE" = true ]; then agir bluetoothctl untrust "$adr"
		elif [ "$A_OUBLIER" = true ]; then
			local nom; nom=$(bt devices | awk -v a="$adr" '$2 == a { $1 = $2 = ""; sub(/^ +/, ""); print; exit }')
			export APPAREIL
			export DIALOGUE; DIALOGUE=$(dialogue_confirmation "$adr" "${nom:-sans nom}")
			"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite oubli' >/dev/null
		fi
		;;
	oubli)
		[ "$EXIT" = oublier ] || exit 0
		local adr=${APPAREIL%%|*}
		adresse_valide "$adr" || { echo "Appareil inconnu : $adr" >&2; exit 1; }
		agir bluetoothctl remove "$adr"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v bluetoothctl >/dev/null || { echo "bluetoothctl est absent : installez le paquet bluez." >&2; exit 1; }
	local etat; etat=$(bt show)
	grep -q '^Controller ' <<<"$etat" ||
		{ echo "Aucun adaptateur Bluetooth (ou le service bluetooth est arrêté : systemctl status bluetooth)." >&2; exit 1; }
	local allume=false resume lignes=()
	grep -q 'Powered: yes' <<<"$etat" && allume=true
	resume="Adaptateur $(awk '/^Controller/ { print $2; exit }' <<<"$etat"), allumé : $(oui_non "$(awk '$1 == "Powered:" { print $2; exit }' <<<"$etat")")"
	mapfile -t lignes < <(appareils)
	[ ${#lignes[@]} -gt 0 ] || echo "Aucun appareil connu : la liste montre un exemple. Lancez une recherche." >&2
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$allume" "$resume" "${lignes[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
