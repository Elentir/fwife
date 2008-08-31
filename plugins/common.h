#include <gtk/gtk.h>
#include <libintl.h>
#include <glib.h>

#include "../setup.h"

plugin_t *info();
int prerun(GList **config);
int run(GList **config);
GtkWidget* load_gtk_widget(GtkWidget *assistant);
char *desc();
