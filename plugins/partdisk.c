/*
 *  partdisk.c for Fwife
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
#include <dialog.h>
#include <parted/parted.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <gtk/gtk.h>

#include "common.h"

//* Lists for preloading partition *//
static GList *parts=NULL;
static GList *allparts=NULL;
static GList *devs=NULL;

//* Current root partition selected */
static GList* rootpart = NULL;
//* count swap partition number *//
static int nbswap = 0;

static GtkWidget *comboparts = NULL;
static GtkWidget *partview = NULL;

enum 
{
	PIXBUF_COL,
 	TEXT_COL
};

enum
{
	TYPE_COLUMN,
	NAME_COLUMN,
 	SIZE_COLUMN,
 	FS_COLUMN,
  	MOUNT_COLUMN,
   	NUM_COLUMN
};

PedExceptionOption peh(PedException* ex)
{
	return(PED_EXCEPTION_IGNORE);
}

plugin_t plugin =
{
	"formatdisk",	
	desc,
	35,
	load_gtk_widget,
 	GTK_ASSISTANT_PAGE_CONTENT,
 	FALSE,
	load_help_widget,
  	prerun,
	run,
	NULL // dlopen handle
};

plugin_t *info()
{
	return &plugin;
}

char *desc()
{
	return _("Partitioning the disk drives");
}

int partdetails(PedPartition *part, int noswap)
{
	char *pname, *ptype, *ptr;
	PedFileSystemType *type;	
		
	if ((type = ped_file_system_probe (&part->geom)) == NULL)
		// prevent a nice segfault
		ptype = strdup("unknown");
	else
		ptype = (char*)type->name;

	if(noswap && !strncmp("linux-swap", ptype, 10))
		return(0);

	pname = ped_partition_get_path(part);

	// remove the unnecessary "p-1" suffix from sw raid device names
	if((ptr = strstr(pname, "p-1"))!=NULL)
		*ptr='\0';

	// for dialog menus
	parts = g_list_append(parts, pname);	
	ptr = fsize(part->geom.length);	
	parts = g_list_append(parts, g_strdup_printf("%s", ptr));
	parts = g_list_append(parts, g_strdup_printf("%s", ptype));
	// used for stock mounts points
	parts = g_list_append(parts, NULL);	
		
	FREE(ptr);
	
	return(0);
}

int listparts(PedDisk *disk, int noswap)
{
	PedPartition *part = NULL;
	PedPartition *extpart = NULL;

	if(ped_disk_next_partition(disk, NULL)==NULL)
		// no partition detected
		return(1);
	for(part=ped_disk_next_partition(disk, NULL);
		part!=NULL;part=part->next)
	{
		if((part->num>0) && (part->type != PED_PARTITION_EXTENDED) && !ped_partition_get_flag(part, PED_PARTITION_RAID))
			partdetails(part, noswap);
		if(part->type == PED_PARTITION_EXTENDED)
			for(extpart=part->part_list;
				extpart!=NULL;extpart=extpart->next)
				if(extpart->num>0 && !ped_partition_get_flag(extpart, PED_PARTITION_RAID))
					partdetails(extpart, noswap);
	}
	return(0);
}

int detect_raids()
{
	FILE *fp;
	char *line, *ptr;
	PedDevice *dev = NULL;
	PedDisk *disk = NULL;
	PedPartition *part = NULL;

	if ((fp = fopen("/proc/mdstat", "r"))== NULL)
	{
		perror("Could not open output file for writing");
		return(1);
	}
	MALLOC(line, PATH_MAX);
	while(!feof(fp))
	{
		if(fgets(line, PATH_MAX, fp) == NULL)
			break;
		if(strstr(line, "md")==line)
		{
			ptr = line;
			while(*ptr!=' ')
				ptr++;
			*ptr='\0';
			dev = ped_device_get(g_strdup_printf("/dev/%s", line));
			disk = ped_disk_new_fresh(dev, ped_disk_type_get ("loop"));
			if(disk)
			{
				part=ped_disk_next_partition(disk, NULL);
				partdetails(part, 0);
			}
		}
	}
	FREE(line);
	fclose(fp);
	return(0);
}


// mode=0: fs, mode=1: mountpoint
char *findmount(char *dev, int mode)
{
	FILE *fp;
	char line[PATH_MAX], *ptr;
	int i;

	if ((fp = fopen("/proc/mounts", "r"))== NULL)
	{
		perror("Could not open /proc/mounts output file for reading");
		return(NULL);
	}
	while(!feof(fp))
	{
		if(fgets(line, PATH_MAX, fp) == NULL)
			break;
		if(strstr(line, dev)==line)
		{
			ptr = strstr(line, " ")+1;
			if(!mode)
				ptr = strstr(ptr, " ")+1;
			for(i=0;*(ptr+i)!='\0';i++)
				if(*(ptr+i)==' ')
					*(ptr+i)='\0';
			fclose(fp);
			return(ptr);
		}
	}
	fclose(fp);
	return(NULL);
}

