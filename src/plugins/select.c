/*
 *  select.c for Fwife
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
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <pacman.h>

#include "common.h"

static GtkWidget *categories = NULL;
static GtkWidget *packetlist = NULL;
static GtkWidget *packetinfo = NULL;
static GtkWidget *pRadioKDE = NULL;
static GtkWidget *basicmode = NULL;
static GtkWidget *expertmode = NULL;

//* Selected Mode : Expert 1 or Basic 0
static int selectmode = 0;

// need some list to stock to manipulate packets more easily
static GList *syncs=NULL;
static GList *allpackets = NULL;
static GList *packets_current = NULL;
static GList *cats = NULL;

enum 
{
	USE_COLUMN,
 	CAT_COLUMN,
  	SIZE_COLUMN
};

plugin_t plugin =
{
	"select",
	desc,
	45,
	load_gtk_widget,
	GTK_ASSISTANT_PAGE_CONTENT,
	TRUE,
	NULL,
	prerun,
	run,
	NULL // dlopen handle
};

plugin_t *info()
{
	return &plugin;
}

char *desc()
{
	return _("Packages selection");
}

static PM_DB *mydatabase;

void cb_db_register(char *section, PM_DB *db)
{
	mydatabase = db;
}

GList* group2pkgs(GList *syncs, char *group)
{
	PM_GRP *grp;
	PM_LIST *pmpkgs, *lp, *junk;
	GList *pkgs=NULL;
	GList *list=NULL;
	int i, optional=0, addpkg=1;
	char *ptr, *pkgname, *pkgfullname, *lang;
	double size;

	// add the core group to the start of the base list
	if(!strcmp(group, "base"))
		list = group2pkgs(syncs, "core");

	// get language suffix
	lang = strdup(getenv("LANG"));
	ptr = rindex(lang, '_');
	*ptr = '\0';

	if(strlen(group) >= strlen(EXGRPSUFFIX) && !strcmp(group + strlen(group) - strlen(EXGRPSUFFIX), EXGRPSUFFIX))
		optional=1;

	for (i=0; i<g_list_length(syncs); i++)
	{
		grp = pacman_db_readgrp(g_list_nth_data(syncs, i), group);
		if(grp)
		{
			pmpkgs = pacman_grp_getinfo(grp, PM_GRP_PKGNAMES);
			for(lp = pacman_list_first(pmpkgs); lp; lp = pacman_list_next(lp))
				pkgs = g_list_append(pkgs, pacman_list_getdata(lp));
			break;
		}
	}
	if(pacman_trans_init(PM_TRANS_TYPE_SYNC, PM_TRANS_FLAG_NODEPS|PM_TRANS_FLAG_NOCONFLICTS, NULL, NULL, NULL) == -1)
	{
		fprintf(stderr, "failed to init transaction (%s)\n",
			pacman_strerror(pm_errno));
		return(NULL);
	}
	for (i=0; i<g_list_length(pkgs); i++)
		if(pacman_trans_addtarget(g_list_nth_data(pkgs, i)))
		{
			fprintf(stderr, "failed to add target '%s' (%s)\n",
				(char*)g_list_nth_data(pkgs, i), pacman_strerror(pm_errno));
			return(NULL);
		}

	if(pacman_trans_prepare(&junk) == -1)
	{
		fprintf(stderr, "failed to prepare transaction (%s)\n",
			pacman_strerror(pm_errno));
		return(NULL);
	}
	pmpkgs = pacman_trans_getinfo(PM_TRANS_PACKAGES);
	for(lp = pacman_list_first(pmpkgs); lp; lp = pacman_list_next(lp))
	{
		PM_SYNCPKG *sync = pacman_list_getdata(lp);
		PM_PKG *pkg = pacman_sync_getinfo(sync, PM_SYNC_PKG);
		
		pkgname = pacman_pkg_getinfo(pkg, PM_PKG_NAME);
		pkgfullname = g_strdup_printf("%s-%s", (char*)pacman_pkg_getinfo(pkg, PM_PKG_NAME),
			(char*)pacman_pkg_getinfo(pkg, PM_PKG_VERSION));
		
		// enable by default the packages in the
		// frugalware repo + enable the
		// language-specific parts from
		// locale-extra
		addpkg = ((strcmp(getenv("LANG"), "en_US") &&
			!strcmp(group, "locale-extra") &&
			strlen(pkgname) >= strlen(lang) &&
			!strcmp(pkgname + strlen(pkgname) -
			strlen(lang), lang)) || !optional);
		
		// add the package to the list
		list = g_list_append(list, strdup(pkgname));
		size = (double)(long)pacman_pkg_getinfo(pkg, PM_PKG_SIZE);
		size = (double)(size/1048576.0);
		if(size < 0.1)
			size=0.1;
		list = g_list_append(list, g_strdup_printf("%6.1f MB", size ));
		list = g_list_append(list, strdup(pacman_pkg_getinfo(pkg, PM_PKG_DESC)));
		list = g_list_append(list, GINT_TO_POINTER(addpkg));
		
		FREE(pkgfullname);
	}
	pacman_trans_release();
	return(list);
}

char* categorysize(GList *syncs, char *category)
{
	int i;
	double size=0;
	PM_GRP *grp;
	PM_LIST *pmpkgs, *lp, *junk;
	GList *pkgs=NULL;

	for (i=0; i<g_list_length(syncs); i++)
	{
		grp = pacman_db_readgrp(g_list_nth_data(syncs, i), category);
		if(grp)
		{
			pmpkgs = pacman_grp_getinfo(grp, PM_GRP_PKGNAMES);
			for(lp = pacman_list_first(pmpkgs); lp; lp = pacman_list_next(lp))
				pkgs = g_list_append(pkgs, pacman_list_getdata(lp));
			break;
		}
	}
	if(pacman_trans_init(PM_TRANS_TYPE_SYNC, PM_TRANS_FLAG_NODEPS|PM_TRANS_FLAG_NOCONFLICTS, NULL, NULL, NULL) == -1)
	{
		fprintf(stderr, "failed to init transaction (%s)\n",
			pacman_strerror(pm_errno));
		return(NULL);
	}
	for (i=0; i<g_list_length(pkgs); i++)
		if(pacman_trans_addtarget(g_list_nth_data(pkgs, i)))
		{
			fprintf(stderr, "failed to add target '%s' (%s)\n",
				(char*)g_list_nth_data(pkgs, i), pacman_strerror(pm_errno));
			return(NULL);
		}

	if(pacman_trans_prepare(&junk) == -1)
	{
		fprintf(stderr, "failed to prepare transaction (%s)\n",
			pacman_strerror(pm_errno));
		return(NULL);
	}
	pmpkgs = pacman_trans_getinfo(PM_TRANS_PACKAGES);
	for(lp = pacman_list_first(pmpkgs); lp; lp = pacman_list_next(lp))
	{
		PM_SYNCPKG *sync = pacman_list_getdata(lp);
		PM_PKG *pkg = pacman_sync_getinfo(sync, PM_SYNC_PKG);
		size += (long)pacman_pkg_getinfo(pkg, PM_PKG_SIZE);
	}
	pacman_trans_release();

	size = (double)(size/1048576.0);
	if(size < 0.1)
		size=0.1;
	return(g_strdup_printf("%6.1f MB", size));
}

GList *getcat(PM_DB *db, GList *syncs)
{
	char *name, *ptr;
	GList *catlist=NULL;
	PM_LIST *lp;

	name = pacman_db_getinfo(db, PM_DB_TREENAME);

	for(lp = pacman_db_getgrpcache(db); lp; lp = pacman_list_next(lp))
	{
		PM_GRP *grp = pacman_list_getdata(lp);

		ptr = (char *)pacman_grp_getinfo(grp, PM_GRP_NAME);

			if(!index(ptr, '-') && strcmp(ptr, "core"))
			{
				catlist = g_list_append(catlist, strdup(ptr));
				catlist = g_list_append(catlist, categorysize(syncs, ptr));
				catlist = g_list_append(catlist, GINT_TO_POINTER(1));
			}
			else if(strstr(ptr, EXGRPSUFFIX) || !strcmp(ptr, "lxde-desktop"))
			{
				catlist = g_list_append(catlist, strdup(ptr));
				catlist = g_list_append(catlist, categorysize(syncs, ptr));
				if(strcmp(ptr, "locale-extra"))
					catlist = g_list_append(catlist, GINT_TO_POINTER(0));
				else
					catlist = g_list_append(catlist, GINT_TO_POINTER(1));
			}
	}

	return catlist;
}

int prepare_pkgdb(char *repo, GList **config, GList **syncs)
{
	char *pkgdb;
	struct stat sbuf;
	int ret;
	//PM_DB *i;	

	pkgdb = g_strdup_printf("%s/var/lib/pacman-g2/%s", TARGETDIR, repo);

	// prepare pkgdb if necessary
	if(stat(pkgdb, &sbuf) || S_ISDIR(sbuf.st_mode))
	{
		// pacman can't lock & log without these
		makepath(g_strdup_printf("%s/tmp", TARGETDIR));
		makepath(g_strdup_printf("%s/var/log", TARGETDIR));
		LOG("parsing the pacman-g2 configuration file");
		if (pacman_parse_config("/etc/pacman-g2.conf", cb_db_register, "") == -1) {
			LOG("Failed to parse pacman-g2 configuration file (%s)", pacman_strerror(pm_errno));
			return(-1);
		}
		
		LOG("getting the database");
		if (mydatabase == NULL)
		{
			LOG("Could not register '%s' database (%s)", PACCONF, pacman_strerror(pm_errno));
			return(-1);
		}
		else
		{
			LOG("updating the database");
			ret = pacman_db_update(1, mydatabase);
			if(ret == 0) {
				LOG("database update done");
			}
			if (ret == -1) {
				LOG("database update failed");
				if(pm_errno == PM_ERR_DB_SYNC) {
					LOG("Failed to synchronize %s", PACCONF);
					return(-1);
				} else {
					LOG("Failed to update %s (%s)", PACCONF, pacman_strerror(pm_errno));
					return(-1);
				}
			}
		}
		
		LOG("cleaning up the database");
		pacman_db_unregister(mydatabase);
		mydatabase = NULL;
	}
	// register the database
	PM_DB *i = pacman_db_register(PACCONF);
	if(i==NULL)
	{
		fprintf(stderr, "could not register '%s' database (%s)\n",
			PACCONF, pacman_strerror(pm_errno));
		return(-1);
	}
	else
	{
		*syncs = g_list_append(*syncs, i);
	}
	return(0);
}


// ------------------------------------------------- Expert Mode functions -------------------------------------------- //

//* A categorie as been toggled *//
static void fixed_toggled_cat (GtkCellRendererToggle *cell,gchar *path_str, gpointer data)
{
	GtkTreeModel *model = (GtkTreeModel *)data;
	GtkTreeIter  iter;
	GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
	gboolean fixed;
	gchar *name;
	
	/* get toggled iter */
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, 0, &fixed, -1);
	gtk_tree_model_get (model, &iter, 1, &name, -1);
		
	gint i = gtk_tree_path_get_indices (path)[0];
  
	/* do something with the value */
	fixed ^= 1;
	
	if(fixed == FALSE)
	{
		GList *elem = g_list_nth(cats, i*3+2);
		elem->data = GINT_TO_POINTER(0);
		g_object_set (packetlist, "sensitive", FALSE, NULL);
	}
	else
	{
		GList *elem = g_list_nth(cats, i*3+2);
		elem->data = GINT_TO_POINTER(1);
		g_object_set (packetlist, "sensitive", TRUE, NULL);
	}
	
	/* set new value */
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, fixed, -1);

	/* clean up */
	gtk_tree_path_free (path);
}

