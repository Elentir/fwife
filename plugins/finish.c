/*
 *  finish.c for Fwife
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
#include <gtk/gtk.h>
#include <string.h>
#include <glib.h>
#include <libfwutil.h>
#include <libfwxconfig.h>
#include <libfwmouseconfig.h>
#include <pacman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <libintl.h>
#include <dialog.h>

#include "common.h"

plugin_t plugin =
{
	"Completed",	
	desc,
	100,
	load_gtk_widget,
	GTK_ASSISTANT_PAGE_CONFIRM,
	TRUE,
	NULL,
	prerun,
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
	GtkWidget *widget = gtk_label_new (_("Installation completed! We hope that you enjoy Frugalware!"));
	
	return widget;
}

GtkWidget *getMouseCombo()
{
	GtkWidget *combo;
	GtkTreeIter iter;
	GtkListStore *store;
	gint i;

	char *modes[] = 
	{
		"ps2", _("PS/2 port mouse (most desktops and laptops)"),
		"imps2", _("Microsoft PS/2 Intellimouse"),
		"bare", _("2 button Microsoft compatible serial mouse"),
		"ms", _("3 button Microsoft compatible serial mouse"),
		"mman", _("Logitech serial MouseMan and similar devices"),
		"msc", _("MouseSystems serial (most 3 button serial mice)"),
		"pnp", _("Plug and Play (serial mice that do not work with ms)"),
		"usb", _("USB connected mouse"),
		"ms3", _("Microsoft serial Intellimouse"),
		"netmouse", _("Genius Netmouse on PS/2 port"),
		"logi", _("Some serial Logitech devices"),
		"logim", _("Make serial Logitech behave like msc"),
		"atibm", _("ATI XL busmouse (mouse card)"),
		"inportbm", _("Microsoft busmouse (mouse card)"),
		"logibm", _("Logitech busmouse (mouse card)"),
		"ncr", _("A pointing pen (NCR3125) on some laptops"),
		"twid", _("Twiddler keyboard, by HandyKey Corp"),
		"genitizer", _("Genitizer tablet (relative mode)"),
		"js", _("Use a joystick as a mouse"),
		"wacom", _("Wacom serial graphics tablet")
	};
	store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
	combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));
	g_object_unref (GTK_TREE_MODEL (store));
	gtk_widget_set_size_request(combo, 350, 40);
	    
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer,"text", 0, NULL);
    
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer,"text", 1, NULL);
	

	for (i = 0; i < 40; i+=2)
	{
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, modes[i], 1, modes[i+1], -1);
	}
	
	return combo;
}

GtkWidget *getPortCombo()
{
	GtkWidget *combo;
	GtkTreeIter iter;
	GtkListStore *store;
	gint i;

	char *ports[] = 
	{
		"/dev/ttyS0", _("(COM1: under DOS)"),
		"/dev/ttyS1", _("(COM2: under DOS)"),
		"/dev/ttyS2", _("(COM3: under DOS)"),
		"/dev/ttyS3", _("(COM4: under DOS)")
	};

	store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
	combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));
	g_object_unref (GTK_TREE_MODEL (store));
	gtk_widget_set_size_request(combo, 300, 40);
	    
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer,"text", 0, NULL);
    
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer,"text", 1, NULL);

	for (i = 0; i < 8; i+=2)
	{
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, ports[i], 1, ports[i+1], -1);
	}
	
	return combo;
}

void change_mouse(GtkComboBox *combo, gpointer data)
{
	char* mouse_type;

	GtkTreeIter iter;
	GtkTreeModel *model;
	gtk_combo_box_get_active_iter(combo, &iter);
	model = gtk_combo_box_get_model(combo);
	gtk_tree_model_get (model, &iter, 0, &mouse_type, -1);

	if(!strcmp(mouse_type, "bar") || !strcmp(mouse_type, "ms") || !strcmp(mouse_type, "mman") ||
		!strcmp(mouse_type, "msc") || !strcmp(mouse_type, "genitizer") || !strcmp(mouse_type, "pnp") ||
		!strcmp(mouse_type, "ms3") || !strcmp(mouse_type, "logi") || !strcmp(mouse_type, "logim") ||
		!strcmp(mouse_type, "wacom") || !strcmp(mouse_type, "twid"))
	{
		g_object_set (GTK_WIDGET(data), "sensitive", TRUE, NULL);
	}
	else
	{
		g_object_set (GTK_WIDGET(data), "sensitive", FALSE, NULL);
	}
}

void mouseconfig()
{
	char *mouse_type=NULL, *mtype=NULL, *link=NULL;
	int ret;
	extern GtkWidget *assistant;
	GtkTreeIter iter;
	GtkTreeModel *model;

	if(fwmouse_detect_usb())
	{
		mtype=strdup("imps2");
		link=strdup("input/mice");
	}
	else
	{
		
	GtkWidget *pBoite = gtk_dialog_new_with_buttons(_("Mouse configuration"),
        				GTK_WINDOW(assistant),
        				GTK_DIALOG_MODAL,
        				GTK_STOCK_OK,GTK_RESPONSE_OK,
        				NULL);
	
	GtkWidget *label = gtk_label_new(_("Selecting your mouse : "));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), label, FALSE, FALSE, 5);
	GtkWidget *combomouse = getMouseCombo();
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), combomouse, TRUE, FALSE, 10);
	GtkWidget *label2 = gtk_label_new(_("Selecting mouse's port :"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), label2, FALSE, FALSE, 5);
	GtkWidget *comboport = getPortCombo();
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), comboport, TRUE, FALSE, 10);
	g_signal_connect(G_OBJECT(combomouse), "changed", G_CALLBACK(change_mouse), comboport);
	gtk_combo_box_set_active (GTK_COMBO_BOX (combomouse), 0);
	gtk_combo_box_set_active (GTK_COMBO_BOX (comboport), 0);

    	/* Affichage des elements de la boite de dialogue */
    	gtk_widget_show_all(GTK_DIALOG(pBoite)->vbox);

   	/* On lance la boite de dialogue et on recupere la reponse */
    		switch (gtk_dialog_run(GTK_DIALOG(pBoite)))
    		{

        	case GTK_RESPONSE_OK:
		
		gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combomouse), &iter);
		model = gtk_combo_box_get_model(GTK_COMBO_BOX(combomouse));
		gtk_tree_model_get (model, &iter, 0, &mouse_type, -1);
		if(!strcmp(mouse_type, "bar") || !strcmp(mouse_type, "ms") || !strcmp(mouse_type, "mman") ||
		!strcmp(mouse_type, "msc") || !strcmp(mouse_type, "genitizer") || !strcmp(mouse_type, "pnp") ||
		!strcmp(mouse_type, "ms3") || !strcmp(mouse_type, "logi") || !strcmp(mouse_type, "logim") ||
		!strcmp(mouse_type, "wacom") || !strcmp(mouse_type, "twid"))
		{
			gtk_combo_box_get_active_iter(GTK_COMBO_BOX(comboport), &iter);
			model = gtk_combo_box_get_model(GTK_COMBO_BOX(comboport));
			gtk_tree_model_get (model, &iter, 0, &link, -1);
			mtype=mouse_type;
			mouse_type=NULL;
		}
		else if(!strcmp(mouse_type, "ps2"))
		{
			link = strdup("psaux");
			mtype = strdup("ps2");
		}
		else if(!strcmp(mouse_type, "ncr"))
		{
			link = strdup("psaux");
			mtype = strdup("ncr");
		}
		else if(!strcmp(mouse_type, "imps2"))
		{
			link = strdup("psaux");
			mtype = strdup("imps2");
		}
		else if(!strcmp(mouse_type, "logibm"))
		{
			link = strdup("logibm");
			mtype = strdup("ps2");
		}
		else if(!strcmp(mouse_type, "atibm"))
		{
			link = strdup("atibm");
			mtype = strdup("ps2");
		}
		else if(!strcmp(mouse_type, "inportbm"))
		{
			link = strdup("inportbm");
			mtype = strdup("bm");
		}
		else if(!strcmp(mouse_type, "js"))
		{
			link = strdup("js0");
			mtype = strdup("js");
		}
		else
		{
			// usb
			link = strdup("input/mice");
			mtype = strdup("imps2");
		}
            	break;

        	case GTK_RESPONSE_CANCEL:
        	case GTK_RESPONSE_NONE:
        	default: 
            		break;
    		}
		gtk_widget_destroy(pBoite);
	}  
	
	pid_t pid = fork();

	if(pid == -1)
		LOG("Error when forking process in mouse config.");
	else if(pid == 0)
	{
		chroot(TARGETDIR);
		
		if(link && mtype)
			fwmouse_writeconfig(link, mtype);
		exit(0);
	}
	else
	{
		wait(&ret);
	}

	FREE(link);
	FREE(mtype);
}

