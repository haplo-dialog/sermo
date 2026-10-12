#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Administrateur
# Besoin   : ouvrir l'un des dix outils d'administration d'une machine Debian depuis une seule fenêtre
# Enrobe   : les dix scripts 01-… à 10-… de ce dossier
# Ports    : gtk3sermo conseillé ; ncursessermo par SSH
# Terminal : rien de perdu
# Usage    : ./lanceur.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            Presque tous les outils demandent les droits root pour agir : ils le disent
#            et affichent la commande à lancer.

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

dialogue_lanceur() {
	local mode=${1:-"à blanc (rien n'est modifié)"}
	cat <<XML
<window title="Boîte à outils : administrateur" default-width="560">
  <vbox>
    <text><label>Choisissez un outil. Il s'ouvre, puis cette fenêtre revient.</label></text>
    <notebook tab-labels="Comptes et fichiers|Logiciels et disques|Système">
      <vbox>
        <frame label="Comptes">
          <vbox>
            <button><label>Groupes et membres (groupadd, gpasswd)</label><action>exit:01-groupes</action></button>
            <button><label>Règles sudo (sudoers.d, visudo -c)</label><action>exit:08-regles-sudo</action></button>
          </vbox>
        </frame>
        <frame label="Fichiers">
          <button><label>Propriétaire, mode et ACL d'un dossier</label><action>exit:02-droits-fichiers</action></button>
        </frame>
      </vbox>
      <vbox>
        <frame label="Logiciels">
          <vbox>
            <button><label>Chercher, installer, retirer un paquet (apt)</label><action>exit:03-paquets</action></button>
            <button><label>Dépôts apt (sources deb822)</label><action>exit:04-depots-apt</action></button>
          </vbox>
        </frame>
        <frame label="Disques">
          <vbox>
            <button><label>Monter un disque (fstab par UUID)</label><action>exit:05-montages</action></button>
            <button><label>Formater un périphérique (mkfs)</label><action>exit:06-formatage</action></button>
          </vbox>
        </frame>
      </vbox>
      <vbox>
        <frame label="Noyau et machine">
          <vbox>
            <button><label>Modules du noyau (modprobe, liste noire)</label><action>exit:07-modules-noyau</action></button>
            <button><label>Nom de la machine (hostnamectl, /etc/hosts)</label><action>exit:09-nom-machine</action></button>
            <button><label>Redémarrer ou éteindre à une heure (shutdown)</label><action>exit:10-redemarrage-programme</action></button>
          </vbox>
        </frame>
      </vbox>
    </notebook>
    <hbox>
      <button cancel></button>
    </hbox>
    <statusbar><variable>ETAT</variable><default>Mode : $(xml "$mode")</default></statusbar>
  </vbox>
</window>
XML
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	lanceur)
		# Abandon (Cancel, abort, closewindow) ou valeur inattendue : on sort.
		[[ $EXIT =~ ^[0-9][0-9]-[a-z0-9-]+$ ]] || exit 0
		local ici; ici=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
		[ -f "$ici/$EXIT.sh" ] || { echo "Outil introuvable : $EXIT" >&2; exit 1; }
		"$ici/$EXIT.sh" "$SERMO"
		exec "$0" "$SERMO"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	local mode="à blanc (rien n'est modifié)"
	[ "$SERMO_A_BLANC" = 1 ] || mode="réel (les commandes sont lancées)"
	export DIALOGUE; DIALOGUE=$(dialogue_lanceur "$mode")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite lanceur' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