//* Packet toggled *//
static void fixed_toggled_pack (GtkCellRendererToggle *cell,gchar *path_str, gpointer data)
{
	GtkTreeModel *model = (GtkTreeModel *)data;
	GtkTreeIter  iter;
	GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
	gboolean fixed;
	gchar *name;
	gint i;

	/* get toggled iter */
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, 0, &fixed, -1);
	gtk_tree_model_get (model, &iter, 1, &name, -1);
  
	/* do something with the value */
	fixed ^= 1;
	
	i = gtk_tree_path_get_indices (path)[0];
	
	GList *elem = g_list_nth(packets_current, i*4+3);
	
	if(fixed == TRUE)
		elem->data = GINT_TO_POINTER(1);
	else
		elem->data = GINT_TO_POINTER(0);
	
	/* set new value */
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, fixed, -1);

	/* clean up */
	gtk_tree_path_free (path);
}

//* Packet selection have changed, update description *//
void packet_changed(GtkTreeSelection *selection, gpointer data)
{
	GtkTreeView *treeview;
	GtkTreeModel *model;
	GtkTreeIter iter;
	char *selected;
	gint i;
	
	treeview = gtk_tree_selection_get_tree_view (selection);
	
	if (gtk_tree_selection_get_selected (selection, &model, &iter))
	{
		GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
		i = gtk_tree_path_get_indices (path)[0];
		gtk_tree_model_get (model, &iter, 1, &selected, -1);
		gtk_label_set_label(GTK_LABEL(packetinfo), (char*)g_list_nth_data(packets_current, i*4+2));
	}	
}