int detect_parts(int noswap)
{
	PedDevice *dev = NULL;
	PedDisk *disk = NULL;

	ped_device_free_all();
	chdir("/");

	ped_exception_set_handler(peh);
	ped_device_probe_all();
	
	if(allparts != NULL)
	{
		int i;
		for(i=0;i<g_list_length(allparts);i++)
		{
			data_t *dat = (data_t*)g_list_nth_data(allparts, i);
			if(dat->data)
				g_list_free(dat->data);
		}
		g_list_free(allparts);
		allparts = NULL;
		parts=NULL;
		gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(partview))));
		
	}

	if(ped_device_get_next(NULL)==NULL)
		// no disk detected already handled before, no need to inform
		// the user about this
		return(1);

	for(dev=ped_device_get_next(NULL);dev!=NULL;dev=dev->next)
	{
		if(dev->read_only)
			// we don't want to partition cds ;-)
			continue;
		disk = ped_disk_new(dev);
		if(disk)
		{
			listparts(disk, noswap);
			data_put(&allparts, dev->path, parts);
			parts = NULL;	
		}
	}

	// software raids
	detect_raids();
	return(0);
}

int buggy_md0()
{
	FILE *fp;
	char line[256];

	fp = fopen("/proc/mdstat", "r");
	if(!fp)
		return(1);
	while(!feof(fp))
	{
		if(fgets(line, 255, fp) == NULL)
			break;
		if(!strncmp(line, "md", 2))
		{
			fclose(fp);
			return(0);
		}
	}
	return(1);
}

int listdevs(void)
{
	PedDevice *dev=NULL;
	char *ptr;
		
	// silly raid autodetect, md0 always created
	if(buggy_md0())
		unlink("/dev/md0");

	ped_exception_set_handler(peh);
	ped_device_probe_all();

	if(ped_device_get_next(NULL)==NULL)
		return(-1);

	for(dev=ped_device_get_next(NULL);dev!=NULL;dev=dev->next)
	{
		if(dev->read_only)
			// we don't want to partition cds ;-)
			continue;
		devs = g_list_append(devs, strdup(dev->path));
		ptr = fsize(dev->length);
		devs = g_list_append(devs, g_strdup_printf("%s\t%s", ptr, dev->model));
		FREE(ptr);
	}

	return(0);
}

//* Create combolist with nice icons *//
GtkTreeModel *create_stock_icon_store ()
{
	GtkStockItem item;
	GdkPixbuf *pixbuf;
	GtkWidget *cellview;
	GtkTreeIter iter;
	GtkListStore *store;
	gint i;

	cellview = gtk_cell_view_new ();
  
	store = gtk_list_store_new (2, GDK_TYPE_PIXBUF, G_TYPE_STRING);

	for (i = 0; i < g_list_length(devs); i+=2)
	{
		pixbuf = gtk_widget_render_icon (cellview, GTK_STOCK_HARDDISK,
					GTK_ICON_SIZE_BUTTON, NULL);
		gtk_stock_lookup (GTK_STOCK_HARDDISK, &item);
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, PIXBUF_COL, pixbuf, TEXT_COL, (char*)g_list_nth_data(devs,i), -1);
		g_object_unref (pixbuf);
	}

	gtk_widget_destroy (cellview);
  
	return GTK_TREE_MODEL (store);
}

//* Add a mountpoint into fstab file *//
int mountdev(char *dev, char *mountpoint, gboolean isswap, GList **config)
{
	char *type=NULL;
	FILE* fp;

	// open fstab
	if ((fp = fopen((char*)data_get(*config, "fstab"), "a")) == NULL)
	{
		perror(_("Could not open fstab output file for writing"));
		return(1);
	}
	
	if(isswap == TRUE)
	{
		fprintf(fp, "%-16s %-16s %-11s %-16s %-3s %s\n",dev, "swap", "swap", "defaults", "0", "0");
	}
	else
	{
	// mount
	makepath(g_strdup_printf("%s/%s", TARGETDIR, mountpoint));
	umount_if_needed(mountpoint);
	fw_system(g_strdup_printf("mount %s %s/%s",
		  dev, TARGETDIR, mountpoint));
	// unlink a possible stale lockfile
	unlink(g_strdup_printf("%s/%s/tmp/pacman-g2.lck", TARGETDIR, mountpoint));

	// make fstab entry
	type = findmount(dev, 0);
	fprintf(fp, "%-16s %-16s %-11s %-16s %-3s %s\n", dev, mountpoint,
		type, "defaults", "1", "1");
	}
	
	fclose(fp);
	return(0);
}

