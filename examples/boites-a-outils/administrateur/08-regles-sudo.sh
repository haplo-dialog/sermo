#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Administrateur
# Besoin   : donner à une personne ou à un groupe le droit de lancer quelques commandes précises en root, par une règle vérifiée
# Enrobe   : fichier /etc/sudoers.d/<nom> (install -m 0440) vérifié par visudo -c ; ce script ne lance jamais sudo
# Ports    : gtk3sermo conseillé ; ncursessermo par SSH
# Terminal : rien de perdu
# Usage    : ./08-regles-sudo.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            Écrire sous /etc/sudoers.d demande les droits root : sans eux, le script le dit et
#            affiche la commande. La règle est vérifiée par visudo -c AVANT d'être écrite,
#            puis toute la configuration l'est après.
#            Commandes : liste blanche courte, arguments toujours fixés (une commande
#            sans argument permettrait tous les arguments, donc souvent un shell root).

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"

# Échappe une valeur avant de l'insérer dans le XML.
xml() { local s=${1//&/"&amp;"}; s=${s//</"&lt;"}; s=${s//>/"&gt;"}; printf '%s' "${s//\"/"&quot;"}"; }

# Lance une commande, ou l'affiche seulement en mode à blanc.
# Sans les droits root, ne lance rien : affiche la commande à lancer en root.
agir() {
	if [ "$SERMO_A_BLANC" = 1 ]; then
		printf 'à blanc :'; printf ' %q' "$@"; printf '\n'
	elif [ "$EUID" -ne 0 ]; then
		printf 'Droits root nécessaires. Commande à lancer en root :'; printf ' %q' "$@"; printf '\n'
		return 1
	else
		"$@"
	fi
}

dialogue_principal() {
	cat <<'XML'
<window title="Règle sudo" default-width="600" default-height="520">
  <vbox>
    <frame label="Qui">
      <vbox>
        <hbox>
          <radiobutton><label>Une personne</label><variable>Q_PERSONNE</variable></radiobutton>
          <radiobutton><label>Un groupe</label><default>true</default><variable>Q_GROUPE</variable></radiobutton>
        </hbox>
        <hbox><text><label>Identifiant ou groupe :</label></text>
          <entry space-expand="true" space-fill="true"><variable>QUI</variable><default>impression</default></entry></hbox>
      </vbox>
    </frame>
    <frame label="Commandes permises (une par ligne, arguments fixés)">
      <vbox>
        <text><label>Seules ces formes sont acceptées : /usr/bin/systemctl restart|reload|start|stop UNITÉ.service ; /usr/bin/apt update ; /usr/bin/apt-get update ; /usr/sbin/cupsenable IMPRIMANTE ; /usr/sbin/cupsdisable IMPRIMANTE.</label></text>
        <edit space-expand="true" space-fill="true" height-request="110"><variable>COMMANDES</variable><default>/usr/sbin/cupsenable Bureau-2
/usr/sbin/cupsdisable Bureau-2</default></edit>
      </vbox>
    </frame>
    <frame label="Conditions">
      <vbox>
        <hbox><text><label>En tant que :</label></text>
          <comboboxtext><variable>EN_TANT_QUE</variable><default>root</default>
            <item>root</item><item>www-data</item><item>postgres</item>
          </comboboxtext></hbox>
        <checkbox><label>Sans redemander le mot de passe (NOPASSWD)</label><variable>SANS_MDP</variable></checkbox>
        <togglebutton><label>Journaliser la saisie et la sortie (log_output)</label><variable>JOURNAL</variable></togglebutton>
      </vbox>
    </frame>
    <hbox><text><label>Nom du fichier sous /etc/sudoers.d :</label></text>
      <entry space-expand="true" space-fill="true"><variable>FICHIER</variable><default>50-impression</default></entry></hbox>
    <hbox>
      <button cancel></button>
      <button><label>Vérifier la règle</label><action>exit:verifier</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Arguments : chemin, contenu, compte rendu de visudo -c.
dialogue_apercu() {
	local cible=${1:-/etc/sudoers.d/50-impression}
	local contenu=${2:-"# Écrit par 08-regles-sudo.sh
%impression ALL=(root) /usr/sbin/cupsenable, /usr/sbin/cupsdisable"}
	local verdict=${3:-"stdin : analyse réussie"}
	cat <<XML
<window title="Règle vérifiée" default-width="620" default-height="360">
  <vbox>
    <text><label>Fichier : $(xml "$cible")</label></text>
    <edit editable="false" space-expand="true" space-fill="true"><variable>APERCU</variable><default>$(xml "$contenu")</default></edit>
    <frame label="visudo -c">
      <text><label>$(xml "$verdict")</label></text>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Écrire la règle</label><action>exit:ecrire</action></button>
    </hbox>
  </vbox>
</window>
XML
}

nom_valide() { [[ $1 =~ ^[a-z_][a-z0-9_-]{0,31}$ ]]; }

# LISTE BLANCHE. Dans sudoers, une commande écrite sans argument permet TOUS les
# arguments ; et beaucoup de programmes (find, awk, tee, cp, éditeurs, pagers…)
# ouvrent un shell root. On n'accepte donc que quelques formes connues, toujours
# avec leurs arguments fixés. Rend 0 si la ligne est permise.
commande_permise() {
	local c=$1 unite
	case "$c" in
	"/usr/bin/apt update"|"/usr/bin/apt-get update") return 0 ;;
	"/usr/sbin/cupsenable "*|"/usr/sbin/cupsdisable "*)
		[[ ${c#* } =~ ^[A-Za-z0-9_-]{1,64}$ ]] ;;
	"/usr/bin/systemctl "*)
		# Pas « status » ni « edit » : ils ouvrent un pager ou un éditeur (shell root).
		[[ $c =~ ^/usr/bin/systemctl\ (restart|reload|start|stop)\ ([A-Za-z0-9_.-]{1,80}\.service)$ ]] || return 1
		unite=${BASH_REMATCH[2]}
		# Unités qui arrêtent la machine, ouvrent une console ou gèrent les droits.
		case "$unite" in systemd-*|rescue*|emergency*|halt*|poweroff*|reboot*|kexec*|initrd-*|debug-shell*|console-*|*getty*|autovt*|dbus*|polkit*|sudo*|ssh*)
			return 1 ;; esac ;;
	*) return 1 ;;
	esac
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = verifier ] || exit 0
		nom_valide "$QUI" || { echo "Nom refusé : $QUI" >&2; exit 1; }
		if [ "$Q_GROUPE" = true ]; then
			getent group -- "$QUI" >/dev/null || { echo "Groupe inconnu : $QUI" >&2; exit 1; }
			sujet=%$QUI
		else
			getent passwd -- "$QUI" >/dev/null || { echo "Utilisateur inconnu : $QUI" >&2; exit 1; }
			sujet=$QUI
		fi
		case "$EN_TANT_QUE" in root|www-data|postgres) ;; *) echo "Compte cible refusé : $EN_TANT_QUE" >&2; exit 1 ;; esac
		# sudo ignore les fichiers dont le nom contient un point ou finit par ~.
		[[ $FICHIER =~ ^[A-Za-z0-9_-]{1,40}$ ]] || { echo "Nom de fichier refusé : $FICHIER" >&2; exit 1; }
		cmds=()
		while IFS= read -r c; do
			[ -n "${c// /}" ] || continue
			# Blancs de tête, de queue et doubles ramenés à un seul espace.
			read -ra mots <<<"$c"; c="${mots[*]}"
			commande_permise "$c" || { echo "Commande refusée (hors liste blanche, ou sans arguments fixés) : $c" >&2; exit 1; }
			[ -x "${c%% *}" ] || { echo "Programme introuvable : ${c%% *}" >&2; exit 1; }
			# Toutes les formes permises ont des arguments : on n'écrit jamais un
			# chemin nu. Garde de plus si la liste blanche s'élargit un jour :
			# « chemin "" » veut dire « sans aucun argument » pour sudo.
			[ "$c" = "${c%% *}" ] && c="$c \"\""
			cmds+=("$c")
		done <<<"$COMMANDES"
		[ ${#cmds[@]} -gt 0 ] || { echo "Aucune commande : une règle sans commande précise n'est pas proposée." >&2; exit 1; }
		tag=; [ "$SANS_MDP" = true ] && tag="NOPASSWD: "
		[ "$JOURNAL" = true ] && tag+="LOG_OUTPUT: "
		liste=$(printf '%s, ' "${cmds[@]}"); liste=${liste%, }
		CONTENU="# Écrit par 08-regles-sudo.sh
$sujet ALL=($EN_TANT_QUE) $tag$liste"
		CIBLE=/etc/sudoers.d/$FICHIER
		[ -e "$CIBLE" ] && { echo "Ce fichier existe déjà : $CIBLE" >&2; exit 1; }
		# Vérification de la seule règle, sur l'entrée standard : rien n'est écrit.
		visudo=$(command -v visudo || echo /usr/sbin/visudo)
		[ -x "$visudo" ] || { echo "visudo est absent : installez le paquet sudo." >&2; exit 1; }
		verdict=$("$visudo" -c -f - <<<"$CONTENU" 2>&1) || { echo "visudo refuse la règle :"; echo "$verdict"; exit 1; }
		export CIBLE CONTENU
		export DIALOGUE; DIALOGUE=$(dialogue_apercu "$CIBLE" "$CONTENU" "$verdict")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite apercu' >/dev/null
		;;
	apercu)
		[ "$EXIT" = ecrire ] || exit 0
		[[ $CIBLE =~ ^/etc/sudoers\.d/[A-Za-z0-9_-]{1,40}$ ]] || exit 1
		agir install -m 0440 -o root -g root -T /dev/stdin "$CIBLE" <<<"$CONTENU" || exit 1
		# Toute la configuration relue : si elle casse, on retire le fichier aussitôt.
		agir visudo -c || { echo "visudo -c échoue : retrait de $CIBLE."; agir rm -f -- "$CIBLE"; exit 1; }
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	PATH=$PATH:/usr/sbin:/sbin
	command -v visudo >/dev/null || { echo "visudo est absent : installez le paquet sudo." >&2; exit 1; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
