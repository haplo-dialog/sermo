#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Administrateur système
# Besoin   : créer un compte utilisateur (identifiant, groupes, shell, mot de passe initial)
# Enrobe   : useradd, chpasswd, passwd --expire
# Ports    : gtk3sermo conseillé ; ncursessermo par SSH
# Terminal : rien de perdu
# Usage    : ./2-comptes.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            En vrai, useradd demande les droits root : le script affiche alors la
#            commande à lancer en root au lieu de la lancer.

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"
# useradd et chpasswd sont dans /usr/sbin, absent du PATH d'un simple utilisateur.
PATH="$PATH:/usr/sbin:/sbin"

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

# Comme agir, mais pour une commande qui exige root : sans les droits, on
# affiche la commande à lancer en root et on s'arrête là.
agir_root() {
	if [ "$SERMO_A_BLANC" != 1 ] && [ "$EUID" != 0 ]; then
		printf 'Droits root nécessaires. À lancer en root :'; printf ' %q' "$@"; printf '\n'
		return 1
	fi
	agir "$@"
}

# Arguments : les shells proposés (lus dans /etc/shells par main).
dialogue_principal() {
	local shells=("$@") s
	[ ${#shells[@]} -gt 0 ] || shells=(/bin/bash /bin/sh /usr/bin/zsh)
	cat <<XML
<window title="Nouveau compte" default-width="480">
  <vbox>
    <frame label="Identité">
      <vbox>
        <hbox><text><label>Identifiant :</label></text>
          <entry space-expand="true" space-fill="true"><variable>IDENTIFIANT</variable><default>cdupont</default></entry></hbox>
        <hbox><text><label>Nom complet :</label></text>
          <entry space-expand="true" space-fill="true"><variable>NOM_COMPLET</variable><default>Camille Dupont</default></entry></hbox>
        <hbox><text><label>Shell de connexion :</label></text>
          <comboboxtext space-expand="true" space-fill="true">
            <variable>SHELL_CONNEXION</variable>
            <default>/bin/bash</default>
XML
	for s in "${shells[@]}"; do printf '            <item>%s</item>\n' "$(xml "$s")"; done
	cat <<XML
          </comboboxtext></hbox>
      </vbox>
    </frame>
    <frame label="Groupes supplémentaires">
      <vbox>
        <checkbox><label>users (utilisateurs ordinaires)</label><default>true</default><variable>GR_USERS</variable></checkbox>
        <checkbox><label>audio et video (son, webcam)</label><default>true</default><variable>GR_MULTIMEDIA</variable></checkbox>
        <checkbox><label>plugdev (clés USB, disques amovibles)</label><default>true</default><variable>GR_PLUGDEV</variable></checkbox>
        <checkbox><label>lpadmin (gérer les imprimantes)</label><variable>GR_LPADMIN</variable></checkbox>
        <checkbox><label>adm (lire les journaux système)</label><variable>GR_ADM</variable></checkbox>
      </vbox>
    </frame>
    <frame label="Mot de passe initial">
      <vbox>
        <hbox><text><label>Mot de passe :</label></text>
          <password space-expand="true" space-fill="true"><variable>MDP1</variable></password></hbox>
        <hbox><text><label>Encore une fois :</label></text>
          <password space-expand="true" space-fill="true"><variable>MDP2</variable></password></hbox>
        <checkbox><label>Le changer à la première connexion</label><default>true</default><variable>EXPIRER</variable></checkbox>
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

# Groupes cochés qui existent sur cette machine, séparés par des virgules.
groupes() {
	local g liste=()
	[ "$GR_USERS" = true ] && liste+=(users)
	[ "$GR_MULTIMEDIA" = true ] && liste+=(audio video)
	[ "$GR_PLUGDEV" = true ] && liste+=(plugdev)
	[ "$GR_LPADMIN" = true ] && liste+=(lpadmin)
	[ "$GR_ADM" = true ] && liste+=(adm)
	local ok=()
	for g in "${liste[@]}"; do
		if getent group "$g" >/dev/null; then ok+=("$g"); else echo "Groupe absent ici, ignoré : $g" >&2; fi
	done
	local IFS=,; printf '%s' "${ok[*]}"
}

suite() {
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = creer ] || exit 0
		[[ "$IDENTIFIANT" =~ ^[a-z_][a-z0-9_-]{0,31}$ ]] ||
			{ echo "Identifiant refusé : minuscules, chiffres, - et _, 32 caractères au plus." >&2; exit 1; }
		getent passwd "$IDENTIFIANT" >/dev/null && { echo "Le compte $IDENTIFIANT existe déjà." >&2; exit 1; }
		[[ "$NOM_COMPLET" != *[:,]* ]] || { echo "Le nom complet ne doit contenir ni : ni ,." >&2; exit 1; }
		grep -qxF -- "$SHELL_CONNEXION" /etc/shells || { echo "Shell inconnu de /etc/shells : $SHELL_CONNEXION" >&2; exit 1; }
		[ -n "$MDP1" ] || { echo "Mot de passe vide." >&2; exit 1; }
		[ "$MDP1" = "$MDP2" ] || { echo "Les deux mots de passe diffèrent." >&2; exit 1; }
		[ ${#MDP1} -ge 8 ] || { echo "Mot de passe trop court (8 caractères au moins)." >&2; exit 1; }
		local g; g=$(groupes)
		local cmd=(useradd --create-home --comment "$NOM_COMPLET" --shell "$SHELL_CONNEXION")
		[ -n "$g" ] && cmd+=(--groups "$g")
		agir_root "${cmd[@]}" -- "$IDENTIFIANT" || exit 1
		# Le mot de passe passe par l'entrée standard, jamais sur la ligne de commande.
		[ "$SERMO_A_BLANC" = 1 ] && echo "  (chpasswd lit « identifiant:mot de passe » sur son entrée : rien d'affiché)"
		printf '%s:%s\n' "$IDENTIFIANT" "$MDP1" | agir_root chpasswd || exit 1
		[ "$EXPIRER" = true ] && agir_root passwd --expire -- "$IDENTIFIANT"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	local c
	for c in useradd chpasswd passwd getent; do
		command -v "$c" >/dev/null || { echo "$c est absent : installez le paquet passwd." >&2; exit 1; }
	done
	local shells=()
	mapfile -t shells < <(grep -E '^/' /etc/shells 2>/dev/null | sort -u)
	export DIALOGUE; DIALOGUE=$(dialogue_principal "${shells[@]}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