GtkWidget *getpacketlist()
{
	GtkListStore *store;
	GtkTreeModel *model;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;
	GtkWidget *pScrollbar;
	GtkTreeSelection *selection;
	
	store = gtk_list_store_new(3, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING);
	model = GTK_TREE_MODEL(store);
	
	packetlist = gtk_tree_view_new_with_model(model);
	g_object_unref (model);
		
	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (renderer, "toggled", G_CALLBACK (fixed_toggled_pack), model);
	col = gtk_tree_view_column_new_with_attributes (_("Install?"), renderer, "active", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(packetlist), col);
	
	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes (_("Package Name"), renderer, "text", 1, NULL);
	gtk_tree_view_column_set_expand (col, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(packetlist), col);
	
	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes (_("Size"), renderer, "text", 2, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(packetlist), col);

	gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (packetlist)), GTK_SELECTION_SINGLE);
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (packetlist));
      
        g_signal_connect (selection, "changed", G_CALLBACK (packet_changed), NULL);
	
	pScrollbar = gtk_scrolled_window_new(NULL, NULL);
	
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pScrollbar), packetlist);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pScrollbar), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	
	return pScrollbar;
}

void categorie_changed(GtkTreeSelection *selection, gpointer data)
{
	GtkTreeView *treeview;
	GtkTreeModel *model;
	GtkTreeIter iter;
	char *selected;
	gboolean checked = FALSE;
	int i;
	gtk_label_set_label(GTK_LABEL(packetinfo), "");
	
	treeview = gtk_tree_selection_get_tree_view (selection);
  		
	if (gtk_tree_selection_get_selected (selection, &model, &iter))
	{
		gtk_tree_model_get (model, &iter, 1, &selected, -1);
		gtk_tree_model_get (model, &iter, 0, &checked, -1);
		if(allpackets)
			packets_current = (GList*)data_get(allpackets, selected);
		if(packets_current)
		{
			gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(packetlist))));
	
			for(i=0; i < g_list_length(packets_current); i+=4 ) 
			{		
				gtk_list_store_append(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(packetlist))), &iter);
					gtk_list_store_set(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(packetlist))), &iter, 0, (gboolean)GPOINTER_TO_INT(g_list_nth_data(packets_current, i+3)), 1, (char*)g_list_nth_data(packets_current, i), 2, (char*)g_list_nth_data(packets_current, i+1), -1);
			}
		}
		
		// disable packet selection if categorie unselected
		if(checked == FALSE)
			g_object_set (packetlist, "sensitive", FALSE, NULL);
		else
			g_object_set (packetlist, "sensitive", TRUE, NULL);
			
	}	
}

void allselect_clicked(GtkWidget *button, gpointer boolselect)
{
	int i;
	GtkTreeIter iter;

	if(packets_current)
	{
		gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(packetlist))));
		for(i=0; i<g_list_length(packets_current); i+=4)
		{
			g_list_nth(packets_current, i+3)->data = boolselect;
			gtk_list_store_append(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(packetlist))), &iter);
			gtk_list_store_set(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(packetlist))), &iter, 0, (gboolean)boolselect, 1, (char*)g_list_nth_data(packets_current, i), 2, (char*)g_list_nth_data(packets_current, i+1), -1);
		}
	}
}

