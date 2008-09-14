/*
 *  timeconf.c for Fwife
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

GtkWidget *drawingmap;
GdkPixbuf *image=NULL;
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
	FALSE,
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

void parselatlong(char *latlong, float *latdec, float *longdec)
{
	int deg, min,sec;
	char *mark = latlong+1;
	while((*mark) != '+' && (*mark) != '-')
	{
		mark++;
		if((*mark) == '\0')
			return;
	}
	int longi = atoi(mark);
	int lati = atoi(latlong); //stop on error (so stop when find '+') -> return latitude
	if((lati>=100000) | (lati<=-100000))
	{
		sec = lati%100;
		min = (lati/100)%100;
		deg = (lati/10000);		
	}
	else
	{
		sec = 0;
		min = lati%100;
		deg = lati/100;
	}
	*latdec = ((float)deg)+((float)min/60)+((float)sec/3600);
	
	if((longi>=1000000) | (longi <=-1000000))
	{
		sec = longi%100;
		min = (longi/100)%100;
		deg = (longi/10000);
	}
	else
	{
		sec = 0;
		min = longi%100;
		deg = longi/100;	
	}
	*longdec = ((float)deg)+((float)min/60)+((float)sec/3600);	
}

void getcartesiancoords(char *latlong, int *xcoord, int *ycoord)
{
	float x,y;
	float lat, lon;
	float x2 = (float)gdk_pixbuf_get_width(image);
	float y2 = (float)gdk_pixbuf_get_height(image);
	
	parselatlong(latlong, &lat, &lon);
	
	x = x2 / 2.0 + (x2 / 2.0) * lon / 180.0;
	y = y2 / 2.0 - (y2 / 2.0) * lat / 90.0;
			
	*xcoord = (int)x;
	*ycoord = (int)y;
}

//* /!\ Need free memory *//
char* get_country(char *elem)
{
	char* token;
	char *saveptr = NULL;
	char* str = elem;
	
	token = strtok_r(str, "/", &saveptr);
	return(strdup(token));		
}
	
char* get_city(char *elem)
{
	char* token;
	char *saveptr = NULL;
	char* str = elem;
	
	token = strtok_r(str, "/", &saveptr);
	
	return(strdup(saveptr));		
}

GtkTreePath *find_country_path(char *country)
{
	GtkTreeIter iter;
	GtkTreeView *treeview = (GtkTreeView *)timeview;
	GtkTreeModel *model = gtk_tree_view_get_model (treeview);
	char *name;

	if(gtk_tree_model_get_iter_first (model, &iter) == FALSE)
	{
		return(NULL);
	}
	else
	{
		do
		{
			gtk_tree_model_get (model, &iter, 0, &name, -1);
			if(!strcmp(country, name))
				return(gtk_tree_model_get_path(model, &iter));
			
		} while(gtk_tree_model_iter_next(model, &iter));
	}
	return(NULL);
}

void selection_changed(GtkTreeSelection *selection, gpointer data)
{
	GtkTreeIter iter, iter_parent;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(timeview));
	
	if (gtk_tree_selection_get_selected (selection, NULL, &iter))
	{
		if(gtk_tree_model_iter_parent(model, &iter_parent, &iter) == FALSE)
		{	
			set_page_incompleted();
			return;
		}
		else
		{
			set_page_completed();
		}		
	}
	else
	{
		set_page_incompleted();
	}
	
	gtk_widget_queue_draw(drawingmap);
}

