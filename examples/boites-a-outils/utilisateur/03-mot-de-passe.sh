#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Utilisateur
# Besoin   : inventer un mot de passe solide (ou une phrase de passe facile à retenir), le voir en grand, sans qu'il soit enregistré nulle part
# Enrobe   : /dev/urandom (via tr et od) ; rien n'est écrit sur le disque
# Ports    : gtk3sermo conseillé ; qt6sermo ; htmlsermo
# Terminal : en ncurses, la police et la couleur choisies ne s'appliquent pas
# Usage    : ./03-mot-de-passe.sh [binaire sermo]
#            Rien n'est modifié ni enregistré. Le mot de passe passe à sermo par
#            l'entrée standard, jamais sur une ligne de commande ni dans l'environnement.

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

# 128 mots courts, sans accent : 7 bits de hasard par mot, tirage sans biais (65536 = 512 × 128).
MOTS=(abeille abricot accord agneau aimant algue ancre anneau arbre argile arome asperge
	astre atelier avion bague baleine balcon bambou banane barque bateau berger biscuit
	bougie boussole brebis brioche bureau cabane cactus cahier camion canard carotte
	castor cerise chalet chameau chapeau charbon chemin cheval chocolat citron colline
	comete corde coton crayon cuivre dauphin desert echelle eclair ecureuil etoile
	falaise farine fenetre ficelle flocon foret fourmi fraise fromage fusee galet gateau
	girafe glacier grenier guitare hamac hibou horloge jardin kayak lagune lanterne
	lapin lavande loutre lumiere maison marmotte melon miroir montagne moulin nuage
	olive orage orange ours palmier panda papillon parapluie piano pinceau planete plume
	poire pomme prairie radis renard requin rivage robot roseau sable sapin sirop soleil
	tambour tomate tortue trompette tulipe valise velo violon voilier volcan yaourt
	zebre)

