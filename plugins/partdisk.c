#include <stdio.h>
#include <dialog.h>
#include <parted/parted.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <gtk/gtk.h>

#include "../setup.h"
#include "../util.h"
#include "common.h"

//* Lists for preloading partition *//
GList *parts=NULL;
GList *allparts=NULL;

//* Current root partition selected */
GList* rootpart = NULL;
//* count swap partition number *//
int nbswap = 0;

GtkWidget *comboparts;

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
	return _("Partionning your harddrive");
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
		perror("Could not open output file for reading");
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

	/*if(parts)
	{
		g_list_free(parts);
		parts = NULL;
	}*/
	ped_device_free_all();
	chdir("/");

	ped_exception_set_handler(peh);
	ped_device_probe_all();

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
			if (allparts)
				g_list_free(allparts);
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

GList *listdevs(void)
{
	GList *devs=NULL;
	PedDevice *dev=NULL;
	char *ptr;
	long long int *taille;

	// silly raid autodetect, md0 always created
	if(buggy_md0())
		unlink("/dev/md0");

	ped_exception_set_handler(peh);
	ped_device_probe_all();

	if(ped_device_get_next(NULL)==NULL)
		return(NULL);

	for(dev=ped_device_get_next(NULL);dev!=NULL;dev=dev->next)
	{
		if(dev->read_only)
			// we don't want to partition cds ;-)
			continue;
		devs = g_list_append(devs, dev->path);
		ptr = fsize(dev->length);
		devs = g_list_append(devs, g_strdup_printf("%s\t%s", ptr, dev->model));
		taille = malloc(sizeof(long long int));
		*taille = dev->length;
		devs = g_list_append(devs, taille);
		FREE(ptr);
	}

	return(devs);
}

GtkTreeModel *create_stock_icon_store (GList *devs)
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

void set_sensitive (GtkCellLayout *cell_layout,GtkCellRenderer *cell, GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data)
{
	GtkTreePath *path;
	gint *indices;
	gboolean sensitive;

	path = gtk_tree_model_get_path (tree_model, iter);
	indices = gtk_tree_path_get_indices (path);
	sensitive = indices[0] != 1;
	gtk_tree_path_free (path);

	g_object_set (cell, "sensitive", sensitive, NULL);
}

int mountdev(char *dev, char *mountpoint, gboolean isswap, GList **config)
{
	char *type=NULL;
	FILE* fp;

	// open fstab
	if ((fp = fopen((char*)data_get(*config, "fstab"), "a")) == NULL)
	{
		perror(_("Could not open output file for writing"));
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

int mkfss(char *dev, char *fs, int check)
{
	char *opts=NULL;

	opts = strdup(check ? "-c" : "");

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
	return(1);
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
					fwife_error("If you want to change your root partition, select another root partition and use appropriate button!");
					return;
				}
				else if(!strcmp(old_text, "swap"))
				{
					switch(fwife_question("Don't use this partition as a swap"))
					{
						case GTK_RESPONSE_YES:
							gtk_list_store_set (GTK_LIST_STORE (model), &iter, TYPE_COLUMN, NULL, -1);
							nbswap--;
							break;
						case GTK_RESPONSE_NO:
							return;
					}
				}
					
			}
			if(new_text && (!strcmp(new_text, "/") || !strcmp(new_text, "swap")))
			{
				fwife_error("Use appropriate button to do that");
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

void update_treeview_list(gpointer view)
{
	int i;
	GtkTreeIter iter;
	
	gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(view))));
	
	for(i=0; i < g_list_length(parts); i+=4 ) 
	{		
		char *mount = (char*)g_list_nth_data(parts,i+3);
		gtk_list_store_append(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(view))), &iter);
		
		gtk_list_store_set(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(view))), &iter, NAME_COLUMN, (char*)g_list_nth_data(parts, i), SIZE_COLUMN, (char*)g_list_nth_data(parts, i+1), FS_COLUMN,(char*)g_list_nth_data(parts, i+2), -1);
		if(mount)
		{
			if(!strcmp(mount, "/"))
			{
				GtkWidget *cellview = gtk_cell_view_new ();	
				GdkPixbuf *pixbuf = gtk_widget_render_icon (cellview, GTK_STOCK_HOME, GTK_ICON_SIZE_BUTTON, NULL);
				gtk_widget_destroy (cellview);
				gtk_list_store_set (GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(view))), &iter, TYPE_COLUMN, pixbuf,MOUNT_COLUMN, mount,-1);
			}
			else if(!strcmp(mount, "swap"))
			{
				GtkWidget *cellview = gtk_cell_view_new ();	
				GdkPixbuf *pixbuf = gtk_widget_render_icon (cellview, GTK_STOCK_CONVERT, GTK_ICON_SIZE_BUTTON, NULL);
				gtk_widget_destroy (cellview);
				gtk_list_store_set (GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(view))), &iter, TYPE_COLUMN, pixbuf,MOUNT_COLUMN, mount,-1);
			}
			else
			{
				gtk_list_store_set (GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(view))), &iter, MOUNT_COLUMN, mount,-1);
			}
		}
	}
}

