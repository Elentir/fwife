#include <stdio.h>
#include <glib.h>
#include <libfwutil.h>
//#include <libfwtimeconfig.h>
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

typedef struct
{
char *code;
char *coord;
char *name;
char *desc;
} zonetime_t;


plugin_t plugin =
{
	"timeconfig",
	desc,
	60,
	load_gtk_widget,
	GTK_ASSISTANT_PAGE_CONTENT,
	TRUE,
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

GtkWidget *load_gtk_widget(GtkWidget *assist)
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
	gtk_tree_view_column_set_attributes(col, renderer, "text", 0, NULL);
	gtk_tree_view_column_set_title(col, "CODE");
	gtk_tree_view_append_column(GTK_TREE_VIEW(timeview), col);
	
	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", 1, NULL);
	gtk_tree_view_column_set_title(col, "Name");
	gtk_tree_view_append_column(GTK_TREE_VIEW(timeview), col);

	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", 2, NULL);
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
	fwifetime_find();
		
	//* Add zones to the list *//
	if(zonetime)
	{
		for(i=0; i < g_list_length(zonetime); i+=4) 
		{		
			gtk_list_store_append(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(timeview))), &iter);
			
			gtk_list_store_set(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(timeview))), &iter,
				0, (char*)g_list_nth_data(zonetime, i), 1, (char*)g_list_nth_data(zonetime, i+2), 2, (char*)g_list_nth_data(zonetime, i+3), -1);	
		
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
  	  gtk_tree_model_get (model, &iter, 1, &ret, -1);
	  if(ret)
		ptr = g_strdup_printf("/usr/share/zoneinfo/%s", ret);
		symlink(ptr, ZONEFILE);
		FREE(ptr);
    	}	
	return 0;
}

