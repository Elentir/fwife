/*
 *  fwife.h for Fwife
 * 
 *  Copyright (c) 2005 by Miklos Vajna <vmiklos@frugalware.org>
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

#ifndef FWIFE_H_INCLUDED
#define FWIFE_H_INCLUDED

#include <glib.h>
#include <gtk/gtk.h>
#include <libintl.h>
#include "util.h"

#define LOGDEV "/dev/tty4"
#define LOGFILE "/var/log/fwife.log"
#define SOURCEDIR "/mnt/source"
#define TARGETDIR "/mnt/target"

#define MKSWAP "/sbin/mkswap"
#define SWAPON "/sbin/swapon"

#define PLUGINDIR "/usr/lib/fwife/plugins"
#define PACCONFPATH "/etc/pacman-g2/repos/"

#ifndef STABLE
#define PACCONF "frugalware-current"
#else
#define PACCONF "frugalware"
#endif

#define ARCH "i686"

#define EXGRPSUFFIX "-extra"

#define SHARED_LIB_EXT ".so"

#define GET_UTF8(chaine) g_locale_to_utf8(chaine, -1, NULL, NULL, NULL)

//* Type definition for a plugins *//

typedef struct
{
	char *name;
	char* (*desc)();
	int priority;
	GtkWidget* (*load_gtk_widget)();
	GtkAssistantPageType type;
    	gboolean complete;
	GtkWidget* (*load_help_widget)();
	int (*prerun)(GList **config);
	int (*run)(GList **config);
	void *handle;
} plugin_t;


//* Structure pour le type de page *//
typedef struct {
  GtkWidget *widget;
  gint index;
  const gchar *title;
  GtkAssistantPageType type;
  gboolean complete;
} PageInfo;

//* Some usefull function *//
void set_page_completed();
void set_page_incompleted();

//* Some functions for basic messages *//
void fwife_fatal_error(char *msg);
void fwife_error(char *msg);
void fwife_info(char *msg);
int fwife_question(char *msg);
char* fwife_entry(char *, char*, char*);

#endif // FWIFE_H_INCLUDED
