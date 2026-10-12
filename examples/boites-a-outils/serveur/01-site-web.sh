#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Serveur
# Besoin   : publier un site nginx (statique, proxy vers une appli, ou PHP) sans écrire le bloc server à la main, et ne recharger que si nginx -t passe
# Enrobe   : tee /etc/nginx/sites-available/<domaine> ; ln -s vers sites-enabled ; nginx -t ; systemctl reload nginx
# Ports    : ncursessermo conseillé (par SSH) ; gtk3sermo sur un poste
# Terminal : la couleur et la police de la page d'attente se saisissent comme du texte (pas de nuancier ni de sélecteur de police) ; l'aperçu du fichier tient sur une ligne
# Usage    : ./01-site-web.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            Écrire sous /etc/nginx demande les droits root : sans eux, les commandes sont affichées.

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
		echo "Il faut les droits root pour écrire sous /etc/nginx : relancez ce script en root."
		echo "Voici les commandes qu'il lancerait :"
		SERMO_A_BLANC=1
	fi
}

dialogue_principal() {
	local domaine=${1:-exemple.org} racine=${2:-/var/www/exemple.org} amont=${3:-127.0.0.1:3000}
	cat <<XML
<window title="Site web nginx" default-width="580">
  <vbox>
    <hbox><text><label>Nom de domaine :</label></text>
      <entry space-expand="true" space-fill="true"><variable>DOMAINE</variable><default>$(xml "$domaine")</default></entry></hbox>
    <hbox><text><label>Racine des fichiers :</label></text>
      <entry space-expand="true" space-fill="true"><variable>RACINE</variable><default>$(xml "$racine")</default></entry></hbox>
    <frame label="Type de site">
      <hbox>
        <radiobutton><label>Statique</label><default>true</default><variable>T_STATIQUE</variable></radiobutton>
        <radiobutton><label>Proxy vers une appli</label><variable>T_PROXY</variable></radiobutton>
        <radiobutton><label>PHP (php-fpm)</label><variable>T_PHP</variable></radiobutton>
      </hbox>
    </frame>
    <hbox><text><label>Proxy vers (adresse:port) :</label></text>
      <entry space-expand="true" space-fill="true"><variable>AMONT</variable><default>$(xml "$amont")</default></entry></hbox>
    <hbox><text><label>Port d'écoute :</label></text>
      <spinbutton range-min="1" range-max="65535" range-step="1"><variable>PORT</variable><default>80</default></spinbutton></hbox>
    <checkbox><label>Activer le site (lien dans sites-enabled)</label><default>true</default><variable>ACTIVER</variable></checkbox>
    <checkbox><label>Répondre aussi pour www.domaine</label><variable>WWW</variable></checkbox>
    <expander label="Page d'attente, si la racine n'a pas d'index.html">
      <vbox>
        <checkbox><label>Écrire une page d'attente</label><variable>PAGE</variable></checkbox>
        <hbox><text><label>Fond :</label></text>
          <colorbutton><variable>COULEUR</variable><default>#2e3440</default></colorbutton>
          <text><label>Police :</label></text>
          <fontbutton><variable>POLICE</variable><default>Sans 14</default></fontbutton></hbox>
      </vbox>
      <variable>PAGE_OUVERTE</variable>
    </expander>
    <hbox>
      <button cancel></button>
      <button><label>Voir le fichier</label><action>exit:apercu</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Le fichier de site arrive par la variable APERCU_SITE (printenv ne fait que lire).
dialogue_apercu() {
	local chemin=${1:-/etc/nginx/sites-available/exemple.org} etat=${2:-nouveau fichier}
	cat <<XML
<window title="Fichier de site nginx" default-width="620">
  <vbox>
    <text><label>Fichier : $(xml "$chemin") ($(xml "$etat"))</label></text>
    <edit editable="false" height-request="260"><variable>CONTENU</variable><input>printenv APERCU_SITE</input></edit>
    <checkbox><label>Recharger nginx si nginx -t réussit</label><default>true</default><variable>RECHARGER</variable></checkbox>
    <text><label>Rien n'est rechargé si la vérification échoue.</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Écrire le fichier</label><action>exit:ecrire</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Le bloc server, construit à partir des choix déjà validés.
conf_site() {
	local noms=$DOMAINE
	[ "$WWW" = true ] && noms="$DOMAINE www.$DOMAINE"
	printf 'server {\n'
	printf '    listen %s;\n    listen [::]:%s;\n' "$PORT" "$PORT"
	printf '    server_name %s;\n' "$noms"
	printf '    access_log /var/log/nginx/%s.access.log;\n' "$DOMAINE"
	printf '    error_log /var/log/nginx/%s.error.log;\n' "$DOMAINE"
	case "$TYPE" in
	proxy)
		printf '    location / {\n'
		printf '        proxy_pass http://%s;\n' "$AMONT"
		printf '        proxy_set_header Host $host;\n'
		printf '        proxy_set_header X-Real-IP $remote_addr;\n'
		printf '        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;\n'
		printf '        proxy_set_header X-Forwarded-Proto $scheme;\n'
		printf '    }\n'
		;;
	php)
		printf '    root %s;\n    index index.php index.html;\n' "$RACINE"
		printf '    location / {\n        try_files $uri $uri/ =404;\n    }\n'
		printf '    location ~ \\.php$ {\n'
		printf '        include snippets/fastcgi-php.conf;\n'
		printf '        fastcgi_pass unix:/run/php/php-fpm.sock;\n'
		printf '    }\n'
		;;
	*)
		printf '    root %s;\n    index index.html;\n' "$RACINE"
		printf '    location / {\n        try_files $uri $uri/ =404;\n    }\n'
		;;
	esac
	printf '}\n'
}

