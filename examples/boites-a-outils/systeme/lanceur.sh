#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Système
# Besoin   : régler son poste Linux (écran, son, Wi-Fi, heure, clavier, souris, énergie, démarrage, applications par défaut, Bluetooth) depuis une seule fenêtre
# Enrobe   : les dix outils de ce dossier (01-ecran.sh … 10-bluetooth.sh)
# Ports    : gtk3sermo ou qt6sermo conseillés ; ncursessermo possible (mais la plupart des réglages visent une session graphique)
# Terminal : rien de perdu dans le lanceur lui-même
# Usage    : ./lanceur.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : chaque outil affiche la commande)

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"

# Dossier du script : les outils sont à côté.
ICI=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

# Échappe une valeur avant de l'insérer dans le XML.
xml() { local s=${1//&/"&amp;"}; s=${s//</"&lt;"}; s=${s//>/"&gt;"}; printf '%s' "${s//\"/"&quot;"}"; }
# Le lanceur ne modifie rien : pas de fonction agir ici, chaque outil a la sienne.

dialogue_lanceur() {
	cat <<XML
<window title="Boîte à outils : Système" default-width="560">
  <vbox>
    <text><label>Choisissez un réglage. Chaque outil montre d'abord la commande ; rien ne change sans votre accord.</label></text>
    <notebook tab-labels="Affichage et son|Réseau et périphériques|Session et bureau">
      <vbox>
        <frame label="Écran">
          <hbox>
            <button><label>Écran</label><action>exit:01-ecran</action></button>
            <text><label>Résolution, fréquence, échelle, disposition</label></text>
          </hbox>
        </frame>
        <frame label="Son">
          <hbox>
            <button><label>Son</label><action>exit:02-son</action></button>
            <text><label>Volume, sortie, entrée, muet</label></text>
          </hbox>
        </frame>
        <frame label="Énergie">
          <hbox>
            <button><label>Énergie</label><action>exit:07-energie</action></button>
            <text><label>Mise en veille de l'écran et délais</label></text>
          </hbox>
        </frame>
      </vbox>
      <vbox>
        <frame label="Wi-Fi">
          <hbox>
            <button><label>Wi-Fi</label><action>exit:03-wifi</action></button>
            <text><label>Réseaux connus, connexion, oubli</label></text>
          </hbox>
        </frame>
        <frame label="Bluetooth">
          <hbox>
            <button><label>Bluetooth</label><action>exit:10-bluetooth</action></button>
            <text><label>Appareils, appairage, confiance</label></text>
          </hbox>
        </frame>
        <frame label="Clavier">
          <hbox>
            <button><label>Clavier et langue</label><action>exit:05-clavier-langue</action></button>
            <text><label>Disposition, variante, options</label></text>
          </hbox>
        </frame>
        <frame label="Souris">
          <hbox>
            <button><label>Souris et pavé tactile</label><action>exit:06-souris-pave</action></button>
            <text><label>Vitesse, défilement naturel, tap</label></text>
          </hbox>
        </frame>
      </vbox>
      <vbox>
        <frame label="Date">
          <hbox>
            <button><label>Date et heure</label><action>exit:04-date-heure</action></button>
            <text><label>Fuseau horaire, synchronisation NTP</label></text>
          </hbox>
        </frame>
        <frame label="Démarrage">
          <hbox>
            <button><label>Applications au démarrage</label><action>exit:08-applications-demarrage</action></button>
            <text><label>Activer, désactiver, ajouter</label></text>
          </hbox>
        </frame>
        <frame label="Par défaut">
          <hbox>
            <button><label>Applications par défaut</label><action>exit:09-applications-par-defaut</action></button>
            <text><label>Navigateur, PDF, images</label></text>
          </hbox>
        </frame>
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
		# Abandon (Cancel, abort, closewindow) : on sort.
		[[ "$EXIT" =~ ^[0-9][0-9]-[a-z0-9-]+$ ]] || exit 0
		[ -f "$ICI/$EXIT.sh" ] || { echo "Outil introuvable : $EXIT" >&2; exit 1; }
		"$ICI/$EXIT.sh" "$SERMO"
		exec "$0" "$SERMO"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	export DIALOGUE; DIALOGUE=$(dialogue_lanceur)
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite lanceur' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