//* get a gtk tree list for categories *//
GtkWidget *getcategorieslist()
{
	GtkListStore *store;
	GtkTreeModel *model;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;
	GtkWidget *pScrollbar;
	GtkTreeSelection *selection;	

	store = gtk_list_store_new(3, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING);
	model = GTK_TREE_MODEL(store);
	
	categories = gtk_tree_view_new_with_model(model);
	g_object_unref (model);
		
	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (renderer, "toggled", G_CALLBACK (fixed_toggled_cat), model);
	col = gtk_tree_view_column_new_with_attributes (_("Install?"), renderer, "active", USE_COLUMN, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(categories), col);
	
	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes (_("Groups"), renderer, "text", CAT_COLUMN, NULL);
	gtk_tree_view_column_set_expand (col, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(categories), col);
	
	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes (_("Size"), renderer, "text", SIZE_COLUMN, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(categories), col);	

	gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (categories)), GTK_SELECTION_SINGLE);
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (categories));
        g_signal_connect (selection, "changed", G_CALLBACK (categorie_changed), NULL);
	
	pScrollbar = gtk_scrolled_window_new(NULL, NULL);
	
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pScrollbar), categories);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pScrollbar), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	
	return pScrollbar;
}

GtkWidget *getExpertModeWidget()
{
	GtkWidget *hsepa1, *hsepa2;
	GtkWidget *image;
		
	GtkWidget *phbox = gtk_hbox_new(FALSE,8);
	
	//* ------------------- Group list ------------------------------ *//
	GtkWidget *pvbox = gtk_vbox_new(FALSE,5);
		
	GtkWidget *label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("<b>Choose groups you want to install :</b>"));
	
	//* Get the lists Widgets *//
	GtkWidget *categl = getcategorieslist();
	
	gtk_box_pack_start(GTK_BOX(pvbox), label, FALSE, TRUE, 7);
	gtk_box_pack_start(GTK_BOX(pvbox), categl, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(phbox), pvbox, TRUE, TRUE, 0);
	
	//* ------------------- Packet list ------------------------------ *//
	GtkWidget *packetl = getpacketlist();
	GtkWidget *phbox2 = gtk_hbox_new(FALSE,8);
	pvbox = gtk_vbox_new(FALSE,5);
	
	//* Two button selecting packages *//
	GtkWidget *hboxbuttons = gtk_hbox_new(TRUE,5);
	GtkWidget *buttonselect = gtk_button_new_with_label(_("Select all this group's packages"));
	GtkWidget *buttonunselect = gtk_button_new_with_label(_("Unselect all this group's packages"));
	g_signal_connect (buttonselect, "clicked", G_CALLBACK (allselect_clicked), GINT_TO_POINTER(1));
	g_signal_connect (buttonunselect, "clicked", G_CALLBACK (allselect_clicked), GINT_TO_POINTER(0));
	gtk_box_pack_start(GTK_BOX(hboxbuttons), buttonselect, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hboxbuttons), buttonunselect, FALSE, TRUE, 0);
	
	//* Two separators for packet description *//
	hsepa1 = gtk_hseparator_new();
	hsepa2 = gtk_hseparator_new();
	
	// description of each package
	packetinfo = gtk_label_new(NULL);
	gtk_label_set_justify(GTK_LABEL(packetinfo), GTK_JUSTIFY_LEFT);
	// load a nice image
	image = gtk_image_new_from_file(g_strdup_printf("%s/packet.png", IMAGEDIR));
	gtk_label_set_line_wrap(GTK_LABEL(packetinfo), TRUE);
	
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("<b>Choose packages into selected group :</b>"));
	
	gtk_box_pack_start(GTK_BOX(pvbox), label, FALSE, TRUE, 7);
	gtk_box_pack_start(GTK_BOX(pvbox), packetl, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), hboxbuttons, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), hsepa1, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(phbox2), image, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(phbox2), packetinfo, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), phbox2, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), hsepa2, FALSE, FALSE, 0);
	
	//* Put the two box into one big *//
	gtk_box_pack_start(GTK_BOX(phbox), pvbox, FALSE, TRUE, 0);	

	return phbox;
}

// --------------------------------------------- Basic Mode functions ----------------------------------------------- //

//* select a categorie *//
void selectcat(char *name, int bool)
{
	int i;
	for(i=0; i<g_list_length(cats); i+=3)
	{
		if(!strcmp(g_list_nth_data(cats, i), name))
		{
			g_list_nth(cats, i+2)->data = GINT_TO_POINTER(bool);
			return;
		}
	}
}

//* Select all file in a categorie *//
void selectallfiles(char *cat, char *exception, int bool)
{
	int i;
	GList *lispack;
	
	if(allpackets)
	{
		lispack = (GList*)data_get(allpackets, cat);
		
		if(exception == NULL)
		{
			for(i=0; i<g_list_length(lispack); i+=4)
			{
				g_list_nth(lispack, i+3)->data = GINT_TO_POINTER(bool);
			}
		}
		else
		{
			for(i=0; i<g_list_length(lispack); i+=4)
			{
				if(strcmp(g_list_nth_data(lispack, i), exception))
					g_list_nth(lispack, i+3)->data = GINT_TO_POINTER(bool);
			}
		}
	}	
}

//* select (or unselect) a file 'name' in categorie 'cat' *//
void selectfile(char *cat, char* name, int bool)
{
	int i;
	GList *lispack;

	if(allpackets)
	{
		lispack = (GList*)data_get(allpackets, cat);
		
		if(lispack)
		{
			for(i=0; i<g_list_length(lispack); i+=4)
			{
				if(!strcmp(name, (char *)g_list_nth_data(lispack, i)))
				{
					g_list_nth(lispack, i+3)->data = GINT_TO_POINTER(bool);
					return;
				}
			}
		}
	}
}

