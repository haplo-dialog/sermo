#!/bin/bash
# SPDX-License-Identifier: CC0-1.0
# certificats-essai.sh — fabrique des certificats D'ESSAI (autosignés, 30 jours)
# pour essayer opcua-lecture.sh contre « faux-serveur-opcua PORT --securise ».
# Jamais pour un vrai équipement : là, le certificat du poste se fait selon les
# règles du site, et celui du serveur se récupère chez son administrateur.
# Usage : ./certificats-essai.sh DOSSIER   (le dossier est créé ; clés en 600)
# Produit : serveur.{cert,cle}.der (URI urn:sermo:faux-serveur-opcua) et
#           client.{cert,cle}.der  (URI urn:sermo:opcua-lire).
set -eu
[ $# = 1 ] || { echo "usage : $0 DOSSIER" >&2; exit 1; }
command -v openssl >/dev/null || { echo "openssl est absent : sudo apt install openssl" >&2; exit 1; }
mkdir -p -- "$1"
cd -- "$1"
umask 077
fabriquer() { # nom uri
	openssl req -x509 -newkey rsa:2048 -nodes -sha256 -days 30 -subj "/CN=$1/O=sermo essai" \
		-keyout "$1.cle.pem" -out "$1.cert.pem" \
		-addext "subjectAltName=URI:$2,DNS:localhost,IP:127.0.0.1" \
		-addext "basicConstraints=critical,CA:FALSE" \
		-addext "keyUsage=critical,digitalSignature,nonRepudiation,keyEncipherment,dataEncipherment,keyCertSign" \
		-addext "extendedKeyUsage=serverAuth,clientAuth" 2>/dev/null
	openssl x509 -in "$1.cert.pem" -outform DER -out "$1.cert.der"
	openssl pkey -in "$1.cle.pem" -outform DER -out "$1.cle.der"
	rm -f -- "$1.cert.pem" "$1.cle.pem"
}
fabriquer serveur urn:sermo:faux-serveur-opcua
fabriquer client urn:sermo:opcua-lire
cat <<FIN
Certificats d'essai dans $(pwd) :
  ./faux-serveur-opcua 4842 --securise $(pwd)/serveur.cert.der $(pwd)/serveur.cle.der $(pwd)/client.cert.der &
  OPCUA_ADRESSE=opc.tcp://127.0.0.1:4842 OPCUA_MODE=chiffre \\
  OPCUA_CERTIFICAT=$(pwd)/client.cert.der OPCUA_CLE=$(pwd)/client.cle.der \\
  OPCUA_CONFIANCE=$(pwd)/serveur.cert.der ./opcua-lecture.sh
FIN
