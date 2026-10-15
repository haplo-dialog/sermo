#!/usr/bin/python3
# SPDX-License-Identifier: CC0-1.0
# faux-automate.py — un automate Modbus/TCP simulé, pour essayer modbus-lecture.sh
# sans vrai automate. Quatre registres de maintien (holding, -t 4) bougent chaque
# seconde :
#   1 : température ×10 (°C)   2 : pression (mbar)   3 : vitesse (tr/min)
#   4 : compteur de pièces
# Usage : ./faux-automate.py [port]   (défaut 5020 ; 502 demande les droits root)
# Écoute sur 127.0.0.1 seulement. Dépend de python3-pymodbus (Debian).

import asyncio
import math
import sys
import time

from pymodbus.server import StartAsyncTcpServer
from pymodbus.simulator import DataType, SimData, SimDevice

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 5020
DEBUT = time.monotonic()


def valeurs():
    """Les quatre registres à l'instant présent (une seconde = un pas)."""
    t = int(time.monotonic() - DEBUT)
    temperature = round(10 * (55 + 15 * math.sin(t / 10)))
    pression = round(1013 + 40 * math.cos(t / 7))
    vitesse = 1400 + (t * 37) % 200
    pieces = (t // 3) % 65536
    return [temperature, pression, vitesse, pieces]


async def a_chaque_acces(function_code, start_address, address, count,
                         current_registers, set_values):
    """Appelée par pymodbus à chaque requête : on recalcule les registres 0 à 3
    (adresses PDU ; mbpoll -r 1 lit l'adresse 0)."""
    for i, v in enumerate(valeurs()):
        j = i - start_address
        if 0 <= j < len(current_registers):
            current_registers[j] = v
    return None


async def principal():
    appareil = SimDevice(
        id=0,
        simdata=[SimData(0, count=16, values=0, datatype=DataType.REGISTERS)],
        action=a_chaque_acces,
    )
    print(f"faux automate Modbus/TCP sur 127.0.0.1:{PORT}", flush=True)
    await StartAsyncTcpServer(context=appareil, address=("127.0.0.1", PORT))


if __name__ == "__main__":
    asyncio.run(principal())
