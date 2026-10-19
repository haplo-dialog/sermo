#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# tests/pilote-sitl.py — haplo-dialog — 2026
#
# Le PILOTE d'essai de garde_sermo_cible_mavlink.sh, sur un second port du
# simulateur ArduPilot (SITL) : il met le drone en vol, et lit son état pour
# le banc. Il parle comme système 254 et n'envoie JAMAIS de battement : il ne
# doit pas passer pour une station sol (sermo refuserait d'émettre).
#
#   pilote-sitl.py PORT etat             « arme mode » (ex. « 1 4 »)
#   pilote-sitl.py PORT decoller HAUTEUR GUIDED, armer, décoller ; attend
#                                        d'être en l'air
#   pilote-sitl.py PORT desarmer         désarmement forcé (fin d'essai)
#   pilote-sitl.py PORT param NOM        valeur du paramètre
#   pilote-sitl.py PORT station SECONDES une station sol (battements GCS)
#   pilote-sitl.py PORT mission          la mission gardée par le drone, un
#                                        élément par ligne « rang commande
#                                        repère lat lon hauteur attente » (0 = départ)
# Codes : 0 = fait · 1 = échec (raison sur la sortie d'erreur)
import sys
import time

from pymavlink import mavutil

port, quoi = int(sys.argv[1]), sys.argv[2]
m = mavutil.mavlink_connection("tcp:127.0.0.1:%d" % port, source_system=254, source_component=191)


def battement(delai=10):
    hb = m.recv_match(type="HEARTBEAT", blocking=True, timeout=delai)
    while hb is not None and (hb.get_srcComponent() != 1 or hb.type == mavutil.mavlink.MAV_TYPE_GCS):
        hb = m.recv_match(type="HEARTBEAT", blocking=True, timeout=delai)
    if hb is None:
        sys.exit("pilote : aucun battement de l'autopilote")
    return hb


def commande(cmd, *p, delai=5):
    p = list(p) + [0] * (7 - len(p))
    m.mav.command_long_send(1, 1, cmd, 0, *p)
    fin = time.time() + delai
    while time.time() < fin:
        a = m.recv_match(type="COMMAND_ACK", blocking=True, timeout=0.5)
        if a and a.command == cmd:
            return a.result
    return None


hb = battement()
if quoi == "etat":
    print("%d %d" % (1 if hb.base_mode & mavutil.mavlink.MAV_MODE_FLAG_SAFETY_ARMED else 0, hb.custom_mode))
elif quoi == "decoller":
    hauteur = float(sys.argv[3])
    # GUIDED = 4. Les vérifications avant armement attendent le GPS et
    # l'estimateur : on réessaie jusqu'à 90 s.
    fin = time.time() + 90
    while True:
        r1 = commande(mavutil.mavlink.MAV_CMD_DO_SET_MODE, 1, 4)
        r2 = commande(mavutil.mavlink.MAV_CMD_COMPONENT_ARM_DISARM, 1)
        if r1 == 0 and r2 == 0:
            break
        if time.time() > fin:
            sys.exit("pilote : armement refusé (mode %s, armement %s)" % (r1, r2))
        time.sleep(2)
    if commande(mavutil.mavlink.MAV_CMD_NAV_TAKEOFF, 0, 0, 0, 0, 0, 0, hauteur) != 0:
        sys.exit("pilote : décollage refusé")
    m.mav.request_data_stream_send(1, 1, mavutil.mavlink.MAV_DATA_STREAM_POSITION, 4, 1)
    fin = time.time() + 40
    while time.time() < fin:
        g = m.recv_match(type="GLOBAL_POSITION_INT", blocking=True, timeout=1)
        if g and g.relative_alt > hauteur * 1000 * 0.8:
            sys.exit(0)
    sys.exit("pilote : le drone n'a pas pris de hauteur")
elif quoi == "desarmer":
    # 21196 : désarmement forcé, même en vol (fin d'essai, simulateur).
    sys.exit(0 if commande(mavutil.mavlink.MAV_CMD_COMPONENT_ARM_DISARM, 0, 21196) == 0 else 1)
elif quoi == "param":
    m.mav.param_request_read_send(1, 1, sys.argv[3].encode(), -1)
    fin = time.time() + 5
    while time.time() < fin:
        v = m.recv_match(type="PARAM_VALUE", blocking=True, timeout=0.5)
        if v and v.param_id == sys.argv[3]:
            print("%g" % v.param_value)
            sys.exit(0)
    sys.exit("pilote : paramètre sans réponse")
elif quoi == "station":
    fin = time.time() + float(sys.argv[3])
    s = mavutil.mavlink.MAVLink(m, srcSystem=255, srcComponent=190)
    while time.time() < fin:
        s.heartbeat_send(mavutil.mavlink.MAV_TYPE_GCS, mavutil.mavlink.MAV_AUTOPILOT_INVALID, 0, 0, 0)
        time.sleep(0.5)
elif quoi == "mission":
    m.mav.mission_request_list_send(1, 1, 0)
    n = m.recv_match(type="MISSION_COUNT", blocking=True, timeout=5)
    if n is None:
        sys.exit("pilote : pas de réponse à la relecture")
    for i in range(n.count):
        m.mav.mission_request_int_send(1, 1, i, 0)
        x = m.recv_match(type="MISSION_ITEM_INT", blocking=True, timeout=3)
        if x is None or x.seq != i:
            sys.exit("pilote : élément %d illisible" % i)
        print("%d %d %d %.7f %.7f %.2f %g" % (i, x.command, x.frame, x.x / 1e7, x.y / 1e7, x.z, x.param1))
    m.mav.mission_ack_send(1, 1, 0, 0)
else:
    sys.exit("pilote : action inconnue")
