/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 S. Cage
 * haplo-dialog <devel@haplo-dialog.fr>
 */
/* render.cpp — Boucle de rendu ImGui pour sdl3sermo
 *
 * sermo
 * Licence : GPL-2.0-or-later
 *
 * Ce fichier est le cœur du port SDL3/ImGui :
 *   - Initialise SDL3 + OpenGL 3.3 + ImGui
 *   - Applique le thème Catppuccin Mocha
 *   - Parcourt l'arbre WidgetNode[] à chaque frame
 *   - Câble chaque WT_* vers son appel ImGui natif
 *   - Exporte les variables d'environnement pour --do
 *
 * PARADIGME :
 *   Le parser XML (C) construit dialog_tree[] à l'init.
 *   render_loop() (C++) parcourt cet arbre à chaque frame.
 *   Les valeurs persistantes sont dans WidgetNode.state.
 *   Les exports env se font via set_widget_env_var().
 *
 * Compilation :
 *   g++ -std=c++17 render.cpp ... -lSDL3 -lGL -limgui
 */

#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_misc.h>
#include "render.h"

/* ImGui + SDL3 backend headers — à adapter selon votre layout ImGui */
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include "image_load.h"
#include "sermo_icon_theme.h"

#include <cstdio>
#include <cstring>
#include <cmath>
#include <cfloat>
#include <ctime>
#include <cstdlib>
#include <string>
#include <vector>

extern "C" {
#include "dialog_state.h"
#include "variables.h"
#include "actions.h"
#include "attributes.h"
#include "sermo_progress.h"
/* machinerie de sortie (actions.c), jamais declaree en en-tete */
void action_exitprogram(GtkWidget *widget, char *string);
/* --render-png FILE : chemin du PNG a produire (option globale, gtkdialog.c). */
extern char *option_render_png;
}

#include <png.h>

/* ═══════════════════════════════════════════════════════════════════════════
 *  Ecriture PNG (rendu offscreen deterministe --render-png)
 *  Ecrit un buffer RGBA (origine bas-gauche, convention OpenGL) en PNG apres
 *  retournement vertical. Renvoie true si le fichier est ecrit.
 * ═══════════════════════════════════════════════════════════════════════════ */
static bool sermo_write_png_rgba_flipped(const char *path, int w, int h,
                                         const unsigned char *rgba)
{
    if (!path || !*path || w <= 0 || h <= 0 || !rgba) return false;

    FILE *fp = fopen(path, "wb");
    if (!fp) { fprintf(stderr, "render-png: fopen %s: %s\n", path, SDL_GetError()); return false; }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) { fclose(fp); return false; }
    png_infop info = png_create_info_struct(png);
    if (!info) { png_destroy_write_struct(&png, NULL); fclose(fp); return false; }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        return false;
    }

    png_init_io(png, fp);
    png_set_IHDR(png, info, (png_uint_32)w, (png_uint_32)h, 8,
                 PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);

    /* glReadPixels renvoie les lignes du bas vers le haut : on retourne. */
    std::vector<png_bytep> rows((size_t)h);
    const size_t stride = (size_t)w * 4u;
    for (int y = 0; y < h; ++y)
        rows[(size_t)y] = (png_bytep)(rgba + (size_t)(h - 1 - y) * stride);

    png_write_image(png, rows.data());
    png_write_end(png, NULL);
    png_destroy_write_struct(&png, &info);
    fclose(fp);
    return true;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Thème Catppuccin Mocha
 * ═══════════════════════════════════════════════════════════════════════════ */

/* Hauteur de contenu mesuree pendant le rendu de WT_WINDOW : sert a
 * dimensionner la fenetre SDL au CONTENU (parite etalon — les bancs
 * cliquent « en bas de fenetre »). 0 = pas encore mesuree. */
static float g_need_h = 0.0f;
/* Fenetre SDL courante : le selecteur de fichier natif veut une fenetre mere
 * (sans elle, le dialogue s'ouvre detache et peut passer derriere). */
static SDL_Window *g_sdl_window = nullptr;
/* vrai pendant le rendu des enfants d'un hbox : les boutons pleine
 * largeur y ecraseraient leurs voisins */
static bool g_in_hbox = false;

/* Vrai si l'arbre contient une barre de menus (pour poser le flag MenuBar
 * sur la fenetre ImGui — sinon BeginMenuBar echoue et le menu n'apparait pas). */
static bool tree_has_menubar(WidgetNode *n)
{
    if (!n) return false;
    if (n->type == WT_MENUBAR) return true;
    for (int i = 0; i < n->child_count; i++)
        if (tree_has_menubar(n->children[i])) return true;
    return false;
}

/* Charge (une fois) l'icone d'un noeud en texture GL. ~0u marque l'echec. */
static bool node_icon_texture(WidgetNode *node)
{
    if (!node->icon_path) return false;
    if (node->icon_tex == ~0u) return false;
    if (node->icon_tex) return true;
    int w = 0, h = 0;
    unsigned char *rgba = sermo_image_load_rgba(node->icon_path, node->icon_px, &w, &h);
    if (!rgba) { node->icon_tex = ~0u; return false; }
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    free(rgba);
    node->icon_tex = tex;
    node->state.image.w = w; node->state.image.h = h;
    return true;
}

/* Applique Catppuccin — Mocha (sombre) ou Latte (clair). Les deux variantes
 * remplissent EXACTEMENT le meme jeu de roles ci-dessous ; seules les 16
 * constantes de palette changent. */
static void apply_catppuccin(bool dark)
{
    ImGuiStyle &s = ImGui::GetStyle();
    ImVec4 *c     = s.Colors;

    ImVec4 base, mantle, surface0, surface1, surface2, overlay0, text, subtext1,
           blue, lavender, green, yellow, red, peach, mauve, teal;
    if (dark) {           /* Catppuccin Mocha */
        base    = {0.118f, 0.118f, 0.180f, 1.0f}; /* #1e1e2e */
        mantle  = {0.094f, 0.094f, 0.145f, 1.0f}; /* #181825 */
        surface0= {0.192f, 0.196f, 0.267f, 1.0f}; /* #313244 */
        surface1= {0.235f, 0.239f, 0.314f, 1.0f}; /* #3c3e50 */
        surface2= {0.278f, 0.282f, 0.361f, 1.0f}; /* #474869 */
        overlay0= {0.373f, 0.376f, 0.451f, 1.0f}; /* #5f6075 */
        text    = {0.804f, 0.839f, 0.957f, 1.0f}; /* #cdd6f4 */
        subtext1= {0.671f, 0.694f, 0.800f, 1.0f}; /* #abb3cd */
        blue    = {0.537f, 0.706f, 0.980f, 1.0f}; /* #89b4fa */
        lavender= {0.702f, 0.741f, 0.988f, 1.0f}; /* #b4befe */
        green   = {0.651f, 0.890f, 0.631f, 1.0f}; /* #a6e3a1 */
        yellow  = {0.976f, 0.886f, 0.686f, 1.0f}; /* #f9e2af */
        red     = {0.953f, 0.545f, 0.659f, 1.0f}; /* #f38ba8 */
        peach   = {0.980f, 0.702f, 0.529f, 1.0f}; /* #fab387 */
        mauve   = {0.796f, 0.651f, 0.969f, 1.0f}; /* #cba6f7 */
        teal    = {0.565f, 0.882f, 0.859f, 1.0f}; /* #94e2d5 */
    } else {              /* Catppuccin Latte (clair) */
        base    = {0.937f, 0.945f, 0.961f, 1.0f}; /* #eff1f5 */
        mantle  = {0.902f, 0.914f, 0.937f, 1.0f}; /* #e6e9ef */
        surface0= {0.800f, 0.816f, 0.855f, 1.0f}; /* #ccd0da */
        surface1= {0.737f, 0.753f, 0.800f, 1.0f}; /* #bcc0cc */
        surface2= {0.675f, 0.690f, 0.745f, 1.0f}; /* #acb0be */
        overlay0= {0.612f, 0.627f, 0.690f, 1.0f}; /* #9ca0b0 */
        text    = {0.298f, 0.310f, 0.412f, 1.0f}; /* #4c4f69 */
        subtext1= {0.361f, 0.373f, 0.467f, 1.0f}; /* #5c5f77 */
        blue    = {0.118f, 0.400f, 0.961f, 1.0f}; /* #1e66f5 */
        lavender= {0.447f, 0.529f, 0.992f, 1.0f}; /* #7287fd */
        green   = {0.251f, 0.627f, 0.169f, 1.0f}; /* #40a02b */
        yellow  = {0.875f, 0.557f, 0.114f, 1.0f}; /* #df8e1d */
        red     = {0.824f, 0.059f, 0.224f, 1.0f}; /* #d20f39 */
        peach   = {0.996f, 0.392f, 0.043f, 1.0f}; /* #fe640b */
        mauve   = {0.533f, 0.224f, 0.937f, 1.0f}; /* #8839ef */
        teal    = {0.090f, 0.573f, 0.600f, 1.0f}; /* #179299 */
    }

    c[ImGuiCol_Text]                  = text;
    c[ImGuiCol_TextDisabled]          = subtext1;
    c[ImGuiCol_WindowBg]              = base;
    c[ImGuiCol_ChildBg]               = mantle;
    c[ImGuiCol_PopupBg]               = mantle;
    c[ImGuiCol_Border]                = surface2;
    c[ImGuiCol_BorderShadow]          = {0, 0, 0, 0};
    c[ImGuiCol_FrameBg]               = surface0;
    c[ImGuiCol_FrameBgHovered]        = surface1;
    c[ImGuiCol_FrameBgActive]         = surface2;
    c[ImGuiCol_TitleBg]               = mantle;
    c[ImGuiCol_TitleBgActive]         = surface0;
    c[ImGuiCol_TitleBgCollapsed]      = mantle;
    c[ImGuiCol_MenuBarBg]             = mantle;
    c[ImGuiCol_ScrollbarBg]           = mantle;
    c[ImGuiCol_ScrollbarGrab]         = surface1;
    c[ImGuiCol_ScrollbarGrabHovered]  = surface2;
    c[ImGuiCol_ScrollbarGrabActive]   = overlay0;
    c[ImGuiCol_CheckMark]             = blue;
    c[ImGuiCol_SliderGrab]            = blue;
    c[ImGuiCol_SliderGrabActive]      = lavender;
    c[ImGuiCol_Button]                = surface0;
    c[ImGuiCol_ButtonHovered]         = surface1;
    c[ImGuiCol_ButtonActive]          = surface2;
    c[ImGuiCol_Header]                = surface0;
    c[ImGuiCol_HeaderHovered]         = surface1;
    c[ImGuiCol_HeaderActive]          = surface2;
    c[ImGuiCol_Separator]             = surface2;
    c[ImGuiCol_SeparatorHovered]      = blue;
    c[ImGuiCol_SeparatorActive]       = lavender;
    c[ImGuiCol_ResizeGrip]            = surface2;
    c[ImGuiCol_ResizeGripHovered]     = blue;
    c[ImGuiCol_ResizeGripActive]      = lavender;
    c[ImGuiCol_Tab]                   = surface0;
    c[ImGuiCol_TabHovered]            = surface1;
    c[ImGuiCol_TabActive]             = blue;
    c[ImGuiCol_TabUnfocused]          = surface0;
    c[ImGuiCol_TabUnfocusedActive]    = surface1;
    c[ImGuiCol_PlotLines]             = teal;
    c[ImGuiCol_PlotLinesHovered]      = green;
    c[ImGuiCol_PlotHistogram]         = blue;
    c[ImGuiCol_PlotHistogramHovered]  = lavender;
    c[ImGuiCol_TextSelectedBg]        = {blue.x, blue.y, blue.z, 0.35f};
    c[ImGuiCol_DragDropTarget]        = yellow;
    c[ImGuiCol_NavHighlight]          = blue;
    c[ImGuiCol_NavWindowingHighlight] = {1, 1, 1, 0.7f};
    c[ImGuiCol_NavWindowingDimBg]     = {0, 0, 0, 0.2f};
    c[ImGuiCol_ModalWindowDimBg]      = {0, 0, 0, 0.35f};

    s.WindowRounding    = 8.0f;
    s.ChildRounding     = 6.0f;
    s.FrameRounding     = 4.0f;
    s.PopupRounding     = 6.0f;
    s.ScrollbarRounding = 4.0f;
    s.GrabRounding      = 4.0f;
    s.TabRounding       = 4.0f;
    s.WindowBorderSize  = 1.0f;
    s.FrameBorderSize   = 0.0f;
    s.ItemSpacing       = {8, 6};
    s.WindowPadding     = {10, 10};
    s.FramePadding      = {6, 4};

    (void)red; (void)peach; (void)mauve; (void)green; (void)yellow;
    (void)teal; (void)lavender;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Helpers — export variables env pour --do
 * ═══════════════════════════════════════════════════════════════════════════ */

static void set_widget_env_var(const char *var_name, const char *value)
{
    if (!var_name || !value) return;
    setenv(var_name, value, 1);
}

static void export_bool(const char *var, bool val)
{
    set_widget_env_var(var, val ? "true" : "false");
}

static void export_float(const char *var, float val)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%.6g", (double)val);
    set_widget_env_var(var, buf);
}

