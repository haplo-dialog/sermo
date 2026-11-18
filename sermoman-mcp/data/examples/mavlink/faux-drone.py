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
#                     ligne chacun (PARAM_SET, COMMAND_LONG:400…, avec le numéro
#                     de commande) : un écrivain qui refuse ne doit envoyer
#                     aucune commande
#   --repond          (TCP) répond aux LECTURES d'un écrivain : paramètres de la
#                     barrière (FENCE_ENABLE 1, FENCE_ACTION 1, FENCE_ALT_MAX
#                     100, FENCE_RADIUS 150, FENCE_TYPE 7) et demandes de
#                     message (MAV_CMD_REQUEST_MESSAGE : battement, état au sol,
#                     position, comme ArduPilot) ;
#                     n'accuse JAMAIS un ordre
#   --barriere-coupee FENCE_ENABLE vaut 0
#   --lent            un battement toutes les 3 s (au lieu de 5 par seconde)
#   --px4             l'autopilote se dit PX4 (pas ArduPilot)
#   --signe           l'autopilote SIGNE ses messages (MAVLink 2)
#   --type N          MAV_TYPE annoncé (défaut 2, quadrirotor ; 1 : avion)
#   --second          un SECOND autopilote (système 2, composant 1) bat aussi
#   --systeme N       numéro de système du drone (défaut 1)
#   --barriere-nan    FENCE_RADIUS vaut NaN
#   --barriere-type V FENCE_TYPE vaut V (ex. 1e30 : fini, mais pas un masque)
#   --sol             l'état au sol dit « posé » même armé (un drone qui ment :
#                     seule la hauteur et la vitesse disent qu'il vole)
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
p.add_argument("--repond", action="store_true")
p.add_argument("--barriere-coupee", action="store_true")
p.add_argument("--lent", action="store_true")
p.add_argument("--px4", action="store_true")
p.add_argument("--signe", action="store_true")
p.add_argument("--sol", action="store_true")
p.add_argument("--type", type=int, default=mav.MAV_TYPE_QUADROTOR)
p.add_argument("--second", action="store_true")
p.add_argument("--systeme", type=int, default=1)
p.add_argument("--barriere-nan", action="store_true")
p.add_argument("--barriere-type", type=float)
a = p.parse_args()
if sum(map(bool, (a.udp, a.tcp, a.serie))) != 1:
    sys.exit("faux-drone.py : --udp HÔTE:PORT, --tcp PORT ou --serie FICHIER")


class Tampon:
    """pymavlink écrit dans un « fichier » ; on garde les octets."""
    def __init__(self):
        self.octets = b""

    def write(self, b):
        self.octets += b


tour = 0


def emetteur(sys_id, comp):
    t = Tampon()
    m = mav.MAVLink(t, srcSystem=sys_id, srcComponent=comp)
    if a.signe and (sys_id, comp) == (a.systeme, 1):
        m.signing.secret_key = bytes(range(32))
        m.signing.link_id = 0
        m.signing.timestamp = int(time.time() * 1e5)
        m.signing.sign_outgoing = True
    return t, m


def paquets():
    """Un tour de télémétrie, encodé : liste d'octets."""
    global tour
    out = []
    tour += 1

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
    if a.second:
        t, m = emetteur(a.systeme + 1, 1)
        m.heartbeat_send(mav.MAV_TYPE_QUADROTOR, mav.MAV_AUTOPILOT_ARDUPILOTMEGA,
                         mav.MAV_MODE_FLAG_CUSTOM_MODE_ENABLED, 0, mav.MAV_STATE_STANDBY)
        out.append(t.octets)
    t, m = emetteur(a.systeme, 1)
    base = mav.MAV_MODE_FLAG_CUSTOM_MODE_ENABLED | (mav.MAV_MODE_FLAG_SAFETY_ARMED if a.arme else 0)
    if not a.lent or tour % 15 == 1:
        m.heartbeat_send(a.type, mav.MAV_AUTOPILOT_PX4 if a.px4 else mav.MAV_AUTOPILOT_ARDUPILOTMEGA,
                         base, a.mode, mav.MAV_STATE_STANDBY)
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