int write_dms(char *dms)
{
	FILE *fd = fopen("/etc/sysconfig/desktop", "w");
	
	if(!fd)
		return -1;
	
	fprintf(fd, "# /etc/sysconfig/desktop\n\n");
	fprintf(fd, "# Which session manager try to use.\n\n");	
	
	if(!strcmp(dms, "KDM"))
	{
		fprintf(fd, "#desktop=\"/usr/bin/xdm -nodaemon\"\n");
		fprintf(fd, "#desktop=\"/usr/bin/slim\"\n");
		fprintf(fd, "#desktop=\"/usr/sbin/gdm --nodaemon\"\n");
		fprintf(fd, "desktop=\"/usr/bin/kdm -nodaemon\"\n");
	}
	else if(!strcmp(dms, "GDM"))
	{
		fprintf(fd, "#desktop=\"/usr/bin/xdm -nodaemon\"\n");
		fprintf(fd, "#desktop=\"/usr/bin/slim\"\n");
		fprintf(fd, "desktop=\"/usr/sbin/gdm --nodaemon\"\n");
		fprintf(fd, "#desktop=\"/usr/bin/kdm -nodaemon\"\n");
	}
	else if(!strcmp(dms, "Slim"))
	{
		fprintf(fd, "#desktop=\"/usr/bin/xdm -nodaemon\"\n");
		fprintf(fd, "desktop=\"/usr/bin/slim\"\n");
		fprintf(fd, "#desktop=\"/usr/sbin/gdm --nodaemon\"\n");
		fprintf(fd, "#desktop=\"/usr/bin/kdm -nodaemon\"\n");
	}
	else // default : XDM
	{
		fprintf(fd, "desktop=\"/usr/bin/xdm -nodaemon\"\n");
		fprintf(fd, "#desktop=\"/usr/bin/slim\"\n");
		fprintf(fd, "#desktop=\"/usr/sbin/gdm --nodaemon\"\n");
		fprintf(fd, "#desktop=\"/usr/bin/kdm -nodaemon\"\n");
	}
	
	return 0;
}
		

