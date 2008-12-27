/*
 *  finish.c for Fwife
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

#include <stdio.h>
#include <gtk/gtk.h>
#include <string.h>
#include <glib.h>
#include <libfwutil.h>
#include <libfwxconfig.h>
#include <libfwmouseconfig.h>
#include <pacman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <libintl.h>
#include <unistd.h>

#include "common.h"

plugin_t plugin =
{
	"Completed",	
	desc,
	100,
	load_gtk_widget,
	GTK_ASSISTANT_PAGE_CONFIRM,
	TRUE,
	NULL,
	NULL,
	run,
	NULL // dlopen handle
};

char *desc()
{
	return _("Installation completed");
}

plugin_t *info()
{
	return &plugin;
}

GtkWidget *load_gtk_widget(GtkWidget *assist)
{
	GtkWidget *widget = gtk_label_new (_("Installation completed! We hope that you enjoy Frugalware!"));
	
	return widget;
}

int run(GList **config)
{
    	char *ptr;
	// unmout system directories
	ptr = g_strdup_printf("umount %s/sys", TARGETDIR);
	fw_system(ptr);
	FREE(ptr);
	ptr = g_strdup_printf("umount %s/proc", TARGETDIR);
	fw_system(ptr);
	FREE(ptr);
	ptr = g_strdup_printf("umount %s/dev", TARGETDIR);
	fw_system(ptr);
	FREE(ptr);
		
	return 0;
}
 
