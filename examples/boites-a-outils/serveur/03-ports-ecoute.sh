#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# Boîte    : Serveur
# Besoin   : voir quels programmes écoutent sur le réseau, et arrêter le service qui n'a rien à y faire
# Enrobe   : ss -tulnp (lecture) ; ps -o unit= pour trouver l'unité ; systemctl stop / disable / mask
# Ports    : ncursessermo conseillé (par SSH) ; gtk3sermo sur un poste
# Terminal : rien de perdu (le tableau montre six lignes à la fois) ; couper ssh demande de retaper son nom, et le masquer est refusé
# Usage    : ./03-ports-ecoute.sh [binaire sermo]
#            SERMO_A_BLANC=0 pour agir pour de vrai (défaut : la commande est affichée)
#            Sans les droits root, ss ne nomme que vos propres programmes et l'arrêt est seulement affiché.

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
		echo "Arrêter un service système demande les droits root : relancez ce script en root."
		echo "Voici les commandes qu'il lancerait :"
		SERMO_A_BLANC=1
	fi
}

dialogue_principal() {
	local ecoutes=${1:-"612|sshd|0.0.0.0|22|tcp
905|nginx|0.0.0.0|80|tcp
905|nginx|0.0.0.0|443|tcp
741|postgres|127.0.0.1|5432|tcp
1288|rpcbind|0.0.0.0|111|udp
1530|smbd|0.0.0.0|445|tcp"}
	cat <<XML
<window title="Ports à l'écoute" default-width="600">
  <vbox>
    <text><label>Programmes qui attendent des connexions (ss -tulnp) :</label></text>
    <table scrollable="true" height-request="170">
      <label>PID|Programme|Adresse|Port|Proto</label>
      <variable>PID_CHOISI</variable>
$(items "$ecoutes")
    </table>
    <frame label="Pour le service de la ligne choisie">
      <hbox>
        <radiobutton><label>Arrêter</label><default>true</default><variable>A_ARRETER</variable></radiobutton>
        <radiobutton><label>+ désactiver</label><variable>A_DESACTIVER</variable></radiobutton>
        <radiobutton><label>+ masquer</label><variable>A_MASQUER</variable></radiobutton>
      </hbox>
    </frame>
    <hbox>
      <button cancel></button>
      <button><label>Relire</label><action>exit:relire</action></button>
      <button><label>Agir sur le service</label><action>exit:agir</action></button>
    </hbox>
    <statusbar><variable>ETAT</variable><default>Lecture seule tant que vous n'avez pas confirmé.</default></statusbar>
  </vbox>
</window>
XML
}

dialogue_confirmation() {
	local unite=${1:-rpcbind.service} quoi=${2:-arrêté puis désactivé} pid=${3:-1288}
	cat <<XML
<window title="Confirmer l'arrêt">
  <vbox>
    <text><label>Le processus $(xml "$pid") appartient à l'unité $(xml "$unite").</label></text>
    <text><label>Elle sera : $(xml "$quoi").</label></text>
    <text><label>Ce qui en dépend s'arrêtera aussi.</label></text>
    <hbox>
      <button cancel></button>
      <button><label>Arrêter le service</label><action>exit:arreter</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Le service SSH : le couper coupe la session par laquelle on administre.
dialogue_confirmation_ssh() {
	local unite=${1:-ssh.service} quoi=${2:-arrêtée}
	cat <<XML
<window title="Couper SSH ?">
  <vbox>
    <text><label>$(xml "$unite") sera : $(xml "$quoi").</label></text>
    <text><label>Votre session SSH sera coupée, et plus personne n'entrera par SSH :</label></text>
    <text><label>il faudra la console de la machine pour le relancer.</label></text>
    <hbox><text><label>Tapez le nom de l'unité pour confirmer :</label></text>
      <entry space-expand="true" space-fill="true"><variable>RETAPE</variable></entry></hbox>
    <hbox>
      <button cancel></button>
      <button><label>Couper SSH</label><action>exit:arreter</action></button>
    </hbox>
  </vbox>
</window>
XML
}

# Code 0 si l'unité ne doit jamais être arrêtée d'ici : session de l'utilisateur
# (ps rend « user@1000.service » pour tout programme d'une session), cœur de systemd,
# bus, consoles, ou toute unité .scope. Arrêter puis masquer l'une d'elles casse la
# machine ou la session pour de bon.
unite_protegee() {
	case "$1" in
	*.scope|user@*.service|user-runtime-dir@*|session-*|systemd-*|dbus*|getty*|serial-getty*|init.*|-.slice) return 0 ;;
	esac
	return 1
}

# Code 0 pour le serveur SSH (« ssh.service », « sshd.service », « ssh@… »).
unite_ssh() { case "$1" in ssh.service|sshd.service|ssh@*.service|sshd@*.service) return 0 ;; esac; return 1; }

