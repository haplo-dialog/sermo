/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * sermo_icon_theme.h — résolution d'icônes de thème freedesktop, sans GTK
 * sermo — cœur partagé
 * Licence : GPL-2.0-or-later
 *
 * Les ports qui n'ont pas de bibliothèque d'icônes (fltk1, efl1, sdl3)
 * consomment ce fichier depuis l'étalon, comme le lexer et le parser.
 * Le port gtk3 ne l'utilise pas : GTK sait faire.
 *
 * Spécification suivie (simplifiée) : Icon Theme Specification 0.13 —
 * index.theme, Directories, Inherits, tailles Fixed/Scalable/Threshold,
 * puis repli hicolor et /usr/share/pixmaps.
 */
#ifndef SERMO_ICON_THEME_H
#define SERMO_ICON_THEME_H

#ifdef __cplusplus
extern "C" {
#endif

/* Nom du thème d'icônes du bureau : SERMO_ICON_THEME, sinon
 * ~/.config/gtk-3.0/settings.ini (gtk-icon-theme-name), sinon les
 * xsettings XFCE (IconThemeName), sinon "hicolor". Chaîne statique. */
const char *sermo_icon_theme_name(void);

/* Chemin du fichier d'icône (svg/png/xpm) le plus proche de `size` px,
 * ou NULL. À libérer par free(). */
char *sermo_icon_lookup(const char *name, int size);

/* Taille en pixels d'un icon-size gtkdialog :
 * menu/small/button = 16, large = 24 (défaut), dnd = 32, dialog = 48. */
int sermo_icon_size_px(const char *icon_size);

/* 1 si le bureau est en thème SOMBRE, 0 si clair (défaut). Ordre :
 * SERMO_DARK (0/1) prime, puis GTK_THEME, puis ~/.config/gtk-3.0/settings.ini
 * (gtk-theme-name / gtk-application-prefer-dark-theme), puis les xsettings
 * XFCE. Permet aux ports sans détection propre (qt6, sdl3) de SUIVRE le thème
 * système au lieu d'imposer le leur. */
int sermo_desktop_is_dark(void);

#ifdef __cplusplus
}
#endif

#endif /* SERMO_ICON_THEME_H */