static void export_int(const char *var, int val)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", val);
    set_widget_env_var(var, buf);
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Rendu d'un seul WidgetNode — appelé récursivement
 * ═══════════════════════════════════════════════════════════════════════════ */

static void render_widget(WidgetNode *node);

/* Rappel du selecteur de fichier natif SDL3. `filelist` est NULL en cas
 * d'erreur, et vide si l'utilisateur a renonce : dans les deux cas on ne
 * touche pas a la valeur courante. */
static void SDLCALL filechooser_retour(void *userdata, const char * const *filelist, int filter)
{
    (void) filter;
    WidgetNode *n = (WidgetNode *) userdata;
    if (!n || !filelist || !filelist[0]) return;
    snprintf(n->state.entry.buf, sizeof(n->state.entry.buf), "%s", filelist[0]);
}

/* ─── Helpers couleur pour infobar / levelbar ─────────────────────────────── */

static ImVec4 infobar_color(const char *message_type)
{
    /* message_type est une chaîne ("info"/"warning"/"error"/…) ; on la mappe
     * vers un niveau 0=info, 1=warning, 2=error, 3=autre. */
    int level = !message_type                       ? 3 :
                !strcmp(message_type, "info")        ? 0 :
                !strcmp(message_type, "warning")     ? 1 :
                !strcmp(message_type, "error")       ? 2 : 3;
    switch (level) {
    case 0:  return {0.537f, 0.706f, 0.980f, 1.0f}; /* blue  info    */
    case 1:  return {0.976f, 0.886f, 0.686f, 1.0f}; /* yellow warning */
    case 2:  return {0.953f, 0.545f, 0.659f, 1.0f}; /* red  error   */
    default: return {0.804f, 0.839f, 0.957f, 1.0f}; /* text other   */
    }
}

static ImVec4 levelbar_color(float frac)
{
    if (frac < 0.33f) return {0.651f, 0.890f, 0.631f, 1.0f}; /* green  low  */
    if (frac < 0.66f) return {0.976f, 0.886f, 0.686f, 1.0f}; /* yellow mid  */
    return                    {0.953f, 0.545f, 0.659f, 1.0f}; /* red    high */
}

/* ─── Rendu récursif ─────────────────────────────────────────────────────── */