# Page d'attente minimale, couleur et police déjà validées.
page_attente() {
	printf '<!doctype html>\n<html lang="fr"><head><meta charset="utf-8"><title>%s</title>\n' "$DOMAINE"
	printf '<style>body{background:%s;color:#eceff4;font-family:"%s",sans-serif;font-size:%spx;text-align:center;padding-top:20vh}</style>\n' \
		"$COULEUR" "$FAMILLE" "$TAILLE"
	printf '</head><body><p>%s ouvre bientôt.</p></body></html>\n' "$DOMAINE"
}

# Contrôle les valeurs et pose TYPE, FAMILLE, TAILLE. Code 1 si une valeur est refusée.
valider() {
	TYPE=statique
	[ "$T_PROXY" = true ] && TYPE=proxy
	[ "$T_PHP" = true ] && TYPE=php
	[[ $DOMAINE =~ ^[a-z0-9]([a-z0-9-]{0,61}[a-z0-9])?(\.[a-z0-9]([a-z0-9-]{0,61}[a-z0-9])?)+$ ]] ||
		{ echo "Nom de domaine refusé : $DOMAINE (minuscules, chiffres, tirets, points)"; return 1; }
	[[ $PORT =~ ^[0-9]+$ ]] && [ "$PORT" -ge 1 ] && [ "$PORT" -le 65535 ] ||
		{ echo "Port refusé : $PORT"; return 1; }
	if [ "$TYPE" = proxy ]; then
		[[ $AMONT =~ ^[A-Za-z0-9.-]+:[0-9]{1,5}$|^\[[0-9A-Fa-f:]+\]:[0-9]{1,5}$ ]] ||
			{ echo "Adresse du proxy refusée : $AMONT (forme attendue : 127.0.0.1:3000)"; return 1; }
	else
		[[ $RACINE =~ ^/[A-Za-z0-9._/-]+$ && $RACINE != *..* ]] ||
			{ echo "Racine refusée : $RACINE (chemin absolu, sans espace ni « .. »)"; return 1; }
		# Chemin réel (« //etc », « /./root », liens suivis), puis liste blanche :
		# un site ne sert qu'un sous-dossier de /var/www ou de /srv.
		local reel
		reel=$(realpath -m -- "$RACINE") || { echo "Racine illisible : $RACINE"; return 1; }
		case "$reel" in
		/var/www/?*|/srv/?*) RACINE=$reel ;;
		*) echo "Racine refusée : $RACINE (= $reel) ; seuls /var/www/… et /srv/… sont servis."; return 1 ;;
		esac
	fi
	if [ "$PAGE" = true ]; then
		[[ $COULEUR =~ ^#[0-9A-Fa-f]{6}$|^rgba?\([0-9.,\ ]+\)$ ]] || { echo "Couleur refusée : $COULEUR"; return 1; }
		# « Sans Bold 14 » : la famille sans la taille, la taille en pixels.
		TAILLE=${POLICE##* }; FAMILLE=${POLICE% *}
		[[ $TAILLE =~ ^[0-9]{1,2}$ ]] || { TAILLE=14; FAMILLE=$POLICE; }
		[[ $FAMILLE =~ ^[A-Za-z0-9\ -]{1,60}$ ]] || { echo "Police refusée : $POLICE"; return 1; }
	fi
	return 0
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	local site
	case "$1" in
	principal)
		[ "$EXIT" = apercu ] || exit 0
		valider || exit 1
		site=/etc/nginx/sites-available/$DOMAINE
		local etat="nouveau fichier"
		[ -e "$site" ] && etat="REMPLACE le fichier existant"
		export DOMAINE RACINE AMONT PORT ACTIVER WWW T_STATIQUE T_PROXY T_PHP PAGE COULEUR POLICE
		export APERCU_SITE; APERCU_SITE=$(conf_site)
		export DIALOGUE; DIALOGUE=$(dialogue_apercu "$site" "$etat")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite ecrire' >/dev/null
		;;
	ecrire)
		[ "$EXIT" = ecrire ] || exit 0
		valider || exit 1
		site=/etc/nginx/sites-available/$DOMAINE
		exiger_root
		conf_site | agir tee -- "$site" || exit 1
		if [ "$PAGE" = true ] && [ "$TYPE" != proxy ] && [ ! -e "$RACINE/index.html" ]; then
			agir install -d -m 0755 -- "$RACINE"
			page_attente | agir tee -- "$RACINE/index.html"
		fi
		[ "$ACTIVER" = true ] && agir ln -sfn -- "../sites-available/$DOMAINE" "/etc/nginx/sites-enabled/$DOMAINE"
		agir nginx -t || { echo "nginx -t a échoué : rien n'est rechargé. Corrigez $site."; exit 1; }
		[ "$RECHARGER" = true ] && agir systemctl reload nginx
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v nginx >/dev/null || [ -x /usr/sbin/nginx ] || { echo "nginx est absent : installez le paquet nginx." >&2; exit 1; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