//* set up desktop configuration *//
void configuredesktop()
{
	GSList *pList;
	char *seldesk = NULL;
	
	pList = gtk_radio_button_get_group(GTK_RADIO_BUTTON(pRadioKDE));
	
	//* Fix locale dependency *//
	char *lang = strdup(getenv("LANG"));
	char *ptr = rindex(lang, '_');
	*ptr = '\0';
	if(strcmp(lang ,"en"))
	{
		selectfile("locale-extra", g_strdup_printf("eric-i18n-%s", lang), 0);
		selectfile("locale-extra", g_strdup_printf("eric4-i18n-%s", lang), 0);
		selectfile("locale-extra", g_strdup_printf("hunspell-%s", lang), 0);
		selectfile("locale-extra", g_strdup_printf("koffice-l10n-%s", lang), 0);
		selectfile("locale-extra", g_strdup_printf("mbrola-%s", lang), 0);
		selectfile("locale-extra", g_strdup_printf("kde-i18n-%s", lang), 0);
	}

	/* Parcours de la liste */
	while(pList)
	{
		/* button selected? */
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pList->data)))
		{
			/* get button label */
			seldesk = strdup((char*)gtk_button_get_label(GTK_BUTTON(pList->data)));
			/* Get out */			
			pList = NULL;
		}
		else
		{
			/* next button */
			pList = g_slist_next(pList);
		}
	}
	
	if(!strcmp(seldesk, "KDE"))
	{
		selectcat("gnome", 0);
		selectcat("xfce4", 0);
		selectfile("locale-extra", g_strdup_printf("koffice-l10n-%s", lang), 1);
		selectfile("locale-extra", g_strdup_printf("kde-i18n-%s", lang), 1);
	}
	else if(!strcmp(seldesk, "Gnome"))
	{
		selectcat("kde", 0);
		selectcat("xfce4", 0);
	}
	else if(!strcmp(seldesk, "XFCE"))
	{
		selectcat("kde", 0);
		selectcat("gnome", 0);
	}
	else if(!strcmp(seldesk, "LXDE"))
	{
		selectcat("kde", 0);
		selectcat("gnome", 0);
		selectcat("xfce4", 0);
		selectcat("lxde-desktop", 1);
		selectallfiles("lxde-desktop", NULL, 1);
	}
	
	FREE(lang);	
	return;
}

//* set up group when selected or unselected *//
void basicmode_change(GtkWidget *button, gpointer data)
{
	char *lang = strdup(getenv("LANG"));
	char *ptr = rindex(lang, '_');
	*ptr = '\0';
	
	if(!strcmp((char*)data, "NET"))
	{
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == TRUE)
			selectcat("network", 1);
		else
			selectcat("network", 0);
	}
	else if(!strcmp((char*)data, "NETEX"))
	{
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == TRUE)
			selectcat("network-extra", 1);
		else
			selectcat("network-extra", 0);
	}
	else if(!strcmp((char*)data, "MUL"))
	{
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == TRUE)
		{
			selectcat("multimedia", 1);
			selectcat("xmultimedia", 1);
		}
		else
		{
			selectcat("multimedia", 0);
			selectcat("xmultimedia", 0);
		}
	}
	else if(!strcmp((char*)data, "MULEX"))
	{
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == TRUE)
		{
			selectcat("multimedia-extra", 1);
			selectcat("xmultimedia-extra", 1);
		}
		else
		{
			selectcat("multimedia-extra", 0);
			selectcat("xmultimedia-extra", 0);
		}
	}
	else if(!strcmp((char*)data, "DEV"))
	{
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == TRUE)
		{
			selectcat("devel", 1);
		}
		else
		{
			selectcat("devel", 0);
		}
	}
	else if(!strcmp((char*)data, "DEVEX"))
	{
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == TRUE)
		{
			selectcat("devel-extra", 1);
		}
		else
		{
			selectcat("devel-extra", 0);
		}
	}
	else if(!strcmp((char*)data, "CONS"))
	{
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == TRUE)
		{
			selectcat("apps", 1);
		}
		else
		{
			selectcat("apps", 0);
		}
	}
	else if(!strcmp((char*)data, "CONSEX"))
	{
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == TRUE)
		{
			selectcat("apps-extra", 1);
		}
		else
		{
			selectcat("apps-extra", 0);
		}
	}
	else if(!strcmp((char*)data, "XAPP"))
	{
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == TRUE)
		{
			selectallfiles("xapps", "openoffice.org", 1);
			selectfile("locale-extra", g_strdup_printf("firefox-%s", lang), 1);
			selectfile("locale-extra", g_strdup_printf("thunderbird-%s", lang), 1);
		}
		else
		{
			selectallfiles("xapps", "openoffice.org", 0);
			selectfile("locale-extra", g_strdup_printf("firefox-%s", lang), 0);
			selectfile("locale-extra", g_strdup_printf("thunderbird-%s", lang), 0);
		}
	}
	else if(!strcmp((char*)data, "XAPPEX"))
	{
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == TRUE)
		{
			selectcat("xapps-extra", 1);
		}
		else
		{
			selectcat("xapps-extra", 0);
		}
	}
	else if(!strcmp((char*)data, "GAME"))
	{
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == TRUE)
		{
			selectcat("games-extra", 1);
		}
		else
		{
			selectcat("games-extra", 0);
		}
	}
	else if(!strcmp((char*)data, "BUR"))
	{
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == TRUE)
		{
			selectfile("xapps", "openoffice.org", 1);
			if(strcmp(lang ,"en"))
				selectfile("locale-extra", g_strdup_printf("openoffice.org-i18n-%s", lang), 1);
		}
		else
		{
			selectfile("xapps", "openoffice.org", 0);
			char *lang = strdup(getenv("LANG"));
			char *ptr = rindex(lang, '_');
			*ptr = '\0';
			if(strcmp(lang ,"en"))
				selectfile("locale-extra", g_strdup_printf("openoffice.org-i18n-%s", lang), 0);
		}
	}
	
	return;
}

