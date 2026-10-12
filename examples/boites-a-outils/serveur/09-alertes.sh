#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Serveur
# Besoin   : être prévenu par courriel avant que le disque, la mémoire ou la charge ne fassent tomber le serveur, sans installer d'outil de supervision
# Enrobe   : écrit /usr/local/sbin/alertes-serveur + une unité .service et .timer ; systemctl daemon-reload ; systemctl enable --now ; mail (bsd-mailx) pour l'envoi
# Ports    : ncursessermo conseillé (par SSH) ; gtk3sermo sur un poste
# Terminal : rien de perdu (les glissières deviennent des barres de caractères)
# Usage    : ./09-alertes.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            Écrire sous /usr/local/sbin et /etc/systemd demande les droits root : sans eux, les commandes sont affichées.
#            Sans commande mail sur le serveur, les alertes vont seulement au journal (journalctl -t alertes-serveur).

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

# Sans les droits root, on ne peut qu'afficher les commandes.
exiger_root() {
	if [ "$SERMO_A_BLANC" != 1 ] && [ "$EUID" -ne 0 ]; then
		echo "Écrire sous /usr/local/sbin et /etc/systemd demande les droits root : relancez ce script en root."
		echo "Voici les commandes qu'il lancerait :"
		SERMO_A_BLANC=1
	fi
}

CONTROLE=/usr/local/sbin/alertes-serveur
UNITES=/etc/systemd/system/alertes-serveur

