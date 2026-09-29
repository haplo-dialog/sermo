/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * tag_attributes.c:
 * Gtkdialog - A small utility for fast and easy GUI building.
 * Copyright (C) 2003-2007  László Pere <pipas@linux.pte.hu>
 * Copyright (C) 2012       Thunor <thunorsif@hotmail.com>
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>
#include "gtk3d.h"
#include "attributes.h"
#include "tag_attributes.h"

typedef struct property {
	gchar *name;
	GType type;
} property;

extern gboolean option_no_warning;

/***********************************************************************
 *                                                                     *
 ***********************************************************************/


/***********************************************************************
 *                                                                     *
 ***********************************************************************/

char *get_tag_attribute(tag_attr *attr, const char *name)
{
	int q;
	
	g_assert(attr != NULL && name != NULL);
#ifdef DEBUG
	g_message("%s(): searching for name = '%s' in %p", 
			__func__, name, attr);
#endif
	for (q = 0; q < attr->n; ++q) 
		if (strcmp(attr->pairs[q].name, name) == 0)
			return attr->pairs[q].value;

	return NULL;
}

/***********************************************************************
 *                                                                     *
 ***********************************************************************/

tag_attr *add_tag_attribute(tag_attr *attr, char *name, char *value)
{
	g_assert(attr != NULL);
	g_assert(name != NULL && value != NULL);
#ifdef DEBUG	
	g_message("%s(): name = '%s' value = '%s'", __func__, name, value);
#endif 
		
	/* R10 fix: guard against empty name before strlen()-1 access.
	 * The parser guarantees non-empty names, but defensive coding is safer. */
	if (name[0] != '\0' && name[strlen(name) - 1] == '=')
		name[strlen(name) - 1] = '\0';
	//
	// If the store is full, we enlarge its size.
	//
	if (attr->n == attr->nmax){
		attr->nmax += 32;
		attr->pairs = g_realloc(attr->pairs,
				attr->nmax * sizeof(namevalue));
	}
	/*
	 * What if this tagattr is already exists?
	 */
	attr->pairs[attr->n].name = g_strdup(name);
	attr->pairs[attr->n].value = g_strdup(value);
	++attr->n;
	return attr;
}

/***********************************************************************
 *                                                                     *
 ***********************************************************************/
/*
 * Simple functions to handle attributesets, name/value pair list.
 */

tag_attr *new_tag_attributeset(char *name, char *value)
{
	tag_attr *New;

	g_assert(name != NULL);
	g_assert(value != NULL);
	
#ifdef DEBUG
	g_message("%s(): Name: '%s' Value: '%s'.", __func__, name, value);
#endif
	/* R10 fix: guard against empty name before strlen()-1 access. */
	if (name[0] != '\0' && name[strlen(name) - 1] == '=')
		name[strlen(name) - 1] = '\0';

	New = g_malloc(sizeof(tag_attr));
	
	New->pairs = g_malloc(sizeof(namevalue) * 32);
	New->nmax = 32;
	New->n = 1;
	
	New->pairs[0].name = g_strdup(name);
	New->pairs[0].value = g_strdup(value);
	return New;
}

/***********************************************************************
 *                                                                     *
 ***********************************************************************/
/* Thunor: The tag attribute utility functions are quite simple and
 * sparse and I require a way to nullify/erase/kill a tag attribute
 * that I don't want gtk applying on widget realization */

void kill_tag_attribute(tag_attr *attr, const char *name)
{
	int q;
	
	g_assert(attr != NULL && name != NULL);
#ifdef DEBUG
	g_message("%s(): searching for name = '%s' in %p", 
			__func__, name, attr);
#endif
	for (q = 0; q < attr->n; ++q) {
		if (strcmp(attr->pairs[q].name, name) == 0) {
			attr->pairs[q].name[0] = 0;
		}
	}

}

