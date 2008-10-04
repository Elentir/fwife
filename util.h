/*
 *  util.c for Fwife
 * 
 *  Copyright (c) 2005-2007 by Miklos Vajna <vmiklos@frugalware.org>
 *  Copyright (c) 2006 by Alex Smith <alex@alex-smith.me.uk>
 *  Copyright (c) 2008 by Albar Boris <boris.a@cegetel.net>
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, 
 *  USA.
 */

#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <signal.h>
#include <stdarg.h>

#ifdef _
#undef _
#endif
#define _(text) gettext(text)

#define MALLOC(p, b) { if((b) > 0) \
	{ p = malloc(b); if (!(p)) \
	{ fprintf(stderr, "malloc failure: could not allocate %d bytes\n", (int)(b)); \
	exit(1); }} else p = NULL; }
#define FREE(p) { if (p) { free(p); (p) = NULL; }}

#define LOG(fmt, args...) setup_log(__FILE__, __LINE__, fmt, ##args)

typedef struct
{
	char *name;
	void *data;
} data_t;

data_t *data_new(void);
void *data_get(GList *config, char *title);
void data_put(GList **config, char *name, void *data);
int copyfile(char *src, char *dest);
int exit_perform(void);
int fw_system(char* cmd);
int fw_system_interactive(char* cmd);
int makepath(char *path);
int rmrf(char *path);
int umount_if_needed(char *sourcedir);
char *drop_version(char *str);
GList *g_list_strremove(GList *list, char *str);
char *g_list_display(GList *list, char *sep);
int msg(char *str);
int disable_cache(char *path);
void signal_handler(int signum);
int setup_log(char *file, int line, char *fmt, ...);
void cb_log(unsigned short level, char *msg);
char *fsize(int length);
int cmp_str(gconstpointer a, gconstpointer b);

#endif
