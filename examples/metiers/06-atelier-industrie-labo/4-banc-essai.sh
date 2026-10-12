#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Atelier, industrie, laboratoire
# Besoin   : régler et lancer un essai sur banc (profil, tension, température, durée), puis suivre son avancement
# Enrobe   : le pilote du banc ($BANC_CMD, fourni avec le banc) ; tail -F pour suivre l'avancement
# Ports    : gtk3sermo conseillé ; lvglsermo sur l'écran du banc
# Terminal : rien de perdu (la barre de progression existe aussi en ncurses)
# Usage    : ./4-banc-essai.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            BANC_CMD : pilote du banc (défaut banc-essai). Il reçoit les réglages en options et
#            écrit son avancement (un entier de 0 à 100 par ligne) dans le fichier --avancement.
#            BANC_DONNEES : dossier des essais (défaut ~/banc)

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"
export BANC_CMD="${BANC_CMD:-banc-essai}"
export BANC_DONNEES="${BANC_DONNEES:-$HOME/banc}"

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

dialogue_principal() {
	local operateur=${1:-mdupuis}
	cat <<XML
<window title="Banc d'essai" default-width="560">
  <vbox>
    <frame label="Éprouvette">
      <grid columns="2" row-spacing="6" column-spacing="8">
        <text><label>N° d'éprouvette :</label></text>
        <entry space-expand="true" space-fill="true"><variable>EPROUVETTE</variable><default>EP-2026-0412</default></entry>
        <text><label>Opérateur :</label></text>
        <entry space-expand="true" space-fill="true"><variable>OPERATEUR</variable><default>$(xml "$operateur")</default></entry>
        <text><label>Profil d'essai :</label></text>
        <comboboxtext space-expand="true" space-fill="true"><variable>PROFIL</variable><default>Endurance</default>
          <item>Endurance</item><item>Cyclage thermique</item><item>Surtension</item><item>Vibrations</item>
        </comboboxtext>
      </grid>
    </frame>
    <frame label="Consignes">
      <vbox>
        <hbox>
          <text><label>Tension d'alimentation (V) :</label></text>
          <spinbutton range-min="0" range-max="400" range-step="1"><variable>TENSION</variable><default>230</default></spinbutton>
          <text><label>Durée (min) :</label></text>
          <spinbutton range-min="1" range-max="1440" range-step="1"><variable>DUREE</variable><default>30</default></spinbutton>
        </hbox>
        <text><label>Température d'enceinte (°C) :</label></text>
        <hscale range-min="-40" range-max="150" range-step="1" draw-value="true"><variable>TEMPERATURE</variable><default>23</default></hscale>
        <checkbox><label>Essai destructif : l'éprouvette ne sera pas réutilisable</label><variable>DESTRUCTIF</variable></checkbox>
      </vbox>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Lancer l'essai</label><action>exit:lancer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_confirmation() {
	local eprouvette=${1:-EP-2026-0412}
	cat <<XML
<window title="Essai destructif">
  <vbox>
    <text><label>L'éprouvette $(xml "$eprouvette") sera détruite par cet essai.</label></text>
    <text><label>Vérifiez que l'enceinte est fermée et que personne n'est à proximité.</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Lancer quand même</label><action>exit:confirmer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Arguments : fichier d'avancement, durée en minutes.
dialogue_suivi() {
	local fichier=${1:-$HOME/banc/EP-2026-0412/avancement} duree=${2:-30}
	# Lecture seule : tail suit le fichier que le pilote remplit, borné à la durée prévue + 5 min.
	local suivre
	suivre="timeout $(( (duree + 5) * 60 )) tail -n 1 -F -- $(shq "$fichier")"
	cat <<XML
<window title="Essai en cours" default-width="480">
  <vbox>
    <text><label>Essai lancé pour $(xml "$duree") min. Avancement :</label></text>
    <progressbar>
      <label>Essai en cours</label>
      <input>$(xml "$suivre")</input>
    </progressbar>
    <text><label>Fermer cette fenêtre n'arrête pas l'essai.</label></text>
    <hbox>
      <button><label>Fermer le suivi</label><action>exit:fermer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

lancer() {
	local dossier="$BANC_DONNEES/$EPROUVETTE-$(date +%Y%m%d-%H%M)"
	local essai=("$BANC_CMD" --profil "$PROFIL" --tension "$TENSION" --temperature "$TEMPERATURE"
		--duree "$DUREE" --eprouvette "$EPROUVETTE" --operateur "$OPERATEUR"
		--journal "$dossier/mesures.csv" --avancement "$dossier/avancement")
	agir mkdir -p -- "$dossier"
	if [ "$SERMO_A_BLANC" = 1 ]; then
		agir "${essai[@]}"
		echo "à blanc : le suivi lirait l'avancement dans $dossier/avancement"
		return
	fi
	# Le pilote tourne en arrière-plan ; la fenêtre de suivi ne fait que lire son avancement.
	agir "${essai[@]}" &
	export DIALOGUE; DIALOGUE=$(dialogue_suivi "$dossier/avancement" "$DUREE")
	"$SERMO" --program=DIALOGUE >/dev/null
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = lancer ] || exit 0
		[[ "$EPROUVETTE" =~ ^[A-Za-z0-9._-]+$ ]] || { echo "N° d'éprouvette : lettres, chiffres, point, tiret."; exit 1; }
		local n
		for n in "$TENSION" "$DUREE" "${TEMPERATURE#-}"; do
			[[ "$n" =~ ^[0-9]+$ ]] || { echo "Consigne illisible : $n"; exit 1; }
		done
		if [ "$DESTRUCTIF" = true ]; then
			export EPROUVETTE OPERATEUR PROFIL TENSION TEMPERATURE DUREE
			export DIALOGUE; DIALOGUE=$(dialogue_confirmation "$EPROUVETTE")
			"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite confirmation' >/dev/null
		else
			lancer
		fi
		;;
	confirmation)
		[ "$EXIT" = confirmer ] || exit 0
		lancer
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	for c in tail timeout; do
		command -v "$c" >/dev/null || { echo "$c est absent : installez coreutils." >&2; exit 1; }
	done
	if [ "$SERMO_A_BLANC" != 1 ] && ! command -v "$BANC_CMD" >/dev/null; then
		echo "Pilote du banc introuvable : $BANC_CMD (variable BANC_CMD)." >&2; exit 1
	fi
	export DIALOGUE; DIALOGUE=$(dialogue_principal "${USER:-operateur}")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
