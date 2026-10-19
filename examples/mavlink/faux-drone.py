#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# examples/mavlink/faux-drone.py — haplo-dialog — 2026
#
# Un FAUX drone MAVLink (protocole v2), pour essayer <input mavlink> sans
# matériel. Il diffuse, 5 fois par seconde, la télémétrie d'un autopilote
# ArduPilot (système 1, composant 1) : battement, batterie, position, GPS,
# vitesse. Écrit avec pymavlink (paquet python3-pymavlink), une implémentation
# indépendante de celle en C que lit sermo : si les deux s'entendent, le
# protocole est bien parlé.
#
#   faux-drone.py --udp 127.0.0.1:14550      envoie vers cette adresse UDP
#   faux-drone.py --tcp 5760                 sert en TCP (comme le simulateur SITL)
#   faux-drone.py --serie FICHIER            liaison série SIMULÉE : un
#                     pseudo-terminal, dont le chemin (/dev/pts/N) est écrit
#                     dans FICHIER ; comme une radio branchée en USB
# Options :
#   --arme            le drone se dit armé (sinon désarmé)
#   --mode N          numéro de mode annoncé (défaut 0)
#   --sans-courant    le courant est « non mesuré » (-1)
#   --intrus          d'autres émetteurs parlent aussi, avec de FAUSSES valeurs :
#                     une station sol (255/190) et une caméra (1/100) ; sermo
#                     doit les ignorer
#   --espion FICHIER  (TCP, série) note le nombre d'octets reçus : un
#                     lecteur de télémétrie ne doit RIEN envoyer
#   --journal FICHIER (TCP) note le NOM de chaque message MAVLink reçu, une
#                     ligne chacun (PARAM_SET, COMMAND_LONG…) : un écrivain
#                     qui refuse ne doit envoyer aucune commande
# Valeurs : 12,60 V, 12,50 A, 76 %, 45,1234567 N 5,7654321 E, 245,50 m
# (niveau de la mer), 30,25 m (au-dessus du départ), cap 90,00°, 11
# satellites, fix 3, vitesse 4,20 m/s, montée 0,50 m/s.
import argparse
import os
import socket
import sys
import threading
import time

from pymavlink.dialects.v20 import common as mav

p = argparse.ArgumentParser()
p.add_argument("--udp")
p.add_argument("--tcp", type=int)
p.add_argument("--serie")
p.add_argument("--arme", action="store_true")
p.add_argument("--mode", type=int, default=0)
p.add_argument("--sans-courant", action="store_true")
p.add_argument("--intrus", action="store_true")
p.add_argument("--espion")
p.add_argument("--journal")
a = p.parse_args()
if sum(map(bool, (a.udp, a.tcp, a.serie))) != 1:
    sys.exit("faux-drone.py : --udp HÔTE:PORT, --tcp PORT ou --serie FICHIER")


class Tampon:
    """pymavlink écrit dans un « fichier » ; on garde les octets."""
    def __init__(self):
        self.octets = b""

    def write(self, b):
        self.octets += b


def paquets():
    """Un tour de télémétrie, encodé : liste d'octets."""
    out = []

    def emetteur(sys_id, comp):
        t = Tampon()
        m = mav.MAVLink(t, srcSystem=sys_id, srcComponent=comp)
        return t, m

    # Les intrus parlent AVANT le drone à chaque tour : un lecteur qui ne trie
    # pas les émetteurs prend forcément leurs fausses valeurs.
    if a.intrus:
        for sid, comp in ((255, 190), (1, 100)):   # station sol, caméra
            t, m = emetteur(sid, comp)
            m.heartbeat_send(mav.MAV_TYPE_GCS if sid == 255 else mav.MAV_TYPE_CAMERA,
                             mav.MAV_AUTOPILOT_INVALID, 0, 0, mav.MAV_STATE_ACTIVE)
            m.sys_status_send(0, 0, 0, 0, 9900, 9900, 1, 0, 0, 0, 0, 0, 0)
            m.global_position_int_send(1000, -10, -10, -10, -10, 0, 0, 0, 0)
            out.append(t.octets)
    t, m = emetteur(1, 1)
    base = mav.MAV_MODE_FLAG_CUSTOM_MODE_ENABLED | (mav.MAV_MODE_FLAG_SAFETY_ARMED if a.arme else 0)
    m.heartbeat_send(mav.MAV_TYPE_QUADROTOR, mav.MAV_AUTOPILOT_ARDUPILOTMEGA, base, a.mode, mav.MAV_STATE_STANDBY)
    m.sys_status_send(0, 0, 0, 500, 12600, -1 if a.sans_courant else 1250, 76, 0, 0, 0, 0, 0, 0)
    m.global_position_int_send(1000, 451234567, 57654321, 245500, 30250, 0, 0, 0, 9000)
    m.gps_raw_int_send(0, 3, 451234567, 57654321, 245500, 100, 100, 420, 9000, 11)
    m.vfr_hud_send(4.3, 4.2, 90, 40, 245.5, 0.5)
    out.append(t.octets)
    return out


if a.udp:
    hote, port = a.udp.rsplit(":", 1)
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    while True:
        for b in paquets():
            try:
                s.sendto(b, (hote, int(port)))
            except OSError:
                pass
        time.sleep(0.2)

recus = 0
verrou = threading.Lock()


def noter():
    if a.espion:
        with open(a.espion + ".tmp", "w") as f:
            f.write(str(recus))
        os.replace(a.espion + ".tmp", a.espion)


if a.serie:
    import select
    import tty
    maitre, esclave = os.openpty()
    tty.setraw(esclave)
    with open(a.serie + ".tmp", "w") as f:
        f.write(os.ttyname(esclave))
    os.replace(a.serie + ".tmp", a.serie)
    noter()
    while True:
        for b in paquets():
            try:
                os.write(maitre, b)
            except OSError:
                pass
        fin = time.time() + 0.2
        while time.time() < fin:
            r, _, _ = select.select([maitre], [], [], max(0, fin - time.time()))
            if r:
                try:
                    recus += len(os.read(maitre, 4096))
                    noter()
                except OSError:
                    pass


def journaliser(lecteur, d):
    if not a.journal:
        return
    for o in d:
        msg = lecteur.parse_char(bytes([o]))
        if msg is not None:
            with verrou, open(a.journal, "a") as f:
                f.write(msg.get_type() + "\n")


def client(c):
    global recus
    c.settimeout(0.05)
    lecteur = mav.MAVLink(Tampon())
    lecteur.robust_parsing = True
    try:
        while True:
            for b in paquets():
                c.sendall(b)
            fin = time.time() + 0.2
            while time.time() < fin:
                try:
                    d = c.recv(4096)
                    if not d:
                        return
                    with verrou:
                        recus += len(d)
                        noter()
                    journaliser(lecteur, d)
                except socket.timeout:
                    pass
    except OSError:
        pass
    finally:
        c.close()


srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
srv.bind(("127.0.0.1", a.tcp))
srv.listen(16)
noter()
if a.journal:
    open(a.journal, "w").close()
while True:
    c, _ = srv.accept()
    threading.Thread(target=client, args=(c,), daemon=True).start()