GtkWidget *getBasicModeWidget()
{
	GtkWidget *pvboxp = gtk_vbox_new(FALSE,0);
	GtkWidget *pvbox, *hboxdesktop, *hboxpackage, *hboxtemp;
	GtkWidget *logo;

	GtkWidget *labelenv = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(labelenv), _("<b>Choose your desktop environment :</b>"));
	gtk_box_pack_start(GTK_BOX(pvboxp), labelenv, FALSE, TRUE, 15);
	hboxdesktop = gtk_hbox_new(FALSE, 5);
	
	//* Desktop radio button *//
	pvbox = gtk_vbox_new(FALSE,2);
	pRadioKDE = gtk_radio_button_new_with_label(NULL, _("KDE"));
	logo =  gtk_image_new_from_file(g_strdup_printf("%s/kdelogo.png", IMAGEDIR));
	gtk_box_pack_start(GTK_BOX(pvbox), logo, FALSE, FALSE, 0);
	hboxtemp = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hboxtemp), pRadioKDE, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), hboxtemp, FALSE, FALSE, 8);
	gtk_box_pack_start(GTK_BOX(hboxdesktop), pvbox, TRUE, TRUE, 0);

	pvbox = gtk_vbox_new(FALSE,2);
	GtkWidget *pRadioGNOME =  gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON (pRadioKDE), _("GNOME"));
	logo =  gtk_image_new_from_file(g_strdup_printf("%s/gnomelogo.png", IMAGEDIR));
	gtk_box_pack_start(GTK_BOX(pvbox), logo, FALSE, FALSE, 0);
	hboxtemp = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hboxtemp), pRadioGNOME, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), hboxtemp, FALSE, FALSE, 8);
	gtk_box_pack_start(GTK_BOX(hboxdesktop), pvbox, TRUE, TRUE, 0);

	pvbox = gtk_vbox_new(FALSE,2);
	GtkWidget *pRadioXFCE =  gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON (pRadioKDE), _("XFCE"));
	logo =  gtk_image_new_from_file(g_strdup_printf("%s/xfcelogo.png", IMAGEDIR));
	gtk_box_pack_start(GTK_BOX(pvbox), logo, FALSE, FALSE, 0);
	hboxtemp = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hboxtemp), pRadioXFCE, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), hboxtemp, FALSE, FALSE, 8);
	gtk_box_pack_start(GTK_BOX(hboxdesktop), pvbox, TRUE, TRUE, 0);

	pvbox = gtk_vbox_new(FALSE,2);
	GtkWidget *pRadioLXDE =  gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON (pRadioKDE), _("LXDE"));
	logo =  gtk_image_new_from_file(g_strdup_printf("%s/lxdelogo.png", IMAGEDIR));
	gtk_box_pack_start(GTK_BOX(pvbox), logo, FALSE, FALSE, 0);
	hboxtemp = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hboxtemp), pRadioLXDE, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), hboxtemp, FALSE, FALSE, 8);
	gtk_box_pack_start(GTK_BOX(hboxdesktop), pvbox, TRUE, TRUE, 0);
	
	gtk_box_pack_start(GTK_BOX(pvboxp), hboxdesktop, FALSE, TRUE, 15);
	
	//* Other check button for other packages *//
	
	hboxpackage = gtk_hbox_new(FALSE, 0);
	
	//* Basic packages *//
	pvbox = gtk_vbox_new(FALSE, 5);
	GtkWidget *labelgroup = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(labelgroup), _("<b>Choose groups you want to install :</b>"));
	gtk_box_pack_start(GTK_BOX(pvbox), labelgroup, TRUE, TRUE, 0);
	
	GtkWidget *phbox = gtk_hbox_new(FALSE,5);
	GtkWidget *pToggleApp =  gtk_check_button_new_with_label(_("Graphical Application (Firefox, Thunderbird,...)"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pToggleApp) , TRUE);
	gtk_signal_connect(GTK_OBJECT(pToggleApp), "toggled", G_CALLBACK(basicmode_change), (gpointer)"XAPP");
	gtk_box_pack_start(GTK_BOX(phbox), pToggleApp, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), phbox, TRUE, TRUE, 0);
	
	phbox = gtk_hbox_new(FALSE,5);
	GtkWidget *pToggleCons =  gtk_check_button_new_with_label(_("Console Applications (Nano, Vim...)"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pToggleCons) , TRUE);
	gtk_signal_connect(GTK_OBJECT(pToggleCons), "toggled", G_CALLBACK(basicmode_change), (gpointer)"CONS");
	gtk_box_pack_start(GTK_BOX(phbox), pToggleCons, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), phbox, TRUE, TRUE, 0);

	phbox = gtk_hbox_new(FALSE,5);
	GtkWidget *pToggleBur =  gtk_check_button_new_with_label(_("OpenOffice Suite"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pToggleBur) , TRUE);
	gtk_signal_connect(GTK_OBJECT(pToggleBur), "toggled", G_CALLBACK(basicmode_change), (gpointer)"BUR");
	gtk_box_pack_start(GTK_BOX(phbox), pToggleBur, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), phbox, TRUE, TRUE, 0);

	phbox = gtk_hbox_new(FALSE,5);
	GtkWidget *pToggleNet =  gtk_check_button_new_with_label(_("Network"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pToggleNet) , TRUE);
	gtk_signal_connect(GTK_OBJECT(pToggleNet), "toggled", G_CALLBACK(basicmode_change), (gpointer)"NET");
	gtk_box_pack_start(GTK_BOX(phbox), pToggleNet, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), phbox, TRUE, TRUE, 0);

	phbox = gtk_hbox_new(FALSE,5);
	GtkWidget *pToggleDev =  gtk_check_button_new_with_label(_("Development"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pToggleDev) , TRUE);
	gtk_signal_connect(GTK_OBJECT(pToggleDev), "toggled", G_CALLBACK(basicmode_change), (gpointer)"DEV");
	gtk_box_pack_start(GTK_BOX(phbox), pToggleDev, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), phbox, TRUE, TRUE, 0);

	phbox = gtk_hbox_new(FALSE,5);
	GtkWidget *pToggleMul =  gtk_check_button_new_with_label(_("Multimedia"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pToggleMul) , TRUE);
	gtk_signal_connect(GTK_OBJECT(pToggleMul), "toggled", G_CALLBACK(basicmode_change), (gpointer)"MUL");
	gtk_box_pack_start(GTK_BOX(phbox), pToggleMul, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), phbox, TRUE, TRUE, 0);
	
	gtk_box_pack_start(GTK_BOX(hboxpackage), pvbox, TRUE, TRUE, 0);
	
	//* Extra packages *//
	pvbox = gtk_vbox_new(FALSE, 5);
	GtkWidget *labelgroupex = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(labelgroupex), _("<b>You can choose extra groups :</b>"));
	gtk_box_pack_start(GTK_BOX(pvbox), labelgroupex, TRUE, TRUE, 0);
	
	phbox = gtk_hbox_new(FALSE,5);
	GtkWidget *pToggleAppex =  gtk_check_button_new_with_label(_("Extra - Graphical Applications"));
	gtk_signal_connect(GTK_OBJECT(pToggleAppex), "toggled", G_CALLBACK(basicmode_change), (gpointer)"APPEX");
	gtk_box_pack_start(GTK_BOX(phbox), pToggleAppex, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), phbox, TRUE, TRUE, 0);
	
	phbox = gtk_hbox_new(FALSE,5);
	GtkWidget *pToggleConsex =  gtk_check_button_new_with_label(_("Extra - Console Applications"));
	gtk_signal_connect(GTK_OBJECT(pToggleConsex), "toggled", G_CALLBACK(basicmode_change), (gpointer)"CONSEX");
	gtk_box_pack_start(GTK_BOX(phbox), pToggleConsex, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), phbox, TRUE, TRUE, 0);

	phbox = gtk_hbox_new(FALSE,5);
	GtkWidget *pToggleGame =  gtk_check_button_new_with_label(_("Extra - Games"));
	gtk_signal_connect(GTK_OBJECT(pToggleGame), "toggled", G_CALLBACK(basicmode_change), (gpointer)"GAME");
	gtk_box_pack_start(GTK_BOX(phbox), pToggleGame, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), phbox, TRUE, TRUE, 0);

	phbox = gtk_hbox_new(FALSE,5);
	GtkWidget *pToggleNetex =  gtk_check_button_new_with_label(_("Extra - Network"));
	gtk_signal_connect(GTK_OBJECT(pToggleNetex), "toggled", G_CALLBACK(basicmode_change), (gpointer)"NETEX");
	gtk_box_pack_start(GTK_BOX(phbox), pToggleNetex, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), phbox, TRUE, TRUE, 0);

	phbox = gtk_hbox_new(FALSE,5);
	GtkWidget *pToggleDevex =  gtk_check_button_new_with_label(_("Extra - Development"));
	gtk_signal_connect(GTK_OBJECT(pToggleDevex), "toggled", G_CALLBACK(basicmode_change), (gpointer)"DEVEX");
	gtk_box_pack_start(GTK_BOX(phbox), pToggleDevex, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), phbox, TRUE, TRUE, 0);

	phbox = gtk_hbox_new(FALSE,5);
	GtkWidget *pToggleMulex =  gtk_check_button_new_with_label(_("Extra - Multimedia"));
	gtk_signal_connect(GTK_OBJECT(pToggleMulex), "toggled", G_CALLBACK(basicmode_change), (gpointer)"MULEX");
	gtk_box_pack_start(GTK_BOX(phbox), pToggleMulex, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), phbox, TRUE, TRUE, 0);
	
	//* Put into hbox *//	
	gtk_box_pack_start(GTK_BOX(hboxpackage), pvbox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(pvboxp), hboxpackage, TRUE, TRUE, 0);
	
	return pvboxp;
}


