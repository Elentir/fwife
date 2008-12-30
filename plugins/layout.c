/*
 *  layout.c for Fwife
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

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>

#include "common.h"

//* Define keymap directories *//
#define XKEYDIR "/usr/share/X11/xkb/symbols"
#define CKEYDIR "/usr/share/keymaps/i386"

//* Use for console keymap *//
static GList *consolekeymap = NULL;

//* Idem for x keymaps *//
static GList *xkeymap = NULL;

//* Main GTK WIdget *//
static GtkWidget *viewlayout = NULL;

plugin_t plugin =
{
	"layout",
	desc,
	15,
	load_gtk_widget,
	GTK_ASSISTANT_PAGE_CONTENT,
	FALSE,
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
	return _("Keyboard map selection");
}

int find(char *dirname)
{
	DIR *dir;
	struct dirent *ent;
	struct stat statbuf;
	char *fn;
	FILE * fp;
	char * line = NULL;
	size_t len = 0;
	ssize_t read;
	int i;
	GtkTreeIter iter;
	
	char *name;
	char *layout;
	char *consolemap;
	char *variante;

	dir = opendir(dirname);
	if (!dir)
	{
		perror(dirname);
		return(1);
	}
	while ((ent = readdir(dir)) != NULL)
	{
		fn = g_strdup_printf("%s/%s", dirname, ent->d_name);
		if(!stat(fn, &statbuf) && S_ISREG(statbuf.st_mode))
		{
	       		fp = fopen(fn, "r");
        		if (fp == NULL)
                		exit(-1);

        		for(i=0; i<4; i++)
			{
				line = NULL;
				if((read = getline(&line, &len, fp)) != -1)
				{
					if(i==0)
					{
						line[strlen(line)-1] = '\0';
						name = line;
					}
					else if(i==1)
					{
						line[strlen(line)-1] = '\0';
						layout = line;
					}
					else if(i==2)
					{
						line[strlen(line)-1] = '\0';
						consolemap = line;
					}
					else
					{
						variante = line;
					}
				}
			}
			
			gtk_tree_store_append(GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(viewlayout))), &iter, NULL);
			gtk_tree_store_set(GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(viewlayout))), &iter,
					   0, name, 1, layout, 2, variante, 3, consolemap, -1);
			
			FREE(name);
			FREE(layout);
			FREE(consolemap);
			FREE(variante);
			fclose(fp);
		}
	}
	closedir(dir);
	return(0);
}

int find_console_layout(char *dirname)
{
	DIR *dir;
	struct dirent *ent;
	struct stat statbuf;
	char *ext, *fn;

	dir = opendir(dirname);
	if (!dir)
	{
		perror(dirname);
		return(1);
	}
	while ((ent = readdir(dir)) != NULL)
	{
		fn = g_strdup_printf("%s/%s", dirname, ent->d_name);
		if(!stat(fn, &statbuf) && S_ISDIR(statbuf.st_mode))
			if(strcmp(ent->d_name, ".") &&
						strcmp(ent->d_name, "..") &&
						strcmp(ent->d_name, "include"))
				find_console_layout(fn);
		if(!stat(fn, &statbuf) && S_ISREG(statbuf.st_mode) &&
				  (ext = strrchr(ent->d_name, '.')) != NULL)
			if (!strcmp(ext, ".gz"))
				consolekeymap = g_list_append(consolekeymap, g_strdup_printf("%s%s", strrchr(dirname, '/')+1, fn+strlen(dirname)));
	}
	closedir(dir);
	return(0);
}

int find_x_layout(char *dirname)
{
	DIR *dir;
	struct dirent *ent;
	struct stat statbuf;
	char *fn;

	dir = opendir(dirname);
	if (!dir)
	{
		perror(dirname);
		return(1);
	}
	while ((ent = readdir(dir)) != NULL)
	{
		fn = g_strdup_printf("%s/%s", dirname, ent->d_name);
		if(!stat(fn, &statbuf) && S_ISREG(statbuf.st_mode) &&
				  (strlen(ent->d_name) <= 3))
			xkeymap = g_list_append(xkeymap, strdup(ent->d_name));
	}
	xkeymap = g_list_sort(xkeymap, cmp_str);
	
	closedir(dir);
	return(0);
}

