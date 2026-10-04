#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# garde_progressbar_ligne_vide.sh — le thread de lecture de la barre de
# progression ne doit jamais lire oneline[length] sans avoir vérifié que
# length >= 0.
#
# Le défaut : « length = strlen(oneline) - 1 » vaut -1 quand la ligne lue
# commence par un octet nul. Lire oneline[-1], c'est lire hors du tampon — et
# y écrire si l'octet vaut un saut de ligne. Mesuré le 2026-10-04 sous
# AddressSanitizer, ports gtk3 et gtk4 : « stack-buffer-overflow, READ of size 1 ».
#
# Statique à dessein : sur un binaire ordinaire, la lecture d'un octet de pile
# ne se voit pas, et un banc comportemental resterait vert avec ou sans la
# garde. Seul un binaire construit sous AddressSanitizer la révèle ; pour
# rejouer la preuve : une barre dont l'<input> émet d'abord « \0abc\n ».
#
# Usage : garde_progressbar_ligne_vide.sh /chemin/vers/widget_progressbar.c
SRC="${1:?usage: garde_progressbar_ligne_vide.sh /chemin/vers/widget_progressbar.c}"

corps() {  # corps <fichier> : la fonction du thread, sans les commentaires
    awk '/^static gpointer widget_progressbar_thread_entry.*[^;]$/{f=1} f{print} f&&/^}/{exit}' "$1" \
        | grep -vE '^\s*(\*|/\*|//)'
}
sans_garde() {  # lignes qui lisent oneline[length] sans « length >= 0 » sur la même ligne
    grep -nE 'oneline\[length\]' | grep -E '\b(if|while)\b' | grep -vE 'length[[:space:]]*>=[[:space:]]*0'
}

# Témoin : la forme fautive doit être refusée, la forme gardée acceptée.
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
printf 'static gpointer widget_progressbar_thread_entry(progr_descr *descr)\n{\n\tlength = (gint)strlen(oneline) - 1;\n\tif (oneline[length] == 10)\n\t\toneline[length] = 0;\n}\n' > "$T/fautif.c"
printf 'static gpointer widget_progressbar_thread_entry(progr_descr *descr)\n{\n\tlength = (gint)strlen(oneline) - 1;\n\tif (length >= 0 && oneline[length] == 10)\n\t\toneline[length] = 0;\n}\n' > "$T/garde.c"
[ -n "$(corps "$T/fautif.c" | sans_garde)" ] || { echo "ÉCHEC DU TÉMOIN : la forme sans garde n'est pas vue"; exit 1; }
[ -z "$(corps "$T/garde.c" | sans_garde)" ] || { echo "ÉCHEC DU TÉMOIN : la forme gardée est refusée"; exit 1; }

BODY=$(corps "$SRC")
[ -z "$BODY" ] && { echo "ÉCHEC : fonction widget_progressbar_thread_entry introuvable"; exit 2; }
printf '%s\n' "$BODY" | grep -q 'oneline\[length\]' || { echo "ÉCHEC : plus aucune lecture de oneline[length] — le banc ne sait plus quoi garder"; exit 2; }
BAD=$(printf '%s\n' "$BODY" | sans_garde)
if [ -n "$BAD" ]; then
    echo "ÉCHEC : oneline[length] lu sans vérifier length >= 0 :"; printf '  %s\n' "$BAD"; exit 1
fi
echo "OK : oneline[length] n'est lu qu'après length >= 0"
