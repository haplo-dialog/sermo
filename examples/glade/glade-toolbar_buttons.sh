#!/bin/sh
# --glade-xml : gtk3sermo et gtk4sermo seulement. GTK 4 ne lit pas le format
# de GTK 3 : chaque toolkit a son fichier. GTKDIALOG choisit le binaire,
# comme dans les autres exemples (GTKDIALOG=gtk4sermo ./glade-toolbar_buttons.sh).

GTKDIALOG=${GTKDIALOG:-gtk3sermo}
case "$GTKDIALOG" in
	*gtk4*) INTERFACE=glade-toolbar_buttons-gtk4.ui ;;
	*)      INTERFACE=glade-toolbar_buttons.glade ;;
esac

$GTKDIALOG --glade-xml="$INTERFACE" \
          --program=MAIN_WINDOW