void add_console_layout(GtkWidget *combo)
{
	int i;
	for(i=0; i<g_list_length(consolekeymap);i++)
	{
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), g_list_nth_data(consolekeymap, i));
	}
	return;
}

void add_x_layout(GtkWidget *combo)
{
	int i;
	for(i=0; i<g_list_length(xkeymap);i++)
	{
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), g_list_nth_data(xkeymap, i));
	}
	return;
}

//* Need to have one selected - disable next button if unselected *//
void selection_changed(GtkTreeSelection *selection, gpointer data)
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(viewlayout));
	char *ptr, *layout, *variante;
	
	if (gtk_tree_selection_get_selected (selection, NULL, &iter))
	{
		gtk_tree_model_get (model, &iter, 1, &layout, -1);
		gtk_tree_model_get (model, &iter, 2, &variante, -1);
		
		if(!strcmp(variante, "default"))
		{
			ptr = g_strdup_printf("setxkbmap -layout %s", layout);
			fw_system(ptr);
			FREE(ptr);
		}
		else
		{
			ptr = g_strdup_printf("setxkbmap -layout %s -variant %s", layout, variante);
			fw_system(ptr);
			FREE(ptr);
		}
		set_page_completed();
	}
	else
	{
		set_page_incompleted();
	}
}

char *miseazero(char *chaine)
{
	int i = 0;
	while((chaine[i] != '\n') && i < strlen(chaine))
		i++;
	chaine[i] = '\0';
	
	return chaine;
}

void model_changed(GtkWidget *combo, gpointer data)
{
	char * line = NULL;
	size_t len = 0;
	ssize_t read;
	FILE * fp;
	
	if(strcmp((char*)gtk_combo_box_get_active_text(GTK_COMBO_BOX(combo)), ""))
	{	
		// TODO : Parse file in C directly
		char *comm = g_strdup_printf("cat /usr/share/X11/xkb/symbols/%s | grep xkb_symbols | cut -f2 -d \"\\\"\" | uniq -i", (char*)gtk_combo_box_get_active_text(GTK_COMBO_BOX(combo)));
		fp = popen(comm, "r");
		
		gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(data))));
		
		gtk_combo_box_append_text(GTK_COMBO_BOX(data), "default");
		
		while ((read = getdelim(&line, &len, '\n', fp)) != -1) {
			line = miseazero(line);
			gtk_combo_box_append_text(GTK_COMBO_BOX(data),line);
			FREE(line)
		}
		
		gtk_combo_box_set_active (GTK_COMBO_BOX (data), 0);
	}
	
	pclose(fp);

}

