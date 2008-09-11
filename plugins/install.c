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
	NULL,
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

GtkWidget *load_gtk_widget()
{
	GtkWidget *vbox;
	vbox = gtk_vbox_new (FALSE, 5);
 	
	labelpkg = gtk_label_new("");
	progress = gtk_progress_bar_new();	
	gtk_box_pack_start (GTK_BOX (vbox), progress, TRUE, FALSE, 5);
	gtk_box_pack_start (GTK_BOX (vbox), labelpkg, FALSE, FALSE, 5);
	return vbox;
}

void progress_install (unsigned char event, char *pkgname, int percent, int howmany, int remain)
{
	char *main_text = NULL;	

	if (!pkgname)
		return;
	if (percent < 0 || percent > 100)
		return;
	while (gtk_events_pending())
		gtk_main_iteration ();

	switch (event)
	{
		case PM_TRANS_PROGRESS_ADD_START:
			main_text = g_strdup (_("Installing packages..."));
			break;
		case PM_TRANS_PROGRESS_UPGRADE_START:
			main_text = g_strdup (_("Upgrading packages..."));
			break;
		case PM_TRANS_PROGRESS_REMOVE_START:
			main_text = g_strdup (_("Removing packages..."));
			break;
		case PM_TRANS_PROGRESS_CONFLICTS_START:
				main_text = g_strdup (_("Checking packages for file conflicts..."));			
			break;
		default:
			return;
	}
	gtk_progress_bar_set_text (GTK_PROGRESS_BAR(progress), main_text);	
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress), (float)percent/100);
	FREE(main_text);

	while (gtk_events_pending())
		gtk_main_iteration ();

	return;
}

void progress_event (unsigned char event, void *data1, void *data2)
{
	char *substr = NULL;
	
	if (data1 == NULL)
		return;
	
	switch (event)
	{
		case PM_TRANS_EVT_CHECKDEPS_START:
			substr = g_strdup(_("Checking dependencies"));			
			break;
		case PM_TRANS_EVT_FILECONFLICTS_START:
			substr = g_strdup (_("Checking for file conflicts"));			
			break;
		case PM_TRANS_EVT_RESOLVEDEPS_START:
			substr = g_strdup (_("Resolving dependencies"));
			break;
		case PM_TRANS_EVT_INTERCONFLICTS_START:
			substr = g_strdup (_("Looking for inter-conflicts"));			
			break;
		case PM_TRANS_EVT_CHECKDEPS_DONE:
		case PM_TRANS_EVT_RESOLVEDEPS_DONE:
		case PM_TRANS_EVT_INTERCONFLICTS_DONE:
			substr = g_strdup (_("Done"));
			break;
		case PM_TRANS_EVT_ADD_START:
			substr = g_strdup_printf (_("Installing %s-%s"),
					(char*)pacman_pkg_getinfo(data1, PM_PKG_NAME),
					(char*)pacman_pkg_getinfo(data1, PM_PKG_VERSION));			
			break;
		case PM_TRANS_EVT_ADD_DONE:
			substr = g_strdup_printf (_("Installed %s-%s"),
					(char*)pacman_pkg_getinfo(data1, PM_PKG_NAME),
					(char*)pacman_pkg_getinfo(data1, PM_PKG_VERSION));
			break;
		case PM_TRANS_EVT_UPGRADE_START:
			substr = g_strdup_printf (_("Upgrading %s"),
					(char*)pacman_pkg_getinfo(data1, PM_PKG_NAME));			
			break;
		case PM_TRANS_EVT_UPGRADE_DONE:
			substr = g_strdup_printf (_("upgraded %s from %s to %s"),
					(char*)pacman_pkg_getinfo(data1, PM_PKG_NAME),
					(char*)pacman_pkg_getinfo(data2, PM_PKG_VERSION),
					(char*)pacman_pkg_getinfo(data1, PM_PKG_VERSION));
			break;
		case PM_TRANS_EVT_INTEGRITY_START:
			substr = g_strdup (_("Checking package integrity"));
			break;
		case PM_TRANS_EVT_INTEGRITY_DONE:
			substr = g_strdup (_("Done"));
			break;
		case PM_TRANS_EVT_SCRIPTLET_INFO:
			substr = g_strdup ((char*)data1);
			break;
		case PM_TRANS_EVT_SCRIPTLET_START:
			substr = g_strdup ((char*)data1);
			break;
		case PM_TRANS_EVT_SCRIPTLET_DONE:
			substr = g_strdup (_("Done"));
			break;
		case PM_TRANS_EVT_RETRIEVE_START:
			substr = g_strdup_printf (_("Retrieving packages from %s"), (char*)data1);
			break;
		default:
			return;
	}
	gtk_label_set_label(GTK_LABEL(labelpkg), substr); 
	FREE(substr);
	

	return;
}