gboolean affiche_dessin(GtkWidget *dessin, GdkEventExpose *event, gpointer data)
{
	GdkColormap *colormap;
	GdkColor couleur_fond;
	int i, xcoord, ycoord;
	char *coords;
	
	GtkTreeView *treeview;
	GtkTreeModel *model;
	GtkTreeIter iter, iter_parent;
	char *country, *city;
	GtkTreeSelection *selection;

	colormap = gdk_drawable_get_colormap(dessin->window);
    
     // On fixe la couleur de fond
	couleur_fond.red=0xFFFF;
	couleur_fond.green=0xFFFF;
	couleur_fond.blue=0;	

	gdk_colormap_alloc_color(colormap,&couleur_fond,FALSE,FALSE);
	
    // Affichage de l'image par dessus le fond vert
	gdk_draw_pixbuf(dessin->window, dessin->style->fg_gc[GTK_WIDGET_STATE (dessin)], image, 0, 0, 0, 0,
        gdk_pixbuf_get_width(image), gdk_pixbuf_get_height(image), GDK_RGB_DITHER_MAX, 0, 0);
	
	gdk_gc_set_foreground(dessin->style->fg_gc[GTK_WIDGET_STATE(dessin)],
			      &couleur_fond);
	
	if(zonetime)
	{
		for(i=0; i<g_list_length(zonetime); i+=4)
		{
			coords = (char*)g_list_nth_data(zonetime, i+1);
			getcartesiancoords(coords, &xcoord, &ycoord);
			gdk_draw_rectangle(drawingmap->window, drawingmap->style->fg_gc[GTK_WIDGET_STATE(drawingmap)], TRUE, xcoord, ycoord, 3, 3);
		}
	}
	
	couleur_fond.red=0xFFFF;
	couleur_fond.green=0;
	couleur_fond.blue=0;
	
	gdk_colormap_alloc_color(colormap,&couleur_fond,FALSE,FALSE);
	
	
	gdk_gc_set_foreground(dessin->style->fg_gc[GTK_WIDGET_STATE(dessin)],
			      &couleur_fond);
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (timeview));
	treeview = gtk_tree_selection_get_tree_view (selection);
  		
	if (gtk_tree_selection_get_selected (selection, &model, &iter))
	{
		gtk_tree_model_get (model, &iter, 0, &city, -1);
					
		if(gtk_tree_model_iter_parent(model, &iter_parent, &iter) == FALSE)
		{	
			gdk_gc_set_foreground(dessin->style->fg_gc[GTK_WIDGET_STATE(dessin)],
					      &dessin->style->black);
			return TRUE;
		}
		else
		{
			gtk_tree_model_get (model, &iter_parent, 0, &country, -1);			
		}
		
		char *total = g_strdup_printf("%s/%s",country, city);
		
		gint pos =  g_list_position(zonetime, g_list_find_custom(zonetime, total, cmp_str));
		if(i>=0)
		{
			getcartesiancoords((char*)g_list_nth_data(zonetime, pos-1), &xcoord, &ycoord);
			gdk_draw_rectangle(drawingmap->window, drawingmap->style->fg_gc[GTK_WIDGET_STATE(drawingmap)], TRUE, xcoord, ycoord, 3, 3);
		}
		FREE(total);
				
	}
		
	// On remet la couleur noir pour le texte.
	gdk_gc_set_foreground(dessin->style->fg_gc[GTK_WIDGET_STATE(dessin)],
			      &dessin->style->black);		

	return TRUE;
}

