#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Sécurité informatique
# Besoin   : relever l'empreinte SHA-256 des fichiers d'un dossier, puis vérifier plus tard que rien n'a changé
# Enrobe   : sha256sum
# Ports    : gtk3sermo conseillé ; ncursessermo par SSH
# Terminal : rien de perdu
# Usage    : ./2-integrite.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)

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

dialogue_principal() {
	local dossier=${1:-/etc} liste=${2:-$HOME/empreintes-etc.sha256}
	cat <<XML
<window title="Intégrité des fichiers" default-width="560">
  <vbox>
    <notebook tab-labels="Relever les empreintes|Vérifier une liste">
      <vbox>
        <text><label>Calcule l'empreinte SHA-256 de chaque fichier du dossier et l'écrit dans une liste.</label></text>
        <hbox><text><label>Dossier :</label></text>
          <entry space-expand="true" space-fill="true"><variable>DOSSIER</variable><default>$(xml "$dossier")</default></entry></hbox>
        <hbox><text><label>Liste à écrire :</label></text>
          <entry space-expand="true" space-fill="true"><variable>SORTIE</variable><default>$(xml "$liste")</default></entry></hbox>
        <checkbox><label>Descendre dans les sous-dossiers</label><variable>RECURSIF</variable><default>true</default></checkbox>
        <checkbox><label>Ignorer les fichiers cachés</label><variable>SANS_CACHES</variable></checkbox>
        <hbox><button><label>Relever</label><action>exit:relever</action></button></hbox>
      </vbox>
      <vbox>
        <text><label>Relit chaque fichier de la liste et signale ceux qui ont changé ou disparu.</label></text>
        <hbox><text><label>Liste d'empreintes :</label></text>
          <entry space-expand="true" space-fill="true"><variable>LISTE</variable><default>$(xml "$liste")</default></entry></hbox>
        <checkbox><label>N'afficher que les écarts (--quiet)</label><variable>SILENCIEUX</variable><default>true</default></checkbox>
        <hbox><button><label>Vérifier</label><action>exit:verifier</action></button></hbox>
      </vbox>
    </notebook>
    <hbox><button cancel></button></hbox>
  </vbox>
</window>
XML
}

dialogue_resultat() {
	local bilan=${1:-2 fichier(s) modifié(s) ou absent(s)}
	# Le détail (plusieurs lignes) entre par <default>, échappé par xml : jamais
	# collé dans une commande <input>, où un nom de fichier deviendrait du shell.
	cat <<XML
<window title="Résultat de la vérification" default-width="560">
  <vbox>
    <text><label>$(xml "$bilan")</label></text>
    <edit editable="false" height-request="220"><variable>DETAIL</variable><default>$(xml "${TEXTE_DETAIL:-}")</default></edit>
    <hbox><button ok></button></hbox>
  </vbox>
</window>
XML
}

dialogue_ecraser() {
	local liste=${1:-$HOME/empreintes-etc.sha256}
	cat <<XML
<window title="Remplacer la liste ?">
  <vbox>
    <text><label>La liste $(xml "$liste") existe déjà.</label></text>
    <text><label>La remplacer par un nouveau relevé ? L'ancienne sera perdue.</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Remplacer</label><action>exit:relever</action></button>
    </hbox>
  </vbox>
</window>
XML
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		case "$EXIT" in
		relever)
			[ -d "$DOSSIER" ] || { echo "Dossier introuvable : $DOSSIER" >&2; exit 1; }
			[[ $SORTIE = /* ]] || { echo "Donnez un chemin absolu pour la liste : $SORTIE" >&2; exit 1; }
			case "$SORTIE/" in "${DOSSIER%/}"/*) echo "La liste ne doit pas être dans le dossier relevé." >&2; exit 1 ;; esac
			export DOSSIER SORTIE RECURSIF SANS_CACHES
			if [ -e "$SORTIE" ]; then
				export DIALOGUE; DIALOGUE=$(dialogue_ecraser "$SORTIE")
				"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite relever' >/dev/null
			else
				EXIT=relever suite relever
			fi
			;;
		verifier)
			[ -r "$LISTE" ] || { echo "Liste illisible : $LISTE" >&2; exit 1; }
			q=(); [ "$SILENCIEUX" = true ] && q=(--quiet)
			# La vérification ne modifie rien : elle est toujours lancée.
			detail=$(cd "$(dirname -- "$LISTE")" && sha256sum --check "${q[@]}" -- "$(basename -- "$LISTE")" 2>&1)
			if [ $? -eq 0 ]; then bilan="Aucun écart : tous les fichiers sont identiques."
			else bilan="$(grep -c -E ': (FAILED|ÉCHEC)' <<<"$detail") fichier(s) modifié(s) ou absent(s)."; fi
			export TEXTE_DETAIL=${detail:-(rien à signaler)}
			export DIALOGUE; DIALOGUE=$(dialogue_resultat "$bilan")
			"$SERMO" --program=DIALOGUE >/dev/null
			;;
		esac
		;;
	relever)
		[ "$EXIT" = relever ] || exit 0
		prof=(); [ "$RECURSIF" = true ] || prof=(-maxdepth 1)
		cache=(); [ "$SANS_CACHES" = true ] && cache=(-not -path '*/.*')
		# Chemins relatifs au dossier : la liste reste valable si le dossier est copié.
		agir sh -c 'd=$1; out=$2; shift 2; cd -- "$d" && find . "$@" -type f -print0 | sort -z | xargs -0 -r sha256sum -- > "$out"' \
			integrite "$DOSSIER" "$SORTIE" "${prof[@]}" "${cache[@]}"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	for c in sha256sum find xargs; do
		command -v "$c" >/dev/null || { echo "$c est absent : installez le paquet coreutils / findutils." >&2; exit 1; }
	done
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