//* Foramt a partition and create filesystem on it *//
int mkfss(char *dev, char *fs, gboolean checked)
{
	char *opts=NULL;
	// TOFIX : Memory leaks
	if(checked == TRUE)
		opts = strdup("-c");
	else
		opts = strdup("");

	umount(findmount(dev, 1));
	if(!strcmp(fs, "ext2"))
		return(fw_system(g_strdup_printf("mke2fs %s %s", opts, dev)));
	else if(!strcmp(fs, "ext3"))
		return(fw_system(g_strdup_printf("mke2fs -j %s %s", opts, dev)));
	else if(!strcmp(fs, "reiserfs"))
		return(fw_system(g_strdup_printf("echo y |mkreiserfs %s", dev)));
	else if(!strcmp(fs, "jfs"))
		return(fw_system(g_strdup_printf("mkfs.jfs -q %s %s", opts, dev)));
	else if(!strcmp(fs, "xfs"))
		return(fw_system(g_strdup_printf("mkfs.xfs -f %s", dev)));
	else if(!strcmp(fs, "swap"))
	{
		fw_system(g_strdup_printf("%s %s %s", MKSWAP, opts, dev));
		fw_system(g_strdup_printf("%s %s", SWAPON, dev));		
		return(0);
	}
		
	// never reached
	return(-1);
}

//* A cell has been edited *//
void cell_edited(GtkCellRendererText *cell, const gchar *path_string, gchar *new_text, gpointer data)
{
	GtkTreeModel *model = (GtkTreeModel *)data;
	GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
	GtkTreeIter iter;
	
	gint column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cell), "column"));

	gtk_tree_model_get_iter (model, &iter, path);

	switch (column)
	{
		case MOUNT_COLUMN:
		{
			gint i;
			gchar *old_text = NULL;

			gtk_tree_model_get (model, &iter, column, &old_text, -1);
			
			if(new_text && old_text && !strcmp(new_text, old_text))
				return;
			
			i = gtk_tree_path_get_indices (path)[0];			
			if(old_text)
			{
				if(!strcmp(old_text, "/"))
				{
					fwife_error(_("If you want to change your root partition, select the new partition and use \"Set root partition\" button!"));
					return;
				}
				else if(!strcmp(old_text, "swap"))
				{
					switch(fwife_question(_("Unselect this swap partition?")))
					{
						case GTK_RESPONSE_YES:
							gtk_list_store_set (GTK_LIST_STORE (model), &iter, TYPE_COLUMN, NULL, -1);
							nbswap--;
							if(nbswap<=0)
							{
								set_page_incompleted();
							}
							break;
						case GTK_RESPONSE_NO:
							return;
					}
				}
					
			}
			if(new_text && (!strcmp(new_text, "/") || !strcmp(new_text, "swap")))
			{
				fwife_error(_("Use the suitable button to perform this action!"));
			}
			else if(new_text)
			{
				if(old_text)
					g_free (old_text);
				
				GList *point = g_list_nth(parts, 4*i+3);
				gchar *text = g_strdup(new_text);
				point->data = text;
				gtk_list_store_set (GTK_LIST_STORE (model), &iter, MOUNT_COLUMN, new_text, -1);
			}
		}
		break;
	}

	gtk_tree_path_free (path);
}


//* Update partview (partition list) *//
void update_treeview_list()
{
	int i;
	GtkTreeIter iter;
	
	gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(partview))));
	
	if(parts != NULL)
	{
		for(i=0; i < g_list_length(parts); i+=4 ) 
		{		
			char *mount = (char*)g_list_nth_data(parts,i+3);
			gtk_list_store_append(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(partview))), &iter);
		
			gtk_list_store_set(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(partview))), &iter, NAME_COLUMN, (char*)g_list_nth_data(parts, i), SIZE_COLUMN, (char*)g_list_nth_data(parts, i+1), FS_COLUMN,(char*)g_list_nth_data(parts, i+2), -1);
			if(mount)
			{
				if(!strcmp(mount, "/"))
				{
					GtkWidget *cellview = gtk_cell_view_new ();	
					GdkPixbuf *pixbuf = gtk_widget_render_icon (cellview, GTK_STOCK_HOME, GTK_ICON_SIZE_BUTTON, NULL);
					gtk_widget_destroy (cellview);
					gtk_list_store_set (GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(partview))), &iter, TYPE_COLUMN, pixbuf,MOUNT_COLUMN, mount,-1);
				}
				else if(!strcmp(mount, "swap"))
				{
					GtkWidget *cellview = gtk_cell_view_new ();	
					GdkPixbuf *pixbuf = gtk_widget_render_icon (cellview, GTK_STOCK_CONVERT, GTK_ICON_SIZE_BUTTON, NULL);
					gtk_widget_destroy (cellview);
					gtk_list_store_set (GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(partview))), &iter, TYPE_COLUMN, pixbuf,MOUNT_COLUMN, mount,-1);
				}
				else
				{
					gtk_list_store_set (GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(partview))), &iter, MOUNT_COLUMN, mount,-1);
				}
			}	
		}
	}
}

