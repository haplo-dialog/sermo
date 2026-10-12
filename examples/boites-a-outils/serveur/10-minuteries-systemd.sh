#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Serveur
# Besoin   : voir les tâches planifiées par systemd, en activer ou en couper une, et en créer une nouvelle sans écrire les deux unités à la main
# Enrobe   : systemctl list-timers et systemctl show (lecture) ; systemd-analyze calendar (contrôle) ; tee des unités .service/.timer ; systemctl daemon-reload, enable/disable --now, start
# Ports    : ncursessermo conseillé (par SSH) ; gtk3sermo sur un poste
# Terminal : rien de perdu (les onglets deviennent des pages ; le calendrier devient une saisie de date ; le tableau montre six minuteries à la fois)
# Usage    : ./10-minuteries-systemd.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            Lire ne demande rien ; activer, couper ou créer demande les droits root : sans eux, les commandes sont affichées.

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"

# Échappe une valeur avant de l'insérer dans le XML.
xml() { local s=${1//&/"&amp;"}; s=${s//</"&lt;"}; s=${s//>/"&gt;"}; printf '%s' "${s//\"/"&quot;"}"; }

# Une ligne de texte par <item>, chacune échappée.
items() { local l; while IFS= read -r l; do [ -n "$l" ] && printf '<item>%s</item>\n' "$(xml "$l")"; done <<<"$1"; }

# Lance une commande, ou l'affiche seulement en mode à blanc.
agir() {
	if [ "$SERMO_A_BLANC" = 1 ]; then
		printf 'à blanc :'; printf ' %q' "$@"; printf '\n'
	else
		"$@"
	fi
}

# Sans les droits root, on ne peut qu'afficher les commandes.
exiger_root() {
	if [ "$SERMO_A_BLANC" != 1 ] && [ "$EUID" -ne 0 ]; then
		echo "Agir sur les unités système demande les droits root : relancez ce script en root."
		echo "Voici les commandes qu'il lancerait :"
		SERMO_A_BLANC=1
	fi
}

dialogue_principal() {
	local minuteries=${1:-"apt-daily.timer|Mon 2026-11-02 06:41|Sun 2026-11-01 06:12|enabled
certbot.timer|Mon 2026-11-02 11:07|Sun 2026-11-01 23:52|enabled
logrotate.timer|Tue 2026-11-03 00:00|Mon 2026-11-02 00:00|enabled
alertes-serveur.timer|Mon 2026-11-02 10:15|Mon 2026-11-02 10:00|enabled
e2scrub_all.timer|Sun 2026-11-08 03:10|Sun 2026-11-01 03:10|disabled"}
	cat <<XML
<window title="Minuteries systemd" default-width="640">
  <vbox>
    <notebook tab-labels="Minuteries|Créer">
      <vbox>
        <tree scrollable="true" height-request="170" exported-column="0">
          <label>Minuterie|Prochaine|Dernière|Au démarrage</label>
          <variable>MINUTERIE</variable>
$(items "$minuteries")
        </tree>
        <hbox>
          <button><label>Activer</label><action>exit:activer</action></button>
          <button><label>Couper</label><action>exit:couper</action></button>
          <button><label>Lancer maintenant</label><action>exit:lancer</action></button>
        </hbox>
      </vbox>
      <vbox>
        <hbox><text><label>Nom :</label></text>
          <entry space-expand="true" space-fill="true"><variable>NOM</variable><default>purge-cache</default></entry>
          <text><label>Compte :</label></text>
          <entry><variable>COMPTE</variable><default>www-data</default></entry></hbox>
        <hbox><text><label>Commande :</label></text>
          <entry space-expand="true" space-fill="true"><variable>COMMANDE</variable><default>/usr/local/bin/purge-cache --plus-vieux-que 7</default></entry></hbox>
        <hbox>
          <radiobutton><label>Régulièrement :</label><default>true</default><variable>R_PERIODE</variable></radiobutton>
          <radiobutton><label>Une seule fois, le :</label><variable>R_UNE_FOIS</variable></radiobutton>
        </hbox>
        <hbox>
          <comboboxtext><variable>PERIODE</variable><default>chaque jour</default>
            <item>chaque heure</item><item>chaque jour</item><item>chaque lundi</item><item>le 1er du mois</item>
          </comboboxtext>
          <calendar><variable>JOUR</variable><default>2026-11-02</default></calendar></hbox>
        <hbox><text><label>À (heure, minute) :</label></text>
          <spinbutton range-min="0" range-max="23" range-step="1"><variable>HEURE</variable><default>3</default></spinbutton>
          <spinbutton range-min="0" range-max="59" range-step="5"><variable>MINUTE</variable><default>30</default></spinbutton></hbox>
        <togglebutton><label>Rattraper un passage manqué machine éteinte (Persistent)</label><default>true</default><variable>RATTRAPER</variable></togglebutton>
        <button><label>Voir les unités</label><action>exit:creer</action></button>
      </vbox>
    </notebook>
    <hbox>
      <button cancel></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_couper() {
	local unite=${1:-e2scrub_all.timer}
	cat <<XML
<window title="Couper une minuterie">
  <vbox>
    <text><label>$(xml "$unite") : arrêtée, et plus relancée au démarrage.</label></text>
    <text><label>La tâche qu'elle lance ne tournera plus jusqu'à ce qu'on la réactive.</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Couper</label><action>exit:couper</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Les deux unités arrivent par la variable APERCU_UNITES (printenv ne fait que lire).
dialogue_creation() {
	local nom=${1:-purge-cache} quand=${2:-*-*-* 03:30:00} prochaine=${3:-Tue 2026-11-03 03:30:00 CET}
	cat <<XML
<window title="Nouvelle minuterie" default-width="600">
  <vbox>
    <text><label>Unités /etc/systemd/system/$(xml "$nom").service et .timer</label></text>
    <text><label>Quand : $(xml "$quand") ; prochaine fois : $(xml "$prochaine")</label></text>
    <edit editable="false" height-request="240"><variable>CONTENU</variable><input>printenv APERCU_UNITES</input></edit>
    <hbox>
      <button cancel></button>
      <button><label>Écrire et activer</label><action>exit:ecrire</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# « unité|prochaine|dernière|état » par minuterie (lecture seule, sans droits).
lister() {
	local unites
	mapfile -t unites < <(systemctl list-timers --all --no-legend --no-pager --plain 2>/dev/null |
		awk '{ for (i = 1; i <= NF; i++) if ($i ~ /\.timer$/) print $i }')
	[ ${#unites[@]} -gt 0 ] || return 0
	systemctl show -p Id,NextElapseUSecRealtime,LastTriggerUSec,UnitFileState -- "${unites[@]}" 2>/dev/null |
		awk -F= 'BEGIN { RS = "" } {
			id = p = d = e = "-"
			n = split($0, l, "\n")
			for (i = 1; i <= n; i++) {
				k = substr(l[i], 1, index(l[i], "=") - 1); v = substr(l[i], index(l[i], "=") + 1)
				if (v == "") v = "-"
				if (k == "Id") id = v; else if (k == "NextElapseUSecRealtime") p = v
				else if (k == "LastTriggerUSec") d = v; else if (k == "UnitFileState") e = v
			}
			print id "|" p "|" d "|" e
		}'
}

# L'expression OnCalendar choisie.
quand() {
	local hm
	printf -v hm '%02d:%02d:00' "$HEURE" "$MINUTE"
	if [ "$R_UNE_FOIS" = true ]; then echo "$JOUR $hm"; return; fi
	case "$PERIODE" in
	"chaque heure")   printf '*-*-* *:%02d:00\n' "$MINUTE" ;;
	"chaque lundi")   echo "Mon *-*-* $hm" ;;
	"le 1er du mois") echo "*-*-01 $hm" ;;
	*)                echo "*-*-* $hm" ;;
	esac
}

unites() {
	# « % » introduit un spécificateur systemd : on le double pour qu'il reste un « % ».
	printf '# %s.service\n[Unit]\nDescription=%s (créée par 10-minuteries-systemd.sh)\n\n' "$NOM" "$NOM"
	printf '[Service]\nType=oneshot\nUser=%s\nExecStart=%s\n\n' "$COMPTE" "${COMMANDE//%/%%}"
	printf '# %s.timer\n[Unit]\nDescription=Déclenche %s\n\n[Timer]\nOnCalendar=%s\n' "$NOM" "$NOM" "$QUAND"
	[ "$RATTRAPER" = true ] && [ "$R_UNE_FOIS" != true ] && printf 'Persistent=true\n'
	printf '\n[Install]\nWantedBy=timers.target\n'
}

valider_creation() {
	[[ $NOM =~ ^[a-z0-9][a-z0-9_-]{0,40}$ ]] || { echo "Nom refusé : $NOM (minuscules, chiffres, - et _)"; return 1; }
	[[ $COMPTE =~ ^[a-z_][a-z0-9_-]{0,31}$ ]] || { echo "Compte refusé : $COMPTE"; return 1; }
	getent passwd "$COMPTE" >/dev/null || echo "Note : le compte $COMPTE n'existe pas sur cette machine."
	# Un chemin absolu, puis des arguments, sur une seule ligne.
	[[ $COMMANDE =~ ^/[A-Za-z0-9._/-]+(\ [^[:cntrl:]]*)?$ ]] || { echo "Commande refusée : elle doit commencer par un chemin absolu, sur une ligne."; return 1; }
	[[ $HEURE =~ ^[0-9]+$ && $HEURE -le 23 ]] || { echo "Heure refusée : $HEURE"; return 1; }
	[[ $MINUTE =~ ^[0-9]+$ && $MINUTE -le 59 ]] || { echo "Minute refusée : $MINUTE"; return 1; }
	case "$PERIODE" in "chaque heure"|"chaque jour"|"chaque lundi"|"le 1er du mois") ;; *) echo "Période refusée : $PERIODE"; return 1 ;; esac
	[[ $JOUR =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}$ ]] || { echo "Date refusée : $JOUR"; return 1; }
	QUAND=$(quand)
	# systemd-analyze ne fait que lire l'expression et dire quand elle tombera.
	PROCHAINE=$(systemd-analyze calendar -- "$QUAND" 2>/dev/null | sed -n 's/^ *Next elapse: *//p')
	[ -n "$PROCHAINE" ] || { echo "systemd ne trouve aucune date future pour « $QUAND »."; return 1; }
	return 0
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		case "$EXIT" in
		activer|couper|lancer)
			[[ $MINUTERIE =~ ^[A-Za-z0-9@._:-]+\.timer$ ]] || { echo "Choisissez d'abord une minuterie dans la liste."; exit 1; }
			case "$EXIT" in
			activer) exiger_root; agir systemctl enable --now -- "$MINUTERIE" ;;
			lancer)
				local service
				service=$(systemctl show -p Unit --value -- "$MINUTERIE" 2>/dev/null)
				[[ $service =~ ^[A-Za-z0-9@._:-]+\.service$ ]] || { echo "Service lancé par $MINUTERIE introuvable."; exit 1; }
				exiger_root; agir systemctl start -- "$service" ;;
			couper)
				export MINUTERIE
				export DIALOGUE; DIALOGUE=$(dialogue_couper "$MINUTERIE")
				"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite couper' >/dev/null ;;
			esac
			;;
		creer)
			valider_creation || exit 1
			if [ -e "/etc/systemd/system/$NOM.service" ] || [ -e "/etc/systemd/system/$NOM.timer" ]; then
				echo "Une unité $NOM existe déjà sous /etc/systemd/system : choisissez un autre nom."; exit 1
			fi
			export NOM COMPTE COMMANDE R_PERIODE R_UNE_FOIS PERIODE JOUR HEURE MINUTE RATTRAPER
			export APERCU_UNITES; APERCU_UNITES=$(unites)
			export DIALOGUE; DIALOGUE=$(dialogue_creation "$NOM" "$QUAND" "$PROCHAINE")
			"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite creation' >/dev/null
			;;
		esac
		;;
	couper)
		[ "$EXIT" = couper ] || exit 0
		[[ $MINUTERIE =~ ^[A-Za-z0-9@._:-]+\.timer$ ]] || exit 1
		exiger_root
		agir systemctl disable --now -- "$MINUTERIE"
		;;
	creation)
		[ "$EXIT" = ecrire ] || exit 0
		valider_creation || exit 1
		exiger_root
		unites | sed -n '/^# .*\.timer$/q;p' | agir tee -- "/etc/systemd/system/$NOM.service" || exit 1
		unites | sed -n '/^# .*\.timer$/,$p' | agir tee -- "/etc/systemd/system/$NOM.timer" || exit 1
		agir systemctl daemon-reload
		agir systemctl enable --now -- "$NOM.timer"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v systemctl >/dev/null || { echo "systemd est absent : pas de minuteries à gérer." >&2; exit 1; }
	local liste
	liste=$(lister)
	[ -n "$liste" ] || liste="(aucune minuterie lue)|-|-|-"
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$liste")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
