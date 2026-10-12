#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Serveur
# Besoin   : durcir le serveur SSH (mot de passe, root, port, comptes permis) sans toucher à sshd_config, et ne recharger que si sshd -t valide
# Enrobe   : tee /etc/ssh/sshd_config.d/10-durcissement.conf ; sshd -t ; systemctl reload ssh
# Ports    : ncursessermo conseillé (par SSH) ; gtk3sermo sur un poste
# Terminal : rien de perdu (l'interrupteur et le bouton à bascule deviennent des cases ; l'aperçu tient sur une ligne) ; gardez une seconde session SSH ouverte pendant l'essai
# Usage    : ./04-ssh-durcissement.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            Écrire sous /etc/ssh demande les droits root : sans eux, les commandes sont affichées.
#            sshd garde la PREMIÈRE valeur lue : le fichier commence par 10- pour passer avant les autres.

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
		echo "Écrire sous /etc/ssh demande les droits root : relancez ce script en root."
		echo "Voici les commandes qu'il lancerait :"
		SERMO_A_BLANC=1
	fi
}

DROPIN=/etc/ssh/sshd_config.d/10-durcissement.conf

dialogue_principal() {
	local comptes=${1:-admin deploiement}
	cat <<XML
<window title="Durcir SSH" default-width="560">
  <vbox>
    <switch><label>Connexion par mot de passe (non = clés seulement)</label><variable>MOT_DE_PASSE</variable><default>false</default></switch>
    <hbox><text><label>Connexion de root :</label></text>
      <comboboxtext><variable>ROOT</variable><default>no</default>
        <item>no</item><item>prohibit-password</item><item>yes</item>
      </comboboxtext></hbox>
    <hbox><text><label>Port :</label></text>
      <spinbutton range-min="1" range-max="65535" range-step="1"><variable>PORT</variable><default>22</default></spinbutton>
      <text><label>Essais permis :</label></text>
      <spinbutton range-min="1" range-max="10" range-step="1"><variable>ESSAIS</variable><default>3</default></spinbutton></hbox>
    <hbox><text><label>Délai pour s'identifier (s) :</label></text>
      <hscale range-min="10" range-max="120" range-step="5" draw-value="true" space-expand="true" space-fill="true"><variable>DELAI</variable><default>30</default></hscale></hbox>
    <hbox><text><label>Comptes permis (vide = tous) :</label></text>
      <entry space-expand="true" space-fill="true"><variable>COMPTES</variable><default>$(xml "$comptes")</default></entry></hbox>
    <togglebutton><label>Renvoi X11 (X11Forwarding)</label><variable>X11</variable></togglebutton>
    <text><label>Avant de couper les mots de passe, vérifiez que votre clé entre déjà.</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Voir le fichier</label><action>exit:apercu</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Le fichier drop-in arrive par la variable APERCU_SSHD (printenv ne fait que lire).
dialogue_confirmation() {
	local avis=${1:-le port reste 22}
	cat <<XML
<window title="Appliquer le durcissement SSH" default-width="560">
  <vbox>
    <text><label>Fichier : $(xml "$DROPIN")</label></text>
    <edit editable="false" height-request="170"><variable>CONTENU</variable><input>printenv APERCU_SSHD</input></edit>
    <text><label>Attention : $(xml "$avis").</label></text>
    <text><label>Gardez une 2e session SSH ouverte jusqu'à l'essai d'une connexion neuve.</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Écrire et recharger</label><action>exit:appliquer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

conf_sshd() {
	printf '# Écrit par 04-ssh-durcissement.sh (boîte à outils serveur)\n'
	printf 'Port %s\n' "$PORT"
	printf 'PermitRootLogin %s\n' "$ROOT"
	if [ "$MOT_DE_PASSE" = true ]; then printf 'PasswordAuthentication yes\n'
	else printf 'PasswordAuthentication no\nKbdInteractiveAuthentication no\n'; fi
	printf 'MaxAuthTries %s\n' "$ESSAIS"
	printf 'LoginGraceTime %s\n' "$DELAI"
	if [ "$X11" = true ]; then printf 'X11Forwarding yes\n'; else printf 'X11Forwarding no\n'; fi
	[ -n "$COMPTES" ] && printf 'AllowUsers %s\n' "$COMPTES"
	return 0
}

valider() {
	case "$ROOT" in no|prohibit-password|yes) ;; *) echo "Valeur refusée pour root : $ROOT"; return 1 ;; esac
	[[ $PORT =~ ^[0-9]+$ ]] && [ "$PORT" -ge 1 ] && [ "$PORT" -le 65535 ] || { echo "Port refusé : $PORT"; return 1; }
	[[ $ESSAIS =~ ^[0-9]+$ ]] && [ "$ESSAIS" -ge 1 ] && [ "$ESSAIS" -le 10 ] || { echo "Nombre d'essais refusé : $ESSAIS"; return 1; }
	DELAI=${DELAI%%.*}
	[[ $DELAI =~ ^[0-9]+$ ]] && [ "$DELAI" -ge 10 ] && [ "$DELAI" -le 120 ] || { echo "Délai refusé : $DELAI"; return 1; }
	# Des noms de comptes séparés par une espace, rien d'autre (une ligne de config, pas deux).
	COMPTES=$(printf '%s' "$COMPTES" | tr -s ' ')
	COMPTES=${COMPTES# }; COMPTES=${COMPTES% }
	[[ -z $COMPTES || $COMPTES =~ ^[a-z_][a-z0-9_-]*(\ [a-z_][a-z0-9_-]*)*$ ]] ||
		{ echo "Comptes refusés : $COMPTES (noms en minuscules séparés par une espace)"; return 1; }
	return 0
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = apercu ] || exit 0
		valider || exit 1
		local avis="le port reste $PORT"
		[ "$PORT" != 22 ] && avis="le port passe à $PORT : ouvrez-le dans le pare-feu AVANT"
		[ "$MOT_DE_PASSE" != true ] && avis="$avis ; sans clé déposée, plus personne n'entre"
		export MOT_DE_PASSE ROOT PORT ESSAIS DELAI COMPTES X11
		export APERCU_SSHD; APERCU_SSHD=$(conf_sshd)
		export DIALOGUE; DIALOGUE=$(dialogue_confirmation "$avis")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite confirmation' >/dev/null
		;;
	confirmation)
		[ "$EXIT" = appliquer ] || exit 0
		valider || exit 1
		exiger_root
		conf_sshd | agir tee -- "$DROPIN" || exit 1
		if ! agir /usr/sbin/sshd -t; then
			echo "sshd -t refuse la configuration : le fichier est retiré, rien n'est rechargé."
			agir rm -f -- "$DROPIN"
			exit 1
		fi
		agir systemctl reload ssh
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	[ -x /usr/sbin/sshd ] || { echo "sshd est absent : installez le paquet openssh-server." >&2; exit 1; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