//* Create a dialog box for formatting namedev device *//
int requestformat(char *namedev)
{
	
	GSList *pList;
   	char *sLabel;
	char *fs = NULL;
	extern GtkWidget *assistant;
	
	GtkWidget* pBoite = gtk_dialog_new_with_buttons(_("Do you want to format partition?"),
			GTK_WINDOW(assistant),
   			GTK_DIALOG_MODAL,
   			GTK_STOCK_OK,GTK_RESPONSE_OK,
   			GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,
   			NULL);

	gtk_window_set_position(GTK_WINDOW(pBoite), GTK_WIN_POS_CENTER_ON_PARENT);

	GtkWidget *pVBox = GTK_DIALOG(pBoite)->vbox;
	
	GtkWidget *pLabelInfo=gtk_label_new(NULL);

	/* On utilise les balises */
	gtk_label_set_markup(GTK_LABEL(pLabelInfo), _("<span face=\"Courier New\" foreground=\"#f90909\"><b>!!! NOTE: This will erase all data on it. Would you like to format this partition? !!!</b></span>\n"));
	
	gtk_box_pack_start(GTK_BOX(pVBox), pLabelInfo, FALSE, FALSE, 0);

	GtkWidget *pLabel = gtk_label_new(_("Choose filesystem :"));
	gtk_box_pack_start(GTK_BOX(pVBox), pLabel, FALSE, FALSE, 0);

	/* First radio button */
	GtkWidget *pRadio1 = gtk_radio_button_new_with_label(NULL, _("ext2 - Standard Linux ext2fs filesystem"));
	gtk_box_pack_start(GTK_BOX (pVBox), pRadio1, FALSE, FALSE, 0);
	
	/* Ext3 filesystem */
	GtkWidget *pRadio2 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON (pRadio1), _("ext3 - Journalising version of the ext2fs filesystem"));
	gtk_box_pack_start(GTK_BOX (pVBox), pRadio2, FALSE, FALSE, 0);
	
	/* Reiser Filesystem */
	GtkWidget *pRadio3 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON (pRadio1), _("reiserfs - Hans Reiser's journalising filesystem"));
	gtk_box_pack_start(GTK_BOX (pVBox), pRadio3, FALSE, FALSE, 0);
	
	/* XFS */
	GtkWidget *pRadio4 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON (pRadio1), _("xfs - SGI's journalising filesystem"));
	gtk_box_pack_start(GTK_BOX (pVBox), pRadio4, FALSE, FALSE, 0);
		
	GtkWidget *pRadio5 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON (pRadio1), _("Noformat - keep filesystem"));
	gtk_box_pack_start(GTK_BOX (pVBox), pRadio5, FALSE, FALSE, 0);
	
	GtkWidget *separator = gtk_hseparator_new();
	gtk_box_pack_start (GTK_BOX (pVBox), separator, FALSE, FALSE, 5);
	
	GtkWidget *pLabel2 = gtk_label_new(_("Options :"));
	gtk_box_pack_start(GTK_BOX(pVBox), pLabel2, FALSE, FALSE, 5);
	
	GtkWidget *check = gtk_check_button_new_with_label(_("Check for bad blocks - slowly"));
	gtk_box_pack_start(GTK_BOX (pVBox), check, FALSE, FALSE, 5);
	
	/* Show vbox  */
	gtk_widget_show_all(GTK_DIALOG(pBoite)->vbox);

	/* Running dialog box and get back response */
	switch (gtk_dialog_run(GTK_DIALOG(pBoite)))
	{
		/* Formatting */
		case GTK_RESPONSE_OK:			

    		/* Recuperation de la liste des boutons */
    		pList = gtk_radio_button_get_group(GTK_RADIO_BUTTON(pRadio1));

    		/* Parcours de la liste */
    		while(pList)
    		{
      			/* button selected? */
      			if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pList->data)))
       			{
            		/* get button label */
            		sLabel = strdup((char*)gtk_button_get_label(GTK_BUTTON(pList->data)));
            		/* Get out */			
            		pList = NULL;
        		}
        		else
        		{
            		/* next button */
            		pList = g_slist_next(pList);
        		}
    		}
		
		if(!strcmp(sLabel, _("ext2 - Standard Linux ext2fs filesystem")))
			fs = "ext2";
		else if(!strcmp(sLabel, _("ext3 - Journalising version of the ext2fs filesystem")))
			fs = "ext3";
		else if(!strcmp(sLabel, _("reiserfs - Hans Reiser's journalising filesystem")))
			fs = "reiserfs";
		else if(!strcmp(sLabel, _("xfs - SGI's journalising filesystem")))
			fs = "xfs";
		else
			fs = NULL;
		
		
		
		if(fs == NULL)
		{
			gtk_widget_destroy(pBoite);
			FREE(sLabel);
			return 0;
		}
		else
		{
			gboolean checked = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check));
			// change label
			gtk_label_set_markup(GTK_LABEL(pLabelInfo), _("<span face=\"Courier New\"><b>Creating filesystem... Please wait ... </b></span>\n"));
			// hide all widgets
			gtk_widget_hide(pLabel);
			gtk_widget_hide(pLabel2);
			gtk_widget_hide(pRadio1);
			gtk_widget_hide(pRadio2);
			gtk_widget_hide(pRadio3);
			gtk_widget_hide(pRadio4);
			gtk_widget_hide(pRadio5);
			gtk_widget_hide(separator);
			gtk_widget_hide(check);

			while (gtk_events_pending())
				gtk_main_iteration ();
			//* if sucessfull, replace into glist and update treeview *//
			if(mkfss(namedev, fs, checked) == 0)
			{
				int i;
				for(i=0; i<g_list_length(parts); i+=4)
				{
					if(!strcmp((char*)g_list_nth_data(parts, i), namedev))
						break;
				}
				
				GList *elem = g_list_nth(parts, i+2);
				if(elem)
				{
					FREE(elem->data);
					elem->data = strdup(fs);
				}
				update_treeview_list();
			}	
				
		}
		break;
		/* Exit */
		case GTK_RESPONSE_CANCEL:
		case GTK_RESPONSE_NONE:
		default:
			gtk_widget_destroy(pBoite);
			FREE(sLabel);
			return -1;
	}
	FREE(sLabel);
	gtk_widget_destroy(pBoite);
	return 0;
}