dialogue_principal() {
	cat <<XML
<window title="Inventer un mot de passe" default-width="520">
  <vbox>
    <frame label="Ce que je veux">
      <vbox>
        <radiobutton><label>Un mot de passe (lettres, chiffres, signes)</label><variable>MDP</variable><default>true</default></radiobutton>
        <hbox><text><label>Longueur :</label></text>
          <spinbutton range-min="8" range-max="64" range-step="1"><variable>LONGUEUR</variable><default>16</default></spinbutton></hbox>
        <checkbox><label>Avec des chiffres</label><variable>CHIFFRES</variable><default>true</default></checkbox>
        <checkbox><label>Avec des signes (! ? # % …)</label><variable>SIGNES</variable><default>true</default></checkbox>
        <hbox><switch><variable>LISIBLE</variable><default>true</default></switch>
          <text><label>Éviter les signes qui se ressemblent (0 et O, 1 et l)</label></text></hbox>
        <radiobutton><label>Une phrase de passe (des mots, plus facile à retenir)</label><variable>PHRASE</variable></radiobutton>
        <hbox><text><label>Nombre de mots :</label></text>
          <spinbutton range-min="4" range-max="10" range-step="1"><variable>NB_MOTS</variable><default>6</default></spinbutton></hbox>
      </vbox>
    </frame>
    <frame label="Pour bien le lire">
      <hbox>
        <text><label>Police :</label></text>
        <fontbutton><variable>POLICE</variable><default>Monospace 20</default></fontbutton>
        <text><label>Couleur :</label></text>
        <colorbutton><variable>COULEUR</variable><default>#1a5fb4</default></colorbutton>
      </hbox>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Inventer</label><action>exit:inventer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Arguments : le secret, la police, la couleur (déjà vérifiées).
dialogue_resultat() {
	local secret=${1:-cheval-tulipe-orage-piano-sable-yaourt} police=${2:-Monospace 20} couleur=${3:-#1a5fb4}
	# Balises Pango entre guillemets : sermo les garde telles quelles. Le secret
	# passe par xml, et Pango décode ses « &amp; ». Police et couleur sont
	# vérifiées avant (lettres, chiffres, espace, #) : pas de guillemet possible.
	cat <<XML
<window title="Votre mot de passe" default-width="560">
  <vbox>
    <text use-markup="true"><label>"<span font='$(xml "$police")' foreground='$(xml "$couleur")'>$(xml "$secret")</span>"</label></text>
    <hbox><text><label>Pour le copier :</label></text>
      <entry space-expand="true" space-fill="true"><variable>A_COPIER</variable><default>$(xml "$secret")</default></entry></hbox>
    <text><label>Il n'est enregistré nulle part : notez-le ou mettez-le dans votre coffre à mots de passe.</label></text>
    <hbox>
      <button><label>Fermer</label><action>exit:fermer</action></button>
      <button><label>En inventer un autre</label><action>exit:encore</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Un nombre au hasard de 0 à 127, lu sur /dev/urandom.
hasard128() { local n; n=$(od -An -N2 -tu2 /dev/urandom); echo $(( ${n// /} % 128 )); }

inventer() {
	if [ "$PHRASE" = true ]; then
		local i mots=()
		for ((i = 0; i < NB_MOTS; i++)); do mots+=("${MOTS[$(hasard128)]}"); done
		local IFS=-
		printf '%s' "${mots[*]}"
	else
		local jeu='a-zA-Z'
		[ "$CHIFFRES" = true ] && jeu+='0-9'
		[ "$SIGNES" = true ] && jeu+='!?#%*+=@_.:-'
		if [ "$LISIBLE" = true ]; then
			LC_ALL=C tr -dc "$jeu" </dev/urandom | LC_ALL=C tr -d '0O1lI' | head -c "$LONGUEUR"
		else
			LC_ALL=C tr -dc "$jeu" </dev/urandom | head -c "$LONGUEUR"
		fi
	fi
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = inventer ] || exit 0
		[[ $LONGUEUR =~ ^[0-9]+$ ]] && [ "$LONGUEUR" -ge 8 ] && [ "$LONGUEUR" -le 64 ] || LONGUEUR=16
		[[ $NB_MOTS =~ ^[0-9]+$ ]] && [ "$NB_MOTS" -ge 4 ] && [ "$NB_MOTS" -le 10 ] || NB_MOTS=6
		[[ $POLICE =~ ^[A-Za-z0-9\ ,.-]{1,60}$ ]] || POLICE="Monospace 20"
		# Pango veut #RRGGBB : une couleur rgb(r,g,b) ou rgba(…) est convertie.
		if [[ $COULEUR =~ ^rgba?\(\ *([0-9]{1,3})\ *,\ *([0-9]{1,3})\ *,\ *([0-9]{1,3}) ]]; then
			COULEUR=$(printf '#%02x%02x%02x' "$((10#${BASH_REMATCH[1]} % 256))" "$((10#${BASH_REMATCH[2]} % 256))" "$((10#${BASH_REMATCH[3]} % 256))")
		fi
		[[ $COULEUR =~ ^#[0-9A-Fa-f]{6}$ ]] || COULEUR="#1a5fb4"
		local secret reponse
		while :; do
			secret=$(inventer)
			[ -n "$secret" ] || { echo "Impossible de lire le hasard du système." >&2; exit 1; }
			# Par l'entrée standard, et sans --do : le secret n'entre ni dans
			# l'environnement d'un autre processus, ni dans ps. On ne relit que EXIT.
			reponse=$(dialogue_resultat "$secret" "$POLICE" "$COULEUR" | "$SERMO" --stdin | grep '^EXIT=')
			[ "$reponse" = 'EXIT="encore"' ] || break
		done
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	[ -r /dev/urandom ] || { echo "/dev/urandom est illisible : impossible de tirer au hasard." >&2; exit 1; }
	local c
	for c in od tr head; do
		command -v "$c" >/dev/null || { echo "$c est absent : installez le paquet coreutils." >&2; exit 1; }
	done
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
