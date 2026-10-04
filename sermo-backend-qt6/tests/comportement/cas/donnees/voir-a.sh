#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Rend ce que le processus enfant voit de la variable A (cas 54).
printf 'vu:%s\n' "${A-}"