int installpkgs(GList *pkgs)
{
	int i = 0;
	PM_LIST *pdata = NULL, *pkgsl;	
	char *ptr;
	
	if(pacman_initialize(TARGETDIR) == -1) {
		printf("failed to initialize pacman library (%s)\n", pacman_strerror(pm_errno));
	}

	if (pacman_parse_config("/etc/pacman-g2.conf", NULL, "") == -1) {
			printf("Failed to parse pacman-g2 configuration file (%s)", pacman_strerror(pm_errno));
			return(-1);
	}
	pacman_set_option(PM_OPT_LOGMASK, -1);
	pacman_set_option(PM_OPT_LOGCB, (long)cb_log);
	/*pacman_set_option (PM_OPT_DLCB, (long)progress_update);
	pacman_set_option (PM_OPT_DLOFFSET, (long)&offset);
	pacman_set_option (PM_OPT_DLRATE, (long)&rate);
	pacman_set_option (PM_OPT_DLFNM, (long)reponame);*/
	
	PM_DB *db_local = pacman_db_register("local");
	if(db_local == NULL) {
		printf("could not register 'local' database (%s)\n", pacman_strerror(pm_errno));
	}
	
	if(pacman_trans_init(PM_TRANS_TYPE_SYNC, PM_TRANS_FLAG_FORCE|PM_TRANS_FLAG_NODEPS, progress_event, NULL, progress_install) == -1)
		printf("Failed to initialize transaction %s\n", pacman_strerror (pm_errno));	
	
	for (i = 0; i<g_list_length(pkgs); i++)
	{
		ptr = strdup((char*)g_list_nth_data(pkgs, i));
		if(pacman_trans_addtarget(strdup(drop_version(ptr))) == -1)
			printf("Error adding packet %s\n", pacman_strerror (pm_errno));
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
		printf("Error getting transaction info %s\n", pacman_strerror (pm_errno));
		return -1;
	}

	/* commit transaction */
	if (pacman_trans_commit(&pdata) == -1)
	{
		printf("Failed to commit transaction (%s)\n", pacman_strerror (pm_errno));					
	}
	
	/* release the transaction */
	pacman_trans_release ();
	pacman_release();
		
	return 0;	
}

int installpkgsnormal(GList *pkgs)
{
	float percent = 0.0;
	int i;
	char *cmd, *ptr, *main_text;

	while (gtk_events_pending())
			gtk_main_iteration ();
	
	for(i=0; i<g_list_length(pkgs); i++)
	{
		percent += 1.0/g_list_length(pkgs);
		ptr = drop_version((char*) g_list_nth_data(pkgs, i));
		cmd = g_strdup_printf("pacman-g2 -S -r ./ --noconfirm %s", ptr);
		fw_system(cmd);
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress), percent);
		main_text = g_strdup_printf("Downloading and installing %s...", ptr);
		gtk_progress_bar_set_text (GTK_PROGRESS_BAR(progress), main_text);
		
		while (gtk_events_pending())
			gtk_main_iteration ();

		FREE(main_text);
		FREE(cmd);		
	}
		
	return 0;	
}


int prerun(GList **config)
{
	char *ptr;
	//mount sytem point to chdir
	ptr = g_strdup_printf("umount %s/proc", TARGETDIR);
	fw_system(ptr);
	FREE(ptr);
	ptr = g_strdup_printf("umount %s/dev", TARGETDIR);
	fw_system(ptr);
	FREE(ptr);
	ptr = g_strdup_printf("umount %s/sys", TARGETDIR);
	fw_system(ptr);
	FREE(ptr);

	copyfile("/proc/mounts", "/etc/mtab");

	installpkgs((GList*)data_get(*config, "packages"));
	
	set_page_completed();
	return(0);
}

int run(GList **config)
{
	char *ptr;
	//mount sytem point to chdir
	ptr = g_strdup_printf("mount /dev -o bind %s/dev", TARGETDIR);
	fw_system(ptr);
	FREE(ptr);
	ptr = g_strdup_printf("mount /proc -o bind %s/proc", TARGETDIR);
	fw_system(ptr);
	FREE(ptr);
	ptr = g_strdup_printf("mount /sys -o bind %s/sys", TARGETDIR);
	fw_system(ptr);
	FREE(ptr);
	//Begin configuration, chroot at targetdir
	chroot(TARGETDIR);
	return 0;
}
 