//* Create a dialog box for formatting namedev device *//
int requestformat(char *namedev)
{
	
	GSList *pList;
   	const gchar *sLabel;
	extern GtkWidget *assistant;
	
	GtkWidget* pBoite = gtk_dialog_new_with_buttons("Do you want to format partition?",
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
	gtk_label_set_markup(GTK_LABEL(pLabelInfo), "<span face=\"Courier New\" foreground=\"#f90909\"><b>!!! This will erase all your data on that partition !!!</b></span>\n");
	
	gtk_box_pack_start(GTK_BOX(pVBox), pLabelInfo, FALSE, FALSE, 0);

	
	GtkWidget *pLabel = gtk_label_new("Choose filesystem :");
	gtk_box_pack_start(GTK_BOX(pVBox), pLabel, FALSE, FALSE, 0);

	/* Creation du premier bouton radio */
	GtkWidget *pRadio1 = gtk_radio_button_new_with_label(NULL, "ext2");
	gtk_box_pack_start(GTK_BOX (pVBox), pRadio1, FALSE, FALSE, 0);
	/* Ajout du deuxieme */
	GtkWidget *pRadio2 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON (pRadio1), "ext3 - Journalistic version of ext2");
	gtk_box_pack_start(GTK_BOX (pVBox), pRadio2, FALSE, FALSE, 0);
	/* Ajout du troisieme */
	GtkWidget *pRadio3 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON (pRadio1), "reiserfs");
	gtk_box_pack_start(GTK_BOX (pVBox), pRadio3, FALSE, FALSE, 0);
	
	GtkWidget *pRadio4 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON (pRadio1), "xfs");
	gtk_box_pack_start(GTK_BOX (pVBox), pRadio4, FALSE, FALSE, 0);
	
	GtkWidget *pRadio5 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON (pRadio1), "noformat - keep filesystem");
	gtk_box_pack_start(GTK_BOX (pVBox), pRadio5, FALSE, FALSE, 0);
	
	GtkWidget *separator = gtk_hseparator_new();
	gtk_box_pack_start (GTK_BOX (pVBox), separator, FALSE, FALSE, 5);
	
	GtkWidget *pLabel2 = gtk_label_new("Options :");
	gtk_box_pack_start(GTK_BOX(pVBox), pLabel2, FALSE, FALSE, 0);
	
	GtkWidget *check = gtk_check_button_new_with_label("formatting check - slowly");
	gtk_box_pack_start(GTK_BOX (pVBox), check, FALSE, FALSE, 0);
	
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
            		sLabel = gtk_button_get_label(GTK_BUTTON(pList->data));
            		/* Get out */
			
            		pList = NULL;
        		}
        		else
        		{
            		/* NON -> on passe au bouton suivant */
            		pList = g_slist_next(pList);
        		}
    		}
			//mkfss(namedev, sLabel, check);
			break;
			/* Exit */
		case GTK_RESPONSE_CANCEL:
		case GTK_RESPONSE_NONE:
		default:
			gtk_widget_destroy(pBoite);
			return -1;
			break;
	}
	gtk_widget_destroy(pBoite);
	return 0;
}

