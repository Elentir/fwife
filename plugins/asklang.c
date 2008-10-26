/*
 *  asklang.c for Fwife
 * 
 *  Copyright (c) 2005, 2007, 2008 by Miklos Vajna <vmiklos@frugalware.org>
 *  Copyright (c) 2005 by Christian Hamar alias krix <krics@linuxforum.hu>
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
#include <locale.h>
#include <stdlib.h>
#include <libintl.h>

#include "common.h"

#define LANGSNUM 13

static GtkWidget *view;

// all languages
char *langs[] =
{
	"en_US", "English",
	"pt_BR", "Brazilian Portuguese / Português do Brasil",
	"cs_CZ", "Czech / Cesky",
	"da_DK", "Danish / Dansk",
	"nl_NL", "Dutch / Nederlands",	
	"fr_FR", "French / Français",
	"de_DE", "German / Deutsch",
	"hu_HU", "Hungarian / Magyar",
        "id_ID", "Indonesian / Bahasa Indonesia",
        "it_IT", "Italian / Italiano",
	"ro_RO", "Romanian / Românã",
	"sk_SK", "Slovak / Slovenèina",
	"sv_SE", "Swedish / Svenska"
};

enum
{
	COLUMN_LANG_FLAG,
	COLUMN_LANG_CODE,
	COLUMN_LANG_NAME,
	COLUMN_LANG_NUMS
};

plugin_t plugin =
{
	"asklang",
	desc,
	01,
	load_gtk_widget,
	GTK_ASSISTANT_PAGE_INTRO,
	TRUE,
	NULL,
	NULL,
	run,
	NULL // dlopen handle
};

char *desc()
{
	return _("Select your language");
}

plugin_t *info()
{
	return &plugin;
}

int setcharset(char *name, GList **config)
{
	char *ptr;

	//TODO: maybe there is a proper system call for this?
	ptr = g_strdup_printf("setfont %s", name);
	fw_system(ptr);
	FREE(ptr);
	// save the font for later usage
	data_put(config, "font", strdup(name));
	return(0);
}

GtkWidget *load_gtk_widget()
{
	int i;
	GtkListStore *store;
	GtkTreeModel *model;
	GtkTreeViewColumn *col;
	GtkTreeIter iter;
	GtkCellRenderer *renderer;
	GtkWidget *pScrollbar;
	GtkTreeSelection *selection;
	GdkPixbuf *pix;
	char path[35];	
	
	// Create a tree view list for displaying languages
	store = gtk_list_store_new(COLUMN_LANG_NUMS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING);
	model = GTK_TREE_MODEL(store);
	
	view = gtk_tree_view_new_with_model(model);
	g_object_unref (model);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(view), TRUE);
	
	//* Column for flag *//
	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "pixbuf", COLUMN_LANG_FLAG, NULL);
	gtk_tree_view_column_set_title(col, _("Flag"));
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	//* Column for lang code (ex : en_US) *//
	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", COLUMN_LANG_CODE, NULL);
	gtk_tree_view_column_set_title(col, _("Code"));
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);	

	//* Column for language name *//
	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", COLUMN_LANG_NAME, NULL);
	gtk_tree_view_column_set_title(col, _("Name"));
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	//* Set all languages with flag *//
	GError *gerror = NULL;
	for(i=0; i<(LANGSNUM*2); i+=2) 
	{		
		//load appropriate image
		strcpy(path, "/usr/share/fwife/images/flags/");
		strcat(path, langs[i]);
		pix = gdk_pixbuf_new_from_file (path, &gerror);
		if(gerror != NULL) {
			LOG("Error when loading image : %s", path);
		}
		gtk_list_store_append(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(view))), &iter);
		gtk_list_store_set(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(view))), &iter,
			0, pix, 1, g_locale_to_utf8(langs[i], -1, NULL, NULL, NULL), 2, g_locale_to_utf8(langs[i+1], -1, NULL, NULL, NULL), -1);
		g_object_unref(pix);
	}

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
        gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
      
        //* Just a scrollbar if many languages *//
	pScrollbar = gtk_scrolled_window_new(NULL, NULL);
	
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pScrollbar), view);
	//* Disable scrollbar if useless (few languages) *//
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pScrollbar), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	return pScrollbar;
}

