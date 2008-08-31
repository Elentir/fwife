#include <stdio.h>
#include <dialog.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pacman.h>

#include "common.h"
#include "../util.h"

GtkWidget *progress;
GtkWidget *labelpkg;

plugin_t plugin =
{
	"install",
	desc,
	50,
	load_gtk_widget,
	GTK_ASSISTANT_PAGE_PROGRESS,
	FALSE,
	prerun,
	run,
	NULL // dlopen handle
};

char *desc()
{
	return _("Installing the selected packages");
}

plugin_t *info()
{
	return &plugin;
}

GtkWidget *load_gtk_widget(GtkWidget *assist)
{
	GtkWidget *vbox;
	vbox = gtk_vbox_new (FALSE, 5);
 	
	labelpkg = gtk_label_new("");
	progress = gtk_progress_bar_new();	
	gtk_box_pack_start (GTK_BOX (vbox), progress, TRUE, FALSE, 5);
	gtk_box_pack_start (GTK_BOX (vbox), labelpkg, FALSE, FALSE, 5);
	return vbox;
}

int installpkgs(GList *pkgs, GList **config)
{
	int i = 0;
	PM_LIST *pdata = NULL, *pkgsl;	
	unsigned int flags = 0;
	char *ptr;

	//flags = PM_TRANS_FLAG_DOWNLOADONLY;
	
	if(pacman_trans_init(PM_TRANS_TYPE_SYNC, flags, NULL, NULL, NULL) == -1)
		printf("Failed to initialize transaction\n");	
	
	for (i = 0; i<g_list_length(pkgs); i++)
	{
		ptr = strdup((char*)g_list_nth_data(pkgs, i));
		if(pacman_trans_addtarget(drop_version(ptr)))
			return(-1);
		FREE(ptr);
	}

	//* prepare transaction *//
	if(pacman_trans_prepare(&pdata) == -1)
	{
		printf("Failed to prepare transaction (%s)\n", pacman_strerror (pm_errno));
		pacman_list_free(pdata);
		return -1;
	}	
	
	pkgsl = pacman_trans_getinfo (PM_TRANS_PACKAGES);
	if (pkgsl == NULL)
	{ 
		printf("Error getting transaction info");
		return -1;
	}

	/* commit transaction */
	if (pacman_trans_commit(NULL) == -1)
	{
		printf("Failed to commit transaction (%s)\n", pacman_strerror (pm_errno));					
		return -1;
	}
	
	/* release the transaction */
	pacman_trans_release ();
	pacman_release();	
	return 0;	
}
int prerun(GList **config)
{
	copyfile("/proc/mounts", "/etc/mtab");

	installpkgs((GList*)data_get(*config, "packages"), config);
	
	set_page_completed();
	return(0);
}

int run(GList **config)
{
	return 0;
}
 
