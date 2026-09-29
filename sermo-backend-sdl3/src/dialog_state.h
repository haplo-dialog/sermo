/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* dialog_state.h — Structure d'état du dialog (immediate mode bridge)
 *
 * sermo — haplo-dialog <devel@haplo-dialog.fr>
 *
 * PARADIGME IMMEDIATE MODE :
 * Le parser XML construit un arbre de WidgetNode[] à l'init.
 * La render_loop() parcourt cet arbre à chaque frame et appelle les
 * fonctions ImGui correspondantes. Les valeurs sont stockées dans les
 * noeuds eux-mêmes (state persistant dans la structure C).
 *
 * v2 — 2026-05-28 : ajout visible/sensitive/width/height,
 *      nouveaux types (PASSWORD, SWITCH, SEARCHENTRY, INFOBAR,
 *      LEVELBAR, CALENDAR, DRAWINGAREA, ASPECTFRAME, SPINNER, IMAGE_W),
 *      union étendue, terminal scrollback.
 */

#ifndef DIALOG_STATE_H
#define DIALOG_STATE_H

#include "sdl3-compat.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ─── Types de widgets supportés ─────────────────────────────────────────── */
typedef enum {
    WT_WINDOW = 0,
    WT_BUTTON,
    WT_CHECKBOX,
    WT_RADIOBUTTON,
    WT_ENTRY,
    WT_TEXT,
    WT_EDIT,
    WT_HBOX,
    WT_VBOX,
    WT_FRAME,
    WT_NOTEBOOK,
    WT_EXPANDER,
    WT_LIST,
    WT_COMBOBOX,
    WT_PROGRESSBAR,
    WT_HSCALE,
    WT_VSCALE,
    WT_SPINBUTTON,
    WT_STATUSBAR,
    WT_TIMER,
    WT_TOGGLEBUTTON,
    WT_SEPARATOR,
    WT_PIXMAP,
    WT_COLORBUTTON,   /* ImGui ColorEdit3 */
    WT_FONTBUTTON,    /* ImGui font list  */
    WT_TABLE,         /* ImGui::BeginTable */
    WT_MENUBAR,       /* ImGui::BeginMenuBar */
    WT_MENUITEM,      /* ImGui::MenuItem */
    WT_TREE,          /* ImGui::TreeNode */
    /* Nouveaux v2 */
    WT_PASSWORD,      /* InputText + ImGuiInputTextFlags_Password */
    WT_SWITCH,        /* Checkbox style toggle */
    WT_SEARCHENTRY,   /* InputText + filtre */
    WT_INFOBAR,       /* TextColored avec type info/warning/error */
    WT_LEVELBAR,      /* ProgressBar colorisée selon niveau */
    WT_CALENDAR,      /* Date picker J/M/A */
    WT_DRAWINGAREA,   /* ImDrawList canvas libre */
    WT_ASPECTFRAME,   /* Frame avec ratio imposé */
    WT_SPINNER,       /* Animation rotation (busy indicator) */
    WT_IMAGE_W,       /* SDL_Texture → ImGui::Image amélioré */
    WT_TERMINAL_W,    /* Console ImGui avec scrollback */
    /* v3 — 2026-09-13 : tags qui n'étaient que des stubs */
    WT_EVENTBOX,      /* conteneur capteur d'évènements (enfants + action) */
    WT_LINKBUTTON,    /* lien cliquable ; URI dans state.entry.buf */
    WT_PULSE,         /* barre indéterminée (ImGui::ProgressBar négatif) */
    WT_FILECHOOSER,   /* sélecteur natif SDL3 ; chemin dans state.entry.buf */
    WT_GRID,          /* mise en page en tableau, en flot */
    WT_PANED,         /* deux zones + poignée déplaçable */
    WT_TOOLBAR,       /* barre d'actions : une rangée stylée */
    WT_STACK,         /* N pages, une seule visible */
    WT_WIZARD,        /* suite d'étapes + navigation */
    WT_MENUBUTTON,    /* bouton qui déroule ses menuitem */
    WT_FLOWBOX,       /* rangement automatique en lignes */
    WT_OVERLAY,       /* enfants empilés l'un sur l'autre */
    WT_REVEALER,      /* un enfant qui se montre et se cache */
    WT_UNKNOWN
} WidgetType;

/* ─── État terminal avec scrollback ring-buffer ───────────────────────────── */
#define TERMINAL_SCROLLBACK 4096
#define TERMINAL_LINE_MAX   512

typedef struct {
    char  lines[TERMINAL_SCROLLBACK][TERMINAL_LINE_MAX]; /* ring buffer */
    int   head;           /* prochain slot à écrire */
    int   count;          /* lignes valides */
    char  cmd[1024];      /* commande associée */
    gboolean autoscroll;
} TerminalState;

/* ─── État colorbutton ────────────────────────────────────────────────────── */
typedef struct {
    float r, g, b, a;    /* composantes [0.0, 1.0] */
    char  hex[10];        /* "#rrggbb\\0" pour l'export */
} ColorState;

/* ─── État calendar ───────────────────────────────────────────────────────── */
typedef struct {
    int day, month, year;
} CalendarState;

/* ─── État drawingarea ────────────────────────────────────────────────────── */
typedef struct {
    float width, height;
    char  bg_color[10];   /* couleur de fond */
} DrawingState;

