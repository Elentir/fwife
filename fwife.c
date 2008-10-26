/*
 *  fwife.c for Fwife
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

#include <stdlib.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <string.h>
#include "fwife.h"

//* list of all loaded plugins *//
GList *plugin_list;

//* used by plugins for get/put config *//
GList *config=NULL;

//* current loaded plugin *//
plugin_t *plugin_active;

//* Widget for the assistant *//
GtkWidget *assistant = NULL;

//* Pages data *//
PageInfo *pages = NULL;

//* Function used for sort plugins with priorities *//
int sort_plugins(gconstpointer a, gconstpointer b)
{
	const plugin_t *pa = a;
	const plugin_t *pb = b;
	return (pa->priority - pb->priority);
}

//* Open a file plugin and add it into list *//
int fwife_add_plugins(char *filename)
{
    	void *handle;
	void *(*infop) (void);

	if ((handle = dlopen(filename, RTLD_NOW)) == NULL)
	{
		fprintf(stderr, "%s", dlerror());
		return(1);
	}

	if ((infop = dlsym(handle, "info")) == NULL)
	{
		fprintf(stderr, "%s", dlerror());
		return(1);
	}
	plugin_t *plugin = infop();
	plugin->handle = handle;
	plugin_list = g_list_append(plugin_list, plugin);

	return(0);
}

//* Find dynamic plugins and add them *//
int fwife_load_plugins(char *dirname)
{
    char *filename, *ext;
	DIR *dir;
	struct dirent *ent;
	struct stat statbuf;

	dir = opendir(dirname);
	if (!dir)
	{
		perror(dirname);
		return(1);
	}
	// remenber that asklang is now special plugin
	while ((ent = readdir(dir)) != NULL)
	{
		filename = g_strdup_printf("%s/%s", dirname, ent->d_name);
		if (!stat(filename, &statbuf) && S_ISREG(statbuf.st_mode) &&
				(ext = strrchr(ent->d_name, '.')) != NULL)
			if (!strcmp(ext, SHARED_LIB_EXT) && strcmp(ent->d_name, "asklang.so"))
				fwife_add_plugins(filename);		
		free(filename);

	}
	closedir(dir);
	return(0);
}

//* Unload dynamic plugins *//
int cleanup_plugins()
{
	int i;
	plugin_t *plugin;

	for (i=0; i<g_list_length(plugin_list); i++)
	{
		plugin = g_list_nth_data(plugin_list, i);
		dlclose(plugin->handle);
	}
	return(0);
}

void fwife_exit()
{
	FREE(pages);
   	cleanup_plugins();
   	gtk_main_quit();	
}

//* Dialog box for quit the installation *//
void cancel_install(GtkWidget *w, gpointer user_data)
{
   int result = fwife_question(_("Are you sure you want to exit from the installer?\n"));
   if(result == GTK_RESPONSE_YES)
   {
        fwife_exit();
   }   
    return;
}

//* Dialog Box when instllation finished *//
void close_install(GtkWidget *w, gpointer user_data)
{
   fwife_info(_("Frugalware installation completed.\n You can now reboot your computer"));
   plugin_active->run(&config); //run finish plugin
   fwife_exit();
   
   return;
}

//* Load next plugin *//
int plugin_next(GtkWidget *w, gpointer user_data)
{
	if(plugin_active->run(&config) == -1)
	{
	   	
		LOG("Error when running plugin %s\n", plugin_active->name);
		fwife_error("Error when running plugin. Please report");
	}
    	// next plugin
	plugin_active = g_list_nth_data(plugin_list, g_list_index(plugin_list, (gconstpointer)plugin_active)+1);
    	// prerun
	if(plugin_active->prerun)
        	plugin_active->prerun(&config);
	return 0;
}