//* Set the partition "namedev" as a swap partition *//
int swapformat(char *namedev)
{
	extern GtkWidget *assistant;
		
	GtkWidget* pBoite = gtk_dialog_new_with_buttons("Do you want to format partition?",
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
	gtk_label_set_markup(GTK_LABEL(pLabelInfo), "<span face=\"Courier New\" foreground=\"#f90909\"><b>!!! You need to format is you want to do a swap partition!\nThis will erase all your data on that partition !!!\n\n Do you want to continue? </b></span>\n");
	
	gtk_box_pack_start(GTK_BOX(pVBox), pLabelInfo, FALSE, FALSE, 0);
	
	/* Affichage des elements de la boite de dialogue */
	gtk_widget_show_all(GTK_DIALOG(pBoite)->vbox);

	/* On lance la boite de dialogue et on recupere la reponse */
	switch (gtk_dialog_run(GTK_DIALOG(pBoite)))
	{
		/* L utilisateur valide */
		case GTK_RESPONSE_OK:
			// TODO : Reactivating real formatting
			//mkfss(namedev, "swap", 0);
			break;
			/* L utilisateur annule */
		case GTK_RESPONSE_CANCEL:
		case GTK_RESPONSE_NONE:
		default:
			gtk_widget_destroy(pBoite);
			return -1;
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
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data));	
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(GTK_TREE_VIEW(data)));
	
	GtkWidget *cellview = gtk_cell_view_new ();
	GdkPixbuf *pixbuf = gtk_widget_render_icon (cellview, GTK_STOCK_HOME, GTK_ICON_SIZE_BUTTON, NULL);
	gtk_widget_destroy (cellview);
	
	if(gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		if(rootpart)
		{
			switch(fwife_question("Change root partition?"))
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
		
		update_treeview_list(data);	
		
		if(rootpart && nbswap>0)
		{
			set_page_completed();
		}		
	}	
	g_object_unref(pixbuf);
}

void set_format_part(GtkWidget *widget, gpointer data)
{
	GtkTreeModel *model = NULL;
	GtkTreeIter iter;
	char* namedev;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data));	
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(GTK_TREE_VIEW(data)));
	if(gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gtk_tree_model_get (model, &iter, NAME_COLUMN, &namedev, -1);
	}

	requestformat(namedev);
}

//* Idem for swap *//
void set_swap_part(GtkWidget *widget, gpointer data)
{
	GtkTreeModel *model = NULL;
	GtkTreeIter iter;
	char* namedev;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data));	
	
	GtkWidget *cellview = gtk_cell_view_new ();	
	GdkPixbuf *pixbuf = gtk_widget_render_icon (cellview, GTK_STOCK_CONVERT, GTK_ICON_SIZE_BUTTON, NULL);
	gtk_widget_destroy (cellview);
	
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(GTK_TREE_VIEW(data)));	
	if(gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
		gchar *mount = g_strdup("swap");
		gint i = gtk_tree_path_get_indices (path)[0];	
		GList *point = g_list_nth  (parts, 4*i+3);
		gtk_tree_model_get (model, &iter, NAME_COLUMN, &namedev, -1);
		
		// Case if we want to change the root partition to a swap one
		if(point->data && !strcmp((char*)point->data,"/"))
			return;
		if(swapformat(namedev) == -1)
			return;
		// Free old pointer
		if(point->data)
			g_free(point->data);
		// add new
		point->data = mount;
		// update Treelist
		gtk_list_store_set (GTK_LIST_STORE (model), &iter, TYPE_COLUMN, pixbuf,MOUNT_COLUMN, mount,-1);
		// yeah we have a new swap partition
		nbswap++;
		// check if completed page conditions are verified
		if(rootpart && nbswap>0)
		{
			set_page_completed();
		}	
	}
	g_object_unref(pixbuf);	
}

void change_part_list(GtkComboBox *combo, gpointer view)
{
	char *selected;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gtk_combo_box_get_active_iter(combo, &iter);
	model = gtk_combo_box_get_model(combo);
	gtk_tree_model_get (model, &iter, TEXT_COL, &selected, -1);
	
	if (allparts)
		parts = (GList*)data_get(allparts, selected);
	
	update_treeview_list(view);
}

