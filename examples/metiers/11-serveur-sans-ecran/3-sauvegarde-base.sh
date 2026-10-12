#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : Serveur sans écran
# Besoin   : sauvegarder une base PostgreSQL ou MariaDB/MySQL dans un fichier daté, avec les bonnes options
# Enrobe   : pg_dump ou mysqldump
# Ports    : ncursessermo conseillé (par SSH) ; gtk3sermo sur un poste
# Terminal : rien de perdu
# Usage    : ./3-sauvegarde-base.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            Le mot de passe n'est jamais demandé ici : ~/.pgpass ou ~/.my.cnf, ou l'invite du client.

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
	local dossier=${1:-/var/backups/bases}
	cat <<XML
<window title="Sauvegarde de base de données" default-width="540">
  <vbox>
    <frame label="Moteur">
      <hbox>
        <radiobutton><label>PostgreSQL</label><variable>POSTGRES</variable><default>true</default></radiobutton>
        <radiobutton><label>MariaDB / MySQL</label><variable>MARIADB</variable></radiobutton>
      </hbox>
    </frame>
    <frame label="Connexion">
      <vbox>
        <hbox><text><label>Hôte :</label></text>
          <entry space-expand="true" space-fill="true"><variable>HOTE</variable><default>localhost</default></entry>
          <text><label>Port :</label></text>
          <spinbutton range-min="1" range-max="65535" range-step="1"><variable>PORT</variable><default>5432</default></spinbutton></hbox>
        <hbox><text><label>Base :</label></text>
          <entry space-expand="true" space-fill="true"><variable>BASE</variable><default>intranet</default></entry></hbox>
        <hbox><text><label>Utilisateur :</label></text>
          <entry space-expand="true" space-fill="true"><variable>UTILISATEUR</variable><default>sauvegarde</default></entry></hbox>
      </vbox>
    </frame>
    <frame label="Fichier">
      <vbox>
        <hbox><text><label>Dossier :</label></text>
          <entry space-expand="true" space-fill="true"><variable>DOSSIER</variable><default>$(xml "$dossier")</default></entry></hbox>
        <hbox><text><label>Format :</label></text>
          <comboboxtext><variable>FORMAT</variable><default>archive compressée</default>
            <item>archive compressée</item>
            <item>script SQL</item>
          </comboboxtext></hbox>
        <checkbox><label>Structure seule, sans les données</label><variable>STRUCTURE</variable></checkbox>
      </vbox>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Sauvegarder</label><action>exit:sauvegarder</action></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_ecraser() {
	local fichier=${1:-/var/backups/bases/intranet-20261011-0300.dump}
	cat <<XML
<window title="Remplacer la sauvegarde ?">
  <vbox>
    <text><label>Le fichier $(xml "$fichier") existe déjà.</label></text>
    <text><label>Le remplacer ? L'ancienne sauvegarde sera perdue.</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Remplacer</label><action>exit:remplacer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Nom du fichier de sortie, daté à la minute.
fichier() {
	local ext=sql
	[ "$POSTGRES" = true ] && [ "$FORMAT" = "archive compressée" ] && ext=dump
	echo "${DOSSIER%/}/$BASE-$(date +%Y%m%d-%H%M).$ext"
}

sauvegarder() {
	local f=$SORTIE
	if [ "$POSTGRES" = true ]; then
		local opts=(--host="$HOTE" --port="$PORT" --username="$UTILISATEUR" --file="$f")
		[ "$FORMAT" = "archive compressée" ] && opts+=(--format=custom) || opts+=(--format=plain)
		[ "$STRUCTURE" = true ] && opts+=(--schema-only)
		agir pg_dump "${opts[@]}" -- "$BASE"
	else
		local opts=(--host="$HOTE" --port="$PORT" --user="$UTILISATEUR" --single-transaction --routines --result-file="$f")
		[ "$STRUCTURE" = true ] && opts+=(--no-data)
		agir mysqldump "${opts[@]}" -- "$BASE"
	fi
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = sauvegarder ] || exit 0
		outil=pg_dump; [ "$MARIADB" = true ] && outil=mysqldump
		if ! command -v "$outil" >/dev/null; then
			echo "$outil est absent : installez le client de la base." >&2
			[ "$SERMO_A_BLANC" = 1 ] || exit 1
		fi
		[[ $BASE =~ ^[A-Za-z0-9_][A-Za-z0-9_.-]*$ ]] || { echo "Nom de base refusé : $BASE" >&2; exit 1; }
		[[ $HOTE =~ ^[A-Za-z0-9_.:/-]+$ && $HOTE != -* ]] || { echo "Hôte refusé : $HOTE" >&2; exit 1; }
		[[ $PORT =~ ^[0-9]+$ ]] || { echo "Port refusé : $PORT" >&2; exit 1; }
		if [ "$MARIADB" = true ] && [ "$PORT" = 5432 ]; then
			echo "Port 5432 (celui de PostgreSQL) remplacé par 3306 pour MariaDB."; PORT=3306
		fi
		[ -d "$DOSSIER" ] || { echo "Dossier introuvable : $DOSSIER" >&2; exit 1; }
		SORTIE=$(fichier)
		export POSTGRES MARIADB HOTE PORT BASE UTILISATEUR DOSSIER FORMAT STRUCTURE SORTIE
		if [ -e "$SORTIE" ]; then
			export DIALOGUE; DIALOGUE=$(dialogue_ecraser "$SORTIE")
			"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite remplacer' >/dev/null
		else
			sauvegarder
		fi
		;;
	remplacer)
		[ "$EXIT" = remplacer ] || exit 0
		sauvegarder
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v pg_dump >/dev/null || command -v mysqldump >/dev/null ||
		{ echo "Ni pg_dump ni mysqldump : installez postgresql-client ou mariadb-client." >&2; exit 1; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
