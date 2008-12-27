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
static GtkWidget *pvbox = NULL;

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

PM_DB *mydatabase;

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
		list = g_list_append(list, strdup(pkgfullname));
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
			else if(strstr(ptr, EXGRPSUFFIX))
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
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(packetlist), TRUE);
	
	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (renderer, "toggled", G_CALLBACK (fixed_toggled_pack), model);
	col = gtk_tree_view_column_new_with_attributes (_("Use?"), renderer, "active", 0, NULL);
	gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (col), GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width (GTK_TREE_VIEW_COLUMN (col), 50);
	gtk_tree_view_append_column(GTK_TREE_VIEW(packetlist), col);
	
	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", 1, NULL);
	gtk_tree_view_column_set_title(col, _("Packet name"));
	gtk_tree_view_append_column(GTK_TREE_VIEW(packetlist), col);
	
	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", 2, NULL);
	gtk_tree_view_column_set_title(col, _("Size"));
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
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(categories), TRUE);
	
	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (renderer, "toggled", G_CALLBACK (fixed_toggled_cat), model);
	col = gtk_tree_view_column_new_with_attributes (_("Use?"), renderer, "active", USE_COLUMN, NULL);
	gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (col), GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width (GTK_TREE_VIEW_COLUMN (col), 50);
	gtk_tree_view_append_column(GTK_TREE_VIEW(categories), col);
	
	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", CAT_COLUMN, NULL);
	gtk_tree_view_column_set_title(col, _("Categorie"));
	gtk_tree_view_append_column(GTK_TREE_VIEW(categories), col);
	
	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", SIZE_COLUMN, NULL);
	gtk_tree_view_column_set_title(col, _("Size"));
	gtk_tree_view_append_column(GTK_TREE_VIEW(categories), col);	

	gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (categories)), GTK_SELECTION_SINGLE);
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (categories));
        g_signal_connect (selection, "changed", G_CALLBACK (categorie_changed), NULL);
	
	pScrollbar = gtk_scrolled_window_new(NULL, NULL);
	
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pScrollbar), categories);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pScrollbar), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	
	return pScrollbar;
}

//* Load main widget *//
GtkWidget *load_gtk_widget()
{
	GtkWidget *hsepa1, *hsepa2, *info;
	GtkWidget *image;
	GtkWidget *pvboxp = gtk_vbox_new(FALSE,5);
	GtkWidget *phbox = gtk_hbox_new(TRUE,8);
	pvbox = gtk_vbox_new(FALSE,5);
	GtkWidget *phbox2 = gtk_hbox_new(FALSE,8);
	GtkWidget *categl = getcategorieslist();
	GtkWidget *packetl = getpacketlist();
	hsepa1 = gtk_hseparator_new();
	hsepa2 = gtk_hseparator_new();
	info = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(info), _("<span face=\"Courier New\"><b>Please select which categories/packages to install</b></span>\n"));
	
	// description of each package (in expert mode only)
	packetinfo = gtk_label_new(NULL);
	gtk_label_set_justify(GTK_LABEL(packetinfo), GTK_JUSTIFY_LEFT);
	// load a nice image
	image = gtk_image_new_from_file(g_strdup_printf("%s/packet.png", IMAGEDIR));
	gtk_label_set_line_wrap(GTK_LABEL(packetinfo), TRUE);
	
	gtk_box_pack_start(GTK_BOX(phbox), categl, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(phbox), pvbox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), packetl, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), hsepa1, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(phbox2), image, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(phbox2), packetinfo, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), phbox2, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), hsepa2, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvboxp), info, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvboxp), phbox, TRUE, TRUE, 0);
	return pvboxp;
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

	switch(fwife_question(_("Do you want to use expert mode?\n\nThe normal mode shows a choice like 'C compiler system', the expert mode show you 'C libs', 'C compiler', 'C include files', etc - each individual package. Obviously, you should know what you're doing if you use the expert mode since it's possible to skip packages that are crucial to the functioning of your system. Choose 'no' for using normal mode that select groups of packages, or choose 'yes' for using expert mode with a switch for each package.")))
	{
		case GTK_RESPONSE_YES:
			break;
		case GTK_RESPONSE_NO:
			gtk_widget_hide(pvbox);
			break;
	}
	
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
	
	gtk_widget_show_all(pBoite);
	
	while (gtk_events_pending())
		gtk_main_iteration ();	
		
	// if an instance of pacman running, set it down
	pacman_release();

	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress), _("Initializing pacman-g2"));	
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress), 0.0);
	
	while (gtk_events_pending())
		gtk_main_iteration ();

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
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress), 0.1);
	
	while (gtk_events_pending())
		gtk_main_iteration ();	
	
	// Register with local database
	i = pacman_db_register("local");
	if(i==NULL)
	{
		LOG("could not register 'local' database (%s)\n", pacman_strerror(pm_errno));
		return(1);
	}
	else
		syncs = g_list_append(syncs, i);	
		
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress), 0.1);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress), _("Udpate and load database"));
	if(prepare_pkgdb(PACCONF, config, &syncs) == -1)
	{
		return(-1);
	}
	
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress), 0.3);
	while (gtk_events_pending())
		gtk_main_iteration ();

	// load categories of packets
	cats = getcat(g_list_nth_data(syncs, 1), syncs);
	if(cats == NULL)
	{
		return(-1);
	}
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress), 0.7);	
	
	while (gtk_events_pending())
		gtk_main_iteration ();

	// preload packets list for each categorie
	for(j=0;j < g_list_length(cats); j+=3)
	{
		pack = group2pkgs(syncs, (char*)g_list_nth_data(cats, j));
		data_put(&allpackets, (char*)g_list_nth_data(cats, j), pack);
	}
	
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress), 0.9);
	
	while (gtk_events_pending())
		gtk_main_iteration ();

	// add cats in treeview list
	for(j=0; j < g_list_length(cats); j+=3) 
	{		
		gtk_list_store_append(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(categories))), &iter);
		
		gtk_list_store_set(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(categories))), &iter,
					   0, (gboolean)GPOINTER_TO_INT(g_list_nth_data(cats, j+2)), 1, (char*)g_list_nth_data(cats, j), 2, (char*)g_list_nth_data(cats, j+1), -1);		
	}	
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress), 1.0);
	
	while (gtk_events_pending())
		gtk_main_iteration ();	
	
	gtk_widget_destroy(pBoite);
				
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
	
	//* For each checked packet, add it in list *//
	for(i=0; i<g_list_length(cats); i+=3)
	{
		if(GPOINTER_TO_INT(g_list_nth_data(cats, i+2)) == 1)
		{
			GList *pkgs = (GList*)data_get(allpackets, (char*)g_list_nth_data(cats, i));
			for(j=0; j<g_list_length(pkgs); j+=4)
			{
				if(GPOINTER_TO_INT(g_list_nth_data(pkgs, j+3)) == 1)
					allpkgs = g_list_append(allpkgs, g_list_nth_data(pkgs, j));
			}
		}
	}	

	//* Check for missing dependency *//
	if(pacman_trans_init(PM_TRANS_TYPE_SYNC, PM_TRANS_FLAG_NOCONFLICTS, NULL, NULL, NULL) == -1)
		return(-1);
	for(i=0;i<g_list_length(allpkgs);i++)
	{
		ptr = strdup(drop_version((char*)g_list_nth_data(allpkgs, i)));
		pacman_trans_addtarget(ptr);		
		FREE(ptr);
	}
	g_list_free(allpkgs);
	allpkgs=NULL;
		
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
 
 