void add_keyboard(GtkWidget *button, gpointer data)
{
	GtkWidget* pBoite;
	GtkWidget *pComboCons, *pEntryName, *pComboModel, *pComboVar;
	char* sName, *sCons, *sModel, *sVar;
	GtkWidget *pVBox;
	GtkWidget *pFrame;
	GtkWidget *pVBoxFrame;
	GtkWidget *pLabel;
	GtkWidget *image;
	
	extern GtkWidget *assistant;

	GtkTreeIter iter;
	
	pBoite = gtk_dialog_new_with_buttons(_("Personalized keyboard"),
					     NULL,
	  					GTK_DIALOG_MODAL,
   						GTK_STOCK_OK,GTK_RESPONSE_OK,
   						GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,
   						NULL);
	gtk_window_set_transient_for(GTK_WINDOW(pBoite), GTK_WINDOW(assistant));
	gtk_window_set_position(GTK_WINDOW(pBoite), GTK_WIN_POS_CENTER_ON_PARENT);
   	
	pVBox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), pVBox, FALSE, FALSE, 5);
	
	image = gtk_image_new_from_file(g_strdup_printf("%s/key48.png", IMAGEDIR));
	gtk_box_pack_start(GTK_BOX(pVBox), image, FALSE, FALSE, 0);

	/* Creation du premier GtkFrame */
	pFrame = gtk_frame_new(_("Name"));
	gtk_box_pack_start(GTK_BOX(pVBox), pFrame, TRUE, FALSE, 0);

	/* Creation et insertion d une boite pour le premier GtkFrame */
	pVBoxFrame = gtk_vbox_new(TRUE, 0);
	gtk_container_add(GTK_CONTAINER(pFrame), pVBoxFrame);

	/* Creation et insertion des elements contenus dans le premier GtkFrame */
	pEntryName = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(pVBoxFrame), pEntryName, TRUE, TRUE, 0);
    
	/* Creation du deuxieme GtkFrame */
	pFrame = gtk_frame_new(_("Console Keymap "));
	gtk_box_pack_start(GTK_BOX(pVBox), pFrame, TRUE, FALSE, 0);

	/* Creation et insertion d une boite pour le deuxieme GtkFrame */
	pVBoxFrame = gtk_vbox_new(TRUE, 0);
	gtk_container_add(GTK_CONTAINER(pFrame), pVBoxFrame);

	//* Combobox for selecting console keymap *//
	pComboCons = gtk_combo_box_new_text();
	add_console_layout(pComboCons);
	gtk_combo_box_set_active (GTK_COMBO_BOX (pComboCons), 0);
	gtk_box_pack_start(GTK_BOX(pVBoxFrame), pComboCons, TRUE, TRUE, 0);	

	/* Creation du troisieme GtkFrame */
	pFrame = gtk_frame_new(_("X Keymap (Used only if X server installed)"));
	gtk_box_pack_start(GTK_BOX(pVBox), pFrame, TRUE, FALSE, 5);

	/* Creation et insertion d une boite pour le troisieme GtkFrame */
	pVBoxFrame = gtk_vbox_new(TRUE, 0);
	gtk_container_add(GTK_CONTAINER(pFrame), pVBoxFrame);

	/* Creation et insertion des elements contenus dans le troisieme GtkFrame */
	pLabel = gtk_label_new(_("Model"));
	gtk_box_pack_start(GTK_BOX(pVBoxFrame), pLabel, TRUE, FALSE, 0);
	pComboModel = gtk_combo_box_new_text();
	gtk_box_pack_start(GTK_BOX(pVBoxFrame), pComboModel, TRUE, FALSE, 0);
	
	pLabel = gtk_label_new(_("Variant"));
	gtk_box_pack_start(GTK_BOX(pVBoxFrame), pLabel, TRUE, FALSE, 0);
	pComboVar = gtk_combo_box_new_text();
	gtk_box_pack_start(GTK_BOX(pVBoxFrame), pComboVar, TRUE, FALSE, 0);
	
	g_signal_connect (pComboModel, "changed", G_CALLBACK (model_changed),(gpointer)pComboVar);
	add_x_layout(pComboModel);
	
	gtk_widget_show_all(pBoite);

	switch (gtk_dialog_run(GTK_DIALOG(pBoite)))
	{
		case GTK_RESPONSE_OK:
			sName = (char*)gtk_entry_get_text(GTK_ENTRY(pEntryName));
			sCons = (char*)gtk_combo_box_get_active_text(GTK_COMBO_BOX(pComboCons));
			sModel = (char*)gtk_combo_box_get_active_text(GTK_COMBO_BOX(pComboModel));
			sVar = (char*)gtk_combo_box_get_active_text(GTK_COMBO_BOX(pComboVar));
			
			// is name is empty
			if(!strcmp(sName, ""))
			{
				fwife_error(_("You must enter a name for this keyboard"));
				gtk_widget_destroy(pBoite);
				break;
			}
			
			// add the new keyboard to the list
			gtk_tree_store_append(GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(viewlayout))), &iter, NULL);
			gtk_tree_store_set(GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(viewlayout))), &iter,
					   0, sName, 1, sModel, 2, sVar, 3, sCons, -1);
	   
			gtk_widget_destroy(pBoite);
			break;
		case GTK_RESPONSE_CANCEL:
		case GTK_RESPONSE_NONE:
		default:
			gtk_widget_destroy(pBoite);
			break;
	}
}

