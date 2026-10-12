#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Serveur
# Besoin   : empêcher le dossier de journaux d'une application de remplir le disque, avec une règle logrotate essayée avant d'être laissée en place
# Enrobe   : tee /etc/logrotate.d/<nom> ; logrotate --debug (essai sans rien tourner)
# Ports    : ncursessermo conseillé (par SSH) ; gtk3sermo sur un poste
# Terminal : rien de perdu (la glissière devient une barre de caractères ; l'aperçu tient sur une ligne)
# Usage    : ./07-rotation-journaux.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            Écrire sous /etc/logrotate.d demande les droits root : sans eux, les commandes sont affichées.

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
		echo "Écrire sous /etc/logrotate.d demande les droits root : relancez ce script en root."
		echo "Voici les commandes qu'il lancerait :"
		SERMO_A_BLANC=1
	fi
}

dialogue_principal() {
	local nom=${1:-appli-stock} dossier=${2:-/srv/appli-stock/journaux}
	cat <<XML
<window title="Rotation des journaux" default-width="560">
  <vbox>
    <hbox><text><label>Nom de la règle :</label></text>
      <entry space-expand="true" space-fill="true"><variable>NOM</variable><default>$(xml "$nom")</default></entry></hbox>
    <hbox><text><label>Dossier des journaux :</label></text>
      <entry space-expand="true" space-fill="true"><variable>DOSSIER</variable><default>$(xml "$dossier")</default></entry></hbox>
    <hbox><text><label>Fichiers visés :</label></text>
      <entry space-expand="true" space-fill="true"><variable>MOTIF</variable><default>*.log</default></entry></hbox>
    <hbox><text><label>Fréquence :</label></text>
      <comboboxtext><variable>FREQUENCE</variable><default>chaque semaine</default>
        <item>chaque jour</item><item>chaque semaine</item><item>chaque mois</item>
      </comboboxtext>
      <text><label>ou dès (Mo) :</label></text>
      <spinbutton range-min="0" range-max="10240" range-step="10" tooltip-text="0 : pas de seuil de taille"><variable>TAILLE</variable><default>100</default></spinbutton></hbox>
    <hbox><text><label>Anciens journaux gardés :</label></text>
      <hscale range-min="1" range-max="52" range-step="1" draw-value="true" space-expand="true" space-fill="true"><variable>GARDER</variable><default>8</default></hscale></hbox>
    <checkbox><label>Compresser les anciens (en gardant le dernier en clair)</label><default>true</default><variable>COMPRESSER</variable></checkbox>
    <checkbox><label>Copier puis vider (l'appli garde son fichier ouvert)</label><variable>COPIER</variable></checkbox>
    <checkbox><label>Ne rien dire si le dossier est vide ou absent</label><default>true</default><variable>TOLERER</variable></checkbox>
    <hbox><text><label>Propriétaire des nouveaux fichiers :</label></text>
      <entry space-expand="true" space-fill="true"><variable>PROPRIETE</variable><default>0640 www-data adm</default></entry></hbox>
    <hbox>
      <button cancel></button>
      <button><label>Voir la règle</label><action>exit:apercu</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# La règle arrive par la variable APERCU_REGLE (printenv ne fait que lire).
dialogue_confirmation() {
	local chemin=${1:-/etc/logrotate.d/appli-stock} etat=${2:-nouveau fichier}
	cat <<XML
<window title="Règle logrotate" default-width="560">
  <vbox>
    <text><label>Fichier : $(xml "$chemin") ($(xml "$etat"))</label></text>
    <edit editable="false" height-request="220"><variable>CONTENU</variable><input>printenv APERCU_REGLE</input></edit>
    <checkbox><label>Essayer aussitôt avec logrotate --debug (rien ne tourne)</label><default>true</default><variable>ESSAYER</variable></checkbox>
    <hbox>
      <button cancel></button>
      <button><label>Écrire la règle</label><action>exit:ecrire</action></button>
    </hbox>
  </vbox>
</window>
XML
}

regle() {
	local frequence=weekly
	case "$FREQUENCE" in "chaque jour") frequence=daily ;; "chaque mois") frequence=monthly ;; esac
	printf '# Écrit par 07-rotation-journaux.sh (boîte à outils serveur)\n'
	printf '%s/%s {\n' "$DOSSIER" "$MOTIF"
	printf '    %s\n' "$frequence"
	[ "$TAILLE" != 0 ] && printf '    maxsize %sM\n' "$TAILLE"
	printf '    rotate %s\n' "$GARDER"
	[ "$COMPRESSER" = true ] && printf '    compress\n    delaycompress\n'
	[ "$COPIER" = true ] && printf '    copytruncate\n'
	[ "$TOLERER" = true ] && printf '    missingok\n    notifempty\n'
	[ -n "$PROPRIETE" ] && [ "$COPIER" != true ] && printf '    create %s\n' "$PROPRIETE"
	printf '    dateext\n}\n'
}