//* load previous plugin *//
int plugin_previous(GtkWidget *w, gpointer user_data)
{
	plugin_active = g_list_nth_data(plugin_list, g_list_index(plugin_list, (gconstpointer)plugin_active)-1);

	// prerun
	if(plugin_active->prerun)
        	plugin_active->prerun(&config);

	return 0;
}

//* Load next plugin *//
int show_help(GtkWidget *w, gpointer user_data)
{
	if((plugin_active->load_help_widget) != NULL)
	{
	
		GtkWidget *helpwidget = plugin_active->load_help_widget();
		
		GtkWidget *pBoite = gtk_dialog_new_with_buttons(_("Plugin help"),
        				GTK_WINDOW(assistant),
        				GTK_DIALOG_MODAL,
        				GTK_STOCK_OK,GTK_RESPONSE_OK,
        				NULL);

		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), helpwidget, TRUE, TRUE, 5);

    		gtk_widget_show_all(GTK_DIALOG(pBoite)->vbox);

    		gtk_dialog_run(GTK_DIALOG(pBoite));
		gtk_widget_destroy(pBoite);
    	}
	else
	{
		fwife_error(_("No help available for this plugin"));
	}
    	
	return 0;
}

//* Asklang is now a special plugin loaded before all others plugins *//
int ask_language()
{
	void *handle;
	void *(*infop) (void);
	
	char *ptr = g_strdup_printf("%s/%s", PLUGINDIR, "asklang.so");
	if ((handle = dlopen(ptr, RTLD_NOW)) == NULL)
	{
		fprintf(stderr, "%s", dlerror());
		FREE(ptr);
		return(1);
	}
	FREE(ptr);

	if ((infop = dlsym(handle, "info")) == NULL)
	{
		fprintf(stderr, "%s", dlerror());
		return(1);
	}
	plugin_t *pluginlang = infop();
	pluginlang->handle = handle;
	
	GtkWidget *pBoite = gtk_dialog_new_with_buttons("Language selection (default : en_US)",
			NULL,
			GTK_DIALOG_MODAL,
       			GTK_STOCK_OK,GTK_RESPONSE_OK,
       			NULL);
	gtk_widget_set_size_request (pBoite, 800, 600);
	gtk_window_set_deletable ( GTK_WINDOW ( pBoite ), FALSE );

	
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file ("/usr/share/fwife/images/headlogo.png", NULL);
	if(!pixbuf) 
	{
		fprintf(stdout, "error message: can't load images/headlogo.png\n");
	}
	else
	{
		/* Set it as icon window */
		gtk_window_set_icon(GTK_WINDOW (pBoite),pixbuf);
	}
	g_object_unref(pixbuf);

	GtkWidget *langtab = pluginlang->load_gtk_widget();
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), langtab, TRUE, TRUE, 5);

	gtk_widget_show_all(GTK_DIALOG(pBoite)->vbox);

	gtk_dialog_run(GTK_DIALOG(pBoite));
	pluginlang->run(&config);
	gtk_widget_destroy(pBoite);
	return 0;
}