static void render_widget(WidgetNode *node)
{
    if (!node) return;
    /* WT_TIMER n'a rien à dessiner et est conventionnellement marqué
     * visible="false" dans le XML : sa cadence (case WT_TIMER plus bas)
     * doit continuer même caché, sinon un timer invisible ne se déclenche
     * plus jamais (masqué par ce garde-fou avant même d'être atteint). */
    /* Une barre relève sa commande à chaque image, même cachée : l'étalon
     * lit son tube dans un fil qui ne regarde pas la visibilité. */
    if (node->type == WT_PROGRESSBAR && node->lecture &&
        !sermo_progress_poll((sermo_progress *)node->lecture))
        node->lecture = NULL;   /* rendue par le cœur */
    if (!node->visible && node->type != WT_TIMER) return;

    /* Grisé */
    if (!node->sensitive) {
        ImGui::BeginDisabled(true);
    }

    /* Taille personnalisée */
    ImVec2 sz = {
        node->width  > 0 ? (float)node->width  : 0.0f,
        node->height > 0 ? (float)node->height : 0.0f
    };

    /* Identifiant ImGui unique : nom du widget ou "##idx" */
    char id[256];
    if (node->var_name && node->var_name[0])
        snprintf(id, sizeof(id), "##%s", node->var_name);
    else
        snprintf(id, sizeof(id), "##node%p", (void *)node);

    char label[512];
    snprintf(label, sizeof(label), "%s%s",
             node->label ? node->label : "",
             id);

    switch (node->type) {

    /* ── WINDOW ──────────────────────────────────────────────────────────── */
    case WT_WINDOW: {
        /* La fenetre X11 (SDL) EST le dialogue : la fenetre ImGui la
         * remplit sans decoration — l'ancienne fenetre ImGui flottante
         * decoree dans un SDL 800x600 mettait les widgets hors de portee
         * des bancs (clic « bas de fenetre » dans le vide). */
        ImGuiWindowFlags wf = ImGuiWindowFlags_NoDecoration |
                              ImGuiWindowFlags_NoMove;
        if (node->state.toggle.active || tree_has_menubar(node))
            wf |= ImGuiWindowFlags_MenuBar;
        ImGui::SetNextWindowPos({0.0f, 0.0f});
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        bool open = true;
        if (ImGui::Begin(node->label ? node->label : "sdl3sermo", &open, wf)) {
            for (int i = 0; i < node->child_count; i++)
                render_widget(node->children[i]);
            /* contenu : hauteur atteinte + marge basse */
            g_need_h = ImGui::GetCursorPosY() + ImGui::GetStyle().WindowPadding.y;
        }
        ImGui::End();
        if (!open) SDL_Event ev; /* signal fermeture */
        break;
    }

    /* ── BUTTON ──────────────────────────────────────────────────────────── */
    case WT_BUTTON: {
        /* pleine largeur par defaut, sauf dans un hbox (voisins en ligne) */
        ImVec2 bsz = sz.x > 0 ? sz
                   : (g_in_hbox ? ImVec2(0, 0) : ImVec2(-FLT_MIN, 0));
        /* icone a gauche du libelle : on reserve la place dans le bouton
         * puis on dessine l'image par-dessus (ImGui n'a pas de bouton
         * image+texte natif) */
        bool has_icon = node->icon_path && node_icon_texture(node);
        float ipx = has_icon ? (float)node->icon_px : 0.0f;
        std::string padded;
        const char *blabel = label;
        if (has_icon) {
            padded = std::string("   ") + label;      /* espace pour l'icone */
            blabel = padded.c_str();
        }
        ImVec2 bpos = ImGui::GetCursorScreenPos();
        bool clicked = ImGui::Button(blabel, bsz);
        if (has_icon) {
            ImVec2 bmin = ImGui::GetItemRectMin(), bmax = ImGui::GetItemRectMax();
            float pad = ImGui::GetStyle().FramePadding.x;
            float y0 = bmin.y + ((bmax.y - bmin.y) - ipx) * 0.5f;
            ImGui::GetWindowDrawList()->AddImage((ImTextureID)(uintptr_t)node->icon_tex,
                ImVec2(bmin.x + pad, y0), ImVec2(bmin.x + pad + ipx, y0 + ipx));
        }
        (void)bpos;
        if (clicked) {
            if (node->action && *node->action) {
                /* type NULL : le prefixe (exit:, refresh:...) est dans la
                 * commande elle-meme — "clicked" court-circuitait le
                 * repartiteur (type inconnu = rien execute). */
                execute_action((GtkWidget *)node, node->action, NULL);
            } else if (!(node->var_name && strcmp(node->var_name, "__fontbutton__") == 0)) {
                /* ⛔ <fontbutton> est construit comme un bouton (widget_fontbutton.c)
                 * portant la marque __fontbutton__ : sans ce test, un clic dessus
                 * FERMAIT le dialogue, EXIT = le nom de la police. Trouvé par
                 * garde_clic_widgets le jour où elle a su voir la fenêtre sdl3. */
                /* gtkdialog : un bouton nu ferme le dialogue — variables +
                 * EXIT = valeur de famille (tooltip) sinon le label DU NOEUD
                 * (le label ImGui porte un suffixe ##id : pas pour EXIT). */
                action_exitprogram((GtkWidget *)node,
                    (char *)(node->tooltip ? node->tooltip
                           : (node->label && node->label[0] ? node->label
                                                            : "OK")));
            }
        }
        break;
    }

    /* ── CHECKBOX ────────────────────────────────────────────────────────── */
    case WT_CHECKBOX: {
        bool checked = node->state.toggle.active;
        if (ImGui::Checkbox(label, &checked)) {
            node->state.toggle.active = checked;
            if (node->var_name)
                export_bool(node->var_name, node->state.toggle.active);
        }
        break;
    }

    /* ── RADIOBUTTON ─────────────────────────────────────────────────────── */
    case WT_RADIOBUTTON: {
        /* Les radiobuttons du même groupe partagent var_name */
        bool selected = node->state.toggle.active;
        if (ImGui::RadioButton(label, selected)) {
            node->state.toggle.active = true;
            if (node->var_name)
                set_widget_env_var(node->var_name, node->label ? node->label : "");
        }
        break;
    }

    /* ── ENTRY ───────────────────────────────────────────────────────────── */
    case WT_ENTRY: {
        ImGui::PushItemWidth(sz.x > 0 ? sz.x : 200.0f);
        if (node->label && node->label[0]) {
            ImGui::LabelText("##lbl", "%s", node->label);
            ImGui::SameLine();
        }
        if (ImGui::InputText(id, node->state.entry.buf,
                             sizeof(node->state.entry.buf))) {
            if (node->var_name)
                set_widget_env_var(node->var_name, node->state.entry.buf);
        }
        ImGui::PopItemWidth();
        break;
    }

    /* ── PASSWORD ────────────────────────────────────────────────────────── */
    case WT_PASSWORD: {
        ImGui::PushItemWidth(sz.x > 0 ? sz.x : 200.0f);
        if (node->label && node->label[0]) {
            ImGui::LabelText("##lbl", "%s", node->label);
            ImGui::SameLine();
        }
        if (ImGui::InputText(id, node->state.entry.buf,
                             sizeof(node->state.entry.buf),
                             ImGuiInputTextFlags_Password)) {
            if (node->var_name)
                set_widget_env_var(node->var_name, node->state.entry.buf);
        }
        ImGui::PopItemWidth();
        break;
    }

    /* ── EDIT (multiline) ────────────────────────────────────────────────── */
    case WT_EDIT: {
        /* pleine largeur ; toute la hauteur restante si space-expand */
        ImVec2 avail = ImGui::GetContentRegionAvail();
        /* space-expand : remplir le cadre. Le noeud porte 200x100 (taille par
         * defaut du « scrolled window » de l'automate) qu'on ignore alors. */
        ImVec2 msz = {
            node->expand == 1 ? -FLT_MIN : (sz.x > 0 ? sz.x : -FLT_MIN),
            node->expand == 1 && avail.y > 60.0f ? avail.y : (sz.y > 0 ? sz.y : 120.0f)
        };
        /* le contenu vit dans state.text (tas), pas dans entry.buf : lui
         * garantir une capacite d'edition avant de le confier a ImGui */
        {
            int need = node->state.text.len + 1;
            if (need < 8192) need = 8192;
            if (node->state.text.cap < need) {
                char *nb = (char *)realloc(node->state.text.content, need);
                if (nb) {
                    if (!node->state.text.content) nb[0] = '\0';
                    node->state.text.content = nb;
                    node->state.text.cap = need;
                }
            }
        }
        int pushed = 0;
        if (node->bg_rgba) { unsigned v = node->bg_rgba; ImGui::PushStyleColor(ImGuiCol_FrameBg,
            IM_COL32((v >> 16) & 0xff, (v >> 8) & 0xff, v & 0xff, 255)); pushed++; }
        if (node->fg_rgba) { unsigned v = node->fg_rgba; ImGui::PushStyleColor(ImGuiCol_Text,
            IM_COL32((v >> 16) & 0xff, (v >> 8) & 0xff, v & 0xff, 255)); pushed++; }
        if (node->state.text.content &&
            ImGui::InputTextMultiline(id, node->state.text.content,
                                      node->state.text.cap, msz)) {
            node->state.text.len = (int)strlen(node->state.text.content);
            if (node->var_name)
                set_widget_env_var(node->var_name, node->state.text.content);
        }
        if (pushed) ImGui::PopStyleColor(pushed);
        break;
    }

    /* ── TEXT ────────────────────────────────────────────────────────────── */
    case WT_TEXT: {
        ImGui::TextWrapped("%s", node->label ? node->label : "");
        break;
    }

    /* ── SEARCHENTRY ─────────────────────────────────────────────────────── */
    case WT_SEARCHENTRY: {
        ImGui::PushItemWidth(sz.x > 0 ? sz.x : 200.0f);
        char search_id[280];
        snprintf(search_id, sizeof(search_id), "%s%s",
                 node->label ? node->label : "Rechercher...", id);
        if (ImGui::InputText(search_id, node->state.list.filter,
                             sizeof(node->state.list.filter))) {
            if (node->var_name)
                set_widget_env_var(node->var_name, node->state.list.filter);
        }
        ImGui::PopItemWidth();
        /* Icône loupe */
        /* pas de glyphe loupe : DejaVu n'a pas l'emoji (rendait « � ») */
        break;
    }

    /* ── SWITCH ──────────────────────────────────────────────────────────── */
    case WT_SWITCH: {
        /* ImGui n'a pas de widget Switch natif — on simule avec un bouton coloré */
        bool on = node->state.toggle.active;
        ImVec4 col_on  = {0.537f, 0.706f, 0.980f, 1.0f}; /* blue  */
        ImVec4 col_off = {0.192f, 0.196f, 0.267f, 1.0f}; /* surface0 */
        ImGui::PushStyleColor(ImGuiCol_Button,        on ? col_on : col_off);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, on ? col_on : col_off);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  on ? col_off : col_on);
        char sw_label[512];
        snprintf(sw_label, sizeof(sw_label), "%s  %s%s",
                 node->label ? node->label : "",
                 on ? "●" : "○", id);
        if (ImGui::Button(sw_label, {sz.x > 0 ? sz.x : 80.0f, 24.0f})) {
            node->state.toggle.active = !on;
            if (node->var_name)
                export_bool(node->var_name, node->state.toggle.active);
        }
        ImGui::PopStyleColor(3);
        break;
    }

    /* ── COMBOBOX ────────────────────────────────────────────────────────── */
    case WT_COMBOBOX: {
        ImGui::PushItemWidth(sz.x > 0 ? sz.x : 150.0f);
        const char *preview = (node->state.list.selected_index >= 0 &&
                               node->state.list.selected_index < node->state.list.item_count)
                              ? node->state.list.items[node->state.list.selected_index]
                              : "—";
        if (ImGui::BeginCombo(id, preview)) {
            for (int i = 0; i < node->state.list.item_count; i++) {
                bool sel = (i == node->state.list.selected_index);
                if (ImGui::Selectable(node->state.list.items[i], sel)) {
                    node->state.list.selected_index = i;
                    if (node->var_name)
                        set_widget_env_var(node->var_name,
                                          node->state.list.items[i]);
                }
                if (sel) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
        break;
    }

    /* ── LIST ────────────────────────────────────────────────────────────── */
    case WT_LIST: {
        ImVec2 lsz = {sz.x > 0 ? sz.x : 200.0f, sz.y > 0 ? sz.y : 120.0f};
        if (ImGui::BeginListBox(id, lsz)) {
            for (int i = 0; i < node->state.list.item_count; i++) {
                bool sel = (i == node->state.list.selected_index);
                if (ImGui::Selectable(node->state.list.items[i], sel)) {
                    node->state.list.selected_index = i;
                    if (node->var_name)
                        set_widget_env_var(node->var_name,
                                          node->state.list.items[i]);
                }
            }
            ImGui::EndListBox();
        }
        break;
    }

    /* ── PROGRESSBAR ─────────────────────────────────────────────────────── */
    case WT_PROGRESSBAR: {
        float frac = (float)node->state.scale.value;
        if (frac > 1.0f) frac = 1.0f;
        if (frac < 0.0f) frac = 0.0f;
        /* Le texte de la barre (son <label>, puis chaque ligne non numérique
         * de sa commande), comme l'étalon ; à défaut, le pourcentage. */
        char overlay[32];
        snprintf(overlay, sizeof(overlay), "%.0f%%", frac * 100.0f);
        ImGui::ProgressBar(frac, {sz.x > 0 ? sz.x : -1.0f, sz.y > 0 ? sz.y : 0},
                           (node->label && *node->label) ? node->label : overlay);
        break;
    }

    /* ── HSCALE ──────────────────────────────────────────────────────────── */
    case WT_HSCALE: {
        ImGui::PushItemWidth(sz.x > 0 ? sz.x : 200.0f);
        float v = (float)node->state.scale.value;
        float mn = (float)node->state.scale.min;
        float mx = (float)node->state.scale.max;
        const char *fmt = (node->state.scale.step >= 1.0) ? "%.0f" : "%.2f";
        if (ImGui::SliderFloat(label, &v, mn, mx, fmt)) {
            node->state.scale.value = v;
            if (node->var_name) export_float(node->var_name, v);
        }
        ImGui::PopItemWidth();
        break;
    }

    /* ── VSCALE ──────────────────────────────────────────────────────────── */
    case WT_VSCALE: {
        float v = (float)node->state.scale.value;
        float mn = (float)node->state.scale.min;
        float mx = (float)node->state.scale.max;
        float h = sz.y > 0 ? sz.y : 150.0f;
        const char *fmt = (node->state.scale.step >= 1.0) ? "%.0f" : "%.2f";
        if (ImGui::VSliderFloat(label, {20.0f, h}, &v, mn, mx, fmt)) {
            node->state.scale.value = v;
            if (node->var_name) export_float(node->var_name, v);
        }
        break;
    }

    /* ── SPINBUTTON ──────────────────────────────────────────────────────── */
    case WT_SPINBUTTON: {
        ImGui::PushItemWidth(sz.x > 0 ? sz.x : 100.0f);
        int v = (int)node->state.scale.value;
        if (ImGui::InputInt(label, &v)) {
            node->state.scale.value = (double)v;
            if (node->var_name) export_int(node->var_name, v);
        }
        ImGui::PopItemWidth();
        break;
    }

    /* ── LEVELBAR ────────────────────────────────────────────────────────── */
    case WT_LEVELBAR: {
        float mn   = node->state.levelbar.min;
        float mx   = node->state.levelbar.max;
        float val  = node->state.levelbar.value;
        float frac = (mx > mn) ? (val - mn) / (mx - mn) : 0.0f;
        frac = frac < 0.0f ? 0.0f : (frac > 1.0f ? 1.0f : frac);

        ImVec4 col = levelbar_color(frac);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, col);
        char ov[32]; snprintf(ov, sizeof(ov), "%.1f", (double)val);
        ImGui::ProgressBar(frac, {sz.x > 0 ? sz.x : -1.0f, sz.y > 0 ? sz.y : 16.0f}, ov);
        ImGui::PopStyleColor();
        break;
    }

    /* ── INFOBAR ─────────────────────────────────────────────────────────── */
    case WT_INFOBAR: {
        if (!node->state.infobar.revealed) break;
        const char *mt = node->state.infobar.message_type;
        ImVec4 col = infobar_color(mt);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, {col.x, col.y, col.z, 0.15f});
        ImGui::BeginChild(id, {sz.x > 0 ? sz.x : -1.0f, sz.y > 0 ? sz.y : 36.0f},
                          ImGuiChildFlags_None);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6.0f);
        const char *icon = !strcmp(mt, "info")    ? "ℹ" :
                           !strcmp(mt, "warning") ? "⚠" : "✗";
        ImGui::TextColored(col, "  %s  %s", icon,
                           node->label ? node->label : "");
        ImGui::EndChild();
        ImGui::PopStyleColor();
        break;
    }

    /* ── CALENDAR ────────────────────────────────────────────────────────── */
    case WT_CALENDAR: {
        ImGui::Text("%s", node->label ? node->label : "Date :");
        ImGui::SameLine();
        ImGui::PushItemWidth(60.0f);
        ImGui::InputInt("##day",   &node->state.calendar.day,   0, 0);
        ImGui::SameLine(); ImGui::Text("/");
        ImGui::SameLine();
        ImGui::InputInt("##month", &node->state.calendar.month, 0, 0);
        ImGui::SameLine(); ImGui::Text("/");
        ImGui::SameLine();
        ImGui::PushItemWidth(80.0f);
        ImGui::InputInt("##year",  &node->state.calendar.year,  0, 0);
        ImGui::PopItemWidth(); ImGui::PopItemWidth();

        /* Clamp valeurs */
        if (node->state.calendar.day   <  1)  node->state.calendar.day   = 1;
        if (node->state.calendar.day   > 31)  node->state.calendar.day   = 31;
        if (node->state.calendar.month <  1)  node->state.calendar.month = 1;
        if (node->state.calendar.month > 12)  node->state.calendar.month = 12;

        /* Export ISO 8601 */
        if (node->var_name) {
            char iso[12];
            snprintf(iso, sizeof(iso), "%04d-%02d-%02d",
                     node->state.calendar.year,
                     node->state.calendar.month,
                     node->state.calendar.day);
            set_widget_env_var(node->var_name, iso);
        }
        break;
    }

    /* ── COLORBUTTON ─────────────────────────────────────────────────────── */
    case WT_COLORBUTTON: {
        float col[3] = {
            node->state.color.r,
            node->state.color.g,
            node->state.color.b
        };
        /* pastille compacte (aperçu couleur, sélecteur au clic) — pas les
         * trois champs RGB : parité avec la « swatch » gtk3/qt6 */
        if (ImGui::ColorEdit3(label, col, ImGuiColorEditFlags_NoInputs)) {
            node->state.color.r = col[0];
            node->state.color.g = col[1];
            node->state.color.b = col[2];
            /* Export #RRGGBB */
            snprintf(node->state.color.hex, sizeof(node->state.color.hex),
                     "#%02X%02X%02X",
                     (int)(col[0] * 255.0f),
                     (int)(col[1] * 255.0f),
                     (int)(col[2] * 255.0f));
            if (node->var_name)
                set_widget_env_var(node->var_name, node->state.color.hex);
        }
        break;
    }

    /* ── FONTBUTTON ──────────────────────────────────────────────────────── */
    case WT_FONTBUTTON: {
        /* ImGui n'a pas de sélecteur de police natif — on affiche le nom actuel */
        ImGui::Text("Police : %s",
                    node->state.entry.buf[0] ? node->state.entry.buf : "Sans Serif");
        ImGui::SameLine();
        if (ImGui::SmallButton("Choisir...")) {
            /* TODO: ouvrir un popup liste de polices système */
        }
        break;
    }

    /* ── SPINNER (busy indicator) ────────────────────────────────────────── */
    case WT_SPINNER: {
        /* Animation d'arc tournant via DrawList */
        ImVec2 pos  = ImGui::GetCursorScreenPos();
        float  r    = sz.x > 0 ? sz.x * 0.5f : 14.0f;
        float  t    = (float)ImGui::GetTime();
        float  spd  = node->state.spinner.speed > 0.0f
                      ? node->state.spinner.speed : 2.0f;
        node->state.spinner.angle = t * spd;

        ImDrawList *dl = ImGui::GetWindowDrawList();
        ImVec2 center  = {pos.x + r + 2.0f, pos.y + r + 2.0f};
        const int N    = 32;
        float a0       = node->state.spinner.angle;
        float arc      = 1.8f; /* longueur de l'arc en radians */

        for (int i = 0; i < N; i++) {
            float a1 = a0 + (float)i / (float)N * arc;
            float a2 = a0 + (float)(i + 1) / (float)N * arc;
            float alpha = (float)(i + 1) / (float)N;
            ImU32 col = IM_COL32(
                (int)(0.537f * 255),
                (int)(0.706f * 255),
                (int)(0.980f * 255),
                (int)(alpha * 255)
            );
            ImVec2 p1 = {center.x + cosf(a1) * r, center.y + sinf(a1) * r};
            ImVec2 p2 = {center.x + cosf(a2) * r, center.y + sinf(a2) * r};
            dl->AddLine(p1, p2, col, 3.0f);
        }
        ImGui::Dummy({(r + 4.0f) * 2.0f, (r + 4.0f) * 2.0f});
        if (node->label && node->label[0]) {
            ImGui::SameLine();
            ImGui::TextDisabled("%s", node->label);
        }
        break;
    }

    /* ── DRAWINGAREA ─────────────────────────────────────────────────────── */
    case WT_DRAWINGAREA: {
        float w = node->state.drawing.width  > 0 ? (float)node->state.drawing.width  : 200.0f;
        float h = node->state.drawing.height > 0 ? (float)node->state.drawing.height : 150.0f;
        ImVec2 p0 = ImGui::GetCursorScreenPos();
        ImDrawList *dl = ImGui::GetWindowDrawList();

        /* Fond */
        ImU32 bg = IM_COL32(
            (int)(node->state.drawing.bg_color[0] * 255),
            (int)(node->state.drawing.bg_color[1] * 255),
            (int)(node->state.drawing.bg_color[2] * 255), 255);
        dl->AddRectFilled(p0, {p0.x + w, p0.y + h}, bg, 4.0f);
        dl->AddRect(p0, {p0.x + w, p0.y + h},
                    IM_COL32(69, 71, 90, 255), 4.0f); /* surface2 border */

        /* Zone cliquable pour futur canvas interactif */
        ImGui::InvisibleButton(id, {w, h});
        if (ImGui::IsItemHovered()) {
            ImVec2 mp = ImGui::GetMousePos();
            if (node->var_name) {
                char coords[32];
                snprintf(coords, sizeof(coords), "%.0f,%.0f",
                         (double)(mp.x - p0.x), (double)(mp.y - p0.y));
                set_widget_env_var(node->var_name, coords);
            }
        }
        break;
    }

    /* ── ASPECTFRAME ─────────────────────────────────────────────────────── */
    case WT_ASPECTFRAME: {
        float ratio = node->state.aspect.ratio > 0.0f
                      ? node->state.aspect.ratio : 1.0f;
        float avail_w = ImGui::GetContentRegionAvail().x;
        if (sz.x > 0) avail_w = sz.x;
        float child_h = avail_w / ratio;
        if (sz.y > 0) child_h = sz.y;

        if (ImGui::BeginChild(id, {avail_w, child_h}, ImGuiChildFlags_Borders)) {
            for (int i = 0; i < node->child_count; i++)
                render_widget(node->children[i]);
        }
        ImGui::EndChild();
        break;
    }

    /* ── NOTEBOOK (tabs) ─────────────────────────────────────────────────── */
    case WT_NOTEBOOK: {
        if (node->state.notebook.side) {
            /* tab-pos="left" : colonne de selecteurs + contenu a droite */
            float colw = 90.0f;
            for (int i = 0; i < node->child_count; i++) {
                const char *l = (node->state.notebook.tab_labels && i < node->state.notebook.tab_count)
                                ? node->state.notebook.tab_labels[i] : NULL;
                if (l) { float w = ImGui::CalcTextSize(l).x + 24.0f; if (w > colw) colw = w; }
            }
            char cid[300]; snprintf(cid, sizeof(cid), "%s##tabs", id);
            if (ImGui::BeginChild(cid, ImVec2(colw, 0), ImGuiChildFlags_Borders)) {
                for (int i = 0; i < node->child_count; i++) {
                    char tl_buf[32];
                    const char *tab_label = (node->state.notebook.tab_labels && i < node->state.notebook.tab_count)
                                            ? node->state.notebook.tab_labels[i] : NULL;
                    if (!tab_label) { snprintf(tl_buf, sizeof(tl_buf), "Onglet %d", i + 1); tab_label = tl_buf; }
                    if (ImGui::Selectable(tab_label, node->state.notebook.current_tab == i))
                        node->state.notebook.current_tab = i;
                }
            }
            ImGui::EndChild();
            ImGui::SameLine();
            snprintf(cid, sizeof(cid), "%s##page", id);
            if (ImGui::BeginChild(cid, ImVec2(0, 0), ImGuiChildFlags_Borders)) {
                int cur = node->state.notebook.current_tab;
                if (cur >= 0 && cur < node->child_count) {
                    WidgetNode *tab = node->children[cur];
                    for (int j = 0; j < tab->child_count; j++) render_widget(tab->children[j]);
                }
            }
            ImGui::EndChild();
            break;
        }
        if (ImGui::BeginTabBar(id)) {
            for (int i = 0; i < node->child_count; i++) {
                WidgetNode *tab = node->children[i];
                char tl_buf[32];
                const char *tab_label = NULL;
                /* libelles d'onglets : tab-labels="A|B|C" du notebook */
                if (node->state.notebook.tab_labels &&
                    i < node->state.notebook.tab_count)
                    tab_label = node->state.notebook.tab_labels[i];
                if (!tab_label && tab && tab->label && tab->label[0])
                    tab_label = tab->label;
                if (!tab_label) {
                    snprintf(tl_buf, sizeof(tl_buf), "Onglet %d", i + 1);
                    tab_label = tl_buf;
                }
                if (ImGui::BeginTabItem(tab_label)) {
                    for (int j = 0; j < tab->child_count; j++)
                        render_widget(tab->children[j]);
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }
        break;
    }

    /* ── HBOX / VBOX / FRAME / EXPANDER ─────────────────────────────────── */
    /* ── EVENTBOX ────────────────────────────────────────────────────────
     * Conteneur qui capte le clic. Le stub PERDAIT ses enfants (il ne
     * depilait pas la pile) ; ici le groupe entoure vraiment son contenu. */
    case WT_EVENTBOX: {
        bool prev = g_in_hbox;
        g_in_hbox = false;
        ImGui::BeginGroup();
        for (int i = 0; i < node->child_count; i++)
            render_widget(node->children[i]);
        ImGui::EndGroup();
        g_in_hbox = prev;
        if (node->action && node->action[0] &&
            ImGui::IsItemClicked(ImGuiMouseButton_Left))
            execute_action((GtkWidget *)node, node->action, NULL);
        break;
    }

    /* ── LINKBUTTON ──────────────────────────────────────────────────────
     * SDL_OpenURL passe la main au navigateur du systeme : ni fork, ni shell,
     * ni dependance supplementaire. L'URI n'est jamais interpretee. */
    case WT_LINKBUTTON: {
        const char *uri = node->state.entry.buf;
        const char *lbl = (node->label && node->label[0]) ? node->label : uri;
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.36f, 0.61f, 0.85f, 1.0f));
        bool clic = ImGui::Button(lbl[0] ? lbl : "lien");
        ImGui::PopStyleColor();
        if (uri[0] && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", uri);
        if (clic && uri[0]) SDL_OpenURL(uri);
        break;
    }

    /* ── PULSE ───────────────────────────────────────────────────────────
     * Une fraction NEGATIVE demande a ImGui sa barre indeterminee native. */
    case WT_PULSE: {
        ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(),
                           {sz.x > 0 ? sz.x : -1.0f, sz.y > 0 ? sz.y : 0},
                           (node->label && node->label[0]) ? node->label : "");
        break;
    }

    /* ── FILECHOOSER ─────────────────────────────────────────────────────
     * Le VRAI selecteur du systeme, fourni par SDL3 (portail XDG sous
     * Wayland). Le rappel s'execute pendant le pompage d'evenements, sur ce
     * meme thread : ecrire dans le noeud y est sur. */
    case WT_FILECHOOSER: {
        if (node->label && node->label[0]) { ImGui::TextUnformatted(node->label); ImGui::SameLine(); }
        ImGui::PushItemWidth(sz.x > 0 ? sz.x : 260.0f);
        ImGui::InputText(id, node->state.entry.buf, sizeof(node->state.entry.buf));
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::Button("Parcourir…")) {
            const char *mode = node->tooltip ? node->tooltip : "open";
            const char *depart = node->state.entry.buf[0] ? node->state.entry.buf : NULL;
            if (!strcasecmp(mode, "select-folder"))
                SDL_ShowOpenFolderDialog(filechooser_retour, node, g_sdl_window, depart, false);
            else if (!strcasecmp(mode, "save"))
                SDL_ShowSaveFileDialog(filechooser_retour, node, g_sdl_window, NULL, 0, depart);
            else
                SDL_ShowOpenFileDialog(filechooser_retour, node, g_sdl_window, NULL, 0, depart, false);
        }
        break;
    }

    /* ── PANED ───────────────────────────────────────────────────────────
     * ImGui n'a pas de séparateur ajustable : on le fabrique. Deux zones
     * (BeginChild) et, entre les deux, un InvisibleButton large de quelques
     * pixels qui se traîne — c'est la recette habituelle en immediate mode.
     * La position vit dans le noeud, donc elle survit d'une frame a l'autre. */
    case WT_PANED: {
        if (node->child_count < 1) break;
        ImVec2 dispo = ImGui::GetContentRegionAvail();
        const float POIGNEE = 6.0f;
        bool vert = node->state.paned.vertical;
        float etendue = vert ? (dispo.y > 40 ? dispo.y : 200.0f)
                             : (dispo.x > 40 ? dispo.x : 400.0f);
        float part = node->state.paned.pixels > 0
                   ? (float) node->state.paned.pixels
                   : (float) (etendue * node->state.paned.fraction);
        if (part < 20.0f) part = 20.0f;
        if (part > etendue - 20.0f) part = etendue - 20.0f;

        char id1[32], id2[32], idp[32];
        snprintf(id1, sizeof(id1), "##pg%p", (void *) node);
        snprintf(id2, sizeof(id2), "##pd%p", (void *) node);
        snprintf(idp, sizeof(idp), "##pp%p", (void *) node);

        ImVec2 t1 = vert ? ImVec2(0, part) : ImVec2(part, 0);
        if (ImGui::BeginChild(id1, t1, ImGuiChildFlags_None)) {
            bool prev = g_in_hbox; g_in_hbox = false;
            render_widget(node->children[0]);
            g_in_hbox = prev;
        }
        ImGui::EndChild();

        if (!vert) ImGui::SameLine();
        ImGui::InvisibleButton(idp, vert ? ImVec2(dispo.x, POIGNEE)
                                         : ImVec2(POIGNEE, dispo.y > 40 ? dispo.y : 200.0f));
        if (ImGui::IsItemHovered())
            ImGui::SetMouseCursor(vert ? ImGuiMouseCursor_ResizeNS : ImGuiMouseCursor_ResizeEW);
        if (node->state.paned.resizable && ImGui::IsItemActive()) {
            float delta = vert ? ImGui::GetIO().MouseDelta.y : ImGui::GetIO().MouseDelta.x;
            part += delta;
            if (part < 20.0f) part = 20.0f;
            if (part > etendue - 20.0f) part = etendue - 20.0f;
            node->state.paned.pixels = (int) part;   /* la poignée retient sa place */
        }
        if (!vert) ImGui::SameLine();

        if (node->child_count > 1) {
            ImVec2 t2 = vert ? ImVec2(0, 0) : ImVec2(0, 0);
            if (ImGui::BeginChild(id2, t2, ImGuiChildFlags_None)) {
                bool prev = g_in_hbox; g_in_hbox = false;
                render_widget(node->children[1]);
                g_in_hbox = prev;
            }
            ImGui::EndChild();
        }
        break;
    }

    /* ── MENUBUTTON ──────────────────────────────────────────────────────
     * BeginPopup d'ImGui : le bouton ouvre, le popup se ferme tout seul au
     * choix. Le libellé retenu vit dans le noeud. */
    case WT_MENUBUTTON: {
        char idb[40], idp[40];
        snprintf(idb, sizeof(idb), "%s##mb%p",
                 (node->label && node->label[0]) ? node->label : "Menu", (void *) node);
        snprintf(idp, sizeof(idp), "##mbp%p", (void *) node);
        if (ImGui::Button(idb)) ImGui::OpenPopup(idp);
        if (ImGui::BeginPopup(idp)) {
            for (int i = 0; i < node->child_count; i++) {
                WidgetNode *it = node->children[i];
                const char *l = it->label ? it->label : "";
                if (l[0] && ImGui::Selectable(l)) {
                    snprintf(node->state.entry.buf, sizeof(node->state.entry.buf), "%s", l);
                    if (it->action && it->action[0])
                        execute_action((GtkWidget *) it, it->action, NULL);
                }
            }
            ImGui::EndPopup();
        }
        if (node->state.entry.buf[0]) {
            ImGui::SameLine();
            ImGui::TextUnformatted(node->state.entry.buf);
        }
        break;
    }

    /* ── WIZARD ──────────────────────────────────────────────────────────
     * L'etape courante, puis les trois boutons. L'index vit dans le noeud,
     * donc la navigation survit d'une frame a l'autre. */
    case WT_WIZARD: {
        int cur = node->state.notebook.current_tab;
        if (cur < 0 || cur >= node->child_count) cur = 0;
        if (node->child_count > 0) render_widget(node->children[cur]);
        ImGui::Separator();
        char idp[40], ids[40], idf[40];
        snprintf(idp, sizeof(idp), "Precedent##wz%p", (void *) node);
        snprintf(ids, sizeof(ids), "Suivant##wz%p", (void *) node);
        snprintf(idf, sizeof(idf), "Terminer##wz%p", (void *) node);
        if (ImGui::Button(idp) && cur > 0) node->state.notebook.current_tab = cur - 1;
        ImGui::SameLine();
        if (ImGui::Button(ids) && cur < node->child_count - 1)
            node->state.notebook.current_tab = cur + 1;
        ImGui::SameLine();
        if (ImGui::Button(idf) && node->actions_attr) {
            AttributeSet *aa = (AttributeSet *) node->actions_attr;
            GList *el = NULL;
            gchar *cmd = attributeset_get_first(&el, aa, ATTR_ACTION);
            while (cmd) {
                if (*cmd) execute_action((GtkWidget *) node, cmd, NULL);
                cmd = attributeset_get_next(&el, aa, ATTR_ACTION);
            }
        }
        ImGui::SameLine();
        ImGui::Text("etape %d/%d", cur + 1, node->child_count);
        break;
    }

    /* ── STACK ───────────────────────────────────────────────────────────
     * Une seule page rendue. Avec switcher, une rangée de boutons au-dessus ;
     * l'index vit dans le noeud, donc il survit d'une frame a l'autre. */
    case WT_STACK: {
        int cur = node->state.notebook.current_tab;
        if (cur < 0 || cur >= node->child_count) cur = 0;
        if (node->state.notebook.side) {
            for (int i = 0; i < node->child_count; i++) {
                char t[16];
                snprintf(t, sizeof(t), "%d##pg%p%d", i + 1, (void *) node, i);
                if (i > 0) ImGui::SameLine();
                if (i == cur) ImGui::PushStyleColor(ImGuiCol_Button,
                                                    ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
                if (ImGui::Button(t)) node->state.notebook.current_tab = i;
                if (i == cur) ImGui::PopStyleColor();
            }
            ImGui::Separator();
            cur = node->state.notebook.current_tab;
            if (cur < 0 || cur >= node->child_count) cur = 0;
        }
        if (node->child_count > 0) render_widget(node->children[cur]);
        break;
    }

    /* ── FLOWBOX ─────────────────────────────────────────────────────────
     * Même table qu'un <grid>, le nombre de colonnes venant de
     * max-children-per-line. */
    case WT_FLOWBOX: {
        int nc = node->state.grid.columns > 0 ? node->state.grid.columns : 1;
        char tid[32];
        snprintf(tid, sizeof(tid), "##fb%p", (void *) node);
        if (ImGui::BeginTable(tid, nc, ImGuiTableFlags_SizingStretchProp)) {
            bool prev = g_in_hbox;
            g_in_hbox = false;
            for (int i = 0; i < node->child_count; i++) {
                ImGui::TableNextColumn();
                render_widget(node->children[i]);
            }
            g_in_hbox = prev;
            ImGui::EndTable();
        }
        break;
    }

    /* ── OVERLAY ─────────────────────────────────────────────────────────
     * ImGui dessine dans l'ordre : en REPLAÇANT le curseur au point de départ
     * avant chaque couche, les enfants se superposent vraiment. */
    case WT_OVERLAY: {
        ImVec2 depart = ImGui::GetCursorPos();
        ImVec2 apres  = depart;
        for (int i = 0; i < node->child_count; i++) {
            ImGui::SetCursorPos(depart);
            render_widget(node->children[i]);
            ImVec2 fin = ImGui::GetCursorPos();
            if (fin.y > apres.y) apres = fin;
        }
        ImGui::SetCursorPos(apres);   /* la suite reprend sous la pile */
        /* ⚠️ Deplacer le curseur ne suffit pas : ImGui ne fait grandir la
         * fenetre que sur un ITEM soumis. Sans ce Dummy, il affiche a chaque
         * image « Code uses SetCursorPos() to extend window/parent
         * boundaries » — bandeau rouge a l'ecran ET texte sur la SORTIE
         * STANDARD, celle-la meme ou sermo ecrit VAR="valeur". Mesure avec
         * deux <button> dans un <overlay>. */
        ImGui::Dummy(ImVec2(0.0f, 0.0f));
        break;
    }

    /* ── REVEALER ────────────────────────────────────────────────────────
     * Montré ou pas : ImGui n'anime pas l'apparition. */
    case WT_REVEALER:
        if (node->state.toggle.active && node->child_count > 0)
            render_widget(node->children[0]);
        break;

    /* ── TOOLBAR ─────────────────────────────────────────────────────────
     * Une rangée d'actions, suivie d'un separateur : en immediate-mode, c'est
     * ce qui marque la barre sans peindre un cadre a la main. */
    case WT_TOOLBAR: {
        bool vertical = (node->state.grid.columns == 1);
        bool prev = g_in_hbox;
        g_in_hbox = !vertical;
        for (int i = 0; i < node->child_count; i++) {
            if (!vertical && i > 0) ImGui::SameLine(0.0f, (float) node->state.grid.col_spacing);
            render_widget(node->children[i]);
        }
        g_in_hbox = prev;
        ImGui::Separator();
        break;
    }

    /* ── GRID ────────────────────────────────────────────────────────────
     * ImGui::BeginTable donne exactement le modele voulu : N colonnes, les
     * cellules s'alignent d'une rangee a l'autre. TableNextColumn() avance
     * en flot et passe a la ligne tout seul en fin de rangee. */
    case WT_GRID: {
        int nc = node->state.grid.columns > 0 ? node->state.grid.columns : 1;
        char tid[32];
        snprintf(tid, sizeof(tid), "##grid%p", (void *)node);
        ImGuiTableFlags flags = ImGuiTableFlags_SizingStretchProp;
        if (!node->state.grid.homogeneous) flags = ImGuiTableFlags_SizingFixedFit;
        if (ImGui::BeginTable(tid, nc, flags)) {
            bool prev = g_in_hbox;
            g_in_hbox = false;      /* chaque cellule est une colonne a elle */
            for (int i = 0; i < node->child_count; i++) {
                ImGui::TableNextColumn();
                render_widget(node->children[i]);
            }
            g_in_hbox = prev;
            ImGui::EndTable();
        }
        break;
    }

    case WT_HBOX: {
        bool prev = g_in_hbox;
        g_in_hbox = true;
        /* gtkdialog empile les enfants d'un hbox par pack_end : sans enfant
         * extensible, la rangee est CALEE A DROITE (etalon gtk3). La largeur
         * est mesuree au rendu precedent (patron ImGui). */
        bool any_expand = false;
        for (int i = 0; i < node->child_count; i++) {
            WidgetNode *ch = node->children[i];
            if (ch->expand == 1) any_expand = true;
            else if (ch->expand == 0 &&
                     (ch->type == WT_ENTRY || ch->type == WT_EDIT || ch->type == WT_FRAME ||
                      ch->type == WT_LIST || ch->type == WT_TREE || ch->type == WT_TABLE ||
                      ch->type == WT_NOTEBOOK || ch->type == WT_VBOX || ch->type == WT_HBOX))
                any_expand = true;
        }
        if (!any_expand && node->row_w > 0.0f) {
            float avail = ImGui::GetContentRegionAvail().x;
            if (avail > node->row_w)
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + avail - node->row_w);
        }
        /* Combien d'enfants extensibles vont etre enveloppes : ils se
         * PARTAGENT la largeur restante. Le premier la prenait entiere
         * (av.x), et ses freres disparaissaient — deux <frame space-expand>
         * cote a cote ne montraient que le premier, ecrase sur son contenu.
         * Mesure du 2026-09-20 : l'etalon gtk3 rend les deux colonnes. */
        int a_partager = 0;
        for (int i = 0; i < node->child_count; i++) {
            WidgetNode *ch = node->children[i];
            if (ch->expand == 1 &&
                (ch->type == WT_VBOX || ch->type == WT_FRAME ||
                 ch->type == WT_EDIT || ch->type == WT_LIST ||
                 ch->type == WT_TREE || ch->type == WT_TABLE ||
                 ch->type == WT_NOTEBOOK))
                a_partager++;
        }

        ImGui::BeginGroup();
        /* SameLine() seulement si le voisin precedent a emis quelque chose :
         * un enfant muet (image sans texture) suivi de SameLine() recollait
         * la ligne a celle de l'hbox PRECEDENT — tout l'en-tete sur une ligne */
        bool emitted = false;
        for (int i = 0; i < node->child_count; i++) {
            if (emitted) ImGui::SameLine();
            WidgetNode *ch = node->children[i];
            /* un enfant CONTENEUR extensible (colonne de droite, zone qui
             * doit remplir) doit occuper la largeur restante ; ImGui ne
             * distribue pas l'espace. On l'enveloppe dans un child de la
             * taille restante. Les colonnes fixes rendues avant ont deja
             * consomme leur largeur naturelle. */
            bool wrap = (ch->expand == 1) &&
                        (ch->type == WT_VBOX || ch->type == WT_FRAME ||
                         ch->type == WT_EDIT || ch->type == WT_LIST ||
                         ch->type == WT_TREE || ch->type == WT_TABLE ||
                         ch->type == WT_NOTEBOOK);
            ImVec2 c0 = ImGui::GetCursorPos();
            if (wrap) {
                ImVec2 av = ImGui::GetContentRegionAvail();
                /* Sa part du restant. On divise par ce qui RESTE a placer,
                 * pas par le total : le dernier recoit ainsi le reste exact,
                 * sans arrondi perdu. */
                float part = av.x;
                if (a_partager > 1) {
                    float sp = ImGui::GetStyle().ItemSpacing.x;
                    part = (av.x - sp * (float)(a_partager - 1)) / (float)a_partager;
                    if (part < 1.0f) part = 1.0f;
                }
                if (a_partager > 0) a_partager--;
                char wid[288]; snprintf(wid, sizeof(wid), "%s##hxp%d", id, i);
                if (ImGui::BeginChild(wid, ImVec2(part, av.y), ImGuiChildFlags_None))
                    render_widget(ch);
                ImGui::EndChild();
                emitted = true;
            } else {
                render_widget(ch);
                ImVec2 c1 = ImGui::GetCursorPos();
                emitted = (c0.x != c1.x || c0.y != c1.y);
            }
        }
        ImGui::EndGroup();
        if (!any_expand) node->row_w = ImGui::GetItemRectSize().x;
        g_in_hbox = prev;
        break;
    }
    case WT_VBOX: {
        /* une vbox DANS un hbox : ses enfants s'empilent (sinon g_in_hbox
         * restait vrai et la colonne entiere partait sur une seule ligne) */
        bool prev = g_in_hbox;
        g_in_hbox = false;
        if (prev) ImGui::BeginGroup();
        /* Un enfant extensible prend la hauteur RESTANTE. Ses freres d'apres
         * — un bandeau bas, une rangee de boutons — sortaient alors du champ :
         * ils etaient dessines sous le bord de la fenetre, invisibles.
         * Mesure du 2026-09-20 sur examples/showcase/02-conteneurs.sh : gtk3
         * montre le bandeau et le bouton, sdl3 n'en montrait aucun.
         *
         * On lui laisse donc la place de ses freres. Leur hauteur est celle
         * MESUREE au rendu precedent (meme patron que row_w pour un hbox) :
         * ImGui ne connait la taille d'un widget qu'apres l'avoir dessine.
         * La premiere frame reserve 0 et deborde ; la suivante est juste — les
         * douze frames de chauffe du rendu PNG suffisent largement. */
        float apres_expand = -1.0f;
        for (int i = 0; i < node->child_count; i++) {
            WidgetNode *ch = node->children[i];
            bool wrap = (ch->expand == 1) && (i + 1 < node->child_count) &&
                        (ch->type == WT_HBOX || ch->type == WT_VBOX ||
                         ch->type == WT_FRAME || ch->type == WT_EDIT ||
                         ch->type == WT_LIST || ch->type == WT_TREE ||
                         ch->type == WT_TABLE || ch->type == WT_NOTEBOOK);
            if (wrap) {
                ImVec2 av = ImGui::GetContentRegionAvail();
                float h = av.y - node->tail_h;
                /* garde-fou : une reserve absurde (freres plus hauts que la
                 * fenetre) ne doit pas ecraser l'enfant a rien */
                if (h < 40.0f) h = av.y;
                char wid[288]; snprintf(wid, sizeof(wid), "%s##vxp%d", id, i);
                if (ImGui::BeginChild(wid, ImVec2(av.x, h), ImGuiChildFlags_None))
                    render_widget(ch);
                ImGui::EndChild();
                apres_expand = ImGui::GetCursorPosY();
            } else {
                render_widget(ch);
            }
        }
        if (apres_expand >= 0.0f) {
            float t = ImGui::GetCursorPosY() - apres_expand;
            if (t >= 0.0f) node->tail_h = t;
        }
        if (prev) ImGui::EndGroup();
        g_in_hbox = prev;
        break;
    }
    case WT_FRAME: {
        /* AutoResizeY : hauteur = contenu. Sans lui, BeginChild taille 0
         * prenait TOUT le reste de la fenetre — les widgets suivants
         * (2e frame, boutons) etaient rendus hors champ. */
        /* dans un hbox : largeur au contenu aussi, sinon le 1er frame
         * prend toute la ligne et le 2e disparait */
        ImGuiChildFlags cf = ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY;
        /* dans un hbox : largeur au contenu — SAUF si le cadre est extensible.
         * Le hbox lui a alors deja reserve sa part de la ligne : se serrer sur
         * son contenu la gaspille. Mesure du 2026-09-20 : un <frame
         * space-expand> seul dans un hbox de 500 px sortait large de 70 px,
         * son libelle coupe sur deux lignes, la ou gtk3 lui donne la ligne. */
        if (g_in_hbox && node->expand != 1) cf |= ImGuiChildFlags_AutoResizeX;
        /* cadre space-expand : il prend la place restante (sa zone de texte la
         * remplit) au lieu de se serrer sur son contenu — en hauteur comme en
         * largeur, dans un hbox comme dans une vbox. */
        if (node->expand == 1) cf &= ~ImGuiChildFlags_AutoResizeY;
        if (ImGui::BeginChild(id, sz, cf)) {
            if (node->label && node->label[0])
                ImGui::SeparatorText(node->label);
            for (int i = 0; i < node->child_count; i++)
                render_widget(node->children[i]);
        }
        ImGui::EndChild();
        break;
    }
    case WT_EXPANDER: {
        if (ImGui::CollapsingHeader(label)) {
            for (int i = 0; i < node->child_count; i++)
                render_widget(node->children[i]);
        }
        break;
    }

    /* ── SEPARATOR ───────────────────────────────────────────────────────── */
    case WT_SEPARATOR: {
        ImGui::Separator();
        break;
    }

    /* ── STATUSBAR ───────────────────────────────────────────────────────── */
    case WT_STATUSBAR: {
        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 28.0f);
        ImGui::Separator();
        ImGui::Text("  %s", node->state.entry.buf[0]
                             ? node->state.entry.buf
                             : (node->label ? node->label : ""));
        break;
    }

    /* ── TIMER ───────────────────────────────────────────────────────────── */
    case WT_TIMER: {
        /* Timer REEL, cadence par la frame (jamais cable : « SDL3_TODO »).
         * label = intervalle en secondes ; state.scale.value = prochaine
         * echeance (horloge SDL en secondes). L'action passe par le
         * repartiteur du coeur (exit:, refresh:, commande...). */
        double now = SDL_GetTicks() / 1000.0;
        double interval = 1.0;
        if (node->label && *node->label)
            interval = atoi(node->label) / 1000.0;   /* label = ms entieres */
        if (interval <= 0) interval = 1.0;
        if (node->state.scale.value <= 0.0) {
            node->state.scale.value = now + interval;
        } else if (now >= node->state.scale.value) {
            node->state.scale.value = now + interval;
            if (node->actions_attr) {
                /* TOUTES les actions du timer, via le repartiteur */
                AttributeSet *aa = (AttributeSet *)node->actions_attr;
                GList *el = NULL;
                gchar *fn = attributeset_get_first(&el, aa, ATTR_ACTION);
                while (fn) {
                    if (*fn) execute_action((GtkWidget *)node, fn, NULL);
                    fn = attributeset_get_next(&el, aa, ATTR_ACTION);
                }
            } else if (node->action && *node->action) {
                execute_action((GtkWidget *)node, node->action, NULL);
            }
        }
        break;
    }

    /* ── TOGGLEBUTTON ────────────────────────────────────────────────────── */
    case WT_TOGGLEBUTTON: {
        bool on = node->state.toggle.active;
        if (on) {
            ImVec4 active_col = {0.537f, 0.706f, 0.980f, 1.0f};
            ImGui::PushStyleColor(ImGuiCol_Button,       active_col);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,active_col);
        }
        if (ImGui::Button(label, sz)) {
            node->state.toggle.active = !on;
            if (node->var_name)
                export_bool(node->var_name, node->state.toggle.active);
        }
        if (on) ImGui::PopStyleColor(2);
        break;
    }

    /* ── PIXMAP / IMAGE ──────────────────────────────────────────────────── */
    case WT_PIXMAP:
    case WT_IMAGE_W: {
        if (node->icon_path) {
            /* icone de theme (ou fichier) resolue a la creation */
            if (node_icon_texture(node))
                ImGui::Image((ImTextureID)(uintptr_t)node->icon_tex,
                             ImVec2((float)node->state.image.w, (float)node->state.image.h));
            else
                ImGui::Dummy(ImVec2((float)node->icon_px, (float)node->icon_px));
            break;
        }
        if (!node->icon_path && node->type == WT_IMAGE_W) {
            /* icone introuvable : place vide de la taille demandee */
            ImGui::Dummy(ImVec2((float)(node->icon_px > 0 ? node->icon_px : 16),
                                (float)(node->icon_px > 0 ? node->icon_px : 16)));
            break;
        }
        if (node->state.image.texture) {
            float iw = sz.x > 0 ? sz.x : (float)node->state.image.w;
            float ih = sz.y > 0 ? sz.y : (float)node->state.image.h;
            ImGui::Image((ImTextureID)(uintptr_t)node->state.image.texture,
                         {iw, ih});
        } else {
            ImGui::TextDisabled("[image: %s]",
                                node->label ? node->label : "?");
        }
        break;
    }

    /* ── TABLE ───────────────────────────────────────────────────────────── */
    case WT_TABLE: {
        int ncols = node->state.table.cols > 0 ? node->state.table.cols : 1;
        ImGuiTableFlags tf = ImGuiTableFlags_Borders |
                             ImGuiTableFlags_RowBg   |
                             ImGuiTableFlags_ScrollY |
                             ImGuiTableFlags_Resizable;
        float th = sz.y > 0 ? sz.y : 0.0f;
        if (ImGui::BeginTable(id, ncols, tf, {0, th})) {
            /* En-têtes. Une table sans <label> n'en a pas (headers vaut
             * NULL) : le déréférencer faisait planter sdl3sermo sur tout
             * dialogue à table sans en-têtes (SIGSEGV, 2.7.0 et avant). */
            for (int c = 0; c < ncols; c++) {
                const char *hdr = (c < node->state.table.cols &&
                                   node->state.table.headers &&
                                   node->state.table.headers[c])
                                  ? node->state.table.headers[c] : "";
                ImGui::TableSetupColumn(hdr);
            }
            ImGui::TableHeadersRow();
            /* Lignes */
            for (int r = 0; r < node->state.table.rows; r++) {
                ImGui::TableNextRow();
                for (int c = 0; c < ncols; c++) {
                    ImGui::TableSetColumnIndex(c);
                    const char *cell = node->state.table.cells
                                       ? node->state.table.cells[r * ncols + c]
                                       : NULL;
                    ImGui::TextUnformatted(cell ? cell : "");
                }
            }
            ImGui::EndTable();
        }
        break;
    }

    /* ── MENUBAR ─────────────────────────────────────────────────────────── */
    case WT_MENUBAR: {
        /* Géré au niveau WINDOW — ici on parcourt les sous-menus */
        if (ImGui::BeginMenuBar()) {
            for (int i = 0; i < node->child_count; i++)
                render_widget(node->children[i]);
            ImGui::EndMenuBar();
        }
        break;
    }
    case WT_MENUITEM: {
        const char *ml = node->label ? node->label : "Menu";
        if (node->child_count > 0) {
            /* Sous-menu */
            if (ImGui::BeginMenu(ml)) {
                for (int i = 0; i < node->child_count; i++)
                    render_widget(node->children[i]);
                ImGui::EndMenu();
            }
        } else {
            /* Item simple : icône de thème à gauche (comme gtk3/qt6) */
            if (node->icon_path && node_icon_texture(node)) {
                ImGui::Image((ImTextureID)(uintptr_t)node->icon_tex,
                             ImVec2((float)node->icon_px, (float)node->icon_px));
                ImGui::SameLine();
            }
            if (ImGui::MenuItem(ml)) {
                if (node->action)
                    execute_action((GtkWidget *)node, node->action, NULL);
            }
        }
        break;
    }

    /* ── TREE ────────────────────────────────────────────────────────────── */
    case WT_TREE: {
        if (ImGui::TreeNode(label)) {
            for (int i = 0; i < node->child_count; i++)
                render_widget(node->children[i]);
            ImGui::TreePop();
        }
        break;
    }

    /* ── TERMINAL ────────────────────────────────────────────────────────── */
    case WT_TERMINAL_W: {
        ImVec2 tsz = {sz.x > 0 ? sz.x : -1.0f, sz.y > 0 ? sz.y : 200.0f};
        ImGui::PushStyleColor(ImGuiCol_ChildBg, {0.094f, 0.094f, 0.145f, 1.0f});
        if (ImGui::BeginChild(id, tsz, ImGuiChildFlags_Borders)) {
            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts.Size > 1
                            ? ImGui::GetIO().Fonts->Fonts[1]   /* mono font */
                            : ImGui::GetIO().Fonts->Fonts[0]);
            TerminalState *ts = &node->state.terminal;
            int count = ts->count;
            for (int i = 0; i < count; i++) {
                int idx = (ts->head - count + i + TERMINAL_SCROLLBACK)
                          % TERMINAL_SCROLLBACK;
                ImGui::TextUnformatted(ts->lines[idx]);
            }
            /* Auto-scroll vers le bas */
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.0f);
            ImGui::PopFont();
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
        break;
    }

    default:
        /* type inconnu : ne RIEN dessiner (l'ancien diagnostic polluait
         * la fenetre — visible sur toutes les captures a <image>) */
        break;
    }

    if (!node->sensitive)
        ImGui::EndDisabled();
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  render_export_all_from_tree() — Export final avant quitter
 *
 *  En mode immediate-mode, les widgets n'exportent leur valeur que lors
 *  d'une interaction utilisateur. Pour garantir que --do reçoit l'état
 *  complet (y compris les widgets jamais touchés), on exporte tout l'arbre
 *  une dernière fois avant le cleanup.
 *
 *  Pour chaque nœud avec var_name : on re-exporte l'état courant de
 *  node->state selon le type de widget.
 * ═══════════════════════════════════════════════════════════════════════════ */

