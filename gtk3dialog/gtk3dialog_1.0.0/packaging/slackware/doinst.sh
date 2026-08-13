#!/bin/sh
# doinst.sh
( cd usr/bin ; rm -f gtkdialog )
( cd usr/bin ; ln -sf gtk3dialog gtkdialog )
( cd usr/man/man1 ; rm -f gtkdialog.1 )
( cd usr/man/man1 ; ln -sf gtk3dialog.1 gtkdialog.1 )