//* Parsing file to get zones, description of each zone, etc
int fwifetime_find()
{
    	char ligne[256];
    	FILE * fIn;
	char *saveptr = NULL;
	char* token, *mark;
	char *str;
	int j = 0;	

    if ((fIn = fopen(READZONE, "r")) == NULL)
       return EXIT_FAILURE;
    while (fgets(ligne, sizeof ligne, fIn))
    {
        if (ligne[0] != '#')
	{
		for (str = ligne, j=0;; j++,str = NULL)
		{
			token = strtok_r(str, "\t", &saveptr);
			if(token)
			{
				mark = token;
				while((*mark) != '\0')
				{
					mark++;
					if((*mark) == '\n')
						(*mark) = '\0';
				}
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
	GtkTreeStore *store;
	GtkTreeModel *model;
	GtkTreeViewColumn *col;
	GtkTreeSelection *selection;
	GtkCellRenderer *renderer;
	GtkWidget *pVbox, *pScrollbar;
	GtkWidget *hboximg;
	
	pVbox = gtk_vbox_new(FALSE, 5);	
	hboximg = gtk_hbox_new(FALSE, 5);
	
	drawingmap=gtk_drawing_area_new();

	image = gdk_pixbuf_new_from_file("images/timemap.png", NULL);
	gtk_widget_set_size_request(drawingmap, gdk_pixbuf_get_width(image), gdk_pixbuf_get_height(image));
	
	g_signal_connect(G_OBJECT(drawingmap),"expose_event", (GCallback)affiche_dessin, NULL);
	
	gtk_box_pack_start(GTK_BOX(hboximg), drawingmap, FALSE, TRUE, 150);
	gtk_box_pack_start(GTK_BOX(pVbox), hboximg, FALSE, TRUE, 5);

	//GtkWidget *separator = gtk_hseparator_new();
	//gtk_box_pack_start (GTK_BOX (pVbox), separator, FALSE, FALSE, 5);
	
	store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	model = GTK_TREE_MODEL(store);
	
	timeview = gtk_tree_view_new_with_model(model);
	g_object_unref (model);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(timeview), TRUE);
	
	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", 0, NULL);
	gtk_tree_view_column_set_title(col, "Continent/Main City");
	gtk_tree_view_append_column(GTK_TREE_VIEW(timeview), col);

	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", 1, NULL);
	gtk_tree_view_column_set_title(col, "Comments");
	gtk_tree_view_append_column(GTK_TREE_VIEW(timeview), col);
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (timeview));
	
	g_signal_connect (selection, "changed",  G_CALLBACK (selection_changed), NULL);

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
	GtkTreeIter child_iter;
	GtkTreePath *path = NULL;
	
	GtkTreeView *treeview = (GtkTreeView *)timeview;
	GtkTreeModel *model = gtk_tree_view_get_model (treeview);	
	char *country, *city, *elem;
	
	fwifetime_find();
	
		
	if(zonetime)
	{
		for(i=0; i < g_list_length(zonetime); i+=4) 
		{		
			elem = strdup((char*)g_list_nth_data(zonetime, i+2));
			country = get_country(elem);
			FREE(elem);
			elem = strdup((char*)g_list_nth_data(zonetime, i+2));
			city = get_city(elem);
			FREE(elem);
			if((path = find_country_path(country)) == NULL)
			{
				gtk_tree_store_append(GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(timeview))), &iter, NULL);
				gtk_tree_store_set(GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(timeview))), &iter,
					   0, country, -1);
				gtk_tree_store_append(GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(timeview))), &child_iter, &iter);
				gtk_tree_store_set(GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(timeview))), &child_iter,
						0, city,1,(char*)g_list_nth_data(zonetime, i+3), -1);
				
			}
			else
			{
				if(gtk_tree_model_get_iter(model, &iter, path) == FALSE)
					return(-1); 				
				gtk_tree_store_append(GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(timeview))), &child_iter, &iter);
				gtk_tree_store_set(GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(timeview))), &child_iter,
						0, city,1,(char*)g_list_nth_data(zonetime, i+3), -1);
			}
			FREE(country);
			FREE(elem);
		}
	}
	
	return 0;
}

int run(GList **config)
{
	char* city = NULL, *country = NULL, *ptr=NULL;
	GtkTreeIter iter, iter_parent;
  	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(timeview));
  	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(timeview));

  	if (gtk_tree_selection_get_selected (selection, NULL, &iter))
        {
  	  gtk_tree_model_get (model, &iter, 0, &city, -1);
	  
					
	  if(gtk_tree_model_iter_parent(model, &iter_parent, &iter) == FALSE)
	  {	
		 return(-1);
	  }
	  else
	  {
		  gtk_tree_model_get (model, &iter_parent, 0, &country, -1);			
	  }
		
	  ptr = g_strdup_printf("%s/%s",country, city);
	  if(ptr)
		symlink(ptr, ZONEFILE);
	  FREE(ptr);
    	 }
	return 0;
}

