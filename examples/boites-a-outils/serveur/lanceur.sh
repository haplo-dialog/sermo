#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Serveur
# Besoin   : choisir l'un des dix outils de la boîte serveur, puis revenir au menu après chaque outil
# Enrobe   : les dix scripts 01-…sh à 10-…sh posés à côté de ce lanceur
# Ports    : ncursessermo conseillé (par SSH, sans écran) ; gtk3sermo sur un poste
# Terminal : rien de perdu (les onglets deviennent des pages ; le menu attend Entrée avant de revenir, pour lire ce que l'outil a écrit)
# Usage    : ./lanceur.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : chaque outil affiche ses commandes)
#            La plupart des outils demandent les droits root pour agir : sinon ils affichent la commande.

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"

# Échappe une valeur avant de l'insérer dans le XML.
xml() { local s=${1//&/"&amp;"}; s=${s//</"&lt;"}; s=${s//>/"&gt;"}; printf '%s' "${s//\"/"&quot;"}"; }

dialogue_lanceur() {
	cat <<XML
<window title="Boîte à outils : serveur" default-width="560">
  <vbox>
    <text><label>Chaque outil montre d'abord la commande ; il n'agit que si SERMO_A_BLANC=0.</label></text>
    <notebook tab-labels="Web|Accès|Entretien">
      <vbox>
        <button><label>01  Site web nginx : fichier de site, nginx -t, rechargement</label><action>exit:01-site-web</action></button>
        <button><label>02  Certificats TLS : demander, renouveler à blanc, lister</label><action>exit:02-certificats</action></button>
        <button><label>03  Ports à l'écoute : voir qui écoute, arrêter le service</label><action>exit:03-ports-ecoute</action></button>
      </vbox>
      <vbox>
        <button><label>04  SSH : durcir sshd par un fichier drop-in</label><action>exit:04-ssh-durcissement</action></button>
        <button><label>05  fail2ban : prisons, adresses bannies, débannir</label><action>exit:05-fail2ban</action></button>
        <button><label>06  Base de données : créer une base et son compte</label><action>exit:06-base-de-donnees</action></button>
        <button><label>08  Partages : export NFS ou partage Samba</label><action>exit:08-partages</action></button>
      </vbox>
      <vbox>
        <button><label>07  Rotation des journaux : règle logrotate</label><action>exit:07-rotation-journaux</action></button>
        <button><label>09  Alertes : seuils disque, charge, mémoire</label><action>exit:09-alertes</action></button>
        <button><label>10  Minuteries systemd : lister, activer, créer</label><action>exit:10-minuteries-systemd</action></button>
      </vbox>
    </notebook>
    <hbox>
      <button cancel></button>
    </hbox>
  </vbox>
</window>
XML
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	lanceur)
		case "$EXIT" in Cancel|abort|closewindow|"") exit 0 ;; esac
		[[ $EXIT =~ ^[0-9][0-9]-[a-z0-9-]+$ ]] || { echo "Choix refusé : $EXIT"; exit 1; }
		local ici
		ici=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
		[ -f "$ici/$EXIT.sh" ] || { echo "Outil introuvable : $ici/$EXIT.sh"; exit 1; }
		"$ici/$EXIT.sh" "$SERMO"
		# Laisser lire ce que l'outil a écrit avant que le menu ne recouvre l'écran.
		if [ -t 0 ]; then read -r -p "Entrée pour revenir au menu… " _; fi
		exec "$0" "$SERMO"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v "$SERMO" >/dev/null || { echo "sermo est absent : installez un port de sermo (ncursessermo par SSH)." >&2; exit 1; }
	export DIALOGUE; DIALOGUE=$(dialogue_lanceur)
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite lanceur' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