int main (int argc, char *argv[])
{
  int i;
  GError *gerror = NULL;
  GdkColor color;
  plugin_t *plugin;
  
  gtk_init (&argc, &argv);

  ask_language();

  /* Create a new assistant widget with no pages. */
  assistant = gtk_assistant_new ();
  gtk_widget_set_size_request (assistant, 800, 600);
  //gtk_window_unfullscreen (GTK_WINDOW (assistant));
  gtk_window_set_title (GTK_WINDOW (assistant), _("Fwife : Frugalware Installer Front-End"));

  /* Connect signals with functions */
  g_signal_connect (G_OBJECT (assistant), "destroy", G_CALLBACK (cancel_install), NULL);
  g_signal_connect (G_OBJECT (assistant), "cancel", G_CALLBACK (cancel_install), NULL);
  g_signal_connect (G_OBJECT (assistant), "close", G_CALLBACK (close_install), NULL);

  /* Some trick to connect buttons with plugin move functions */
  g_signal_connect(G_OBJECT(((GtkAssistant *) assistant)->forward), "clicked", G_CALLBACK(plugin_next), NULL);
  g_signal_connect(G_OBJECT(((GtkAssistant *) assistant)->back), "clicked", G_CALLBACK(plugin_previous), NULL);

  // Remove useless button and add usefull one
  gtk_assistant_remove_action_widget(GTK_ASSISTANT(assistant), (GtkWidget*)(((GtkAssistant *) assistant)->last));
  GtkWidget *help = gtk_button_new_from_stock(GTK_STOCK_HELP);
  gtk_widget_show(help);
  g_signal_connect (G_OBJECT (help), "clicked", G_CALLBACK (show_help), NULL);
  gtk_assistant_add_action_widget(GTK_ASSISTANT(assistant), help);
  /* Load a nice image */
  GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file ("/usr/share/fwife/images/headlogo.png", &gerror);
  if(!pixbuf) {
       fprintf(stdout, "error message: %s\n", gerror->message);
  }

  /* Set it as icon window */
  gtk_window_set_icon(GTK_WINDOW (assistant),pixbuf);

  /* Make ugly colors ;) */
  gdk_color_parse ("#94b6db", &color);
  gtk_widget_modify_bg (assistant, GTK_STATE_SELECTED, &color);
  /*gdk_color_parse ("black", &color);
  gtk_widget_modify_fg (assistant, GTK_STATE_SELECTED, &color);*/


  /* Load plugins.... */
  fwife_load_plugins(PLUGINDIR);

  plugin_list = g_list_sort(plugin_list, sort_plugins);

  /*Allocate memory for stocking pages */
  MALLOC(pages, sizeof(PageInfo)*g_list_length(plugin_list));

  //* Initializing pages... *//
  for(i=0; i<g_list_length(plugin_list); i++)
  {
      if ((plugin = g_list_nth_data(plugin_list, i)) == NULL)
        LOG("Error when loading plugins...\n");
      else
        LOG("Loading plugin : %s\n", plugin->name);

      pages[i] = (PageInfo) {NULL, -1, GET_UTF8(plugin->desc()), plugin->type, plugin->complete};

      if ((pages[i].widget = plugin->load_gtk_widget()) == NULL)
       { LOG("Error when loading plugin's widget @ %s\n", plugin->name); }

      pages[i].index = gtk_assistant_append_page (GTK_ASSISTANT (assistant), pages[i].widget);
      gtk_assistant_set_page_title (GTK_ASSISTANT (assistant), pages[i].widget, pages[i].title);
      gtk_assistant_set_page_type (GTK_ASSISTANT (assistant), pages[i].widget, pages[i].type);
      gtk_assistant_set_page_complete (GTK_ASSISTANT (assistant), pages[i].widget, pages[i].complete);
      gtk_assistant_set_page_header_image(GTK_ASSISTANT (assistant), pages[i].widget,pixbuf);
  }

  //* Begin with first plugin *//
  plugin_active = g_list_nth_data(plugin_list, 0);

  gtk_widget_show_all (assistant);
  // begin event loop
  gtk_main ();
  return 0;
}

//* Others functions *//

//* Just an error *//
void fwife_error(char* error_str)
{
	GtkWidget *error_dlg = NULL;

	if (!strlen(error_str))
    	return;
	error_dlg = gtk_message_dialog_new (GTK_WINDOW(assistant),
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_MESSAGE_ERROR,
                                         GTK_BUTTONS_OK,
                                         "%s",
                                         error_str);

   	 gtk_window_set_resizable (GTK_WINDOW(error_dlg), FALSE);
    	 gtk_window_set_title (GTK_WINDOW(error_dlg), _("Fwife error"));

   	 gtk_dialog_run (GTK_DIALOG(error_dlg));

    	gtk_widget_destroy (error_dlg); 

    	return;
}

