#include <stdio.h>
#include <gtk/gtk.h>

#include "common.h"

GtkWidget *assistant = NULL;

plugin_t plugin =
{
	"Completed",	
	desc,
	100,
	load_gtk_widget,
	GTK_ASSISTANT_PAGE_CONFIRM,
	TRUE,
	NULL,
	run,
	NULL // dlopen handle
};

char *desc()
{
	return _("Installation completed");
}

plugin_t *info()
{
	return &plugin;
}

GtkWidget *load_gtk_widget(GtkWidget *assist)
{
	assistant = assist;
	GtkWidget *widget = gtk_label_new (_("Installation completed! We hope that you enjoy Frugalware!"));
	
	return widget;
}

int run(GList **config)
{
    return 0;
}
 