// ------------------------------------------- Main plugin functions -------------------------------------------- //

//* Load main widget *//
GtkWidget *load_gtk_widget()
{
	GtkWidget *vboxmode = gtk_vbox_new(FALSE,5);
	basicmode = getBasicModeWidget();
	expertmode = getExpertModeWidget();
	gtk_box_pack_start(GTK_BOX(vboxmode), expertmode, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vboxmode), basicmode, TRUE, TRUE, 0);

	return vboxmode;
}

int prerun(GList **config)
{
	int j;
	PM_DB *i;
	GtkTreeIter iter;	
	GList *pack = NULL;
	
	GtkWidget* pBoite;
	GtkWidget *progress, *vbox;
	extern GtkWidget *assistant;
	
	gtk_widget_hide(basicmode);
	gtk_widget_hide(expertmode);
	
	//* if previous loaded (previous button used) do nothing *//
	if(syncs == NULL && allpackets == NULL && cats == NULL)
	{
	
	pBoite = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_transient_for(GTK_WINDOW(pBoite), GTK_WINDOW(assistant));
	gtk_window_set_default_size(GTK_WINDOW(pBoite), 320, 70);
	gtk_window_set_position(GTK_WINDOW(pBoite), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_modal(GTK_WINDOW(pBoite), TRUE);
	gtk_window_set_title(GTK_WINDOW(pBoite), _("Loading package's database"));
 	gtk_window_set_deletable(GTK_WINDOW(pBoite), FALSE);

	vbox = gtk_vbox_new (FALSE, 5);

	progress = gtk_progress_bar_new();

	gtk_box_pack_start (GTK_BOX (vbox), progress, TRUE, FALSE, 5);
	
	gtk_container_add(GTK_CONTAINER(pBoite), vbox);
	
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress), _("Initializing pacman-g2"));	
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress), 0.0);	
	gtk_widget_show_all(pBoite);	
	
	// if an instance of pacman running, set it down
	pacman_release();
	
	// Initialize pacman
	if(pacman_initialize(TARGETDIR) == -1)
	{
		LOG("failed to initialize pacman library (%s)\n", pacman_strerror(pm_errno));
		return(1);
	}
	
	pacman_set_option(PM_OPT_LOGMASK, -1);
	pacman_set_option(PM_OPT_LOGCB, (long)cb_log);
	if(pacman_set_option(PM_OPT_DBPATH, (long)PM_DBPATH) == -1)
	{
		LOG("failed to set option DBPATH (%s)\n", pacman_strerror(pm_errno));
		return(1);
	}
		
	// Register with local database
	i = pacman_db_register("local");
	if(i==NULL)
	{
		LOG("could not register 'local' database (%s)\n", pacman_strerror(pm_errno));
		return(1);
	}
	else
		syncs = g_list_append(syncs, i);	
		
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress), 0.3);
	while (gtk_events_pending())
		gtk_main_iteration ();
	
	if(prepare_pkgdb(PACCONF, config, &syncs) == -1)
	{
		return(-1);
	}
	
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress), 0.5);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress), _("Udpate and load database"));
	while (gtk_events_pending())
		gtk_main_iteration ();

	// load categories of packets
	cats = getcat(g_list_nth_data(syncs, 1), syncs);
	if(cats == NULL)
	{
		return(-1);
	}
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress), 0.8);
	while (gtk_events_pending())
		gtk_main_iteration ();

	// preload packets list for each categorie
	for(j=0;j < g_list_length(cats); j+=3)
	{
		pack = group2pkgs(syncs, (char*)g_list_nth_data(cats, j));
		data_put(&allpackets, (char*)g_list_nth_data(cats, j), pack);
	}
	
	gtk_widget_destroy(pBoite);
	
	}

	switch(fwife_question(_("Do you want to use expert mode?\n\nThe normal mode shows a choice like 'C compiler system', the expert mode show you 'C libs', 'C compiler', 'C include files', etc - each individual package. Obviously, you should know what you're doing if you use the expert mode since it's possible to skip packages that are crucial to the functioning of your system. Choose 'no' for using normal mode that select groups of packages, or choose 'yes' for using expert mode with a switch for each package.")))
	{
		case GTK_RESPONSE_YES:			
			gtk_widget_show(expertmode);
			selectmode = 1;
			gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(categories))));
			
			// add cats in treeview list
			for(j=0; j < g_list_length(cats); j+=3) 
			{		
				gtk_list_store_append(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(categories))), &iter);
		
				gtk_list_store_set(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(categories))), &iter,
						   0, (gboolean)GPOINTER_TO_INT(g_list_nth_data(cats, j+2)), 1, (char*)g_list_nth_data(cats, j), 2, (char*)g_list_nth_data(cats, j+1), -1);
			}
			
			break;
		case GTK_RESPONSE_NO:
			selectmode = 0;
			gtk_widget_show(basicmode);
			break;
	}
	
	return(0);
}
	