void checkdms(GtkListStore *store)
{
	PM_DB *db;
	GtkTreeIter iter;

	if(pacman_initialize(TARGETDIR)==-1)
		return;
	if(!(db = pacman_db_register("local")))
	{
		pacman_release();
		return;
	}

	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, 0, "XDM", 1, _("   X Window Display Manager"), -1);
	if(pacman_db_readpkg(db, "kdebase"))
	{
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, "KDM", 1, _("  KDE Display Manager"), -1);
	}
	if(pacman_db_readpkg(db, "gdm"))
	{
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, "GDM", 1, _("  Gnome Display Manager"), -1);
	}
	if(pacman_db_readpkg(db, "slim"))
	{
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, "Slim", 1, _("  Simple Login Manager"), -1);
	}
	pacman_db_unregister(db);
	pacman_release();
	return;
}

int xconfigbox ()
{
	GtkWidget* pBoite;
  	GtkWidget *pEntryRes, *pEntryDepth;
  	char* sRes, *sDepth, *mdev, *sDms;
	int needrelease, ret;
	
	GtkWidget *pVBox;
    	GtkWidget *pFrame;
    	GtkWidget *pVBoxFrame;
    	GtkWidget *pLabel;

	GtkWidget *combo;
	GtkTreeIter iter;
	GtkListStore *store;
	GtkTreeModel *model;	
	
	extern GtkWidget *assistant;

	pBoite = gtk_dialog_new_with_buttons(_("Configuring X11"),
      		  NULL,
      		  GTK_DIALOG_MODAL,
      		  GTK_STOCK_OK,GTK_RESPONSE_OK,
      		  NULL);
	gtk_window_set_transient_for(GTK_WINDOW(pBoite), GTK_WINDOW(assistant));
	gtk_window_set_position(GTK_WINDOW(pBoite), GTK_WIN_POS_CENTER_ON_PARENT);
   	
	pVBox = gtk_vbox_new(TRUE, 0);
   	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), pVBox, TRUE, FALSE, 5);	

    	/* Creation du premier GtkFrame */
    	pFrame = gtk_frame_new(_("X11 Configuration"));
    	gtk_box_pack_start(GTK_BOX(pVBox), pFrame, TRUE, FALSE, 0);

    	/* Creation et insertion d une boite pour le premier GtkFrame */
    	pVBoxFrame = gtk_vbox_new(TRUE, 0);
    	gtk_container_add(GTK_CONTAINER(pFrame), pVBoxFrame);   

    	/* Creation et insertion des elements contenus dans le premier GtkFrame */
    	pLabel = gtk_label_new(_("Resolution :"));
    	gtk_box_pack_start(GTK_BOX(pVBoxFrame), pLabel, TRUE, FALSE, 0);
	pEntryRes = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(pEntryRes), "1024x768");
    	gtk_box_pack_start(GTK_BOX(pVBoxFrame), pEntryRes, TRUE, FALSE, 0);
    
    	pLabel = gtk_label_new(_("Color depth :"));
    	gtk_box_pack_start(GTK_BOX(pVBoxFrame), pLabel, TRUE, FALSE, 0);
    	pEntryDepth = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(pEntryDepth), "24");
	gtk_box_pack_start(GTK_BOX(pVBoxFrame), pEntryDepth, TRUE, FALSE, 0);

    	/* Creation du deuxieme GtkFrame */
    	pFrame = gtk_frame_new(_("Select your default display manager : "));
    	gtk_box_pack_start(GTK_BOX(pVBox), pFrame, TRUE, FALSE, 0);

    	/* Creation et insertion d une boite pour le deuxieme GtkFrame */
    	pVBoxFrame = gtk_vbox_new(TRUE, 0);
    	gtk_container_add(GTK_CONTAINER(pFrame), pVBoxFrame);

	store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
	combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));
	g_object_unref (GTK_TREE_MODEL (store));	
	
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer,"text", 0, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer,"text", 1, NULL);

	checkdms(store);
	gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);

    	/* Creation et insertion des elements contenus dans le deuxieme GtkFrame */
    	gtk_box_pack_start(GTK_BOX(pVBoxFrame), combo, TRUE, FALSE, 0);

    	gtk_widget_show_all(pBoite);

    	switch (gtk_dialog_run(GTK_DIALOG(pBoite)))
    	{
        	case GTK_RESPONSE_OK:
            		sRes = (char*)gtk_entry_get_text(GTK_ENTRY(pEntryRes));
	    		sDepth = (char*)gtk_entry_get_text(GTK_ENTRY(pEntryDepth));
			
			gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combo), &iter);
			model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
			gtk_tree_model_get (model, &iter, 0, &sDms, -1);

			if((sRes == NULL) || !strcmp(sRes, "") || (sDepth == NULL) || !strcmp(sDepth, ""))
				return -1;

			pid_t pid = fork();

			if(pid == -1)
				LOG("Error when forking process (X11 config)");
			else if (pid == 0)
			{
				chroot(TARGETDIR);
				needrelease = fwutil_init();
				mdev = fwx_get_mousedev();

				if(fwx_doprobe())
				{
					if(needrelease)
						fwutil_release();
					exit(-1);
				}
	
	
				fwx_doconfig(mdev, sRes, sDepth);	
				unlink("/root/xorg.conf.new");
				ret = fwx_dotest();
				write_dms(sDms);
				exit(ret);
			}
			else
			{
				wait(&ret);
				gtk_widget_destroy(pBoite);
				if(ret)
					xconfigbox();
			}
           		break;
        	case GTK_RESPONSE_CANCEL:
        	case GTK_RESPONSE_NONE:
        	default:
			gtk_widget_destroy(pBoite);
        		break;
    	}
	return 0;
}

