"""Redessine une interface Glade GTK2/GTK3 pour GTK 4.

GTK 4 a supprimé les conteneurs historiques (GtkVBox, GtkTable, GtkToolbar,
GtkAlignment, GtkHButtonBox) et le mécanisme des propriétés d'enfant
(<packing>). La conversion n'est pas cosmétique : elle change la façon dont
la position d'un enfant est décrite.
"""
import re, sys, xml.etree.ElementTree as ET

# ── classes disparues → leur équivalent, avec l'orientation à poser ──────────
CLASSES = {
    'GtkVBox':               ('GtkBox',       'vertical'),
    'GtkHBox':               ('GtkBox',       'horizontal'),
    'GtkVButtonBox':         ('GtkBox',       'vertical'),
    'GtkHButtonBox':         ('GtkBox',       'horizontal'),
    'GtkTable':              ('GtkGrid',      None),
    'GtkToolbar':            ('GtkBox',       'horizontal'),
    'GtkToolButton':         ('GtkButton',    None),
    'GtkSeparatorToolItem':  ('GtkSeparator', 'vertical'),
    'GtkRadioButton':        ('GtkCheckButton', None),
    'GtkFileChooserButton':  ('GtkButton',      None),
}

# ── propriétés que GTK 4 ne connaît plus ────────────────────────────────────
OBSOLETES = {
    'action',
    'draw-indicator',
    'use-stock',
    'type', 'window_position', 'destroy_with_parent', 'skip_taskbar_hint',
    'skip_pager_hint', 'type_hint', 'gravity', 'focus_on_map', 'urgency_hint',
    'visible_horizontal', 'visible_vertical', 'is_important', 'show_arrow',
    'toolbar_style', 'n_rows', 'n_columns', 'shadow_type', 'label_xalign',
    'label_yalign', 'xpad', 'ypad', 'xalign', 'yalign', 'draw', 'has_separator',
    'events', 'extension_events', 'xscale', 'yscale', 'x_options', 'y_options',
    'invisible_char', 'width_chars_set', 'position', 'tab_pos', 'scrollable',
    'enable_popup', 'homogeneous_spacing', 'layout_style', 'stock',
    'response_id', 'padding', 'pack_type', 'can_default', 'has_default',
    'angle', 'single_line_mode', 'width_chars', 'max_length',
}

# ── icônes de stock GTK2 → noms d'icônes du thème ───────────────────────────
STOCK_LABELS = {
    'gtk-ok': 'OK',          'gtk-cancel': 'Annuler',
    'gtk-close': 'Fermer',   'gtk-apply':  'Appliquer',
    'gtk-quit': 'Quitter',   'gtk-yes':    'Oui',        'gtk-no': 'Non',
}

STOCK = {
    'gtk-execute': 'system-run',       'gtk-info':      'dialog-information',
    'gtk-home':    'go-home',          'gtk-directory': 'folder',
    'gtk-file':    'text-x-generic',   'gtk-quit':      'application-exit',
    'gtk-ok':      'emblem-ok',        'gtk-cancel':    'window-close',
    'gtk-close':   'window-close',     'gtk-apply':     'emblem-ok',
}

def prop(obj, nom):
    for p in obj.findall('property'):
        if (p.get('name') or '').replace('_', '-') == nom.replace('_', '-'):
            return p
    return None

def pose(obj, nom, valeur):
    if prop(obj, nom) is not None:
        return
    p = ET.Element('property'); p.set('name', nom); p.text = valeur
    obj.insert(0, p)

