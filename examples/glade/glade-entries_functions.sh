#!/bin/sh
# --glade-xml : gtk3sermo et gtk4sermo seulement. GTK 4 ne lit pas le format
# de GTK 3 : chaque toolkit a son fichier. GTKDIALOG choisit le binaire,
# comme dans les autres exemples (GTKDIALOG=gtk4sermo ./glade-entries_functions.sh).

GTKDIALOG=${GTKDIALOG:-gtk3sermo}
case "$GTKDIALOG" in
	*gtk4*) INTERFACE=glade-entries_functions-gtk4.ui ;;
	*)      INTERFACE=glade-entries_functions.glade ;;
esac

$GTKDIALOG --glade-xml="$INTERFACE" \
          --include=glade-entries_functions.functions \
          --program=login_window