valider() {
	[[ $NOM =~ ^[a-z0-9][a-z0-9-]{0,40}$ ]] || { echo "Nom refusé : $NOM (minuscules, chiffres, tirets)"; return 1; }
	[[ $DOSSIER =~ ^/[A-Za-z0-9._/-]+$ && $DOSSIER != *..* ]] || { echo "Dossier refusé : $DOSSIER"; return 1; }
	# logrotate tourne en root chaque nuit : chemin réel (« //etc », liens suivis), puis
	# liste blanche. Jamais /etc ni un dossier système, ni la racine d'une des quatre.
	local reel
	reel=$(realpath -m -- "$DOSSIER") || { echo "Dossier illisible : $DOSSIER"; return 1; }
	case "$reel" in
	/var/log/?*|/srv/?*|/opt/?*|/var/www/?*) DOSSIER=$reel ;;
	*) echo "Dossier refusé : $DOSSIER (= $reel) ; seuls /var/log/…, /srv/…, /opt/… et /var/www/… sont permis."; return 1 ;;
	esac
	# Un nom de fichier, jamais un chemin : pas de « / » (la classe ci-dessous l'exclut).
	[[ $MOTIF =~ ^[A-Za-z0-9*?._-]{1,40}$ && $MOTIF != */* ]] || { echo "Motif refusé : $MOTIF"; return 1; }
	case "$FREQUENCE" in "chaque jour"|"chaque semaine"|"chaque mois") ;; *) echo "Fréquence refusée : $FREQUENCE"; return 1 ;; esac
	[[ $TAILLE =~ ^[0-9]+$ ]] && [ "$TAILLE" -le 10240 ] || { echo "Taille refusée : $TAILLE"; return 1; }
	GARDER=${GARDER%%.*}
	[[ $GARDER =~ ^[0-9]+$ ]] && [ "$GARDER" -ge 1 ] && [ "$GARDER" -le 52 ] || { echo "Nombre gardé refusé : $GARDER"; return 1; }
	# « mode propriétaire groupe », trois mots simples, ou vide.
	[[ -z $PROPRIETE || $PROPRIETE =~ ^[0-7]{3,4}\ [a-z_][a-z0-9_-]*\ [a-z_][a-z0-9_-]*$ ]] ||
		{ echo "Propriété refusée : $PROPRIETE (forme : 0640 www-data adm)"; return 1; }
	return 0
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	local chemin
	case "$1" in
	principal)
		[ "$EXIT" = apercu ] || exit 0
		valider || exit 1
		chemin=/etc/logrotate.d/$NOM
		local etat="nouveau fichier"
		[ -e "$chemin" ] && etat="REMPLACE le fichier existant"
		[ -d "$DOSSIER" ] || echo "Note : $DOSSIER n'existe pas encore."
		export NOM DOSSIER MOTIF FREQUENCE TAILLE GARDER COMPRESSER COPIER TOLERER PROPRIETE
		export APERCU_REGLE; APERCU_REGLE=$(regle)
		export DIALOGUE; DIALOGUE=$(dialogue_confirmation "$chemin" "$etat")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite confirmation' >/dev/null
		;;
	confirmation)
		[ "$EXIT" = ecrire ] || exit 0
		valider || exit 1
		chemin=/etc/logrotate.d/$NOM
		exiger_root
		regle | agir tee -- "$chemin" || exit 1
		# --debug lit la règle et dit ce qu'il ferait, sans rien tourner.
		[ "$ESSAYER" = true ] && agir /usr/sbin/logrotate --debug -- "$chemin"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	[ -x /usr/sbin/logrotate ] || { echo "logrotate est absent : installez le paquet logrotate." >&2; exit 1; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