void fwife_info(char *info_str)
{
	GtkWidget *info_dlg = NULL;

	if (!strlen(info_str))
    		return;
	info_dlg = gtk_message_dialog_new (GTK_WINDOW(assistant),
        					GTK_DIALOG_DESTROY_WITH_PARENT,
       						GTK_MESSAGE_INFO,
        					GTK_BUTTONS_OK,
        					"%s",
        					info_str);

   	 gtk_window_set_resizable (GTK_WINDOW(info_dlg), FALSE);
    	 gtk_window_set_title (GTK_WINDOW(info_dlg), _("Fwife information"));

   	 gtk_dialog_run (GTK_DIALOG(info_dlg));

    	gtk_widget_destroy (info_dlg); 

    	return;
}


//* Fatal error : quit fwife *//
void fwife_fatal_error(char* error_str)
{
	GtkWidget *error_dlg = NULL;
	if (!strlen(error_str))
	    return;
	error_dlg = gtk_message_dialog_new (GTK_WINDOW(assistant),
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_MESSAGE_ERROR,
                                         GTK_BUTTONS_OK,
                                         "%s",
                                         error_str);

    	gtk_window_set_resizable (GTK_WINDOW(error_dlg), FALSE);
    	gtk_window_set_title (GTK_WINDOW(error_dlg), _("Fwife error"));

	gtk_dialog_run (GTK_DIALOG(error_dlg));

   	 gtk_widget_destroy (error_dlg);
    
   	 fwife_exit();

    	return;
}

//* A basic question *//
int fwife_question(char* msg_str)
{
	GtkWidget *pQuestion;

	pQuestion = gtk_message_dialog_new (GTK_WINDOW(assistant),
					GTK_DIALOG_MODAL,
       					GTK_MESSAGE_QUESTION,
       					GTK_BUTTONS_YES_NO,
       					msg_str);
	int rep = gtk_dialog_run(GTK_DIALOG(pQuestion));
	gtk_widget_destroy(pQuestion);
	return(rep);
}


//* A simple entry *//
char* fwife_entry(char* title, char* msg_str, char* defaul)
{
	char *str;
	GtkWidget *pBoite = gtk_dialog_new_with_buttons(title,
        				GTK_WINDOW(assistant),
        				GTK_DIALOG_MODAL,
        				GTK_STOCK_OK,GTK_RESPONSE_OK,
        				NULL);

	GtkWidget *labelinfo = gtk_label_new(msg_str);
	GtkWidget *pEntry = gtk_entry_new();
	if(defaul)
		gtk_entry_set_text(GTK_ENTRY(pEntry), defaul);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), labelinfo, FALSE, FALSE, 5);
   	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), pEntry, TRUE, FALSE, 5);

    	/* Affichage des elements de la boite de dialogue */
   	 gtk_widget_show_all(GTK_DIALOG(pBoite)->vbox);

    	/* On lance la boite de dialogue et on recupere la reponse */
    	switch (gtk_dialog_run(GTK_DIALOG(pBoite)))
    	{
        /* L utilisateur valide */
        case GTK_RESPONSE_OK:
            str = strdup((char*)gtk_entry_get_text(GTK_ENTRY(pEntry)));
            break;
        /* L utilisateur annule */
        case GTK_RESPONSE_CANCEL:
        case GTK_RESPONSE_NONE:
        default:
            str = NULL;
            break;
    	}

    	/* Destruction de la boite de dialogue */
    	gtk_widget_destroy(pBoite);
	return str;
}

//* Set current page complete *//
void set_page_completed()
{
    gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant),  gtk_assistant_get_nth_page(GTK_ASSISTANT(assistant),gtk_assistant_get_current_page(GTK_ASSISTANT (assistant))), TRUE);
}

//* Set current page incomplete *//
void set_page_incompleted()
{
    gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant),  gtk_assistant_get_nth_page(GTK_ASSISTANT(assistant),gtk_assistant_get_current_page(GTK_ASSISTANT (assistant))), FALSE);
}