dialogue_principal() {
	local adresse=${1:-admin@exemple.org}
	cat <<XML
<window title="Alertes du serveur" default-width="580">
  <vbox>
    <text><label>Prévenir quand :</label></text>
    <hbox>
      <checkbox><label>le disque est plein à (%)</label><default>true</default><variable>V_DISQUE</variable></checkbox>
      <hscale range-min="50" range-max="99" range-step="1" draw-value="true" space-expand="true" space-fill="true"><variable>DISQUE</variable><default>90</default></hscale></hbox>
    <hbox>
      <checkbox><label>la mémoire est prise à (%)</label><default>true</default><variable>V_MEMOIRE</variable></checkbox>
      <hscale range-min="50" range-max="99" range-step="1" draw-value="true" space-expand="true" space-fill="true"><variable>MEMOIRE</variable><default>92</default></hscale></hbox>
    <hbox>
      <checkbox><label>la charge sur 5 min dépasse</label><default>true</default><variable>V_CHARGE</variable></checkbox>
      <spinbutton range-min="1" range-max="256" range-step="1"><variable>CHARGE</variable><default>8</default></spinbutton></hbox>
    <hbox><text><label>Adresse d'alerte :</label></text>
      <entry space-expand="true" space-fill="true"><variable>ADRESSE</variable><default>$(xml "$adresse")</default></entry></hbox>
    <hbox><text><label>Vérifier :</label></text>
      <comboboxtext><variable>FREQUENCE</variable><default>toutes les 15 min</default>
        <item>toutes les 5 min</item><item>toutes les 15 min</item><item>toutes les heures</item>
      </comboboxtext></hbox>
    <expander label="Avancé">
      <vbox>
        <hbox><text><label>Points de montage surveillés :</label></text>
          <entry space-expand="true" space-fill="true"><variable>POINTS</variable><default>/ /var /srv</default></entry></hbox>
        <checkbox><label>Aussi dans le journal (logger -t alertes-serveur)</label><default>true</default><variable>JOURNAL</variable></checkbox>
      </vbox>
      <variable>AVANCE</variable>
    </expander>
    <hbox>
      <button cancel></button>
      <button><label>Installer le contrôle</label><action>exit:installer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_confirmation() {
	local resume=${1:-disque 90 %, mémoire 92 %, charge 8 ; toutes les 15 min} adresse=${2:-admin@exemple.org}
	cat <<XML
<window title="Installer les alertes">
  <vbox>
    <text><label>Seuils : $(xml "$resume").</label></text>
    <text><label>Courriel vers : $(xml "$adresse").</label></text>
    <text><label>Fichiers écrits, remplacés s'ils existent :</label></text>
    <text><label>$(xml "$CONTROLE")</label></text>
    <text><label>$(xml "$UNITES").service et .timer</label></text>
    <checkbox><label>Lancer un premier contrôle tout de suite</label><default>true</default><variable>ESSAI</variable></checkbox>
    <hbox>
      <button cancel></button>
      <button><label>Écrire et activer</label><action>exit:activer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Le script de contrôle : seuils en tête (valeurs déjà validées), logique fixe ensuite.
script_controle() {
	printf '#!/bin/sh\n# Écrit par 09-alertes.sh (boîte à outils serveur). Seuil vide = contrôle coupé.\n'
	printf 'SEUIL_DISQUE=%s\nSEUIL_MEMOIRE=%s\nSEUIL_CHARGE=%s\n' "$S_DISQUE" "$S_MEMOIRE" "$S_CHARGE"
	printf "ADRESSE='%s'\nPOINTS='%s'\nJOURNAL=%s\n" "$ADRESSE" "$POINTS" "$JOURNAL"
	cat <<'SCRIPT'
msg=""
if [ -n "$SEUIL_DISQUE" ]; then
	for p in $POINTS; do
		u=$(df --output=pcent -- "$p" 2>/dev/null | tail -n 1 | tr -dc 0-9)
		[ -n "$u" ] && [ "$u" -ge "$SEUIL_DISQUE" ] && msg="$msg
- disque $p plein à $u % (seuil $SEUIL_DISQUE %)"
	done
fi
if [ -n "$SEUIL_MEMOIRE" ]; then
	m=$(awk '/^MemTotal:/ {t = $2} /^MemAvailable:/ {a = $2} END {if (t) printf "%d", (t - a) * 100 / t}' /proc/meminfo)
	[ -n "$m" ] && [ "$m" -ge "$SEUIL_MEMOIRE" ] && msg="$msg
- mémoire prise à $m % (seuil $SEUIL_MEMOIRE %)"
fi
if [ -n "$SEUIL_CHARGE" ]; then
	c=$(cut -d ' ' -f 2 /proc/loadavg)
	[ "${c%.*}" -ge "$SEUIL_CHARGE" ] && msg="$msg
- charge sur 5 min à $c (seuil $SEUIL_CHARGE)"
fi
[ -n "$msg" ] || exit 0
machine=$(hostname)
[ "$JOURNAL" = true ] && printf '%s\n' "$msg" | logger -t alertes-serveur
if command -v mail >/dev/null; then
	printf 'Alertes sur %s :%s\n' "$machine" "$msg" | mail -s "Alerte serveur $machine" "$ADRESSE"
fi
exit 0
SCRIPT
}

unite_service() {
	printf '[Unit]\nDescription=Contrôle des seuils disque, mémoire et charge\n\n'
	printf '[Service]\nType=oneshot\nExecStart=%s\nNice=10\n' "$CONTROLE"
}

unite_timer() {
	printf '[Unit]\nDescription=Contrôle régulier des seuils du serveur\n\n'
	printf '[Timer]\nOnBootSec=5min\nOnUnitActiveSec=%s\n\n[Install]\nWantedBy=timers.target\n' "$PERIODE"
}

# Contrôle les saisies ; pose S_DISQUE, S_MEMOIRE, S_CHARGE (vides si coupés) et PERIODE.
valider() {
	local v
	for v in DISQUE MEMOIRE; do
		printf -v "$v" '%s' "${!v%%.*}"
		[[ ${!v} =~ ^[0-9]+$ ]] && [ "${!v}" -ge 50 ] && [ "${!v}" -le 99 ] || { echo "Seuil refusé : $v=${!v}"; return 1; }
	done
	[[ $CHARGE =~ ^[0-9]+$ ]] && [ "$CHARGE" -ge 1 ] && [ "$CHARGE" -le 256 ] || { echo "Charge refusée : $CHARGE"; return 1; }
	S_DISQUE=; S_MEMOIRE=; S_CHARGE=
	[ "$V_DISQUE" = true ] && S_DISQUE=$DISQUE
	[ "$V_MEMOIRE" = true ] && S_MEMOIRE=$MEMOIRE
	[ "$V_CHARGE" = true ] && S_CHARGE=$CHARGE
	[ -n "$S_DISQUE$S_MEMOIRE$S_CHARGE" ] || { echo "Aucun contrôle coché : rien à installer."; return 1; }
	[[ $ADRESSE =~ ^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$ && $ADRESSE != -* ]] || { echo "Adresse refusée : $ADRESSE"; return 1; }
	# Points de montage : chemins absolus simples, séparés par une espace (ils finissent entre apostrophes).
	[[ $POINTS =~ ^/[A-Za-z0-9._/-]*(\ /[A-Za-z0-9._/-]*)*$ ]] || { echo "Points de montage refusés : $POINTS"; return 1; }
	case "$FREQUENCE" in
	"toutes les 5 min") PERIODE=5min ;;
	"toutes les 15 min") PERIODE=15min ;;
	"toutes les heures") PERIODE=1h ;;
	*) echo "Fréquence refusée : $FREQUENCE"; return 1 ;;
	esac
	[ "$JOURNAL" = true ] || JOURNAL=false
	return 0
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = installer ] || exit 0
		valider || exit 1
		export V_DISQUE V_MEMOIRE V_CHARGE DISQUE MEMOIRE CHARGE ADRESSE FREQUENCE POINTS JOURNAL
		local resume="disque ${S_DISQUE:-coupé} %, mémoire ${S_MEMOIRE:-coupée} %, charge ${S_CHARGE:-coupée} ; $FREQUENCE"
		export DIALOGUE; DIALOGUE=$(dialogue_confirmation "$resume" "$ADRESSE")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite confirmation' >/dev/null
		;;
	confirmation)
		[ "$EXIT" = activer ] || exit 0
		valider || exit 1
		exiger_root
		command -v mail >/dev/null || echo "Note : pas de commande mail ici (paquet bsd-mailx) ; les alertes iront au journal seulement."
		script_controle | agir tee -- "$CONTROLE" || exit 1
		agir chmod 0755 -- "$CONTROLE"
		unite_service | agir tee -- "$UNITES.service" || exit 1
		unite_timer | agir tee -- "$UNITES.timer" || exit 1
		agir systemctl daemon-reload
		agir systemctl enable --now alertes-serveur.timer
		[ "$ESSAI" = true ] && agir systemctl start alertes-serveur.service
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v systemctl >/dev/null || { echo "systemd est absent : ce contrôle s'appuie sur une minuterie systemd." >&2; exit 1; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