//* Set the partition "namedev" as a swap partition *//
int swapformat(char *namedev)
{
	extern GtkWidget *assistant;
		
	GtkWidget* pBoite = gtk_dialog_new_with_buttons(_("Do you want to format partition?"),
			GTK_WINDOW(assistant),
   			GTK_DIALOG_MODAL,
   			GTK_STOCK_OK,GTK_RESPONSE_OK,
   			GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,
   			NULL);
	gtk_window_set_position(GTK_WINDOW(pBoite), GTK_WIN_POS_CENTER_ON_PARENT);

	/* Creation de la zone de saisie */
	GtkWidget *pVBox = GTK_DIALOG(pBoite)->vbox;
		
	GtkWidget *pLabelInfo=gtk_label_new(NULL);

	/* On utilise les balises */
	gtk_label_set_markup(GTK_LABEL(pLabelInfo), _("<span face=\"Courier New\" foreground=\"#f90909\"><b>You need to format is you want to use it as a swap partition!\nThis will erase all your data on that partition !\n\n Do you want to continue? </b></span>\n"));
	
	gtk_box_pack_start(GTK_BOX(pVBox), pLabelInfo, FALSE, FALSE, 0);
	
	/* Affichage des elements de la boite de dialogue */
	gtk_widget_show_all(GTK_DIALOG(pBoite)->vbox);

	/* On lance la boite de dialogue et on recupere la reponse */
	switch (gtk_dialog_run(GTK_DIALOG(pBoite)))
	{
		/* L utilisateur valide */
		case GTK_RESPONSE_OK:
			if(mkfss(namedev, "swap", FALSE) == 0)
			{
				int i;
				for(i=0; i<g_list_length(parts); i+=4)
				{
					if(!strcmp((char*)g_list_nth_data(parts, i), namedev))
						break;
				}
				
				GList *elem = g_list_nth(parts, i+2);
				if(elem)
				{
					FREE(elem->data);
					elem->data = strdup("linux-swap");
				}
				update_treeview_list();
			}			
			break;
			/* L utilisateur annule */
		case GTK_RESPONSE_CANCEL:
		case GTK_RESPONSE_NONE:
		default:
			gtk_widget_destroy(pBoite);
			return (-1);
			break;
	}
	gtk_widget_destroy(pBoite);
	return 0;
}

