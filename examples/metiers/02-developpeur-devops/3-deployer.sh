#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Développeur / DevOps
# Besoin   : déployer une version sur un serveur, avec le nom de l'environnement à taper
# Enrobe   : rsync, ssh (ln -sfn, systemctl reload-or-restart)
# Ports    : gtk3sermo conseillé ; ncursessermo par SSH
# Terminal : rien de perdu
# Usage    : ./3-deployer.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            Disposition distante : <dossier>/versions/<version>, lien <dossier>/actuelle.

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"

# Échappe une valeur avant de l'insérer dans le XML.
xml() { local s=${1//&/"&amp;"}; s=${s//</"&lt;"}; s=${s//>/"&gt;"}; printf '%s' "${s//\"/"&quot;"}"; }

# Cite un mot pour un AUTRE shell (/bin/sh, shell distant) : 'texte', chaque ' devenant
# '\''. printf %q ne convient pas : en locale C, il écrit $'…', que dash lit de travers.
shq() { local s=${1//\'/\'\\\'\'}; printf "'%s'" "$s"; }

# Lance une commande, ou l'affiche seulement en mode à blanc.
agir() {
	if [ "$SERMO_A_BLANC" = 1 ]; then
		printf 'à blanc :'; printf ' %q' "$@"; printf '\n'
	else
		"$@"
	fi
}

# Argument : version proposée (git describe du dossier courant).
dialogue_principal() {
	local version=${1:-v2.4.1}
	cat <<XML
<window title="Déployer" default-width="540">
  <vbox>
    <frame label="Où">
      <vbox>
        <hbox>
          <radiobutton><label>recette</label><default>true</default><variable>ENV_RECETTE</variable></radiobutton>
          <radiobutton><label>preprod</label><variable>ENV_PREPROD</variable></radiobutton>
          <radiobutton><label>production</label><variable>ENV_PRODUCTION</variable></radiobutton>
        </hbox>
        <hbox><text><label>Serveur (ssh) :</label></text>
          <entry space-expand="true" space-fill="true"><variable>HOTE</variable><default>deploie@recette.exemple.lan</default></entry></hbox>
        <hbox><text><label>Dossier distant :</label></text>
          <entry space-expand="true" space-fill="true"><variable>CHEMIN</variable><default>/srv/appli</default></entry></hbox>
      </vbox>
    </frame>
    <frame label="Quoi">
      <vbox>
        <hbox><text><label>Version :</label></text>
          <entry space-expand="true" space-fill="true"><variable>VERSION</variable><default>$(xml "$version")</default></entry></hbox>
        <hbox><text><label>Dossier local construit :</label></text>
          <entry space-expand="true" space-fill="true"><variable>SOURCE</variable><default>./dist</default></entry></hbox>
        <checkbox><label>Supprimer dans la version distante ce qui n'existe plus ici (--delete)</label><variable>SUPPRIMER</variable></checkbox>
        <hbox>
          <checkbox><label>Puis recharger le service :</label><default>true</default><variable>RECHARGER</variable></checkbox>
          <entry space-expand="true" space-fill="true"><variable>SERVICE</variable><default>appli.service</default></entry>
        </hbox>
      </vbox>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Préparer le déploiement</label><action>exit:preparer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Arguments : environnement, serveur, version.
dialogue_confirmation() {
	local env=${1:-production} hote=${2:-deploie@prod.exemple.lan} version=${3:-v2.4.1}
	cat <<XML
<window title="Confirmer le déploiement">
  <vbox>
    <text><label>Version $(xml "$version") vers $(xml "$hote") ($(xml "$env")).</label></text>
    <text><label>Pour confirmer, tapez le nom de l'environnement : $(xml "$env")</label></text>
    <entry><variable>CONFIRMATION</variable></entry>
    <hbox>
      <button cancel></button>
      <button><label>Déployer</label><action>exit:deployer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

suite() {
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = preparer ] || exit 0
		ENVIRONNEMENT=recette
		[ "$ENV_PREPROD" = true ] && ENVIRONNEMENT=preprod
		[ "$ENV_PRODUCTION" = true ] && ENVIRONNEMENT=production
		# Rien qui commence par « - » (pris pour une option) ni caractère du shell distant.
		[[ "$HOTE" =~ ^[A-Za-z0-9._@-]+$ && "$HOTE" != -* ]] || { echo "Serveur refusé : $HOTE" >&2; exit 1; }
		[[ "$CHEMIN" =~ ^/[A-Za-z0-9._/-]+$ ]] || { echo "Dossier distant refusé (chemin absolu simple) : $CHEMIN" >&2; exit 1; }
		[[ "$VERSION" =~ ^[A-Za-z0-9._+-]+$ && "$VERSION" != -* ]] || { echo "Version refusée : $VERSION" >&2; exit 1; }
		if [ "$RECHARGER" = true ]; then
			[[ "$SERVICE" =~ ^[A-Za-z0-9@._-]+$ && "$SERVICE" != -* ]] || { echo "Service refusé : $SERVICE" >&2; exit 1; }
		fi
		[ -d "$SOURCE" ] || { echo "Dossier local introuvable : $SOURCE" >&2; exit 1; }
		export ENVIRONNEMENT HOTE CHEMIN VERSION SOURCE SUPPRIMER RECHARGER SERVICE
		export DIALOGUE; DIALOGUE=$(dialogue_confirmation "$ENVIRONNEMENT" "$HOTE" "$VERSION")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite confirmation' >/dev/null
		;;
	confirmation)
		[ "$EXIT" = deployer ] || exit 0
		[ "$CONFIRMATION" = "$ENVIRONNEMENT" ] ||
			{ echo "Confirmation « $CONFIRMATION » différente de « $ENVIRONNEMENT » : rien n'est fait." >&2; exit 1; }
		local cible="$CHEMIN/versions/$VERSION" opts=(-az --human-readable)
		[ "$SUPPRIMER" = true ] && opts+=(--delete)
		agir rsync "${opts[@]}" -- "$SOURCE/" "$HOTE:$cible/" || exit 1
		# La commande distante est une chaîne pour le shell du serveur : chaque mot protégé.
		agir ssh -- "$HOTE" "ln -sfn -- $(shq "$cible") $(shq "$CHEMIN/actuelle")" || exit 1
		[ "$RECHARGER" = true ] && agir ssh -- "$HOTE" "systemctl reload-or-restart -- $(shq "$SERVICE")"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	local c
	for c in rsync ssh; do
		command -v "$c" >/dev/null || { echo "$c est absent : installez le paquet $c (openssh-client pour ssh)." >&2; exit 1; }
	done
	local version
	version=$(git describe --tags --always 2>/dev/null) || version=""
	export DIALOGUE; DIALOGUE=$(dialogue_principal "${version:-1.0.0}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
