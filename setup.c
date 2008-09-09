#include <stdlib.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <string.h>
#include "setup.h"

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
int setup_add_plugins(char *filename)
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
int setup_load_plugins(char *dirname)
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
	while ((ent = readdir(dir)) != NULL)
	{
		filename = g_strdup_printf("%s/%s", dirname, ent->d_name);
		if (!stat(filename, &statbuf) && S_ISREG(statbuf.st_mode) &&
				(ext = strrchr(ent->d_name, '.')) != NULL)
			if (!strcmp(ext, SHARED_LIB_EXT))
				setup_add_plugins(filename);
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

//* Dialog box for quit the installation *//
void cancel_install(GtkWidget *w, gpointer user_data)
{
   int result = fwife_question("Do you want to cancel frugalware installation?\n");
   if(result == GTK_RESPONSE_YES)
   {
        FREE(pages);
        cleanup_plugins();
        gtk_main_quit();
   }   
    return;
}

//* Dialog Box when instllation finished *//
void close_install(GtkWidget *w, gpointer user_data)
{
   GtkWidget *dialog, *label;

   dialog = gtk_dialog_new_with_buttons(_("Installation completed"),
                                        NULL,
                                        GTK_DIALOG_MODAL,
                                        GTK_STOCK_OK,
                                        GTK_RESPONSE_ACCEPT,
                                        NULL);
   label = gtk_label_new (_("Frugalware installation was sucessfully completed!\n\nYou can reboot your computer..."));

   gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), label);
   gtk_widget_show_all (dialog);
   gtk_dialog_run (GTK_DIALOG (dialog));

   FREE(pages);
   cleanup_plugins();
   gtk_main_quit();
   //fw_system_interactive("/sbin/reboot");
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
	return 0;
}

int main (int argc, char *argv[])
{
  int i;
  GError *gerror = NULL;
  GdkColor color;
  plugin_t *plugin;
  
  gtk_init (&argc, &argv);

  /* Create a new assistant widget with no pages. */
  assistant = gtk_assistant_new ();
  gtk_widget_set_size_request (assistant, 800, 600);
  gtk_window_set_title (GTK_WINDOW (assistant), _("Fwife : Frugalware Installer Front-End"));

  /* Connect signals with functions */
  g_signal_connect (G_OBJECT (assistant), "destroy", G_CALLBACK (cancel_install), NULL);
  g_signal_connect (G_OBJECT (assistant), "cancel", G_CALLBACK (cancel_install), NULL);
  g_signal_connect (G_OBJECT (assistant), "close", G_CALLBACK (close_install), NULL);

  /* Some trick to connect buttons with plugin move functions */
  g_signal_connect(G_OBJECT(((GtkAssistant *) assistant)->forward), "clicked", G_CALLBACK(plugin_next), NULL);
  g_signal_connect(G_OBJECT(((GtkAssistant *) assistant)->back), "clicked", G_CALLBACK(plugin_previous), NULL);

  /* Load a nice image */
  GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file ("images/headlogo.png", &gerror);
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
  setup_load_plugins(PLUGINDIR);

  plugin_list = g_list_sort(plugin_list, sort_plugins);

  /*Allocate memory for stocking pages */
  MALLOC(pages, sizeof(PageInfo)*g_list_length(plugin_list));

  // TODO : Change language before loading gtk_plugins 

  //* Initializing pages... *//
  for(i=0; i<g_list_length(plugin_list); i++)
  {
      if ((plugin = g_list_nth_data(plugin_list, i)) == NULL)
        LOG("Error when loading plugins...\n");
      else
        LOG("Loading plugin : %s\n", plugin->name);

      pages[i] = (PageInfo) {NULL, -1, GET_UTF8(plugin->desc()), plugin->type, plugin->complete};

      if ((pages[i].widget = plugin->load_gtk_widget(assistant)) == NULL)
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

    gtk_dialog_run (GTK_DIALOG(error_dlg));

    gtk_widget_destroy (error_dlg); 

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

    gtk_dialog_run (GTK_DIALOG(error_dlg));

    gtk_widget_destroy (error_dlg);
    
    //
    FREE(pages);
    cleanup_plugins();
    gtk_main_quit();

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