//* Button set as root partition clicked *//
void set_root_part(GtkWidget *widget, gpointer data)
{
	GtkTreeModel *model = NULL;
	GtkTreeIter iter;
	char* namedev;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(partview));	
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(GTK_TREE_VIEW(partview)));
	
	if(gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		// one root partition as been selected before ; ask for changing
		if(rootpart)
		{
			switch(fwife_question(_("Select this partition as root partition?")))
			{
				case GTK_RESPONSE_YES:
					if(rootpart->data)
					{
						g_free(rootpart->data);
						rootpart->data = NULL;
					}
					break;
				case GTK_RESPONSE_NO:
					return;
			}
		}
		
		GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
		gint i = gtk_tree_path_get_indices (path)[0];
		gtk_tree_model_get (model, &iter, NAME_COLUMN, &namedev, -1);
		
		if(requestformat(namedev) == -1)
			return;
		gchar *mount = g_strdup("/");		
		// get list mount element for new root partition
		GList *point = g_list_nth(parts, 4*i+3);
		if(point->data)
			g_free(point->data);
		point->data = mount;		
		
		rootpart = point;
		
		// check if at least one swap partition and one root partition is selected
		if(rootpart && nbswap>0)
		{
			set_page_completed();
		}
				
		update_treeview_list();
	}	
}

void set_format_part(GtkWidget *widget, gpointer data)
{
	GtkTreeModel *model = NULL;
	GtkTreeIter iter;
	char* namedev;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(partview));	
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(GTK_TREE_VIEW(partview)));
	
	//get partition name
	if(gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gtk_tree_model_get (model, &iter, NAME_COLUMN, &namedev, -1);
	}

	// ask for formatting
	requestformat(namedev);
}

//* Idem for swap *//
void set_swap_part(GtkWidget *widget, gpointer data)
{
	GtkTreeModel *model = NULL;
	GtkTreeIter iter;
	char* namedev;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(partview));	
	
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(GTK_TREE_VIEW(partview)));	
	if(gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
		gchar *mount = strdup("swap");
		gint i = gtk_tree_path_get_indices (path)[0];	
		GList *point = g_list_nth  (parts, 4*i+3);
		gtk_tree_model_get (model, &iter, NAME_COLUMN, &namedev, -1);
		
		// Case if we want to change the root partition to a swap one
		if(point->data && !strcmp((char*)point->data,"/"))
		{
			fwife_error(_("Can't do this operation directly! Change your root partition first.")); 
			return;
		}
		if(swapformat(namedev) == -1)
			return;
		// Free old pointer
		if(point->data)
			g_free(point->data);
		// add new
		point->data = mount;
		// yeah we have a new swap partition
		nbswap++;
		// check if completed page conditions are verified
		if(rootpart && nbswap>0)
		{
			set_page_completed();
		}	
	}
	update_treeview_list();	
}

//* Change combo so load correct glist and refresh treeview *//
void change_part_list(GtkComboBox *combo, gpointer data)
{
	char *selected;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gtk_combo_box_get_active_iter(combo, &iter);
	model = gtk_combo_box_get_model(combo);
	gtk_tree_model_get (model, &iter, TEXT_COL, &selected, -1);
	
	GList *elem = g_list_find_custom(devs, selected, cmp_str);
	if(elem != NULL)
	{
		int pos = g_list_position(devs, elem);
		gtk_label_set_label(GTK_LABEL(data), (char*)g_list_nth_data(devs, pos+1));
	}
	else
		gtk_label_set_label(GTK_LABEL(data), "");
	
	if (allparts)
		parts = (GList*)data_get(allparts, selected);
	
	update_treeview_list();
}

//* Launch gparted *//
void run_gparted(GtkWidget *widget, gpointer data)
{
	switch(fwife_question(_("Do you want to run gparted to create/modify partitions?")))
	{
		case GTK_RESPONSE_YES:
			fw_system_interactive("gparted");
			detect_parts(0);
			gtk_combo_box_set_active (GTK_COMBO_BOX (comboparts), 0);
			change_part_list(GTK_COMBO_BOX (comboparts), data);
			//* Mountpoint are reset *//
			nbswap = 0;
			rootpart = NULL;
			set_page_incompleted();
			break;
		case GTK_RESPONSE_NO:
			break;
	}
}

