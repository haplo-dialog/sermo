#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/garde_progressbar_thread.sh — haplo-dialog — 2026 — GPL-2.0-or-later
# Garde-fou : le thread de lecture de la barre de progression ne doit toucher
# NI GTK NI GDK. Toute mise a jour passe par la boucle principale (g_idle_add).
#
# Statique et deterministe a dessein : la faute qu'il empeche est une course
# qui ne se declenche qu'une fois sur dix. Un test comportemental aurait ete
# vert la plupart du temps, donc n'aurait rien prouve.
SRC="${1:?usage: guard.sh /chemin/vers/widget_progressbar.c}"

# Temoin : une fonction de thread qui appelle GTK doit etre refusee, sinon ce
# banc dirait OK sans rien savoir voir.
T=$(mktemp); trap 'rm -f "$T"' EXIT
printf 'static gpointer widget_progressbar_thread_entry(gpointer data)\n{\n  gtk_widget_show(data);\n  g_idle_add(f, data);\n}\n' > "$T"
if [ -z "$(awk '/^static gpointer widget_progressbar_thread_entry.*[^;]$/{f=1} f{print} f&&/^}/{exit}' "$T" | grep -oE '\b(gtk|gdk)_[a-z0-9_]+')" ]; then
  echo "ECHEC DU TEMOIN : un appel GTK dans le thread n'est pas vu"; exit 1
fi

BODY=$(awk '/^static gpointer widget_progressbar_thread_entry.*[^;]$/{f=1} f{print} f&&/^}/{exit}' "$SRC" \
        | grep -vE '^\s*(\*|/\*|//)')
[ -z "$BODY" ] && { echo "ECHEC : fonction widget_progressbar_thread_entry introuvable"; exit 2; }
BAD=$(printf '%s\n' "$BODY" | grep -oE '\b(gtk|gdk)_[a-z0-9_]+' | sort -u)
if [ -n "$BAD" ]; then
  echo "ECHEC : le thread appelle GTK/GDK directement :"; printf '  %s\n' $BAD; exit 1
fi
printf '%s\n' "$BODY" | grep -q 'g_idle_add' || { echo "ECHEC : le thread ne delegue rien a la boucle principale"; exit 1; }
echo "OK : le thread ne touche ni GTK ni GDK, et delegue par g_idle_add"
