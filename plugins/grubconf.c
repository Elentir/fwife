/*
 *  grubconf.c for Fwife
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

#include <stdio.h>
#include <glib.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libintl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <libfwgrubconfig.h>
#include <libfwutil.h>

#include "common.h"

static GtkWidget *pRadio1 = NULL;

plugin_t plugin =
{
	"grubconfig",
	desc,
	55,
	load_gtk_widget,
	GTK_ASSISTANT_PAGE_CONTENT,
	TRUE,
	load_help_widget,
	NULL,
	run,
	NULL // dlopen handle
};

char *desc()
{
	return _("Configuring GRUB");
}

plugin_t *info()
{
	return &plugin;
}

GtkWidget *load_gtk_widget()
{
	/* Creation de la zone de saisie */
	GtkWidget *pVBox = gtk_vbox_new(FALSE, 5);
		
	GtkWidget *pLabelInfo=gtk_label_new(NULL);

	/* On utilise les balises */
	gtk_label_set_markup(GTK_LABEL(pLabelInfo), _("<span face=\"Courier New\"><b>Installing GRUB bootloader</b></span>"));
	
	gtk_box_pack_start(GTK_BOX(pVBox), pLabelInfo, FALSE, FALSE, 6);

	GtkWidget *pLabel = gtk_label_new(_("Choose install type :"));
	gtk_box_pack_start(GTK_BOX(pVBox), pLabel, FALSE, FALSE, 5);

	/* Creation du premier bouton radio */
	pRadio1 = gtk_radio_button_new_with_label(NULL, _("MBR  -  Install to Master Boot Record"));
	gtk_box_pack_start(GTK_BOX (pVBox), pRadio1, FALSE, FALSE, 0);
	/* Ajout du deuxieme */
	GtkWidget *pRadio2 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON (pRadio1), _("Floppy  -  Install to a formatted floppy in /dev/fd0"));
	gtk_box_pack_start(GTK_BOX (pVBox), pRadio2, FALSE, FALSE, 0);
	/* Ajout du troisieme */
	GtkWidget *pRadio3 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON (pRadio1), _("Root  -  Install to superblock (do NOT use with XFS)"));
	gtk_box_pack_start(GTK_BOX (pVBox), pRadio3, FALSE, FALSE, 0);
	
	GtkWidget *pRadio4 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON (pRadio1), _("Skip  -  Skip the installation of GRUB."));
	gtk_box_pack_start(GTK_BOX (pVBox), pRadio4, FALSE, FALSE, 0);	

	return pVBox;
}

int run(GList **config)
{
	GSList *pList;
	const char *sLabel = "";
	int mode, needrelease;
	int ret;
	FILE *fp;
	struct stat buf;	
	
	/* get button list */
    	pList = gtk_radio_button_get_group(GTK_RADIO_BUTTON(pRadio1));
    	
	while(pList)
    	{
      		/* selected? */
      		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pList->data)))
       		{
           		/* yes -> get label */
           		sLabel = gtk_button_get_label(GTK_BUTTON(pList->data));
           		// get out
           		pList = NULL;
        	}
        	else
        	{
           		/* no -> next button */
           		pList = g_slist_next(pList);
        	}
    	}
	if(!strcmp(sLabel, _("MBR  -  Install to Master Boot Record")))
		mode = 0;
	else if(!strcmp(sLabel, _("Floppy  -  Install to a formatted floppy in /dev/fd0")))
		mode = 1;
	else if(!strcmp(sLabel, _("Root  -  Install to superblock (do NOT use with XFS)")))
		mode = 2;
	else
		mode = 3;

	pid_t pid = fork();

	if(pid == -1)
		LOG("Error when fork process in grubconf plugin.");
	else if(pid == 0)
	{
		chroot(TARGETDIR);
	
		//* Install grub and menu *//
		if(mode!=3)
		{
			needrelease = fwutil_init();
			fwgrub_install(mode);
			// backup the old config if there is any
			if(!stat("/boot/grub/menu.lst", &buf))
				rename("/boot/grub/menu.lst", "/boot/grub/menu.lst.old");
			fp = fopen("/boot/grub/menu.lst", "w");
			if(fp)
			{
				fwgrub_create_menu(fp);
				fclose(fp);
			}
			if(needrelease)
				fwutil_release();
		}
		exit(0);
	}
	else
	{
		wait(&ret);
	}
	
	return 0;
}

GtkWidget *load_help_widget()
{
	GtkWidget* help = gtk_label_new(_("GRUB can be installed to a variety of places:\n\n\t1. The Master Boot Record of your first hard drive.\n\t2. A formatted floppy disk.\n\t3. The superblock of your root Linux partition.\n\nOption 3 requires setting the partition bootable with (c)fdisk\nHint: Choose option 3 if you already have a boot manager installed.\n"));
	
	return help;
}

 
