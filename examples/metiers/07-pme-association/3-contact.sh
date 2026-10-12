#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Métier   : PME, association (accueil, relation client)
# Besoin   : saisir la fiche d'un contact et l'enregistrer en vCard, importable dans tout carnet d'adresses
# Enrobe   : tee (fichier .vcf, vCard 3.0)
# Ports    : gtk3sermo conseillé ; qt6sermo, efl1sermo
# Terminal : rien de perdu
# Usage    : ./3-contact.sh [binaire sermo]
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

dialogue_principal() {
	local dossier=${1:-$HOME/Contacts}
	cat <<XML
<window title="Fiche contact" default-width="620">
  <vbox>
    <hbox>
      <frame label="Personne">
        <vbox>
          <text xalign="0"><label>Civilité :</label></text>
          <comboboxtext>
            <variable>CIVILITE</variable>
            <default>Mme</default>
            <item>Mme</item>
            <item>M.</item>
            <item>(aucune)</item>
          </comboboxtext>
          <text xalign="0"><label>Prénom :</label></text>
          <entry><variable>PRENOM</variable><default>Nadia</default></entry>
          <text xalign="0"><label>Nom :</label></text>
          <entry><variable>NOM</variable><default>Benali</default></entry>
          <text xalign="0"><label>Organisation :</label></text>
          <entry><variable>ORGANISATION</variable><default>Imprimerie du Centre</default></entry>
          <text xalign="0"><label>Fonction :</label></text>
          <entry><variable>FONCTION</variable><default>Responsable commerciale</default></entry>
        </vbox>
      </frame>
      <frame label="Coordonnées">
        <vbox>
          <text xalign="0"><label>Téléphone :</label></text>
          <entry><variable>TELEPHONE</variable><default>04 75 00 00 00</default></entry>
          <text xalign="0"><label>Mobile :</label></text>
          <entry><variable>MOBILE</variable><default>06 00 00 00 00</default></entry>
          <text xalign="0"><label>Courriel :</label></text>
          <entry><variable>COURRIEL</variable><default>n.benali@example.fr</default></entry>
          <text xalign="0"><label>Rue :</label></text>
          <entry><variable>RUE</variable><default>3 avenue Victor-Hugo</default></entry>
          <hbox>
            <entry width-chars="6"><variable>CODE_POSTAL</variable><default>26000</default></entry>
            <entry space-expand="true" space-fill="true"><variable>VILLE</variable><default>Valence</default></entry>
          </hbox>
        </vbox>
      </frame>
    </hbox>
    <hbox>
      <text><label>Catégorie :</label></text>
      <comboboxtext>
        <variable>CATEGORIE</variable>
        <default>Fournisseur</default>
        <item>Client</item>
        <item>Fournisseur</item>
        <item>Partenaire</item>
        <item>Bénévole</item>
        <item>Administration</item>
      </comboboxtext>
    </hbox>
    <text xalign="0"><label>Notes :</label></text>
    <edit height-request="70" wrap-mode="word"><variable>NOTES</variable><default>Devis papier recyclé à relancer fin du mois.</default></edit>
    <hbox>
      <text><label>Dossier des fiches :</label></text>
      <entry space-expand="true" space-fill="true"><variable>DOSSIER</variable><default>$(xml "$dossier")</default></entry>
    </hbox>
    <hbox>
      <button cancel></button>
      <button><label>Enregistrer la vCard</label><action>exit:enregistrer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

dialogue_ecraser() {
	local fichier=${1:-$HOME/Contacts/Benali_Nadia.vcf}
	cat <<XML
<window title="Fiche existante">
  <vbox>
    <text><label>Le fichier $(xml "$fichier") existe déjà.</label></text>
    <text><label>Le remplacer par la nouvelle fiche ? L'ancienne sera perdue.</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Remplacer</label><action>exit:remplacer</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Échappement vCard : « \ », « , », « ; » et sauts de ligne.
vc() { local s=${1//\\/\\\\}; s=${s//,/\\,}; s=${s//;/\\;}; printf '%s' "${s//$'\n'/\\n}"; }

# Nom de fichier sûr : lettres, chiffres, tiret, souligné.
fichier_de() { local n="${NOM}_${PRENOM}"; n=${n//[^[:alnum:]_-]/_}; printf '%s/%s.vcf' "$DOSSIER" "$n"; }

vcard() {
	local civ=$CIVILITE
	[ "$civ" = "(aucune)" ] && civ=""
	printf 'BEGIN:VCARD\r\nVERSION:3.0\r\n'
	printf 'N:%s;%s;;%s;\r\n' "$(vc "$NOM")" "$(vc "$PRENOM")" "$(vc "$civ")"
	printf 'FN:%s\r\n' "$(vc "$PRENOM $NOM")"
	[ -n "$ORGANISATION" ] && printf 'ORG:%s\r\n' "$(vc "$ORGANISATION")"
	[ -n "$FONCTION" ] && printf 'TITLE:%s\r\n' "$(vc "$FONCTION")"
	[ -n "$TELEPHONE" ] && printf 'TEL;TYPE=WORK,VOICE:%s\r\n' "$(vc "$TELEPHONE")"
	[ -n "$MOBILE" ] && printf 'TEL;TYPE=CELL:%s\r\n' "$(vc "$MOBILE")"
	[ -n "$COURRIEL" ] && printf 'EMAIL;TYPE=INTERNET:%s\r\n' "$(vc "$COURRIEL")"
	[ -n "$RUE$VILLE" ] && printf 'ADR;TYPE=WORK:;;%s;%s;;%s;France\r\n' "$(vc "$RUE")" "$(vc "$VILLE")" "$(vc "$CODE_POSTAL")"
	printf 'CATEGORIES:%s\r\n' "$(vc "$CATEGORIE")"
	[ -n "$NOTES" ] && printf 'NOTE:%s\r\n' "$(vc "$NOTES")"
	printf 'REV:%s\r\nEND:VCARD\r\n' "$(date -u +%Y%m%dT%H%M%SZ)"
}

enregistrer() {
	local f
	f=$(fichier_de)
	[ -d "$DOSSIER" ] || agir mkdir -p -- "$DOSSIER"
	printf 'Fiche pour %s :\n' "$f"
	# À blanc, tee ne lit rien : la fiche est montrée ici. Pour de vrai, tee la recopie.
	[ "$SERMO_A_BLANC" = 1 ] && vcard | tr -d '\r'
	vcard | agir tee -- "$f"
}

suite() {
	# La sortie standard de sermo part vers /dev/null (les lignes NOM=…) et
	# l'étape --do en hérite : écrire sur la sortie d'erreur, qui reste visible.
	exec >&2
	case "$1" in
	principal)
		[ "$EXIT" = enregistrer ] || exit 0
		[ -n "$NOM" ] || { echo "Le nom est obligatoire." >&2; exit 1; }
		f=$(fichier_de)
		if [ ! -e "$f" ]; then enregistrer; exit; fi
		export CIVILITE PRENOM NOM ORGANISATION FONCTION TELEPHONE MOBILE COURRIEL RUE CODE_POSTAL VILLE CATEGORIE NOTES DOSSIER
		export DIALOGUE; DIALOGUE=$(dialogue_ecraser "$f")
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite ecraser' >/dev/null
		;;
	ecraser)
		[ "$EXIT" = remplacer ] || exit 0
		enregistrer
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v tee >/dev/null || { echo "tee est absent : installez le paquet coreutils." >&2; exit 1; }
	export DIALOGUE; DIALOGUE=$(dialogue_principal)
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
