#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/outils/fermer-fenetres.py — haplo-dialog — 2026
#
# Demande la fermeture de chaque fenêtre de premier niveau de l'écran $DISPLAY,
# comme le fait un gestionnaire de fenêtres quand on clique sur la croix :
# message WM_DELETE_WINDOW (protocole ICCCM). Sous Xvfb il n'y a pas de
# gestionnaire de fenêtres, et xdotool ne sait que DÉTRUIRE la fenêtre, ce que
# le programme ne voit pas comme une fermeture demandée.
#
# libX11 par ctypes : aucun module à installer. Rend 0 si au moins une fenêtre
# a reçu la demande, 1 sinon.
import ctypes
import ctypes.util
import sys

x = ctypes.CDLL(ctypes.util.find_library("X11") or "libX11.so.6")
x.XOpenDisplay.restype = ctypes.c_void_p
x.XOpenDisplay.argtypes = [ctypes.c_char_p]
x.XDefaultRootWindow.restype = ctypes.c_ulong
x.XDefaultRootWindow.argtypes = [ctypes.c_void_p]
x.XInternAtom.restype = ctypes.c_ulong
x.XInternAtom.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int]
x.XQueryTree.argtypes = [ctypes.c_void_p, ctypes.c_ulong, ctypes.POINTER(ctypes.c_ulong),
                         ctypes.POINTER(ctypes.c_ulong), ctypes.POINTER(ctypes.POINTER(ctypes.c_ulong)),
                         ctypes.POINTER(ctypes.c_uint)]
x.XGetWMProtocols.argtypes = [ctypes.c_void_p, ctypes.c_ulong, ctypes.POINTER(ctypes.POINTER(ctypes.c_ulong)),
                              ctypes.POINTER(ctypes.c_int)]
x.XSendEvent.argtypes = [ctypes.c_void_p, ctypes.c_ulong, ctypes.c_int, ctypes.c_long, ctypes.c_void_p]
x.XFree.argtypes = [ctypes.c_void_p]
x.XFlush.argtypes = [ctypes.c_void_p]


class Message(ctypes.Structure):       # XClientMessageEvent, format 32
    _fields_ = [("type", ctypes.c_int), ("serial", ctypes.c_ulong), ("send_event", ctypes.c_int),
                ("display", ctypes.c_void_p), ("window", ctypes.c_ulong), ("message_type", ctypes.c_ulong),
                ("format", ctypes.c_int), ("data", ctypes.c_long * 5)]


class Evenement(ctypes.Union):         # XEvent : 24 longs
    _fields_ = [("message", Message), ("remplissage", ctypes.c_long * 24)]


d = x.XOpenDisplay(None)
if not d:
    sys.exit("fermer-fenetres : écran X injoignable")
protocoles = x.XInternAtom(d, b"WM_PROTOCOLS", 0)
fermer = x.XInternAtom(d, b"WM_DELETE_WINDOW", 0)
envoyes = 0


def parcourir(w, profondeur):
    global envoyes
    liste = ctypes.POINTER(ctypes.c_ulong)()
    n = ctypes.c_int(0)
    if x.XGetWMProtocols(d, w, ctypes.byref(liste), ctypes.byref(n)):
        veut = any(liste[i] == fermer for i in range(n.value))
        x.XFree(liste)
        if veut:
            e = Evenement()
            e.message.type = 33            # ClientMessage
            e.message.window = w
            e.message.message_type = protocoles
            e.message.format = 32
            e.message.data[0] = fermer
            x.XSendEvent(d, w, 0, 0, ctypes.byref(e))
            envoyes += 1
            return
    if profondeur >= 3:
        return
    racine, parent, n2 = ctypes.c_ulong(), ctypes.c_ulong(), ctypes.c_uint(0)
    enfants = ctypes.POINTER(ctypes.c_ulong)()
    if x.XQueryTree(d, w, ctypes.byref(racine), ctypes.byref(parent), ctypes.byref(enfants), ctypes.byref(n2)):
        ids = [enfants[i] for i in range(n2.value)]
        if n2.value:
            x.XFree(enfants)
        for c in ids:
            parcourir(c, profondeur + 1)


parcourir(x.XDefaultRootWindow(d), 0)
x.XFlush(d)
sys.exit(0 if envoyes else 1)
