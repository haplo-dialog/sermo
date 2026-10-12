#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Serveur
# Besoin   : créer une base et le compte d'une application (PostgreSQL ou MariaDB) sans taper le SQL, mot de passe compris
# Enrobe   : runuser -u postgres -- psql (SQL lu sur l'entrée) ; mariadb (SQL lu sur l'entrée, root par le socket)
# Ports    : ncursessermo conseillé (par SSH) ; gtk3sermo sur un poste
# Terminal : rien de perdu (le mot de passe reste masqué)
# Usage    : ./06-base-de-donnees.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée, sans le SQL)
#            Il faut les droits root (pour devenir postgres, ou entrer dans MariaDB par le socket).
#            Le mot de passe passe par l'entrée standard du client SQL, jamais sur la ligne de commande.

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

# Sans les droits root, on ne peut qu'afficher les commandes.
exiger_root() {
	if [ "$SERMO_A_BLANC" != 1 ] && [ "$EUID" -ne 0 ]; then
		echo "Il faut les droits root pour parler au serveur de bases : relancez ce script en root."
		echo "Voici la commande qu'il lancerait :"
		SERMO_A_BLANC=1
	fi
}

dialogue_principal() {
	local base=${1:-gestion_stock} compte=${2:-appli_stock}
	cat <<XML
<window title="Nouvelle base de données" default-width="540">
  <vbox>
    <frame label="Serveur">
      <hbox>
        <radiobutton><label>PostgreSQL</label><default>true</default><variable>S_POSTGRES</variable></radiobutton>
        <radiobutton><label>MariaDB</label><variable>S_MARIADB</variable></radiobutton>
      </hbox>
    </frame>
    <hbox><text><label>Nom de la base :</label></text>
      <entry space-expand="true" space-fill="true"><variable>BASE</variable><default>$(xml "$base")</default></entry></hbox>
    <hbox><text><label>Compte de l'application :</label></text>
      <entry space-expand="true" space-fill="true"><variable>COMPTE</variable><default>$(xml "$compte")</default></entry></hbox>
    <hbox><text><label>Mot de passe :</label></text>
      <password space-expand="true" space-fill="true"><variable>MDP</variable></password></hbox>
    <hbox><text><label>Une seconde fois :</label></text>
      <password space-expand="true" space-fill="true"><variable>MDP2</variable></password></hbox>
    <hbox><text><label>Jeu de caractères :</label></text>
      <comboboxtext><variable>CODAGE</variable><default>UTF-8 (utf8mb4 pour MariaDB)</default>
        <item>UTF-8 (utf8mb4 pour MariaDB)</item>
        <item>Latin-1 (ancien logiciel)</item>
      </comboboxtext></hbox>
    <hbox><text><label>Connexion permise depuis (MariaDB) :</label></text>
      <entry space-expand="true" space-fill="true"><variable>HOTE</variable><default>localhost</default></entry></hbox>
    <checkbox><label>Le compte possède la base (sinon : droits complets sur la base)</label><default>true</default><variable>PROPRIETAIRE</variable></checkbox>
    <hbox>
      <button cancel></button>
      <button><label>Créer</label><action>exit:creer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Une chaîne SQL entre apostrophes : l'apostrophe se double (barre oblique refusée avant).
sql_texte() { printf "'%s'" "${1//\'/\'\'}"; }

sql_postgres() {
	local codage=UTF8
	case "$CODAGE" in Latin-1*) codage=LATIN1 ;; esac
	# psql tourne en superutilisateur postgres : ces réglages de session lui sont permis.
	# Rien de cette session ne part au journal, même une instruction en échec.
	printf "SET log_statement = 'none';\nSET log_min_error_statement = 'panic';\n"
	printf "SET log_min_duration_statement = -1;\nSET log_duration = off;\n"
	# Compte ou base déjà là : on s'arrête AVANT d'envoyer une instruction vouée à l'échec.
	printf "SELECT EXISTS (SELECT 1 FROM pg_roles WHERE rolname = %s) AS compte_existe,\n" "$(sql_texte "$COMPTE")"
	printf "       EXISTS (SELECT 1 FROM pg_database WHERE datname = %s) AS base_existe \\gset\n" "$(sql_texte "$BASE")"
	printf '\\if :compte_existe\n\\echo Le compte existe deja : rien n est cree.\n\\quit\n\\endif\n'
	printf '\\if :base_existe\n\\echo La base existe deja : rien n est cree.\n\\quit\n\\endif\n'
	printf 'CREATE ROLE "%s" LOGIN PASSWORD %s;\n' "$COMPTE" "$(sql_texte "$SECRET")"
	if [ "$PROPRIETAIRE" = true ]; then
		printf 'CREATE DATABASE "%s" OWNER "%s" ENCODING %s TEMPLATE template0;\n' "$BASE" "$COMPTE" "'$codage'"
	else
		printf 'CREATE DATABASE "%s" ENCODING %s TEMPLATE template0;\n' "$BASE" "'$codage'"
		printf 'GRANT ALL PRIVILEGES ON DATABASE "%s" TO "%s";\n' "$BASE" "$COMPTE"
	fi
}

