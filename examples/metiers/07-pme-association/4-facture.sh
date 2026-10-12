#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : PME, association (facturation)
# Besoin   : établir une facture (client, lignes, TVA), la relire calculée, l'enregistrer en texte et l'imprimer
# Enrobe   : awk (calcul) ; tee (fichier de facture) ; lp (impression)
# Ports    : gtk3sermo conseillé ; qt6sermo, fltk1sermo
# Terminal : rien de perdu (le tableau d'aperçu défile en ncurses)
# Usage    : ./4-facture.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)

export SERMO="${SERMO:-sermo}"
export SERMO_A_BLANC="${SERMO_A_BLANC:-1}"
# Le script lui-même, pour --do : le shell de sermo lit la variable, sans
# citation à refaire (printf %q écrit $'…' en locale C, que dash lit de travers).
export SERMO_SCRIPT="$0"

# Échappe une valeur avant de l'insérer dans le XML.
# (« \& » : depuis bash 5.2, un « & » nu dans le remplacement recopie le motif.)
xml() { local s=${1//&/"&amp;"}; s=${s//</"&lt;"}; s=${s//>/"&gt;"}; printf '%s' "${s//\"/"&quot;"}"; }

# Lance une commande, ou l'affiche seulement en mode à blanc.
agir() {
	if [ "$SERMO_A_BLANC" = 1 ]; then
		printf 'à blanc :'; printf ' %q' "$@"; printf '\n'
	else
		"$@"
	fi
}

# Un texte sur plusieurs lignes s'écrit entre guillemets dans <default>,
# sinon sermo ne garde que la dernière ligne.
dialogue_principal() {
	local numero=${1:-F-2026-042} date=${2:-2026-10-12} dossier=${3:-$HOME/Documents/Factures}
	cat <<XML
<window title="Nouvelle facture" default-width="600">
  <vbox>
    <frame label="En-tête">
      <vbox>
        <hbox><text><label>Numéro :</label></text>
          <entry><variable>NUMERO</variable><default>$(xml "$numero")</default></entry>
          <text><label>Date :</label></text>
          <entry><variable>DATE</variable><default>$(xml "$date")</default></entry></hbox>
        <hbox><text><label>Client :</label></text>
          <entry space-expand="true" space-fill="true"><variable>CLIENT</variable><default>Mairie de Bourg-lès-Valence</default></entry></hbox>
        <text xalign="0"><label>Adresse du client :</label></text>
        <edit height-request="50"><variable>ADRESSE</variable><default>"Place de la République
26500 Bourg-lès-Valence"</default></edit>
      </vbox>
    </frame>
    <frame label="Lignes : désignation ; quantité ; prix unitaire HT">
      <vbox>
        <edit height-request="110"><variable>LIGNES</variable><default>"Atelier numérique (demi-journée) ; 2 ; 180
Support imprimé ; 25 ; 3.50
Frais de déplacement ; 1 ; 42"</default></edit>
      </vbox>
    </frame>
    <hbox>
      <text><label>TVA :</label></text>
      <comboboxtext>
        <variable>TVA</variable>
        <default>20 %</default>
        <item>20 %</item>
        <item>10 %</item>
        <item>5,5 %</item>
        <item>Non applicable (art. 293 B du CGI)</item>
      </comboboxtext>
      <text><label>Paiement à (jours) :</label></text>
      <spinbutton range-min="0" range-max="60" range-step="15"><variable>ECHEANCE</variable><default>30</default></spinbutton>
    </hbox>
    <hbox>
      <text><label>Dossier :</label></text>
      <entry space-expand="true" space-fill="true"><variable>DOSSIER</variable><default>$(xml "$dossier")</default></entry>
    </hbox>
    <hbox>
      <button cancel></button>
      <button><label>Calculer et relire</label><action>exit:calculer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Aperçu calculé : $1 numéro, $2 client, $3 totaux, puis une ligne « n°|désignation|qté|PU|total » par argument.
# Le n° sur deux chiffres garde l'ordre de saisie si le tableau trie sa première colonne.
dialogue_apercu() {
	local numero=${1:-F-2026-042} client=${2:-Mairie de Bourg-lès-Valence} totaux=${3:-Total HT 489,50 € · TVA 97,90 € · Total TTC 587,40 €}
	shift 3 2>/dev/null || set --
	[ $# -gt 0 ] || set -- "01|Atelier numérique (demi-journée)|2|180,00|360,00" "02|Support imprimé|25|3,50|87,50" "03|Frais de déplacement|1|42,00|42,00"
	cat <<XML
<window title="Relire la facture" default-width="560" default-height="340">
  <vbox>
    <text><label>Facture $(xml "$numero") pour $(xml "$client")</label></text>
    <table scrollable="true" height-request="140">
      <label>N°|Désignation|Qté|PU HT (€)|Total HT (€)</label>
      <variable>LIGNE</variable>
XML
	local l
	for l in "$@"; do printf '      <item>%s</item>\n' "$(xml "$l")"; done
	cat <<XML
    </table>
    <text><label>$(xml "$totaux")</label></text>
    <checkbox><label>Imprimer la facture (imprimante par défaut)</label><variable>IMPRIMER</variable></checkbox>
    <hbox>
      <button cancel></button>
      <button><label>Enregistrer la facture</label><action>exit:enregistrer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Taux de TVA en pour cent, point décimal.
taux() { case "$TVA" in 20*) echo 20 ;; 10*) echo 10 ;; 5,5*) echo 5.5 ;; *) echo 0 ;; esac; }

# Lignes saisies → « n°|désignation|qté|PU|total », montants à virgule.
calcul_lignes() {
	printf '%s\n' "$LIGNES" | LC_ALL=C awk -F';' '
		function nb(s) { gsub(/[ €]/, "", s); sub(/,/, ".", s); return s + 0 }
		function fr(x) { s = sprintf("%.2f", x); sub(/\./, ",", s); return s }
		NF >= 3 { d = $1; gsub(/^ +| +$/, "", d); gsub(/\|/, "/", d); q = nb($2); p = nb($3)
			printf "%02d|%s|%s|%s|%s\n", ++n, d, q, fr(p), fr(q * p) }'
}

# Somme HT, puis TVA et TTC : « ht|tva|ttc » à virgule.
calcul_totaux() {
	printf '%s\n' "$LIGNES" | LC_ALL=C awk -F';' -v t="$(taux)" '
		function nb(s) { gsub(/[ €]/, "", s); sub(/,/, ".", s); return s + 0 }
		function fr(x) { s = sprintf("%.2f", x); sub(/\./, ",", s); return s }
		NF >= 3 { ht += nb($2) * nb($3) }
		END { tva = ht * t / 100; print fr(ht) "|" fr(tva) "|" fr(ht + tva) }'
}

facture_texte() {
	local ht tva ttc
	IFS='|' read -r ht tva ttc < <(calcul_totaux)
	printf 'FACTURE %s\nDate : %s · Paiement à %s jours\n\nClient :\n%s\n%s\n\n' "$NUMERO" "$DATE" "$ECHEANCE" "$CLIENT" "$ADRESSE"
	calcul_lignes | awk -F'|' '{ printf "%s. %-40s %6s x %10s = %10s €\n", $1, $2, $3, $4, $5 }'
	printf '\nTotal HT  : %s €\n' "$ht"
	if [ "$(taux)" = 0 ]; then
		printf 'TVA non applicable, art. 293 B du CGI\nNet à payer : %s €\n' "$ttc"
	else
		printf 'TVA %s : %s €\nTotal TTC : %s €\n' "$TVA" "$tva" "$ttc"
	fi
}

suite() {
	# La sortie standard de sermo part vers /dev/null (les lignes NOM=…) et
	# l'étape --do en hérite : écrire sur la sortie d'erreur, qui reste visible.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = calculer ] || exit 0
		[ -n "$CLIENT" ] || { echo "Client vide : pas de facture." >&2; exit 1; }
		mapfile -t lignes < <(calcul_lignes)
		[ ${#lignes[@]} -gt 0 ] || { echo "Aucune ligne au format « désignation ; quantité ; prix »." >&2; exit 1; }
		IFS='|' read -r ht tva ttc < <(calcul_totaux)
		export NUMERO DATE CLIENT ADRESSE LIGNES TVA ECHEANCE DOSSIER
		export DIALOGUE; DIALOGUE=$(dialogue_apercu "$NUMERO" "$CLIENT" "Total HT $ht € · TVA $tva € · Total TTC $ttc €" "${lignes[@]}")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite apercu' >/dev/null
		;;
	apercu)
		[ "$EXIT" = enregistrer ] || exit 0
		f="$DOSSIER/${NUMERO//[^[:alnum:]_-]/_}.txt"
		# Une facture émise ne se remplace pas : un numéro déjà pris est refusé.
		[ -e "$f" ] && { echo "La facture $f existe déjà : choisissez un autre numéro." >&2; exit 1; }
		[ -d "$DOSSIER" ] || agir mkdir -p -- "$DOSSIER"
		printf 'Facture pour %s :\n' "$f"
		# À blanc, tee ne lit rien : la facture est montrée ici. Pour de vrai, tee la recopie.
		[ "$SERMO_A_BLANC" = 1 ] && facture_texte
		facture_texte | agir tee -- "$f"
		if [ "$IMPRIMER" = true ]; then agir lp -t "Facture $NUMERO" -- "$f"; fi
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	for c in awk tee; do
		command -v "$c" >/dev/null || { echo "$c est absent : installez-le d'abord." >&2; exit 1; }
	done
	command -v lp >/dev/null || { echo "lp est absent : installez le paquet cups-client." >&2; exit 1; }
	local dossier=$HOME/Documents/Factures n
	# Numéro proposé : année + rang suivant parmi les factures du dossier.
	n=$(find "$dossier" -maxdepth 1 -name "F-$(date +%Y)-*.txt" 2>/dev/null | wc -l)
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$(printf 'F-%s-%03d' "$(date +%Y)" $((n + 1)))" "$(date +%F)" "$dossier")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
