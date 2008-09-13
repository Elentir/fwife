/*
 *  timeconf.c for Fwife setup
 * 
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
#include <glib.h>
#include <libfwutil.h>
#include <libfwtimeconfig.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libintl.h>

#define CLOCKFILE "/etc/hardwareclock"
#define READZONE "/usr/share/zoneinfo/zone.tab"
#define ZONEFILE "/etc/localtime" 

#include "common.h"
#include "../util.h"

GtkWidget *timeview;

GList *zonetime = NULL;

enum
{
	COLUMN_TIME_CODE,
 	COLUMN_TIME_NAME,
 	COLUMN_TIME_COMMENT
};


plugin_t plugin =
{
	"timeconfig",
	desc,
	60,
	load_gtk_widget,
	GTK_ASSISTANT_PAGE_CONTENT,
	TRUE,
	NULL,
	prerun,
	run,
	NULL // dlopen handle
};

char *desc()
{
	return _("Configuring timezone");
}

plugin_t *info()
{
	return &plugin;
}

int sort_zones(gconstpointer a, gconstpointer b)
{
	return(strcmp(a, b));
}

char* select_mode()
{
	char *modes[] = 
	{
		"localtime", _("Hardware clock is set to local time"),
		"UTC", _("Hardware clock is set to UTC/GMT")
	};
	
	GtkWidget *combo;
	GtkTreeIter iter;
	GtkListStore *store;
	gint i;
	char *ptr;

	extern GtkWidget *assistant;
	GtkTreeModel *model;

	store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
	combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));
	g_object_unref (GTK_TREE_MODEL (store));
	gtk_widget_set_size_request(combo, 350, 40);
	    
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer,"text", 0, NULL);
    
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer,"text", 1, NULL);
	

	for (i = 0; i < 4; i+=2)
	{
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, modes[i], 1, modes[i+1], -1);
	}	

	GtkWidget *pBoite = gtk_dialog_new_with_buttons("Time mode configuration",
        				GTK_WINDOW(assistant),
        				GTK_DIALOG_MODAL,
        				GTK_STOCK_OK,GTK_RESPONSE_OK,
        				NULL);
	
	GtkWidget *label = gtk_label_new("Select UTC or local");
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), label, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), combo, TRUE, FALSE, 10);	
	gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
	

    	/* Affichage des elements de la boite de dialogue */
    	gtk_widget_show_all(GTK_DIALOG(pBoite)->vbox);

   	/* On lance la boite de dialogue et on recupere la reponse */
    	switch (gtk_dialog_run(GTK_DIALOG(pBoite)))
    	{
        	case GTK_RESPONSE_OK:
	
		gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combo), &iter);
		model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
		gtk_tree_model_get (model, &iter, 0, &ptr, -1);
		break;

        	case GTK_RESPONSE_CANCEL:
        	case GTK_RESPONSE_NONE:
        	default: 
            		ptr = NULL;
			break;
    	}
	gtk_widget_destroy(pBoite);
	return ptr;
}

//* Parsing file to get zones, description of each zone, etc
int fwifetime_find()
{
    	char ligne[256];
    	FILE * fIn;
	char *saveptr = NULL;
	char* token;
	char *str2;
	int j = 0;	

    if ((fIn = fopen(READZONE, "r")) == NULL)
       return EXIT_FAILURE;
    while (fgets(ligne, sizeof ligne, fIn))
    {
        if (ligne[0] != '#')
	{
		for (str2 = ligne, j=0;; j++,str2 = NULL)
		{
			token = strtok_r(str2, "\t", &saveptr);
			if(token)
			{
				zonetime = g_list_append(zonetime, strdup(token));	
			}
			else if(j<4)
			{
				zonetime = g_list_append(zonetime, strdup(""));				
				break;
			}
			else
				break;
		}		
	}
    }

    fclose(fIn);

    return EXIT_SUCCESS;
}

GtkWidget *load_gtk_widget()
{
	GtkListStore *store;
	GtkTreeModel *model;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;
	GtkWidget *image, *pVbox, *pScrollbar;
	
	pVbox = gtk_vbox_new(FALSE, 5);	

	image = gtk_image_new_from_file("images/timemap.png");
	gtk_box_pack_start(GTK_BOX(pVbox), image, FALSE, TRUE, 0);

	//GtkWidget *separator = gtk_hseparator_new();
	//gtk_box_pack_start (GTK_BOX (pVbox), separator, FALSE, FALSE, 5);
	
	store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	model = GTK_TREE_MODEL(store);
	
	timeview = gtk_tree_view_new_with_model(model);
	g_object_unref (model);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(timeview), TRUE);
	
	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", COLUMN_TIME_CODE, NULL);
	gtk_tree_view_column_set_title(col, "CODE");
	gtk_tree_view_append_column(GTK_TREE_VIEW(timeview), col);
	
	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", COLUMN_TIME_NAME, NULL);
	gtk_tree_view_column_set_title(col, "Name");
	gtk_tree_view_append_column(GTK_TREE_VIEW(timeview), col);

	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", COLUMN_TIME_COMMENT, NULL);
	gtk_tree_view_column_set_title(col, "Comments");
	gtk_tree_view_append_column(GTK_TREE_VIEW(timeview), col);

	pScrollbar = gtk_scrolled_window_new(NULL, NULL);
	
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pScrollbar), timeview);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pScrollbar), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_box_pack_start(GTK_BOX(pVbox), pScrollbar, TRUE, TRUE, 0);	

	return pVbox;
}

int prerun(GList **config)
{
	int i;
	GtkTreeIter iter;
	
	char *ptr = select_mode();
	if(ptr)
		fwtime_hwclockconf(CLOCKFILE, ptr);
		
	//* Add zones to the list *//
	if(zonetime == NULL)
	{
		// search zones
		fwifetime_find();
		// add them to the list
		for(i=0; i < g_list_length(zonetime); i+=4) 
		{		
			gtk_list_store_append(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(timeview))), &iter);
			
			gtk_list_store_set(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(timeview))), &iter,
				COLUMN_TIME_CODE, (char*)g_list_nth_data(zonetime, i), COLUMN_TIME_NAME, (char*)g_list_nth_data(zonetime, i+2), COLUMN_TIME_COMMENT, (char*)g_list_nth_data(zonetime, i+3), -1);	
		
		}
	}
	return 0;
}

int run(GList **config)
{
	char* ret = NULL, *ptr;
	GtkTreeIter iter;
  	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW(timeview));
  	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(timeview));

  	if (gtk_tree_selection_get_selected (selection, NULL, &iter))
        {
  	  gtk_tree_model_get (model, &iter, COLUMN_TIME_NAME, &ret, -1);
	  if(ret)
		ptr = g_strdup_printf("/usr/share/zoneinfo/%s", ret);
		symlink(ptr, ZONEFILE);
		FREE(ptr);
    	}	
	return 0;
}