sql_mariadb() {
	local codage="utf8mb4 COLLATE utf8mb4_unicode_ci"
	case "$CODAGE" in Latin-1*) codage=latin1 ;; esac
	printf 'CREATE DATABASE `%s` CHARACTER SET %s;\n' "$BASE" "$codage"
	printf "CREATE USER '%s'@'%s' IDENTIFIED BY %s;\n" "$COMPTE" "$HOTE" "$(sql_texte "$SECRET")"
	printf "GRANT ALL PRIVILEGES ON \`%s\`.* TO '%s'@'%s';\n" "$BASE" "$COMPTE" "$HOTE"
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = creer ] || exit 0
		# Noms : lettres minuscules, chiffres, souligné ; pas de guillemet possible dans le SQL.
		[[ $BASE =~ ^[a-z_][a-z0-9_]{0,62}$ ]] || { echo "Nom de base refusé : $BASE (minuscules, chiffres, _)"; exit 1; }
		[[ $COMPTE =~ ^[a-z_][a-z0-9_]{0,31}$ ]] || { echo "Nom de compte refusé : $COMPTE (minuscules, chiffres, _ ; 32 au plus)"; exit 1; }
		[ -n "$MDP" ] || { echo "Le mot de passe est vide."; exit 1; }
		[ "$MDP" = "$MDP2" ] || { echo "Les deux mots de passe diffèrent."; exit 1; }
		[ ${#MDP} -ge 12 ] || { echo "Mot de passe trop court : 12 caractères au moins."; exit 1; }
		[[ $MDP != *\\* && $MDP != *$'\n'* ]] || { echo "Le mot de passe ne doit contenir ni barre oblique inverse ni saut de ligne."; exit 1; }
		# Le secret quitte l'environnement : psql et mariadb ne le reçoivent que par le tuyau.
		SECRET=$MDP
		unset MDP MDP2
		case "$CODAGE" in UTF-8*|Latin-1*) ;; *) echo "Jeu de caractères refusé : $CODAGE"; exit 1 ;; esac
		exiger_root
		if [ "$S_MARIADB" = true ]; then
			[[ $HOTE =~ ^(localhost|[0-9.%]{1,15}|[a-z0-9.-]{1,253})$ ]] || { echo "Hôte refusé : $HOTE"; exit 1; }
			command -v mariadb >/dev/null || { echo "Le client mariadb est absent : installez mariadb-client."; exit 1; }
			sql_mariadb | agir mariadb --batch
		else
			command -v psql >/dev/null || { echo "psql est absent : installez postgresql-client."; exit 1; }
			sql_postgres | agir runuser -u postgres -- psql --quiet -v ON_ERROR_STOP=1
		fi
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v psql >/dev/null || command -v mariadb >/dev/null ||
		{ echo "Ni psql ni mariadb : installez postgresql ou mariadb-server." >&2; exit 1; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