BARRIERE = {"FENCE_ENABLE": 1, "FENCE_ACTION": 1, "FENCE_ALT_MAX": 100, "FENCE_RADIUS": 150, "FENCE_TYPE": 7}


def reponses(msg):
    """Ce que le drone répond à une LECTURE (--repond) ; jamais d'accusé d'ordre."""
    if not a.repond:
        return b""
    t, m = emetteur(a.systeme, 1)
    if msg.get_type() == "PARAM_REQUEST_READ":
        nom = msg.param_id.rstrip("\0") if isinstance(msg.param_id, str) else msg.param_id.decode().rstrip("\0")
        if nom in BARRIERE:
            v = 0 if (nom == "FENCE_ENABLE" and a.barriere_coupee) else BARRIERE[nom]
            if nom == "FENCE_RADIUS" and a.barriere_nan:
                v = float("nan")
            if nom == "FENCE_TYPE" and a.barriere_type is not None:
                v = a.barriere_type
            m.param_value_send(nom.encode(), float(v), mav.MAV_PARAM_TYPE_REAL32, 999, 0)
    elif msg.get_type() == "COMMAND_LONG" and msg.command == mav.MAV_CMD_REQUEST_MESSAGE:
        quoi = int(msg.param1)
        if quoi == mav.MAVLINK_MSG_ID_HEARTBEAT and not a.lent:
            base = mav.MAV_MODE_FLAG_CUSTOM_MODE_ENABLED | (mav.MAV_MODE_FLAG_SAFETY_ARMED if a.arme else 0)
            m.heartbeat_send(a.type,
                             mav.MAV_AUTOPILOT_PX4 if a.px4 else mav.MAV_AUTOPILOT_ARDUPILOTMEGA,
                             base, a.mode, mav.MAV_STATE_STANDBY)
        elif quoi == mav.MAVLINK_MSG_ID_EXTENDED_SYS_STATE:
            m.extended_sys_state_send(0, mav.MAV_LANDED_STATE_IN_AIR if a.arme and not a.sol
                                      else mav.MAV_LANDED_STATE_ON_GROUND)
        elif quoi == mav.MAVLINK_MSG_ID_HOME_POSITION:
            m.home_position_send(451234567, 57654321, 215250, 0, 0, 0, [1, 0, 0, 0], 0, 0, 0)
        elif quoi == mav.MAVLINK_MSG_ID_GLOBAL_POSITION_INT:
            m.global_position_int_send(1000, 451234567, 57654321, 245500, 30250, 0, 0, 0, 9000)
        m.command_ack_send(mav.MAV_CMD_REQUEST_MESSAGE, mav.MAV_RESULT_ACCEPTED)
    return t.octets


def journaliser(lecteur, d):
    """Note chaque message reçu ; rend les réponses à envoyer."""
    out = b""
    for o in d:
        msg = lecteur.parse_char(bytes([o]))
        if msg is None:
            continue
        if a.journal:
            nom = msg.get_type()
            if nom == "SET_POSITION_TARGET_GLOBAL_INT":
                with verrou, open(a.journal + ".parametres", "a") as f:
                    f.write("cible %d 0x%04X %d %d %g\n" % (msg.coordinate_frame, msg.type_mask, msg.lat_int,
                                                          msg.lon_int, msg.alt))
            if nom == "COMMAND_LONG":
                nom += ":%d" % msg.command
                if msg.command != mav.MAV_CMD_REQUEST_MESSAGE:
                    with verrou, open(a.journal + ".parametres", "a") as f:
                        f.write("%d %g %g %g\n" % (msg.command, msg.param1, msg.param2, msg.param7))
            with verrou, open(a.journal, "a") as f:
                f.write(nom + "\n")
        out += reponses(msg)
    return out


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
                    r = journaliser(lecteur, d)
                    if r:
                        c.sendall(r)
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