# Une ligne « pid|programme|adresse|port|proto » par socket à l'écoute (lecture seule).
lister() {
	ss -Htulnp 2>/dev/null | awk '{
		proto = $1; loc = $5; prog = "?"; pid = "-"
		if (match($0, /users:\(\("[^"]*",pid=[0-9]+/)) {
			u = substr($0, RSTART, RLENGTH); split(u, a, "\""); prog = a[2]
			sub(/.*pid=/, "", u); pid = u
		}
		port = loc; sub(/.*:/, "", port); adr = loc; sub(/:[^:]*$/, "", adr)
		print pid "|" prog "|" adr "|" port "|" proto
	}' | sort -t'|' -k4,4n -k5,5 -k3,3 | uniq
}

suite() {
	# La sortie standard de sermo part vers /dev/null : écrire sur la sortie d'erreur.
	exec >&2
	case "$1" in
	principal)
		case "$EXIT" in
		relire) exec "$0" "$SERMO" ;;
		agir) ;;
		*) exit 0 ;;
		esac
		[[ $PID_CHOISI =~ ^[0-9]+$ ]] || { echo "Choisissez une ligne dont le PID est connu (sans root, ss ne montre que vos programmes)."; exit 1; }
		local unite quoi="arrêtée"
		unite=$(ps -o unit= -p "$PID_CHOISI" 2>/dev/null | tr -d ' ')
		if unite_protegee "$unite"; then
			echo "Refus : le processus $PID_CHOISI relève de $unite (session, systemd, bus ou console) ; on ne l'arrête pas d'ici."; exit 1
		fi
		[[ $unite =~ ^[A-Za-z0-9@._-]+\.service$ ]] ||
			{ echo "Le processus $PID_CHOISI n'appartient à aucun service systemd (${unite:-inconnu}) : rien à arrêter ici."; exit 1; }
		[ "$A_DESACTIVER" = true ] && quoi="arrêtée puis désactivée"
		[ "$A_MASQUER" = true ] && quoi="arrêtée, désactivée et masquée"
		export UNITE=$unite A_ARRETER A_DESACTIVER A_MASQUER
		if unite_ssh "$unite"; then
			[ "$A_MASQUER" = true ] && { echo "Refus : masquer $unite fermerait SSH pour de bon. Choisissez Arrêter ou + désactiver."; exit 1; }
			export DIALOGUE; DIALOGUE=$(dialogue_confirmation_ssh "$unite" "$quoi")
		else
			export DIALOGUE; DIALOGUE=$(dialogue_confirmation "$unite" "$quoi" "$PID_CHOISI")
		fi
		"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite confirmation' >/dev/null
		;;
	confirmation)
		[ "$EXIT" = arreter ] || exit 0
		[[ $UNITE =~ ^[A-Za-z0-9@._-]+\.service$ ]] || exit 1
		unite_protegee "$UNITE" && { echo "Refus : $UNITE est protégée."; exit 1; }
		if unite_ssh "$UNITE"; then
			[ "$A_MASQUER" != true ] || { echo "Refus : on ne masque pas $UNITE."; exit 1; }
			[ "$RETAPE" = "$UNITE" ] || { echo "Nom retapé différent de $UNITE : rien n'est arrêté."; exit 1; }
		fi
		exiger_root
		if [ "$A_MASQUER" = true ]; then
			agir systemctl disable --now -- "$UNITE" && agir systemctl mask -- "$UNITE"
		elif [ "$A_DESACTIVER" = true ]; then
			agir systemctl disable --now -- "$UNITE"
		else
			agir systemctl stop -- "$UNITE"
		fi
		;;
	esac
}

main() {
	if [ "${1:-}" = --suite ]; then suite "$2"; exit; fi
	[ -n "${1:-}" ] && SERMO=$1
	command -v ss >/dev/null || { echo "ss est absent : installez le paquet iproute2." >&2; exit 1; }
	local liste
	liste=$(lister)
	[ -n "$liste" ] || liste="-|(aucun port à l'écoute lu)|||"
	export DIALOGUE; DIALOGUE=$(dialogue_principal "$liste")
	"$SERMO" --program=DIALOGUE --do='"$SERMO_SCRIPT" --suite principal' >/dev/null
}

[ "${BASH_SOURCE[0]}" = "$0" ] && main "$@"
