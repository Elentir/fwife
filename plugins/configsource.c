#include <stdio.h>
#include <gtk/gtk.h>
#include <unistd.h>
#include <stdlib.h>

#include "../setup.h"
#include "../util.h"
#include "common.h"

//* Initial mirror list *//
GList *mirrorlist = NULL;
//* Selected mirror list *//
GList *newmirrorlist = NULL;
//* Custom mirror list *//
GList *custommirrorlist = NULL;

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
	10,
	load_gtk_widget,
	GTK_ASSISTANT_PAGE_CONTENT,
	TRUE,
	NULL,
	run,
	NULL // dlopen handle
};

plugin_t *info()
{
	return &plugin;
}

char *desc()
{
	return _("Configuring the source of the installation");
}

GList *getmirrors(char *fn)
{
	FILE *fp;
	char line[PATH_MAX], *ptr, *country, *preferred;
	GList *mirrors=NULL;

	if ((fp = fopen(fn, "r"))== NULL) { //fopen error
		LOG("Could not open output file %s for reading", fn);
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

//* When a server is toggled *//
static void fixed_toggled (GtkCellRendererToggle *cell, gchar *path_str, gpointer  data)
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
  
  //* Add server to the favourite list *//
  if(!(strcmp(from, "CUSTOM"))) 
  {
	if(fixed == FALSE)
		custommirrorlist = g_list_insert(custommirrorlist, strdup(name),0);
  	else
		custommirrorlist =  g_list_remove (custommirrorlist, (gconstpointer) name);
  }
  else
  {
	if(fixed == FALSE)
		newmirrorlist = g_list_insert(newmirrorlist, strdup(name),0);
  	else
		newmirrorlist =  g_list_remove (newmirrorlist, (gconstpointer) name);
  }

  /* do something with the value */
  fixed ^= 1;

  /* insert new value */
  gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_USE, fixed, -1);

  /* clean up */
  gtk_tree_path_free (path);
}

//* Add a new custom mirror *//
void add_mirror (GtkWidget *button, gpointer data)
{
  GtkTreeIter iter;
  GtkTreeView *treeview = (GtkTreeView *)data;
  GtkTreeModel *model = gtk_tree_view_get_model (treeview);
  
  GtkWidget* pBoite;
  GtkWidget* pEntry, *label;
  const gchar* sName;
  extern GtkWidget *assistant;

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
                      0, TRUE, 1, sName, 2, "CUSTOM",
                      -1);
	    custommirrorlist = g_list_insert(custommirrorlist, strdup(sName),0);
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
      custommirrorlist =  g_list_remove (custommirrorlist, (gconstpointer) old_text);
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
	int i;	
	GtkListStore *store;
	GtkTreeModel *model;
	GtkTreeViewColumn *col;
	GtkTreeIter iter;
	GtkCellRenderer *renderer;
	GtkWidget *view;	
	char *fn;
	
	//* Load mirrors from file *//
	fn = g_strdup_printf("%s/%s", PACCONFPATH, PACCONF);
	mirrorlist = getmirrors(fn);
	FREE(fn);

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

	//* Insert mirrors into treeview *//
	for(i=0; i < g_list_length(mirrorlist); i+=3) 
	{		
		gboolean checked = FALSE;
		gtk_list_store_append(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(view))), &iter);
		
		gtk_list_store_set(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(view))), &iter,
			0, checked,1, (gchar*)g_list_nth_data(mirrorlist, i), 2,(gchar*)g_list_nth_data(mirrorlist, i+1), -1);	
		
	}

	return view;
}

GtkWidget *load_gtk_widget(GtkWidget *assist)
{	
	GtkWidget *button, *view, *pScrollbar, *pSeparator, *doclabel;
	GtkTreeSelection *selection;
	GtkWidget *pVBox = gtk_vbox_new(FALSE, 5);
	GtkWidget *pHBox = gtk_hbox_new(FALSE, 5);
	
	// help label
	doclabel = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(doclabel), "<span face=\"Courier New\"><b>This you can select your favorite mirrors, and blablabla</b></span>\n");
	gtk_box_pack_start(GTK_BOX(pVBox), doclabel, FALSE, FALSE, 0);
	
	// first separator
	pSeparator = gtk_hseparator_new();
    	gtk_box_pack_start(GTK_BOX(pVBox), pSeparator, FALSE, FALSE, 0);
	
	// array of mirrors
	view = mirrorview();
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
        gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);      
        pScrollbar = gtk_scrolled_window_new(NULL, NULL);	
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pScrollbar), view);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pScrollbar), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(pVBox), pScrollbar, TRUE, TRUE, 0);
	
	pSeparator = gtk_hseparator_new();
    	gtk_box_pack_start(GTK_BOX(pVBox), pSeparator, FALSE, FALSE, 0);

	
	gtk_box_pack_start(GTK_BOX(pVBox), pHBox, FALSE, FALSE, 0);
	// buttons for custom mirrors
	button = gtk_button_new_from_stock (GTK_STOCK_ADD );
        g_signal_connect (button, "clicked",
                        G_CALLBACK (add_mirror), view);
        gtk_box_pack_start (GTK_BOX (pHBox), button, TRUE, FALSE, 0);

        button = gtk_button_new_from_stock (GTK_STOCK_REMOVE);
        g_signal_connect (button, "clicked",
                        G_CALLBACK (remove_mirror), view);
        gtk_box_pack_start (GTK_BOX (pHBox), button, TRUE, FALSE, 0);

	return pVBox;
}

int run(GList **config)
{
	int i, j;
	char *fn;
	
	fn = g_strdup_printf("%s/%s", PACCONFPATH, PACCONF);

	for (i=0; i<g_list_length(mirrorlist); i++) {
		if (!strcmp(g_list_nth_data(mirrorlist, i), "Off") ||
				!strcmp(g_list_nth_data(mirrorlist, i), "On"))
		{
			mirrorlist = g_list_remove(mirrorlist, g_list_nth_data(mirrorlist, i));
		}
	}
	// adds country info to the selected mirrors
	// also removes the duplicate mirrors
	for (i=0; i<g_list_length(mirrorlist); i+=2) {
		for (j=0; j<g_list_length(newmirrorlist); j++) {
			if (g_list_nth_data(mirrorlist, i) &&
				!strcmp((char*)g_list_nth_data(mirrorlist, i), (char*)g_list_nth_data(newmirrorlist, j))) {
				newmirrorlist = g_list_insert(newmirrorlist, g_list_nth_data(mirrorlist, i+1), j+1);
				mirrorlist = g_list_remove(mirrorlist, g_list_nth_data(mirrorlist, i));
				mirrorlist = g_list_remove(mirrorlist, g_list_nth_data(mirrorlist, i));
			}
		}
	}
	newmirrorlist = g_list_concat(newmirrorlist, mirrorlist);

	for(i=0; i < g_list_length(custommirrorlist); i++)
	{
		newmirrorlist = g_list_insert(newmirrorlist,g_list_nth_data(custommirrorlist, i),0);
		newmirrorlist = g_list_insert(newmirrorlist,"CUSTOM",1);
	}

	updateconfig(fn, newmirrorlist);
	
	// TOFIX : Keep it if user go previous but need to be free
	//g_list_free(custommirrorlist);
	FREE(fn); 
	return(0);
}
 