def convertir(obj, parent_grille=False):
    """Convertit un <object> et sa descendance. Renvoie l'objet à conserver."""
    classe = obj.get('class')

    # GtkAlignment n'existe plus : on le remplace par son enfant, ses marges
    # reprenant le rembourrage qu'il imposait.
    if classe == 'GtkAlignment':
        enfant = obj.find('child')
        interne = enfant.find('object') if enfant is not None else None
        if interne is None:
            return None
        marges = {}
        for cote, nom in (('top_padding','margin-top'), ('bottom_padding','margin-bottom'),
                          ('left_padding','margin-start'), ('right_padding','margin-end')):
            p = prop(obj, cote)
            if p is not None and (p.text or '0') != '0':
                marges[nom] = p.text
        interne = convertir(interne, parent_grille)
        for nom, val in marges.items():
            pose(interne, nom, val)
        return interne

    if classe in CLASSES:
        neuve, orientation = CLASSES[classe]
        obj.set('class', neuve)
        if classe == 'GtkFileChooserButton':
            # GTK 4 l'a supprimé sans équivalent déclaratif : le choix de
            # fichier y passe par GtkFileDialog, qui s'ouvre depuis du code.
            # On garde un bouton, pour que l'interface reste complète.
            pose(obj, 'label', 'Choisir un fichier…')
        if classe == 'GtkRadioButton':
            obj.set('_etait_radio', '1')
        if orientation:
            pose(obj, 'orientation', orientation)
        if classe == 'GtkToolbar':
            st = ET.Element('style'); cl = ET.SubElement(st, 'class'); cl.set('name', 'toolbar')
            obj.append(st)
        if classe in ('GtkHButtonBox', 'GtkVButtonBox'):
            pose(obj, 'halign', 'end')
            pose(obj, 'spacing', '6')

    # GtkButton porte SOIT une étiquette SOIT une icône : les deux posées, la
    # dernière l'emporte et l'autre disparaît sans prévenir. Les boutons de
    # barre d'outils d'origine avaient les deux, l'étiquette souvent vide.
    if classe == 'GtkToolButton':
        lab, ico = prop(obj, 'label'), prop(obj, 'icon-name')
        if lab is not None and (lab.text or '').strip():
            if ico is not None: obj.remove(ico)
        elif lab is not None:
            obj.remove(lab)

    # GTK 4 n'a plus de « border-width » sur les conteneurs : la marge se pose
    # sur les enfants. On la reporte plutôt que de la perdre.
    bw = prop(obj, 'border-width')
    if bw is None: bw = prop(obj, 'border_width')
    marge_heritee = None
    if bw is not None:
        marge_heritee = (bw.text or '0').strip()
        obj.remove(bw)

    grille_ici = obj.get('class') == 'GtkGrid'

    for p in list(obj.findall('property')):
        nom = (p.get('name') or '').replace('_', '-')
        if nom in {o.replace('_', '-') for o in OBSOLETES}:
            obj.remove(p); continue
        if nom == 'label' and (p.text or '').strip() in STOCK_LABELS:
            p.text = STOCK_LABELS[p.text.strip()]
            continue
        if nom == 'color':
            p.set('name', 'rgba')      # GdkColor (GTK2) -> GdkRGBA
            continue
        if nom == 'stock-id':
            obj.remove(p)
            pose(obj, 'icon-name', STOCK.get((p.text or '').strip(), 'image-missing'))
            continue
        p.set('name', nom)
        # les constantes GTK2 en toutes lettres
        if p.text and p.text.startswith('GTK_'):
            p.text = p.text.rsplit('_', 1)[-1].lower()

    # GTK 4 n'a plus GtkRadioButton : un GtkCheckButton devient radio dès qu'il
    # rejoint un groupe. Le premier du parent fait référence, les suivants s'y
    # rattachent — c'est ce que le regroupement implicite de GTK2 faisait.
    premier_radio = None

    for sig in obj.findall('signal'):
        sig.attrib.pop('last_modification_time', None)
        cible = sig.get('object')
        if cible and not re.fullmatch(r'[A-Za-z_][\w-]*', cible):
            del sig.attrib['object']      # texte de remplissage, pas un identifiant

    # GtkFrame en GTK 4 : un seul enfant, plus une étiquette marquée
    # type="label". Les fichiers libglade posaient les deux comme des enfants
    # ordinaires, et en GTK 4 le second écrasait le premier — le cadre
    # s'affichait vide, avec son seul titre.
    if obj.get('class') == 'GtkFrame':
        enfants = obj.findall('child')
        if len(enfants) > 1:
            for e in enfants:
                o = e.find('object')
                if o is not None and o.get('class') == 'GtkLabel' and not e.get('type'):
                    e.set('type', 'label')
                    break

    for enfant in list(obj.findall('child')):
        interne = enfant.find('object')
        packing = enfant.find('packing')
        if interne is None:
            obj.remove(enfant); continue

        remplace = convertir(interne, grille_ici)
        if remplace is None:
            obj.remove(enfant); continue
        if remplace is not interne:
            enfant.remove(interne); enfant.append(remplace)
            interne = remplace

        if interne.get('class') == 'GtkCheckButton' and \
           interne.get('_etait_radio') == '1':
            del interne.attrib['_etait_radio']
            if premier_radio is None:
                premier_radio = interne.get('id')
            elif premier_radio:
                pose(interne, 'group', premier_radio)

        if marge_heritee and marge_heritee != '0':
            for cote in ('margin-top', 'margin-bottom', 'margin-start', 'margin-end'):
                pose(interne, cote, marge_heritee)

        # Un enfant de grille sans coordonnées : GTK 2 le posait en (0,0), GTK 4
        # ne le garantit pas. On rend la position explicite dans tous les cas.
        if grille_ici and packing is None:
            packing = ET.Element('packing')

        if packing is not None:
            enfant.remove(packing) if packing in list(enfant) else None
            if grille_ici:
                # les coordonnées d'enfant deviennent un <layout> sur l'objet
                lay = ET.Element('layout')
                coord = {}
                for pp in packing.findall('property'):
                    coord[(pp.get('name') or '').replace('_', '-')] = pp.text
                def ajoute(nom, val):
                    e = ET.SubElement(lay, 'property'); e.set('name', nom); e.text = str(val)
                gauche = int(coord.get('left-attach', 0)); droite = int(coord.get('right-attach', gauche + 1))
                haut   = int(coord.get('top-attach', 0));  bas    = int(coord.get('bottom-attach', haut + 1))
                ajoute('column', gauche); ajoute('row', haut)
                if droite - gauche > 1: ajoute('column-span', droite - gauche)
                if bas - haut > 1:      ajoute('row-span', bas - haut)
                interne.append(lay)
        enfant.attrib.pop('internal-child', None) if False else None
    return obj

def indenter(e, niveau=0):
    pad = '\n' + '  ' * niveau
    if len(e):
        if not (e.text or '').strip(): e.text = pad + '  '
        for f in e: indenter(f, niveau + 1)
        if not (e[-1].tail or '').strip(): e[-1].tail = pad
    if niveau and not (e.tail or '').strip(): e.tail = pad

for chemin in sys.argv[1:]:
    arbre = ET.parse(chemin)
    racine = arbre.getroot()
    for enfant in list(racine.findall('object')):
        neuf = convertir(enfant)
        if neuf is not enfant:
            racine.remove(enfant); racine.append(neuf)
    indenter(racine)
    arbre.write(chemin, encoding='unicode', xml_declaration=False)
    with open(chemin) as f: s = f.read()
    with open(chemin, 'w') as f:
        f.write('<?xml version="1.0" encoding="UTF-8"?>\n' + s.rstrip() + '\n')
    print(f"  {chemin.split('/')[-1]} : converti")