//* Load gtk widget for this plugin *//
GtkWidget *load_gtk_widget()
{
	GtkWidget *pVBox, *pHBox, *info;
	GtkWidget *diskinfo, *device;	

	listdevs();		
	
	//* Combobox containing devices *//
	pVBox = gtk_vbox_new(FALSE, 5);	
	
	info = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(info), _("<span face=\"Courier New\"><b>Select and format your partitions or launch GParted to modify them.</b></span>\n"));
	gtk_box_pack_start (GTK_BOX (pVBox),info, FALSE, FALSE, 10);
	
	/* HBox for disk information and selection */
	pHBox = gtk_hbox_new(FALSE,5);
	gtk_box_pack_start (GTK_BOX (pVBox), pHBox, FALSE, FALSE, 0);
	
	device = gtk_label_new(_(" Devices : "));
	gtk_box_pack_start (GTK_BOX (pHBox), device, FALSE, FALSE, 0);	
	
	/*GTK Widget for selecting harddisk */
	GtkTreeModel *model = create_stock_icon_store (devs);
	comboparts = gtk_combo_box_new_with_model (model);
	g_object_unref (model);
	gtk_widget_set_size_request(comboparts, 120, 40);
	gtk_box_pack_start (GTK_BOX (pHBox), comboparts, FALSE, FALSE, 10);
    
	GtkCellRenderer *renderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (comboparts), renderer, FALSE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (comboparts), renderer, "pixbuf", PIXBUF_COL, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (comboparts), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (comboparts), renderer,"text", TEXT_COL, NULL);

	diskinfo = gtk_label_new("");
	gtk_box_pack_start (GTK_BOX (pHBox), diskinfo, FALSE, TRUE, 50);
	
	/* List of device partition with mountpoints and other*/
	
	GtkListStore *store;
	GtkTreeViewColumn *col;
	GtkWidget *pScrollbar;
	GtkTreeSelection *selection;
	GtkWidget *hboxlist;
	
	hboxlist = gtk_hbox_new(TRUE, 5);
	gtk_box_pack_start (GTK_BOX (pVBox), hboxlist, TRUE, TRUE, 10);	
	
	store = gtk_list_store_new(NUM_COLUMN, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	model = GTK_TREE_MODEL(store);
	
	partview = gtk_tree_view_new_with_model(model);
	g_object_unref (model);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(partview), TRUE);
	
	// a small icon for swap and root partition
	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "pixbuf", TYPE_COLUMN, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(partview), col);
	
	// partition name
	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", NAME_COLUMN, NULL);
	gtk_tree_view_column_set_title(col, _("Partition"));
	gtk_tree_view_append_column(GTK_TREE_VIEW(partview), col);
		
	// partition size
	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", SIZE_COLUMN, NULL);
	gtk_tree_view_column_set_title(col, _("Size"));
	gtk_tree_view_append_column(GTK_TREE_VIEW(partview), col);	
	
	// current filesystem
	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", FS_COLUMN, NULL);
	gtk_tree_view_column_set_title(col, _("Filesystem"));
	gtk_tree_view_append_column(GTK_TREE_VIEW(partview), col);
	
	// mountpoint
	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "editable", TRUE, NULL);
	g_signal_connect(renderer, "edited", G_CALLBACK (cell_edited), model);
	g_object_set_data(G_OBJECT (renderer), "column", GINT_TO_POINTER (MOUNT_COLUMN));
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", MOUNT_COLUMN, NULL);
	gtk_tree_view_column_set_title(col, _("Mountpoint"));
	gtk_tree_view_append_column(GTK_TREE_VIEW(partview), col);	

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (partview));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
      
	pScrollbar = gtk_scrolled_window_new(NULL, NULL);
	
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pScrollbar), partview);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pScrollbar), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_box_pack_start(GTK_BOX(hboxlist), pScrollbar, FALSE, TRUE, 0);
	
	//* Buttons at end page *//
	GtkWidget *mainpart, *swappart, *format, *buttonlist, *gparted;
	GtkWidget *image;
	
	buttonlist = gtk_hbox_new(TRUE, 10);
	gtk_box_pack_start (GTK_BOX (pVBox), buttonlist, FALSE, FALSE, 10);
	
	//* Set buttons *//
	mainpart = gtk_button_new_with_label(_("Set as root partition"));
	swappart = gtk_button_new_with_label(_("Set as swap partition"));
	format = gtk_button_new_with_label(_("Format partition"));
	gparted = gtk_button_new_with_label(_("Run Gparted"));
	
	//* Set images *//
	image = gtk_image_new_from_stock (GTK_STOCK_DELETE, 3);
	gtk_button_set_image(GTK_BUTTON(format), image);
	image = gtk_image_new_from_stock (GTK_STOCK_HOME, 3);
	gtk_button_set_image(GTK_BUTTON(mainpart), image);
	image = gtk_image_new_from_stock (GTK_STOCK_CONVERT, 3);
	gtk_button_set_image(GTK_BUTTON(swappart), image);
	image = gtk_image_new_from_file("images/gparted.png");
	gtk_button_set_image(GTK_BUTTON(gparted), image);
	
	//* connect button to the select root part function *//
	g_signal_connect (mainpart, "clicked",G_CALLBACK (set_root_part), NULL);
	g_signal_connect (swappart, "clicked", G_CALLBACK (set_swap_part), NULL);
	g_signal_connect (format, "clicked", G_CALLBACK (set_format_part), partview);
	g_signal_connect (gparted, "clicked", G_CALLBACK (run_gparted), diskinfo);
	g_signal_connect(G_OBJECT(comboparts), "changed", G_CALLBACK(change_part_list), diskinfo);	
	
	//* Add them to the box *//
	gtk_box_pack_start (GTK_BOX (buttonlist), mainpart, TRUE, FALSE, 10);
	gtk_box_pack_start (GTK_BOX (buttonlist), swappart, TRUE, FALSE, 10);
	gtk_box_pack_start (GTK_BOX (buttonlist), format, TRUE, FALSE, 10);
	gtk_box_pack_start (GTK_BOX (buttonlist), gparted, TRUE, FALSE, 10);
		
	return pVBox;
}

