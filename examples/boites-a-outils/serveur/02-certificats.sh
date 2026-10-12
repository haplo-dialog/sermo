#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Serveur
# Besoin   : obtenir un certificat TLS Let's Encrypt, vérifier que le renouvellement marchera, voir ce qui expire bientôt
# Enrobe   : certbot (--nginx, --webroot ou --standalone) ; certbot renew --dry-run ; certbot certificates ; certbot delete
# Ports    : ncursessermo conseillé (par SSH) ; gtk3sermo sur un poste
# Terminal : rien de perdu (les onglets deviennent des pages ; le tableau montre six certificats à la fois)
# Usage    : ./02-certificats.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            certbot demande les droits root : sans eux, la liste reste vide et les commandes sont affichées.

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"

# Échappe une valeur avant de l'insérer dans le XML.
xml() { local s=${1//&/"&amp;"}; s=${s//</"&lt;"}; s=${s//>/"&gt;"}; printf '%s' "${s//\"/"&quot;"}"; }

# Une ligne de texte par <item>, chacune échappée.
items() { local l; while IFS= read -r l; do [ -n "$l" ] && printf '<item>%s</item>\n' "$(xml "$l")"; done <<<"$1"; }

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
		echo "certbot demande les droits root : relancez ce script en root."
		echo "Voici la commande qu'il lancerait :"
		SERMO_A_BLANC=1
	fi
}

dialogue_principal() {
	local certificats=${1:-"exemple.org|exemple.org www.exemple.org|2026-12-14|VALID: 64 days
api.exemple.org|api.exemple.org|2026-10-30|VALID: 19 days
ancien.exemple.net|ancien.exemple.net|2026-09-02|EXPIRED"}
	cat <<XML
<window title="Certificats TLS (certbot)" default-width="620">
  <vbox>
    <notebook tab-labels="Demander|Lister et renouveler">
      <vbox>
        <hbox><text><label>Domaines (séparés par une espace) :</label></text>
          <entry space-expand="true" space-fill="true"><variable>DOMAINES</variable><default>exemple.org www.exemple.org</default></entry></hbox>
        <hbox><text><label>Adresse pour les avis d'expiration :</label></text>
          <entry space-expand="true" space-fill="true"><variable>COURRIEL</variable><default>admin@exemple.org</default></entry></hbox>
        <vbox>
          <radiobutton><label>Valider par nginx (--nginx)</label><default>true</default><variable>M_NGINX</variable></radiobutton>
          <radiobutton><label>Valider par un dossier servi (--webroot)</label><variable>M_WEBROOT</variable></radiobutton>
          <radiobutton><label>Serveur autonome, port 80 libre (--standalone)</label><variable>M_AUTONOME</variable></radiobutton>
        </vbox>
        <hbox><text><label>Dossier servi (webroot) :</label></text>
          <entry space-expand="true" space-fill="true"><variable>WEBROOT</variable><default>/var/www/exemple.org</default></entry></hbox>
        <checkbox><label>Autorité d'essai (--test-cert), pour ne pas user les quotas</label><default>true</default><variable>ESSAI</variable></checkbox>
        <checkbox><label>Rediriger HTTP vers HTTPS (--redirect, avec nginx)</label><variable>REDIRIGER</variable></checkbox>
        <checkbox><label>J'accepte les conditions de Let's Encrypt (--agree-tos)</label><variable>CONDITIONS</variable></checkbox>
        <button><label>Demander le certificat</label><action>exit:demander</action></button>
      </vbox>
      <vbox>
        <tree scrollable="true" height-request="160" exported-column="0">
          <label>Certificat|Domaines|Expire le|État</label>
          <variable>CERTIFICAT</variable>
$(items "$certificats")
        </tree>
        <hbox>
          <button><label>Renouveler à blanc (--dry-run)</label><action>exit:renouveler</action></button>
          <button><label>Supprimer le certificat choisi</label><action>exit:supprimer</action></button>
        </hbox>
      </vbox>
    </notebook>
    <hbox>
      <button cancel></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_suppression() {
	local nom=${1:-ancien.exemple.net}
	cat <<XML
<window title="Supprimer un certificat">
  <vbox>
    <text><label>Le certificat « $(xml "$nom") » sera supprimé de /etc/letsencrypt.</label></text>
    <text><label>Un site nginx qui le cite encore ne démarrera plus :</label></text>
    <text><label>retirez-le d'abord du fichier de site.</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Supprimer</label><action>exit:supprimer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Les certificats connus, une ligne « nom|domaines|date|état » chacun (lecture seule).
lister() {
	certbot certificates 2>/dev/null | awk '
		/Certificate Name:/ { n = $3 }
		/Domains:/          { d = $0; sub(/^ *Domains: */, "", d) }
		/Expiry Date:/      { e = $0; sub(/.*\(/, "", e); sub(/\).*/, "", e); print n "|" d "|" $3 "|" e }'
}

demander() {
	local d doms args=(--non-interactive --agree-tos --no-eff-email -m "$COURRIEL")
	read -r -a doms <<<"$DOMAINES"   # pas de « for d in $DOMAINES » : « *.exemple.org » serait un motif de fichiers
	for d in "${doms[@]}"; do args+=(-d "$d"); done
	[ "$ESSAI" = true ] && args+=(--test-cert)
	if [ "$M_WEBROOT" = true ]; then
		agir certbot certonly --webroot -w "$WEBROOT" "${args[@]}"
	elif [ "$M_AUTONOME" = true ]; then
		agir certbot certonly --standalone "${args[@]}"
	else
		if [ "$REDIRIGER" = true ]; then args+=(--redirect); else args+=(--no-redirect); fi
		agir certbot --nginx "${args[@]}"
	fi
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		case "$EXIT" in
		demander)
			local d doms n=0
			[ "$CONDITIONS" = true ] || { echo "Cochez l'acceptation des conditions : certbot ne demande rien sans elle."; exit 1; }
			read -r -a doms <<<"$DOMAINES"
			for d in "${doms[@]}"; do
				[[ $d =~ ^[a-z0-9]([a-z0-9-]{0,61}[a-z0-9])?(\.[a-z0-9]([a-z0-9-]{0,61}[a-z0-9])?)+$ ]] ||
					{ echo "Domaine refusé : $d"; exit 1; }
				n=$((n + 1))
			done
			[ "$n" -ge 1 ] && [ "$n" -le 20 ] || { echo "Donnez entre 1 et 20 domaines."; exit 1; }
			[[ $COURRIEL =~ ^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$ ]] || { echo "Adresse refusée : $COURRIEL"; exit 1; }
			if [ "$M_WEBROOT" = true ]; then
				[[ $WEBROOT =~ ^/[A-Za-z0-9._/-]+$ && $WEBROOT != *..* ]] || { echo "Dossier refusé : $WEBROOT"; exit 1; }
			fi
			exiger_root
			demander
			;;
		renouveler)
			exiger_root
			# Essai complet auprès de l'autorité de test : aucun certificat n'est remplacé.
			agir certbot renew --dry-run
			;;
		supprimer)
			[[ $CERTIFICAT =~ ^[A-Za-z0-9._-]+$ ]] || { echo "Choisissez d'abord un certificat dans la liste."; exit 1; }
			export CERTIFICAT
			export DIALOGUE; DIALOGUE=$(dialogue_suppression "$CERTIFICAT")
			"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite suppression' >/dev/null
			;;
		esac
		;;
	suppression)
		[ "$EXIT" = supprimer ] || exit 0
		[[ $CERTIFICAT =~ ^[A-Za-z0-9._-]+$ ]] || exit 1
		exiger_root
		agir certbot delete --non-interactive --cert-name "$CERTIFICAT"
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v certbot >/dev/null || { echo "certbot est absent : installez les paquets certbot et python3-certbot-nginx." >&2; exit 1; }
	local liste
	liste=$(lister)
	[ -n "$liste" ] || liste="(aucun certificat lu : liste vide, ou il faut les droits root)|||"
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$liste")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