static void render_export_node(WidgetNode *node)
{
    if (!node) return;

    if (node->var_name && node->var_name[0]) {
        switch (node->type) {

        case WT_CHECKBOX:
        case WT_TOGGLEBUTTON:
        case WT_SWITCH:
            export_bool(node->var_name, node->state.toggle.active);
            break;

        case WT_ENTRY:
        case WT_EDIT:
        case WT_PASSWORD:
            set_widget_env_var(node->var_name,
                node->state.entry.buf[0] ? node->state.entry.buf : "");
            break;

        case WT_SEARCHENTRY:
            set_widget_env_var(node->var_name,
                node->state.list.filter[0] ? node->state.list.filter : "");
            break;

        case WT_HSCALE:
        case WT_VSCALE:
        case WT_LEVELBAR:
            export_float(node->var_name, node->state.scale.value);
            break;

        case WT_SPINBUTTON:
            export_int(node->var_name, node->state.spin.value);
            break;

        case WT_PROGRESSBAR:
            export_float(node->var_name, node->state.scale.value);
            break;

        case WT_RADIOBUTTON:
            export_bool(node->var_name, node->state.toggle.active);
            break;

        case WT_COMBOBOX: {
            /* Label de l'item sélectionné, lu depuis state.list. */
            int si = node->state.list.selected_index;
            set_widget_env_var(node->var_name,
                (si >= 0 && si < node->state.list.item_count &&
                 node->state.list.items && node->state.list.items[si])
                ? node->state.list.items[si] : "");
            break;
        }

        case WT_CALENDAR: {
            /* Format ISO 8601 (YYYY-MM-DD) reconstruit depuis state.calendar. */
            char iso[12];
            snprintf(iso, sizeof(iso), "%04d-%02d-%02d",
                     node->state.calendar.year,
                     node->state.calendar.month,
                     node->state.calendar.day);
            set_widget_env_var(node->var_name, iso);
            break;
        }

        default:
            /* Widgets sans valeur exportable (boutons, labels, conteneurs) */
            break;
        }
    }

    /* Récursion sur les enfants */
    for (int i = 0; i < node->child_count; i++)
        render_export_node(node->children[i]);
}