int run(GList **config)
{	
	int i, j;
	PM_LIST *lp, *junk, *sorted, *x;
	char *ptr;
	GList *allpkgs = NULL;
		
	if(cats == NULL)
		return -1;
	
	if(selectmode == 0)
		configuredesktop();
	
	//* Check for missing dependency *//
	if(pacman_trans_init(PM_TRANS_TYPE_SYNC, PM_TRANS_FLAG_NOCONFLICTS, NULL, NULL, NULL) == -1)
		return(-1);
	
	//* For each checked packet, add it in list *//
	for(i=0; i<g_list_length(cats); i+=3)
	{
		if(GPOINTER_TO_INT(g_list_nth_data(cats, i+2)) == 1)
		{
			GList *pkgs = (GList*)data_get(allpackets, (char*)g_list_nth_data(cats, i));
			for(j=0; j<g_list_length(pkgs); j+=4)
			{
				if(GPOINTER_TO_INT(g_list_nth_data(pkgs, j+3)) == 1)
				{
					ptr = strdup((char*)g_list_nth_data(pkgs, j));
					pacman_trans_addtarget(ptr);
					FREE(ptr);
				}
			}
		}
	}
		
	if(pacman_trans_prepare(&junk) == -1) {
		LOG("pacman-g2 error: %s", pacman_strerror(pm_errno));

		// Well well well, LOG pacman deps error at tty4 
		for(x = pacman_list_first(junk); x; x = pacman_list_next(x))
		{
			PM_DEPMISS *miss = pacman_list_getdata(x);
			LOG(":: %s: %s %s",
			    (char*)pacman_dep_getinfo(miss, PM_DEP_TARGET),
			     (long)pacman_dep_getinfo(miss, PM_DEP_TYPE) == PM_DEP_TYPE_DEPEND ? "requires" : "is required by",
			      (char*)pacman_dep_getinfo(miss, PM_DEP_NAME));
		}
		pacman_list_free(junk);
		pacman_trans_release();
		return(-1);
	}
	sorted = pacman_trans_getinfo(PM_TRANS_PACKAGES);
	for(lp = pacman_list_first(sorted); lp; lp = pacman_list_next(lp))
	{
		PM_SYNCPKG *sync = pacman_list_getdata(lp);
		PM_PKG *pkg = pacman_sync_getinfo(sync, PM_SYNC_PKG);
		ptr = g_strdup_printf("%s-%s", (char*)pacman_pkg_getinfo(pkg, PM_PKG_NAME),
				      (char*)pacman_pkg_getinfo(pkg, PM_PKG_VERSION));
		allpkgs = g_list_append(allpkgs, ptr);
	}
	pacman_trans_release();
	data_put(config, "packages", allpkgs);
	
	pacman_release();
	return 0;
}
 
 