int prerun(GList **config)
{
	if(allparts == NULL)
	{
		detect_parts(0);
		gtk_combo_box_set_active (GTK_COMBO_BOX (comboparts), 0);
	}
	return 0;
}

//* Run plugin and create fstab file *//
int run(GList **config)
{
	char *fn, *dev, *mount, *type, *op, *np;
	GList *partition;
	FILE* fp;
	int i, j;

	// create an initial fstab
	fn = strdup("/tmp/setup_XXXXXX");
	mkstemp(fn);
	if ((fp = fopen(fn, "w")) == NULL)
	{
		perror(_("Could not open output file for writing"));
		return(1);
	}
	fprintf(fp, "%-16s %-16s %-11s %-16s %-3s %s\n",
		"none", "/proc", "proc", "defaults", "0", "0");
	fprintf(fp, "%-16s %-16s %-11s %-16s %-3s %s\n",
		"none", "/sys", "sysfs", "defaults", "0", "0");
	fprintf(fp, "%-16s %-16s %-11s %-16s %-3s %s\n",
		"devpts", "/dev/pts", "devpts", "gid=5,mode=620", "0", "0");
	fprintf(fp, "%-16s %-16s %-11s %-16s %-3s %s\n",
		"usbfs", "/proc/bus/usb", "usbfs", "devgid=23,devmode=664", "0", "0");
	fprintf(fp, "%-16s %-16s %-11s %-16s %-3s %s\n",
		"tmpfs", "/dev/shm", "tmpfs", "defaults", "0", "0");
	
	fclose(fp);
	
	// save fstab location for later
	data_put(config, "fstab", fn);	
	
	// add all mounts points into fstab
	for(i=0; i<g_list_length(allparts); i++)
	{
		partition = (GList*)(((data_t*)g_list_nth_data(allparts, i))->data);
		for(j=0; j<g_list_length(partition); j+=4)
		{
			if((char*)g_list_nth_data(partition, j+3) != NULL)
			{
				dev = (char*)g_list_nth_data(partition, j);
				mount = (char*)g_list_nth_data(partition, j+3);
				type = (char*)g_list_nth_data(partition, j+2);
				if(!strcmp(mount, "swap") || !strcmp(type, "linux-swap"))
					mountdev(dev, mount, TRUE, config);
				else
					mountdev(dev, mount, FALSE, config);
			}
		}
	}
	
	// Copy files into new locations
	chdir(TARGETDIR);
	makepath(g_strdup_printf("%s/%s", TARGETDIR, "/etc/profile.d"));
	op = (char*)data_get(*config, "fstab");
	np = g_strdup_printf("%s/%s", TARGETDIR, "/etc/fstab");
	copyfile(op, np);
	unlink(op);
	chmod (np, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	data_put(config, "fstab", strdup(np));
	FREE(np);

	op = (char*)data_get(*config, "lang.sh");
	np = g_strdup_printf("%s/%s", TARGETDIR, "/etc/profile.d/lang.sh");
	copyfile(op, np);
	unlink(op);
	chmod(np, S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
	FREE(np);
	
	makepath(g_strdup_printf("%s/%s", TARGETDIR, "/etc/sysconfig"));
	op = (char*)data_get(*config, "keymap");
	np = g_strdup_printf("%s/%s", TARGETDIR, "/etc/sysconfig/keymap");
	copyfile(op, np);
	unlink(op);
	chmod (np, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	FREE(np);
		
	return(0);
}

GtkWidget *load_help_widget()
{
	GtkWidget *help = gtk_label_new(_("You must select at least one swap partition and one root partition to continue to the next stage.\n For example, to assign a root partition, select the device in list located in top of the page,\n then select a partition and use suitable button to make it as a root partition.\n\nYou may use your other partitions to distribute your Linux system across more than one partition.\n You might want to mount directories such as /boot, /home or /usr/local on separate partitions.\n You should not try to mount /usr, /etc, /sbin or /bin on their own partitions\n since they contain utilities needed to bring the system up and mount partitions.\n Also, do not reuse a partition that you've already entered before.\n To set a mountpoint for a partition, edit the cell in the table of the partitions (column \"Mountpoint\") by double-clicking on it.\n\nIf you want to modify partition table (create or modify partitions),\n you can run GParted but this will erase all your selected partitions."));
	
	return help;
}
 