int run(GList **config)
{
	char *fn;
	FILE* fp;

	GtkTreeModel *model = NULL;
	GtkTreeIter iter;
	char* selected;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));	
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(GTK_TREE_VIEW(view)));
	if(gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gtk_tree_model_get (model, &iter, COLUMN_LANG_CODE, &selected, -1);
	}
	else
		selected = "en_US";	

	setenv("LC_ALL", selected, 1);
	setenv("LANG",   selected, 1);
	setlocale(LC_ALL, selected);
	bindtextdomain("fwife", "/usr/share/locale");
	
	if(!strcmp("en_US", selected))
		setenv("CHARSET", "iso-8859-1", 1);
	else if(!strcmp("es_AR", selected))
	{
		setenv("CHARSET", "iso-8859-1", 1);
		setcharset("lat1-16.psfu.gz", config);
	}
	else if(!strcmp("de_DE", selected))
		setenv("CHARSET", "iso-8859-15", 1);
	else if(!strcmp("fr_FR", selected))
		setenv("CHARSET", "iso-8859-15", 1);
        else if(!strcmp("id_ID", selected))
		setenv("CHARSET", "iso-8859-1", 1);
	else if(!strcmp("it_IT", selected))
	{
		setenv("CHARSET", "iso-8859-1", 1);
		setcharset("lat9w-16.psfu.gz", config);
	}
	else if(!strcmp("cs_CZ", selected))
	{
		setenv("CHARSET", "iso-8859-2", 1);
		setcharset("lat2-16.psfu.gz", config);
	}
	else if(!strcmp("hu_HU", selected))
	{
		setenv("CHARSET", "iso-8859-2", 1);
		setcharset("lat2-16.psfu.gz", config);
	}
	else if(!strcmp("nl_NL", selected))
		setenv("CHARSET", "iso-8859-1", 1);
	else if(!strcmp("pl_PL", selected))
	{
		setenv("CHARSET", "iso-8859-2", 1);
		setcharset("lat2-16.psfu.gz", config);
	}
	else if(!strcmp("pt_PT", selected))
	{
		setenv("CHARSET", "iso-8859-1", 1);
		setcharset("lat1-16.psfu.gz", config);
	}
	else if(!strcmp("sk_SK", selected))
	{
		setenv("CHARSET", "iso-8859-2", 1);
		setcharset("lat2-16.psfu.gz", config);
	}	

	//* Create language files *//
	fn = strdup("/tmp/setup_XXXXXX");
	mkstemp(fn);
	if ((fp = fopen(fn, "w")) == NULL)
	{
		perror(_("Could not open output file for writing"));
		return(-1);
	}
	fprintf(fp, "#!/bin/sh\n\n"
		"# /etc/profile.d/lang.sh\n\n"
		"# Set the system locale\n"
		"# For a list of locales which are supported by this machine, "
		"type: locale -a\n\n");
	if(!getenv("LANG") || strcmp(getenv("LANG"), "zh_CN"))
		fprintf(fp, "export LANG=%s\n", getenv("LANG"));
	else
	{
		fprintf(fp, "if [ \"$TERM\" = \"linux\" ]; then\n\techo -ne \"\\e%%G\"\nfi\n");
		fprintf(fp, "export LANG=zh_CN.utf8\n");
	}
	fprintf(fp, "export LC_ALL=$LANG\n");
	if(getenv("CHARSET"))
		fprintf(fp, "export CHARSET=%s\n", getenv("CHARSET"));
	fclose(fp);

	// sample: adds a "content" string titled "stuff" to the config list
	data_put(config, "lang.sh", fn);
	return 0;
}
 
