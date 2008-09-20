/*
 *  configsource.c for Fwife
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
#include <unistd.h>
#include <stdlib.h>

#include "common.h"

static GList *mirrorlist = NULL;

static GtkWidget *viewserver;

enum
{
  COLUMN_USE,
  COLUMN_NAME,
  COLUMN_FROM,
  NUM_COLUMNS
};

plugin_t plugin =
{
	"configsource",	
	desc,
	20,
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
	return (_("Configuring the source of the installation"));
}

GList *getmirrors(char *fn)
{
	FILE *fp;
	char line[PATH_MAX], *ptr, *country, *preferred;
	GList *mirrors=NULL;

	if ((fp = fopen(fn, "r"))== NULL) { //fopen error
		printf("Could not open output file for reading");
		return(NULL);
	}

	/* this string should be the best mirror for the given language from
	 * /etc/pacman-g2/repos */
	preferred = strdup(_("ftp://ftp5."));
	while(!feof(fp)) {
		if(fgets(line, PATH_MAX, fp) == NULL)
			break;
		if(line == strstr(line, "# - ")) { // country
			ptr = strrchr(line, ' ');
			*ptr = '\0';
			ptr = strrchr(line, ' ')+1;
			country = strdup(ptr);
		}
		if(line == strstr(line, "Server = ")) { // server
			ptr = strrchr(line, '/'); // drops frugalware-ARCH
			*ptr = '\0';
			ptr = strstr(line, "Server = ")+9; //drops 'Server = ' part
			mirrors = g_list_append(mirrors, strdup(ptr));
			mirrors = g_list_append(mirrors, strdup(country));
			if(!strncmp(ptr, preferred, strlen(preferred)))
				mirrors = g_list_append(mirrors, strdup("On"));
			else
				mirrors = g_list_append(mirrors, strdup("Off")); //unchecked by default in checkbox
		}
	}
	free(preferred);
	fclose(fp);
	return (mirrors);
}

int updateconfig(char *fn, GList *mirrors)
{
	FILE *fp;
	short i;

	if ((fp = fopen(fn, "w"))== NULL)
	{
		perror(_("Could not open output file for writing"));
		return(1);
	}
	fprintf(fp, "#\n# %s repository\n#\n\n[%s]\n\n", PACCONF, PACCONF);
	for (i=0; i<g_list_length(mirrors); i+=2) {
		// do not change the country style as it will cause getmirrors() misbehaviour
		fprintf(fp, "# - %s -\n", (char *)g_list_nth_data(mirrors, i+1));
		fprintf(fp, "Server = %s/frugalware-%s\n", (char *)g_list_nth_data(mirrors, i), ARCH);
	}
	fclose(fp);
	g_list_free(mirrors);
	return(0);
}

static void fixed_toggled (GtkCellRendererToggle *cell,
	       gchar                 *path_str,
	       gpointer               data)
{
  GtkTreeModel *model = (GtkTreeModel *)data;
  GtkTreeIter  iter;
  GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
  gboolean fixed;
  gchar *name, *from;

  /* get toggled iter */
  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (model, &iter, COLUMN_USE, &fixed, -1);
  gtk_tree_model_get (model, &iter, COLUMN_NAME, &name, -1);
  gtk_tree_model_get (model, &iter, COLUMN_FROM, &from, -1);
  
  /* do something with the value */
  fixed ^= 1;

  /* set new value */
  gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_USE, fixed, -1);

  /* clean up */
  gtk_tree_path_free (path);
}

void add_mirror (GtkWidget *button, gpointer data)
{
  extern GtkWidget *assistant;
  GtkTreeIter iter;
  GtkTreeView *treeview = (GtkTreeView *)data;
  GtkTreeModel *model = gtk_tree_view_get_model (treeview);
  
  GtkWidget* pBoite;
  GtkWidget* pEntry, *label;
  const gchar* sName;

  pBoite = gtk_dialog_new_with_buttons(_("Add a personalized server"),
        GTK_WINDOW(assistant),
        GTK_DIALOG_MODAL,
        GTK_STOCK_OK,GTK_RESPONSE_OK,
        GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,
        NULL);
  gtk_window_set_position(GTK_WINDOW(pBoite), GTK_WIN_POS_CENTER_ON_PARENT);

    pEntry = gtk_entry_new();
    label = gtk_label_new(_("Enter here the address of the server you want to add!\n This can be a local server or a distant one!")); 
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), label, TRUE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), pEntry, TRUE, FALSE, 5);
    gtk_widget_show_all(pBoite);

    switch (gtk_dialog_run(GTK_DIALOG(pBoite)))
    {
        case GTK_RESPONSE_OK:
            sName = gtk_entry_get_text(GTK_ENTRY(pEntry));
            gtk_list_store_append (GTK_LIST_STORE (model), &iter);
            gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                      0, TRUE, 1, sName, 2, "CUSTOM", -1);
	    gtk_widget_destroy(pBoite);
            break;
        case GTK_RESPONSE_CANCEL:
        case GTK_RESPONSE_NONE:
        default:
	gtk_widget_destroy(pBoite);
        break;
    }
  
}

