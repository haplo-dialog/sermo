#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Intégrateur de distribution
# Besoin   : créer le premier compte du poste (nom, identifiant, mot de passe saisi deux fois, administrateur ou non)
# Enrobe   : useradd, chpasswd
# Ports    : gtk3sermo conseillé ; ncursessermo sur une console sans bureau
# Terminal : rien de perdu (le mot de passe reste masqué)
# Usage    : ./3-utilisateur-initial.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            La création demande les droits d'administration : lancer alors en root.
#            Le mot de passe passe par l'entrée standard de chpasswd, jamais par la ligne de commande.

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

# Chemin d'un outil d'administration : /usr/sbin n'est pas toujours dans le PATH d'un utilisateur.
outil() { command -v "$1" 2>/dev/null || { [ -x "/usr/sbin/$1" ] && echo "/usr/sbin/$1"; }; }

# Le groupe des administrateurs porte le GID 27 sur Debian (base-passwd) : il est lu, pas supposé.
dialogue_principal() {
	local groupe=${1:-administrateurs} message=${2:-} nom=${3-Camille Durand} ident=${4-camille}
	cat <<XML
<window title="Compte de l'utilisateur" default-width="520">
  <vbox>
    <text><label>Créez le compte de la personne qui utilisera ce poste.</label></text>
$([ -n "$message" ] && printf '    <text><label>%s</label></text>' "$(xml "$message")")
    <frame label="Identité">
      <vbox>
        <hbox><text><label>Nom complet :</label></text>
          <entry space-expand="true" space-fill="true"><variable>NOM_COMPLET</variable><default>$(xml "$nom")</default></entry></hbox>
        <hbox><text><label>Identifiant (minuscules) :</label></text>
          <entry space-expand="true" space-fill="true"><variable>IDENTIFIANT</variable><default>$(xml "$ident")</default></entry></hbox>
      </vbox>
    </frame>
    <frame label="Mot de passe (8 caractères au moins)">
      <vbox>
        <hbox><text><label>Mot de passe :</label></text>
          <password space-expand="true" space-fill="true"><variable>MDP1</variable></password></hbox>
        <hbox><text><label>Le retaper :</label></text>
          <password space-expand="true" space-fill="true"><variable>MDP2</variable></password></hbox>
      </vbox>
    </frame>
    <frame label="Droits et interpréteur">
      <vbox>
        <checkbox><label>Administrateur du poste (groupe $(xml "$groupe"))</label><variable>ADMIN</variable><default>true</default></checkbox>
        <hbox><text><label>Interpréteur de commandes :</label></text>
          <comboboxtext><variable>COQUILLE</variable><default>/bin/bash</default>
            <item>/bin/bash</item><item>/bin/sh</item><item>/usr/bin/zsh</item><item>/usr/bin/fish</item>
          </comboboxtext></hbox>
      </vbox>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Créer le compte</label><action>exit:creer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Redemande la saisie avec un message ; les mots de passe sont à retaper.
redemander() {
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$GROUPE_ADMIN" "$1" "$NOM_COMPLET" "$IDENTIFIANT")
	unset MDP1 MDP2
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
	exit
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = creer ] || exit 0
		[[ "$IDENTIFIANT" =~ ^[a-z_][a-z0-9_-]{0,31}$ ]] ||
			redemander "Identifiant refusé : minuscules, chiffres, - et _, 32 caractères au plus."
		getent passwd "$IDENTIFIANT" >/dev/null && redemander "Le compte « $IDENTIFIANT » existe déjà."
		[[ "$NOM_COMPLET" == *[:,=]* ]] && redemander "Le nom complet ne peut contenir ni « : » ni « , » ni « = »."
		[ "$MDP1" = "$MDP2" ] || redemander "Les deux mots de passe diffèrent."
		[ ${#MDP1} -ge 8 ] || redemander "Mot de passe trop court : 8 caractères au moins."
		[ -x "$COQUILLE" ] || redemander "Interpréteur absent de ce poste : $COQUILLE"
		local groupes=()
		[ "$ADMIN" = true ] && [ -n "$GROUPE_ADMIN" ] && groupes=(-G "$GROUPE_ADMIN")
		[ "$(id -u)" = 0 ] || echo "Droits d'administration nécessaires : lancez ce script en root pour créer le compte."
		agir "$(outil useradd)" -m -c "$NOM_COMPLET" -s "$COQUILLE" "${groupes[@]}" -- "$IDENTIFIANT"
		# Le mot de passe va sur l'entrée standard : il n'apparaît ni à l'écran ni dans ps.
		printf '%s:%s\n' "$IDENTIFIANT" "$MDP1" | agir "$(outil chpasswd)"
		unset MDP1 MDP2
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	for c in useradd chpasswd getent; do
		[ -n "$(outil "$c")" ] || { echo "$c est absent : installez passwd (useradd, chpasswd) ou libc-bin (getent)." >&2; exit 1; }
	done
	export GROUPE_ADMIN; GROUPE_ADMIN=$(getent group 27 | cut -d: -f1)
	export DIALOGUE; DIALOGUE=$(dialogue_principal "${GROUPE_ADMIN:-introuvable}" "" "" "")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