GtkWidget *load_gtk_widget()
{
	GtkTreeStore *store;
	GtkTreeModel *model;
	GtkTreeViewColumn *col;	
	GtkCellRenderer *renderer;
	GtkWidget *pScrollbar, *pvbox;
	GtkTreeSelection *selection;
		
	store = gtk_tree_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	model = GTK_TREE_MODEL(store);
	
	viewlayout = gtk_tree_view_new_with_model(model);
	g_object_unref (model);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(viewlayout), TRUE);
		
	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", 0, NULL);
	gtk_tree_view_column_set_title(col, "Name");
	gtk_tree_view_append_column(GTK_TREE_VIEW(viewlayout), col);
		
	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", 1, NULL);
	gtk_tree_view_column_set_title(col, "X Layout");
	gtk_tree_view_append_column(GTK_TREE_VIEW(viewlayout), col);
	
	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", 2, NULL);
	gtk_tree_view_column_set_title(col, "Variant");
	gtk_tree_view_append_column(GTK_TREE_VIEW(viewlayout), col);
	
	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", 3, NULL);
	gtk_tree_view_column_set_title(col, "Console keymap");
	gtk_tree_view_append_column(GTK_TREE_VIEW(viewlayout), col);
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (viewlayout));
        gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);	
      
        g_signal_connect (selection, "changed", G_CALLBACK (selection_changed),NULL);
	
	pScrollbar = gtk_scrolled_window_new(NULL, NULL);
	
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pScrollbar), viewlayout);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pScrollbar), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	pvbox = gtk_vbox_new(FALSE, 5);

	GtkWidget *labelhelp = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(labelhelp), _("<span face=\"Courier New\"><b>You may select one of the following keyboard maps.</b></span>\n"));
	gtk_box_pack_start(GTK_BOX(pvbox), labelhelp, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(pvbox), pScrollbar, TRUE, TRUE, 0);
	
	GtkWidget *hboxbas = gtk_hbox_new(FALSE, 5);
	GtkWidget *entrytest = gtk_entry_new();
	GtkWidget *labeltest = gtk_label_new("Test your keyboard here :");
	GtkWidget *persokeyb = gtk_button_new_with_label("Personalized keyboard");
	GtkWidget *image = gtk_image_new_from_file(g_strdup_printf("%s/key24.png", IMAGEDIR));
	gtk_button_set_image(GTK_BUTTON(persokeyb), image);
	g_signal_connect (persokeyb, "clicked", G_CALLBACK (add_keyboard),NULL);
	
	gtk_box_pack_start(GTK_BOX(hboxbas), labeltest, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hboxbas), entrytest, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hboxbas), persokeyb, FALSE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX(pvbox), hboxbas, FALSE, FALSE, 5);

	return pvbox;
}

int prerun(GList **config)
{
	//* Load keymaps use for personalized keyboard *//
	find_console_layout(CKEYDIR);
	find_x_layout(XKEYDIR);

	//* Loads keymaps from files *//
	find(KEYDIR);	

	return 0;
}

int run(GList **config)
{
	char *fn, *ptr;
	FILE* fp;
	GtkTreeModel *model = NULL;
	GtkTreeIter iter;
	char *laycons, *xlayout, *xvariant;
	
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(viewlayout));	
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(viewlayout));
	
	if (gtk_tree_selection_get_selected (selection, NULL, &iter))
	{
		gtk_tree_model_get (model, &iter, 1, &xlayout, 2, &xvariant, 3, &laycons, -1);	
	}

	//* Create /sysconfig/keymap temporary file *//
	fn = strdup("/tmp/setup_XXXXXX");
	mkstemp(fn);
	if ((fp = fopen(fn, "w")) == NULL)
	{
		perror(_("Could not open output file for writing"));
		return(1);
	}
	fprintf(fp, "# /etc/sysconfig/keymap\n\n"
		"# specify the keyboard map, maps are in "
		"/usr/share/keymaps\n\n");
	if(strstr(laycons, "/"))
		fprintf(fp, "keymap=%s\n", strstr(laycons, "/")+1);
	
	fclose(fp);

	//* Pur data in config *//
	data_put(config, "keymap", fn);
	data_put(config, "xlayout", strdup(xlayout));
	data_put(config, "xvariant", strdup(xvariant));

	return 0;
}
 
 
