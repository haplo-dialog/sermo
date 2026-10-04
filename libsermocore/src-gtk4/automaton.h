/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * automaton.h: An automaton executing the program, creating widgets. 
 * Gtkdialog - A small utility for fast and easy GUI building.
 * Copyright (C) 2003-2007  László Pere <pipas@linux.pte.hu>
 * Copyright (C) 2011-2012  Thunor <thunorsif@hotmail.com>
 * Copyright (C) 2026  haplo-dialog <devel@haplo-dialog.fr> (GTK3 port, security)
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef AUTOMATON_H
#define AUTOMATON_H

#define _GN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "gtkdialog.h"
#include "stack.h"
#include "attributes.h"
#include "stringman.h"


typedef int token;

typedef struct _instruction {
	token      command;
	char      *argument;
	gint       ival;
	tag_attr  *tag_attributes;
} instruction;

typedef struct actioncommand {
  GtkWidget   *source_widget;
  char        *destination_name;
} actioncommand;

#include "widget_types.h"
/*                                  - Command            */
/*                              xxxxxxxx                 */
#define COMMAND               0x0000F000
#define PUSH                  0x00001000
#define SUM                   0x00002000
#define SET                   0x00003000
#define SHOW		      0x00004000
#define IFNGOTO               0x00005000
#define IMPUSH                0x00006000
#define IMASSG                0x00007000
#define GOTO                  0x00008000

/*                                    -- Attribute           */
/*                              xxxxxxxx                 */
#define ATTRIBUTE             0x000000ff
#define ATTR_LABEL            0x00000001
#define ATTR_DEFAULT          0x00000002
#define ATTR_VARIABLE         0x00000003
#define ATTR_HEIGHT           0x00000004
#define ATTR_WIDTH            0x00000005
#define ATTR_INPUT            0x00000006
#define ATTR_SENSITIVE        0x00000007
#define ATTR_ACTION           0x00000008
#define ATTR_ITEM             0x00000009
#define ATTR_OUTPUT           0x0000000A
#define ATTR_SOCKET           0x0000000B


/*                               - Sub attribute         */
/*                              xxxxxxxx                 */
#define SUB_ATTRIBUTE         0x0F000000
#define SUB_ATTR_SHELL        0x01000000
#define SUB_ATTR_FILE         0x02000000

#if 0
#define SUB_ATTR_CLEAR        0x03000000
#define SUB_ATTR_APPEND       0x04000000
#define SUB_ATTR_EXIT         0x05000000
#define SUB_ATTR_FILESEL      0x0A000000
#define SUB_ATTR_REMOVE       0x0B000000
#define SUB_ATTR_REFRESH      0x0C000000
#define SUB_ATTR_LAUNCH       0x0D000000
#endif


/*************************************************************************
 * Public functions:                                                     *
 *                                                                       *
 *                                                                       *
 *************************************************************************/
stackelement _sum( stackelement a, stackelement b);

void fileselection_made( GtkWidget *w, actioncommand *action );

int token_store(token command);
int token_store_attr(token command, tag_attr *attributes);
int token_store_with_argument_attr(token command, const char *argument, tag_attr *attributes);
int token_store_with_argument( token command, const char *argument);
gboolean token_store_with_tag_attributes(token command, GList *attr);

gint instruction_get_pc(void);
void instruction_set_jump(gint from, gint where);
int instruction_execute(instruction command);
void run_program(void);
void print_program(void);

#endif
