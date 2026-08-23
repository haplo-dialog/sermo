#!/bin/bash
# tests/garde_progressbar_thread.sh — haplo-dialog — 2026 — GPL-2.0-or-later
# Garde-fou : le thread de lecture de la barre de progression ne doit toucher
# NI GTK NI GDK. Toute mise a jour passe par la boucle principale (g_idle_add).
#
# Statique et deterministe a dessein : la faute qu'il empeche est une course
# qui ne se declenche qu'une fois sur dix. Un test comportemental aurait ete
# vert la plupart du temps, donc n'aurait rien prouve.
SRC="${1:?usage: guard.sh /chemin/vers/widget_progressbar.c}"
BODY=$(awk '/^static gpointer widget_progressbar_thread_entry.*[^;]$/{f=1} f{print} f&&/^}/{exit}' "$SRC" \
        | grep -vE '^\s*(\*|/\*|//)')
[ -z "$BODY" ] && { echo "ECHEC : fonction widget_progressbar_thread_entry introuvable"; exit 2; }
BAD=$(printf '%s\n' "$BODY" | grep -oE '\b(gtk|gdk)_[a-z0-9_]+' | sort -u)
if [ -n "$BAD" ]; then
  echo "ECHEC : le thread appelle GTK/GDK directement :"; printf '  %s\n' $BAD; exit 1
fi
printf '%s\n' "$BODY" | grep -q 'g_idle_add' || { echo "ECHEC : le thread ne delegue rien a la boucle principale"; exit 1; }
echo "OK : le thread ne touche ni GTK ni GDK, et delegue par g_idle_add"
