#include <stdio.h>
#include <gtk/gtk.h>
#include<string.h>

#include "common.h"
#include "../util.h"

plugin_t plugin =
{
	"greet",
	desc,
	10,
	load_gtk_widget,
	GTK_ASSISTANT_PAGE_CONTENT,
	TRUE,
	NULL,
	run,
	NULL // dlopen handle
};

char *desc()
{
	return _("Welcome");
}

plugin_t *info()
{
	return &plugin;
}

GtkWidget *load_gtk_widget(GtkWidget *assist)
{
	GtkWidget *widget = gtk_label_new (NULL);
	gtk_label_set_markup(GTK_LABEL(widget), _("<span face=\"Times New Roman 12\"><b>Welcome among the users of Frugalware!\n\n</b></span>\n"
	"The aim of creating Frugalware was to help you to do your work faster and simpler.\n"
	" We hope that you will like our product.\n\n"
        "<span font_desc=\"Times New Roman italic 12\" foreground=\"#0000FF\">The Frugalware Developer Team</span>\n"));
	
	return widget;
}

int run(GList **config)
{
	data_put(config, "netinstall", "");
	return 0;
}