static void render_export_all_from_tree(WidgetNode *root)
{
    render_export_node(root);
    /* Déclencher aussi variables_export_all() du core pour les variables
     * gérées par le parser (actions, --do, signaux) */
    variables_export_all();
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  render_loop() — Initialisation SDL3 + ImGui + boucle principale
 * ═══════════════════════════════════════════════════════════════════════════ */

#if defined(__has_include)
#  if __has_include(<X11/Xlib.h>)
#    define SERMO_HAVE_X11 1
#  endif
#endif
#ifdef SERMO_HAVE_X11
#include <X11/Xlib.h>
#include <X11/Xatom.h>
static void sermo_x11_set_utf8_title(SDL_Window *w, const char *title)
{
    SDL_PropertiesID props = SDL_GetWindowProperties(w);
    Display *dpy = (Display *)SDL_GetPointerProperty(props,
        SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);
    Uint64 xw = (Uint64)SDL_GetNumberProperty(props,
        SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
    if (!dpy || !xw) return;
    Atom net_name = XInternAtom(dpy, "_NET_WM_NAME", False);
    Atom utf8     = XInternAtom(dpy, "UTF8_STRING", False);
    XChangeProperty(dpy, (Window)xw, net_name, utf8, 8, PropModeReplace,
        (const unsigned char *)title, (int)SDL_strlen(title));
    XFlush(dpy);
}
#else
static void sermo_x11_set_utf8_title(SDL_Window *, const char *) {}
#endif

int render_loop(WidgetNode *root, const char *title, int win_w, int win_h)
{
    /* Mode rendu offscreen deterministe (--render-png FILE) : on cree une
     * fenetre CACHEE, on dessine quelques frames pour que la mise en page se
     * stabilise (auto-dimension au contenu), on lit le framebuffer OpenGL
     * (glReadPixels) et on ecrit un PNG, sans jamais entrer dans la boucle
     * d'evenements. Fonctionne sans display via le driver video « offscreen »
     * de SDL3 (SDL_VIDEODRIVER=offscreen, sinon xvfb-run -a). */
    const bool render_png_mode = (option_render_png && *option_render_png);

    /* ── SDL3 init ─────────────────────────────────────────────────────── */
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    /* OpenGL 3.3 Core */
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);

    SDL_WindowFlags win_flags =
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (render_png_mode)
        win_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN;   /* offscreen, pas de HiDPI */

    SDL_Window *window = SDL_CreateWindow(
        title ? title : "sdl3sermo",
        win_w > 0 ? win_w : 800,
        win_h > 0 ? win_h : 600,
        win_flags);

    if (!window) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    g_sdl_window = window;   /* fenetre mere du selecteur de fichier natif */

    SDL_GLContext gl_ctx = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_ctx);
    /* PAS de vsync : sous Xvfb (llvmpipe/DRI3) et certains contextes,
     * l'attente de present-complete BLOQUE glXSwapBuffers pour toujours —
     * la fenetre n'apparaissait jamais. La cadence vient du SDL_Delay
     * de la boucle. */
    SDL_GL_SetSwapInterval(0);

    /* Titre UTF-8 fiable : sous une locale non-UTF8 (le script force LC_ALL=C)
     * SDL3 ne pose pas _NET_WM_NAME, seulement WM_NAME en octets bruts lus en
     * Latin-1 par le WM. On pose _NET_WM_NAME (UTF8_STRING) via X11. */
    sermo_x11_set_utf8_title(window, title ? title : "sdl3sermo");

    /* Icone de fenetre = icone d'appli du port (sdl3sermo/sdl3dialog) */
    {
        char *ip = sermo_icon_lookup("sdl3sermo", 32);
        if (!ip) ip = sermo_icon_lookup("sdl3dialog", 32);
        if (ip) {
            int iw = 0, ih = 0;
            unsigned char *rgba = sermo_image_load_rgba(ip, 32, &iw, &ih);
            if (rgba) {
                SDL_Surface *surf = SDL_CreateSurfaceFrom(iw, ih,
                    SDL_PIXELFORMAT_RGBA32, rgba, iw * 4);
                if (surf) { SDL_SetWindowIcon(window, surf); SDL_DestroySurface(surf); }
                free(rgba);
            }
            free(ip);
        }
    }

    /* ── ImGui init ────────────────────────────────────────────────────── */
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    /* Police par défaut + police mono pour le terminal */
    /* Police du systeme si presente (la ProggyClean d'ImGui fait « jeu
     * video ») ; Fonts[1] reste la mono du terminal. */
    {
        const char *sans = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
        const char *mono = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf";
        const char *env  = getenv("SERMO_FONT");
        if (env && *env) sans = env;
        if (!io.Fonts->AddFontFromFileTTF(sans, 15.0f)) io.Fonts->AddFontDefault();
        if (!io.Fonts->AddFontFromFileTTF(mono, 14.0f)) io.Fonts->AddFontDefault();
    }

    apply_catppuccin(sermo_desktop_is_dark());   /* suit le theme systeme (defaut clair) */

    ImGui_ImplSDL3_InitForOpenGL(window, gl_ctx);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    /* ── Rendu offscreen (--render-png) ────────────────────────────────── */
    if (render_png_mode) {
        /* On dessine plusieurs frames : la 1re construit l'atlas de polices,
         * l'auto-dimension au contenu se declenche a la frame >= 3, puis on
         * laisse quelques frames pour que le redimensionnement du drawable
         * (back buffer) soit pris en compte avant la lecture des pixels. */
        const int WARMUP = 12;
        int cap_w = 0, cap_h = 0;
        bool redimensionne = false;   /* une seule fois — voir plus bas */
        std::vector<unsigned char> pixels;

        for (int frame = 0; frame < WARMUP; ++frame) {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();

            render_widget(root);

            /* Auto-dimension au contenu — UNE SEULE FOIS, comme la boucle
             * vivante (qui pose son drapeau « resized »).
             *
             * Ce drapeau manquait ici, et le commentaire affirmait pourtant
             * « meme logique ». Sans lui, les douze frames de chauffe
             * redimensionnaient l'une apres l'autre, et la mesure s'emballait :
             * un enfant space-expand prend la hauteur RESTANTE, donc agrandir
             * la fenetre augmente ce qu'il reclame, ce qui agrandit encore.
             * Mesure du 2026-09-20 sur examples/showcase/02-conteneurs.sh
             * (demande 560x420) : 560x1530 en PNG contre 560x531 a l'ecran,
             * pour le meme arbre. La galerie du site publiait l'image emballee,
             * vide sur les deux tiers. */
            if (!redimensionne && frame >= 2 && g_need_h > 20.0f) {
                int cur_w, cur_h;
                SDL_GetWindowSize(window, &cur_w, &cur_h);
                if (root->height <= 0 || (int)g_need_h > cur_h) {
                    SDL_SetWindowSize(window,
                                      root->width > 0 ? root->width : cur_w,
                                      (int)g_need_h);
                }
                redimensionne = true;
            }

            ImGui::Render();
            int fw, fh;
            SDL_GetWindowSizeInPixels(window, &fw, &fh);
            glViewport(0, 0, fw, fh);
            glClearColor(0.118f, 0.118f, 0.180f, 1.0f);   /* base Mocha */
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            cap_w = fw;
            cap_h = fh;
        }

        /* Lecture du framebuffer (back buffer, non presente). */
        glFinish();
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadBuffer(GL_BACK);
        if (cap_w > 0 && cap_h > 0) {
            pixels.resize((size_t)cap_w * (size_t)cap_h * 4u);
            glReadPixels(0, 0, cap_w, cap_h, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        }

        bool ok = (cap_w > 0 && cap_h > 0) &&
                  sermo_write_png_rgba_flipped(option_render_png, cap_w, cap_h, pixels.data());
        if (!ok)
            fprintf(stderr, "render-png: echec ecriture %s\n", option_render_png);

        /* Cleanup identique a la sortie normale. */
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        SDL_GL_DestroyContext(gl_ctx);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return ok ? 0 : 1;
    }

    /* ── Boucle principale ─────────────────────────────────────────────── */
    bool running = true;
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            ImGui_ImplSDL3_ProcessEvent(&ev);
            if (ev.type == SDL_EVENT_QUIT)
                running = false;
            if (ev.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                ev.window.windowID == SDL_GetWindowID(window))
                running = false;
        }

        /* Nouvelle frame */
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        /* Rendu de l'arbre de widgets */
        render_widget(root);

        /* Auto-dimension au contenu : hauteur mesuree pendant le rendu,
         * appliquee une fois que la mise en page est stable. */
        {
            static int frames = 0;
            static int resized = 0;
            frames++;
            if (!resized && frames >= 3 && g_need_h > 20.0f) {
                int cur_w, cur_h;
                SDL_GetWindowSize(window, &cur_w, &cur_h);
                /* sans hauteur demandee : au contenu ; avec : la hauteur
                 * demandee est un MINIMUM (GTK agrandit si le contenu
                 * deborde — le pied de page etait coupe) */
                if (root->height <= 0 || (int)g_need_h > cur_h) {
                    SDL_SetWindowSize(window,
                                      root->width > 0 ? root->width : cur_w,
                                      (int)g_need_h);
                }
                resized = 1;
            }
        }

        /* Rendu OpenGL */
        ImGui::Render();
        int fw, fh;
        SDL_GetWindowSizeInPixels(window, &fw, &fh);
        glViewport(0, 0, fw, fh);
        glClearColor(0.118f, 0.118f, 0.180f, 1.0f); /* base Mocha */
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
        SDL_Delay(16);   /* ~60 Hz, la vsync est coupee */
    }

    /* ── Export final — garantit que --do reçoit tous les états widgets ── */
    render_export_all_from_tree(root);

    /* ── Cleanup ───────────────────────────────────────────────────────── */
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DestroyContext(gl_ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