//* remove a mirror from the array widget *//
void remove_mirror (GtkWidget *widget, gpointer data)
{
  GtkTreeIter iter;
  GtkTreeView *treeview = (GtkTreeView *)data;
  GtkTreeModel *model = gtk_tree_view_get_model (treeview);
  GtkTreeSelection *selection = gtk_tree_view_get_selection (treeview);

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
      gint i;
      GtkTreePath *path;
      gchar *old_text, *from;
      gtk_tree_model_get (model, &iter, 1, &old_text, -1);
      gtk_tree_model_get (model, &iter, 2, &from, -1);
      
      // don't delete an "default" mirror
      if(strcmp(from, "CUSTOM"))
		return;
      path = gtk_tree_model_get_path (model, &iter);
      i = gtk_tree_path_get_indices (path)[0];
      gtk_list_store_remove (GTK_LIST_STORE (model), &iter);

      gtk_tree_path_free (path);
    }
    return;
}

//* Create the array of mirror widget *//
GtkWidget *mirrorview()
{
	GtkListStore *store;
	GtkTreeModel *model;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;
	GtkWidget *view;		

	store = gtk_list_store_new(3, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING);
	model = GTK_TREE_MODEL(store);
	
	view = gtk_tree_view_new_with_model(model);
	g_object_unref (model);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(view), TRUE);
	
	renderer = gtk_cell_renderer_toggle_new ();
  	g_signal_connect (renderer, "toggled", G_CALLBACK (fixed_toggled), model);
  	col = gtk_tree_view_column_new_with_attributes ("Use?",
						     renderer,
						     "active", 0,
						     NULL);
	gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (col), GTK_TREE_VIEW_COLUMN_FIXED);
  	gtk_tree_view_column_set_fixed_width (GTK_TREE_VIEW_COLUMN (col), 50);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	
	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", 1, NULL);
	gtk_tree_view_column_set_title(col, "Mirrors");
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	
	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", 2, NULL);
	gtk_tree_view_column_set_title(col, "Froms");
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	return view;
}

GtkWidget *load_gtk_widget()
{	
	GtkWidget *button, *pScrollbar;
	GtkTreeSelection *selection;
	GtkWidget *pVBox = gtk_vbox_new(FALSE, 5);
	GtkWidget *pHBox = gtk_hbox_new(FALSE, 5);
	
	// array of mirrors
	viewserver = mirrorview();
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (viewserver));
        gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);      
        pScrollbar = gtk_scrolled_window_new(NULL, NULL);	
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pScrollbar), viewserver);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pScrollbar), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(pVBox), pScrollbar, TRUE, TRUE, 8);
	
	gtk_box_pack_start(GTK_BOX(pVBox), pHBox, FALSE, FALSE, 5);
	button = gtk_button_new_from_stock (GTK_STOCK_ADD );
        g_signal_connect (button, "clicked", G_CALLBACK (add_mirror), viewserver);
        gtk_box_pack_start (GTK_BOX (pHBox), button, TRUE, FALSE, 0);

        button = gtk_button_new_from_stock (GTK_STOCK_REMOVE);
        g_signal_connect (button, "clicked", G_CALLBACK (remove_mirror), viewserver);
        gtk_box_pack_start (GTK_BOX (pHBox), button, TRUE, FALSE, 0);

	return pVBox;
}

int prerun(GList **config)
{
	GtkTreeIter iter;
	char *fn;
	int i;
	gboolean checked;

	switch(fwife_question(_("You need a net connexion running,\n do you want to configure your network?")))
	{
		case GTK_RESPONSE_YES:
			fw_system_interactive("gnetconfig");
			break;
		case GTK_RESPONSE_NO:
			break;
	}
	
	if(mirrorlist == NULL)
	{
		fn = g_strdup_printf("%s/%s", PACCONFPATH, PACCONF);
		mirrorlist = getmirrors(fn);
		FREE(fn);

		for(i=0; i < g_list_length(mirrorlist); i+=3) 
		{		
			checked = FALSE;
			if(!strcmp((char*)g_list_nth_data(mirrorlist, i+2), "On"))
				checked = TRUE;
			gtk_list_store_append(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(viewserver))), &iter);
		
			gtk_list_store_set(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(viewserver))), &iter,
				0, checked,1, (gchar*)g_list_nth_data(mirrorlist, i), 2,(gchar*)g_list_nth_data(mirrorlist, i+1), -1);	
		
		}
	}
	return 0;
}

int run(GList **config)
{
	char *fn, *name, *from;
	gboolean toggled;
	GList *newmirrorlist = NULL;
	
	fn = g_strdup_printf("%s/%s", PACCONFPATH, PACCONF);

	GtkTreeIter iter;
	GtkTreeView *treeview = (GtkTreeView *)viewserver;
  	GtkTreeModel *model = gtk_tree_view_get_model (treeview);

	if(gtk_tree_model_get_iter_first (model, &iter) == FALSE)
	{
		return(0);
	}
	else
	{
		do
		{
			gtk_tree_model_get (model, &iter, COLUMN_USE, &toggled, COLUMN_NAME, &name, COLUMN_FROM, &from, -1);
			if(toggled == TRUE)
			{
				newmirrorlist = g_list_prepend(newmirrorlist, strdup(from));
				newmirrorlist = g_list_prepend(newmirrorlist, strdup(name));
			}
			else
			{
				newmirrorlist = g_list_append(newmirrorlist, strdup(name));
				newmirrorlist = g_list_append(newmirrorlist, strdup(from));
			}
		} while(gtk_tree_model_iter_next(model, &iter));
	}	

	updateconfig(fn, newmirrorlist);
	FREE(fn); 
	return(0);
}
 
