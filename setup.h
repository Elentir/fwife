#ifndef SETUP_H_INCLUDED
#define SETUP_H_INCLUDED

#include <glib.h>
#include <gtk/gtk.h>
#include <libintl.h>
#include "util.h"

#define LOGDEV "/dev/tty4"
#define LOGFILE "/var/log/setup.log"
#define SOURCEDIR "/mnt/source"
#define TARGETDIR "/mnt/target"

#define MKSWAP "/sbin/mkswap"
#define SWAPON "/sbin/swapon"

#define PLUGINDIR "plugins"
#define PACCONFPATH "/etc/pacman-g2/repos/"

#define PACCONF "frugalware-current"

#define ARCH "i686"

#define EXGRPSUFFIX "-extra"

#define SHARED_LIB_EXT ".so"

#define GET_UTF8(chaine) g_locale_to_utf8(chaine, -1, NULL, NULL, NULL)

//* Type definition for a plugins *//

typedef struct
{
	char *name;
	char* (*desc)();
	int priority;
	GtkWidget* (*load_gtk_widget)(GtkWidget *assistant);
	GtkAssistantPageType type;
    	gboolean complete;
	int (*prerun)(GList **config);
	int (*run)(GList **config);
	void *handle;
} plugin_t;


//* Structure pour le type de page *//
typedef struct {
  GtkWidget *widget;
  gint index;
  const gchar *title;
  GtkAssistantPageType type;
  gboolean complete;
} PageInfo;

//* Some usefull function *//
void set_page_completed();
void set_page_incompleted();

//* Some functions for basic messages *//
void fwife_fatal_error(char *msg);
void fwife_error(char *msg);
int fwife_question(char *msg);

#endif // SETUP_H_INCLUDED