int xconfig()
{
	char *ptr=NULL;
	struct stat buf;	

	ptr = g_strdup_printf("%s/usr/bin/xinit", TARGETDIR);
	if(stat(ptr, &buf))
	{
		LOG("Xconfig : xinit missing");
		return(-1);
	}
	FREE(ptr);

	ptr = g_strdup_printf("%s/usr/bin/xmessage", TARGETDIR);
	if(stat(ptr, &buf))
	{
		LOG("Xconfig : xmessage missing");
		return(-1);
	}
	FREE(ptr);
	
	ptr = g_strdup_printf("%s/usr/bin/xsetroot", TARGETDIR);
	if(stat(ptr, &buf))
	{
		LOG("Xconfig : xsetroot missing");
		return(-1);
	}
	FREE(ptr);

	if(xconfigbox() == -1)
		return -1;

	return 0;
}

int prerun(GList **config)
{
	struct stat buf;
	char *ptr;

	// configure kernel modules	
	ptr = g_strdup_printf("chroot %s /sbin/depmod -a", TARGETDIR);
	fw_system(ptr);
	FREE(ptr);

	//* Mouse configuration *//
	mouseconfig();

	ptr = g_strdup_printf("%s/usr/bin/X", TARGETDIR);
	if(!stat(ptr, &buf))
	{
		if(xconfig() == -1)
			fwife_error(_("Error when configuring X11."));
	}
	
	return 0;
}

int run(GList **config)
{
    	char *ptr;
	// unmout system directories
	ptr = g_strdup_printf("umount %s/sys", TARGETDIR);
	fw_system(ptr);
	FREE(ptr);
	ptr = g_strdup_printf("umount %s/proc", TARGETDIR);
	fw_system(ptr);
	FREE(ptr);
	ptr = g_strdup_printf("umount %s/dev", TARGETDIR);
	fw_system(ptr);
	FREE(ptr);
	ptr = g_strdup_printf("umount %s", TARGETDIR);
	fw_system(ptr);
	FREE(ptr);
	
	return 0;
}
 