//* Load gtk widget for this plugin *//
GtkWidget *load_gtk_widget(GtkWidget *assist)
{
	GtkWidget *pVBox, *pHBox, *info;
	GtkWidget *diskinfo, *device, *separatorh1;	

	GList* devs= listdevs();		
	
	/* Drawing partition with first harddisk */
	//panel = draw_partition();
	pVBox = gtk_vbox_new(FALSE, 5);	
	
	info = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(info), "<span face=\"Courier New\"><b>This you can select main mountpoints launch gparted and format partitions and mabe many other things</b></span>\n");
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

	gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (comboparts), renderer, set_sensitive, NULL, NULL);
    
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (comboparts), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (comboparts), renderer,"text", TEXT_COL, NULL);

	gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (comboparts), renderer, set_sensitive, NULL, NULL);
		
	diskinfo = gtk_label_new((char*)g_list_nth_data(devs,1));
	gtk_box_pack_start (GTK_BOX (pHBox), diskinfo, FALSE, TRUE, 50);
	
	separatorh1 = gtk_hseparator_new();
	gtk_box_pack_start (GTK_BOX (pVBox), separatorh1, FALSE, FALSE, 5);	
	
	/* List of device partition with mountpoints and other*/
	
	GtkListStore *store;
	GtkTreeViewColumn *col;
	GtkWidget *view;
	GtkWidget *pScrollbar;
	GtkTreeSelection *selection;
	GtkWidget *hboxlist;
	
	hboxlist = gtk_hbox_new(TRUE, 5);
	gtk_box_pack_start (GTK_BOX (pVBox), hboxlist, TRUE, TRUE, 0);	
	
	store = gtk_list_store_new(NUM_COLUMN, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	model = GTK_TREE_MODEL(store);
	
	view = gtk_tree_view_new_with_model(model);
	g_object_unref (model);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(view), TRUE);
	
	// a small icon for swap and root partition
	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "pixbuf", TYPE_COLUMN, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	
	// partition name
	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", NAME_COLUMN, NULL);
	gtk_tree_view_column_set_title(col, "Partition");
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
		
	// partition size
	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", SIZE_COLUMN, NULL);
	gtk_tree_view_column_set_title(col, "Size");
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);	
	
	// current filesystem
	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", FS_COLUMN, NULL);
	gtk_tree_view_column_set_title(col, "Filesystem");
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	
	// mountpoint
	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	g_object_set(renderer, "editable", TRUE, NULL);
	g_signal_connect(renderer, "edited", G_CALLBACK (cell_edited), model);
	g_object_set_data(G_OBJECT (renderer), "column", GINT_TO_POINTER (MOUNT_COLUMN));
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", MOUNT_COLUMN, NULL);
	gtk_tree_view_column_set_title(col, "Mountpoint");
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);	

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
      
	pScrollbar = gtk_scrolled_window_new(NULL, NULL);
	
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pScrollbar), view);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pScrollbar), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_box_pack_start(GTK_BOX(hboxlist), pScrollbar, FALSE, TRUE, 0);
	
	GtkWidget *mainpart, *swappart, *format, *buttonlist;
	
	buttonlist = gtk_hbox_new(TRUE, 10);
	gtk_box_pack_start (GTK_BOX (pVBox), buttonlist, FALSE, FALSE, 10);
	
	GtkWidget *image;
	mainpart = gtk_button_new_with_label("Set as root partition");
	swappart = gtk_button_new_with_label("Set as swap partition");
	format = gtk_button_new_with_label("Format partition");
	image = gtk_image_new_from_stock (GTK_STOCK_DELETE, 3);
	gtk_button_set_image(GTK_BUTTON(format), image);
	image = gtk_image_new_from_stock (GTK_STOCK_HOME, 3);
	gtk_button_set_image(GTK_BUTTON(mainpart), image);
	//* connect button to the select root part function *//
	g_signal_connect (mainpart, "clicked",G_CALLBACK (set_root_part), view);
	image = gtk_image_new_from_stock (GTK_STOCK_CONVERT, 3);
	gtk_button_set_image(GTK_BUTTON(swappart), image);
	g_signal_connect (swappart, "clicked", G_CALLBACK (set_swap_part), view);
	g_signal_connect (format, "clicked", G_CALLBACK (set_format_part), view);
	g_signal_connect(G_OBJECT(comboparts), "changed", G_CALLBACK(change_part_list), view);	
	
	gtk_box_pack_start (GTK_BOX (buttonlist), mainpart, TRUE, FALSE, 10);
	gtk_box_pack_start (GTK_BOX (buttonlist), swappart, TRUE, FALSE, 10);
	gtk_box_pack_start (GTK_BOX (buttonlist), format, TRUE, FALSE, 10);
		
	return pVBox;
}

int prerun(GList **config)
{
	switch(fwife_question("Do you want to run gparted to create/modify partitions?"))
	{
		case GTK_RESPONSE_YES:
			fw_system_interactive("gparted");
			break;
		case GTK_RESPONSE_NO:
			break;
	}

	if(allparts == NULL)
	{
		detect_parts(0);
		gtk_combo_box_set_active (GTK_COMBO_BOX (comboparts), 0);
	}
	return 0;
}

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
				if(!strcmp(mount, "swap") && !strcmp(type, "linux-swap"))
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
 