/* ─── Noeud de l'arbre de widgets ────────────────────────────────────────── */
typedef struct WidgetNode {
    WidgetType   type;
    char        *var_name;    /* variable name pour l'export stdout */
    char        *label;       /* texte affiché */
    char        *action;      /* commande shell à exécuter */
    void        *actions_attr; /* AttributeSet* : TOUTES les actions (timer) */
    void        *lecture;      /* sermo_progress* : commande <input> d'une barre, lue au fil de l'eau */
    char        *icon_path;    /* bouton : fichier d'icone de theme resolu */
    unsigned     icon_tex;     /* texture GL de l'icone (0 = pas chargee, ~0u = echec) */
    int          icon_px;      /* taille demandee de l'icone */
    int          expand;       /* space-expand : 1 vrai, -1 faux, 0 non dit */
    float        row_w;        /* hbox : largeur mesuree de la rangee (cadre a droite) */
    float        tail_h;       /* vbox : hauteur mesuree des freres QUI SUIVENT l'enfant
                                  extensible — il la leur laisse au lieu de tout prendre */
    unsigned     bg_rgba, fg_rgba;   /* background/foreground (0 = theme) */
    char        *tooltip;

    /* Propriétés de présentation (v2) */
    gboolean     visible;     /* FALSE = caché (pas rendu) */
    gboolean     sensitive;   /* FALSE = grisé */
    int          width;       /* 0 = auto */
    int          height;      /* 0 = auto */

    /* Premier refresh déjà passé. Chez l'étalon gtk3sermo, le cœur le note
     * dans « _initialised » ; le cœur neutre n'en garde rien (son
     * g_object_set_data est vide). Un widget dont un réglage ne vaut qu'au
     * démarrage — <default> d'un comboboxtext ou d'un edit, 1re rangée
     * choisie d'une list ou d'un tree — le pose donc lui-même à la fin de
     * son refresh. */
    gboolean     initialised;

    /* État persistant (mis à jour par ImGui à chaque frame) */
    union {
        struct { gboolean checked; }              checkbox;
        struct { int selected; }                  radio;
        struct { char buf[1024]; }                entry;
        struct { char *content; int len; int cap; } text;
        struct {
            double value, min, max, step;
            gboolean integer_mode;
            int      digits;       /* <hscale>/<vscale> : décimales exportées,
                                    * comme l'attribut digits= de l'étalon */
        } scale;
        struct { int current_tab; char **tab_labels; int tab_count; int side; } notebook;   /* side = onglets a gauche */
        struct { gboolean open; }                 expander;
        struct {
            int    selected_index;
            char **items;
            int    item_count;
            char   filter[256]; /* pour searchentry */
        } list;
        struct { double value; gboolean pulse_mode; } progress;
        struct { double value, min, max; int digits; } spin;
        struct { gboolean active; }               toggle;
        ColorState   color;
        char         font[256];        /* fontbutton */
        TerminalState terminal;        /* console */
        CalendarState calendar;        /* date picker */
        DrawingState  drawing;         /* canvas */
        struct {
            void *texture;             /* SDL_Texture* / id GL (NULL = aucune) */
            int   w, h;                /* dimensions natives */
        } image;                       /* pixmap / image */
        struct {
            double value, min, max;
            int    level;   /* 0=low 1=medium 2=high */
        } levelbar;
        struct {
            float  angle;   /* rotation actuelle (spinner) */
            double speed;   /* degrés/frame */
        } spinner;
        struct {
            /* table */
            int    cols, rows;
            char **headers;
            char **cells;   /* [row * cols + col] */
        } table;
        struct {
            char   message_type[16]; /* "info" "warning" "error" "question" */
            gboolean revealed;
        } infobar;
        struct {
            float ratio;  /* aspectframe */
        } aspect;
        struct {
            int      columns;      /* nombre de colonnes (>= 1) */
            int      row_spacing;
            int      col_spacing;
            gboolean homogeneous;
        } grid;                    /* <grid> : mise en page en tableau */
        struct {
            gboolean vertical;     /* TRUE = l'une au-dessus de l'autre */
            double   fraction;     /* part du 1er enfant (0..1) */
            int      pixels;       /* position en pixels/colonnes (0 = non dit) */
            gboolean resizable;    /* FALSE = poignée figée */
        } paned;                   /* <paned> : deux zones et une poignée */
    } state;

    /* Arbre */
    struct WidgetNode  *parent;
    struct WidgetNode **children;
    int                 child_count;
    int                 child_capacity;
} WidgetNode;

/* ─── Dialog global ───────────────────────────────────────────────────────── */
typedef struct {
    WidgetNode *root;        /* fenêtre principale */
    int         exit_code;   /* code retour quand l'utilisateur ferme */
    gboolean    should_quit;
    char        title[256];  /* titre de la fenêtre SDL3 */
} DialogState;

/* ─── API ─────────────────────────────────────────────────────────────────── */
DialogState *dialog_state_new(void);
void         dialog_state_free(DialogState *ds);
WidgetNode  *widget_node_new(WidgetType type, const char *name, const char *label);
void         widget_node_free(WidgetNode *node);
void         widget_node_add_child(WidgetNode *parent, WidgetNode *child);
void         dialog_state_export(DialogState *ds);
void         terminal_append_line(TerminalState *t, const char *line);
const char  *terminal_get_line(const TerminalState *t, int idx);

/* Conversion couleur hex ↔ RGB float */
void  color_from_hex(const char *hex, ColorState *out);
void  color_to_hex(const ColorState *in, char *out, int out_size);

#ifdef __cplusplus
}
#endif

#endif /* DIALOG_STATE_H */
