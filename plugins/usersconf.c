/*
 *  userconf.c for Fwife
 * 
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

#include "common.h"

static GtkWidget *rootpass, *rootverify;
static GtkWidget *userorigimg;

enum
{
	COLUMN_USR_IMAGE,
 	COLUMN_USR_NAME,
 	COLUMN_USR_FULLNAME,
 	COLUMN_USR_SHELL,
 	COLUMN_USR_HOME,
 	COLUMN_USR_PASS
};

plugin_t plugin =
{
	"usersconf",
	desc,
	55,
	load_gtk_widget,
	GTK_ASSISTANT_PAGE_CONTENT,
	TRUE,
 	NULL,
	NULL,
	run,
	NULL // dlopen handle
};

char *desc()
{
	return _("Configure users");
}

plugin_t *info()
{
	return &plugin;
}

void remove_user (GtkWidget *widget, gpointer data)
{
	GtkTreeIter iter;
  	GtkTreeView *treeview = (GtkTreeView *)data;
  	GtkTreeModel *model = gtk_tree_view_get_model (treeview);
  	GtkTreeSelection *selection = gtk_tree_view_get_selection (treeview);
	char *old_name, *ptr;
	
  	if (gtk_tree_selection_get_selected (selection, NULL, &iter))
        {
		gtk_tree_model_get (model, &iter, COLUMN_USR_NAME, &old_name, -1);
		ptr = g_strdup_printf("chroot %s /usr/sbin/userdel %s", TARGETDIR, old_name);
		if(fw_system(ptr) != 0)
		{
			fwife_error(_("User can be deleted!"));
		}
		FREE(ptr);
	  	
     	  	gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
    	}
    return;
}

void add_user (GtkWidget *widget, gpointer data)
{
	GtkWidget* pBoite;
  	GtkWidget *pEntryPass, *pVerifyPass, *pEntryName, *pEntryHome, *pEntryShell, *pEntryFn;
  	char* sName, *sFn, *sPass, *sVerify, *sShell, *sHome;
	char* ptr = NULL;

	GtkWidget *pVBox;
    	GtkWidget *pFrame;
    	GtkWidget *pVBoxFrame;
    	GtkWidget *pLabel;
	
	extern GtkWidget *assistant;

	GtkTreeIter iter;
  	GtkTreeView *treeview = (GtkTreeView *)data;
  	GtkTreeModel *model = gtk_tree_view_get_model (treeview);
	GdkPixbuf *userimg, *passimg;
	GtkWidget *cellview;

  	pBoite = gtk_dialog_new_with_buttons(_("Add a new user"),
      		  NULL,
      		  GTK_DIALOG_MODAL,
      		  GTK_STOCK_OK,GTK_RESPONSE_OK,
      		  GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,
      		  NULL);
	gtk_window_set_transient_for(GTK_WINDOW(pBoite), GTK_WINDOW(assistant));
	gtk_window_set_position(GTK_WINDOW(pBoite), GTK_WIN_POS_CENTER_ON_PARENT);
   	
	pVBox = gtk_vbox_new(TRUE, 0);
   	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), pVBox, TRUE, FALSE, 5);	

    	/* Creation du premier GtkFrame */
    	pFrame = gtk_frame_new(_("General Configuration"));
    gtk_box_pack_start(GTK_BOX(pVBox), pFrame, TRUE, FALSE, 0);

    /* Creation et insertion d une boite pour le premier GtkFrame */
    pVBoxFrame = gtk_vbox_new(TRUE, 0);
    gtk_container_add(GTK_CONTAINER(pFrame), pVBoxFrame);   

    /* Creation et insertion des elements contenus dans le premier GtkFrame */
    pLabel = gtk_label_new(_("Name :"));
    gtk_box_pack_start(GTK_BOX(pVBoxFrame), pLabel, TRUE, FALSE, 0);
    pEntryName = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(pVBoxFrame), pEntryName, TRUE, FALSE, 0);
    
    pLabel = gtk_label_new(_("Full Name :"));
    gtk_box_pack_start(GTK_BOX(pVBoxFrame), pLabel, TRUE, FALSE, 0);
    pEntryFn = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(pVBoxFrame), pEntryFn, TRUE, FALSE, 0);

    /* Creation du deuxieme GtkFrame */
    pFrame = gtk_frame_new(_("Password : "));
    gtk_box_pack_start(GTK_BOX(pVBox), pFrame, TRUE, FALSE, 0);

    /* Creation et insertion d une boite pour le deuxieme GtkFrame */
    pVBoxFrame = gtk_vbox_new(TRUE, 0);
    gtk_container_add(GTK_CONTAINER(pFrame), pVBoxFrame);

    /* Creation et insertion des elements contenus dans le deuxieme GtkFrame */
    pLabel = gtk_label_new(_("Enter password :"));
    gtk_box_pack_start(GTK_BOX(pVBoxFrame), pLabel, TRUE, FALSE, 0);
    pEntryPass = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(pEntryPass), FALSE);
    gtk_box_pack_start(GTK_BOX(pVBoxFrame), pEntryPass, TRUE, FALSE, 0);

    pLabel = gtk_label_new(_("Re-enter password :"));
    gtk_box_pack_start(GTK_BOX(pVBoxFrame), pLabel, TRUE, FALSE, 0);
    pVerifyPass = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(pVerifyPass), FALSE);
    gtk_box_pack_start(GTK_BOX(pVBoxFrame), pVerifyPass, TRUE, FALSE, 0);

    /* Creation du troisieme GtkFrame */
    pFrame = gtk_frame_new(_("Other (optionnal)"));
    gtk_box_pack_start(GTK_BOX(pVBox), pFrame, TRUE, FALSE, 5);

    /* Creation et insertion d une boite pour le troisieme GtkFrame */
    pVBoxFrame = gtk_vbox_new(TRUE, 0);
    gtk_container_add(GTK_CONTAINER(pFrame), pVBoxFrame);

    /* Creation et insertion des elements contenus dans le troisieme GtkFrame */
    pLabel = gtk_label_new(_("Shell"));
    gtk_box_pack_start(GTK_BOX(pVBoxFrame), pLabel, TRUE, FALSE, 0);
    pEntryShell = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(pVBoxFrame), pEntryShell, TRUE, FALSE, 0);

    pLabel = gtk_label_new(_("Home directory"));
    gtk_box_pack_start(GTK_BOX(pVBoxFrame), pLabel, TRUE, FALSE, 0);
    pEntryHome = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(pVBoxFrame), pEntryHome, TRUE, FALSE, 0);
    
    gtk_widget_show_all(pBoite);

    switch (gtk_dialog_run(GTK_DIALOG(pBoite)))
    {
        case GTK_RESPONSE_OK:
            sName = (char*)gtk_entry_get_text(GTK_ENTRY(pEntryName));
	    sFn = (char*)gtk_entry_get_text(GTK_ENTRY(pEntryFn));
	    sPass = (char*)gtk_entry_get_text(GTK_ENTRY(pEntryPass));
	    sVerify = (char*)gtk_entry_get_text(GTK_ENTRY(pVerifyPass));
	    sShell = (char*)gtk_entry_get_text(GTK_ENTRY(pEntryShell));
	    sHome = (char*)gtk_entry_get_text(GTK_ENTRY(pEntryHome));
	    if(strcmp(sPass, sVerify))
	    {
		    gtk_widget_destroy(pBoite);
		    fwife_error(_("Password verification failed! Please try again."));
		    return;
	    }
	   
	    //* Add user into the system *//
	    if(!strlen(sShell) && !strlen(sHome))
		    ptr = g_strdup_printf("chroot %s /usr/sbin/useradd '%s'", TARGETDIR, sName);
	    else if(!strlen(sShell))
		    ptr = g_strdup_printf("chroot %s /usr/sbin/useradd -d '%s' -m '%s'", TARGETDIR, sHome, sName);
	    else if(!strlen(sHome))
		    ptr = g_strdup_printf("chroot %s /usr/sbin/useradd -s '%s' -m '%s'", TARGETDIR, sShell, sName);
	    else
		    ptr = g_strdup_printf("chroot %s /usr/sbin/useradd -d '%s' -s '%s' -m '%s'", TARGETDIR, sHome, sShell, sName);
	
	    if(fw_system(ptr) != 0)
	    {
		    fwife_error(_("A user can be added! Check that you use lowercase name or that there is not an other user bearing the same name."));
		    FREE(ptr);
		    gtk_widget_destroy(pBoite);
		    return;
	    }
	    FREE(ptr);
	   
	    if(strlen(sPass))
	    {
		    ptr = g_strdup_printf("echo '%s:%s' |chroot %s /usr/sbin/chpasswd", sName, sPass, TARGETDIR);
		    fw_system(ptr);
		    FREE(ptr);
	    }
	    
	    if(strlen(sFn))
	    {
		    ptr = g_strdup_printf("chroot %s chfn -f '%s' '%s'", TARGETDIR, sFn, sName);
		    fw_system(ptr);
		    FREE(ptr);
	    }
	    
	    //* Adding new user to the list *//
            gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	    userimg = gtk_image_get_pixbuf (GTK_IMAGE(userorigimg));
	    cellview = gtk_cell_view_new ();
	    passimg = gtk_widget_render_icon (cellview, GTK_STOCK_DIALOG_AUTHENTICATION,
					GTK_ICON_SIZE_BUTTON, NULL);
	    gtk_widget_destroy (cellview);
	    if(!strcmp(sPass, ""))
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
			COLUMN_USR_IMAGE, userimg, COLUMN_USR_NAME, sName, COLUMN_USR_FULLNAME, sFn, COLUMN_USR_SHELL, sShell, COLUMN_USR_HOME, sHome, COLUMN_USR_PASS, NULL, -1);
	    else
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
				    COLUMN_USR_IMAGE, userimg, COLUMN_USR_NAME, sName, COLUMN_USR_FULLNAME, sFn, COLUMN_USR_SHELL, sShell, COLUMN_USR_HOME, sHome, COLUMN_USR_PASS, passimg, -1);
	   g_object_unref (passimg);
	   g_object_unref (userimg);
	   
	   gtk_widget_destroy(pBoite);
           break;
        case GTK_RESPONSE_CANCEL:
        case GTK_RESPONSE_NONE:
        default:
	gtk_widget_destroy(pBoite);
        break;
    }
}

//* Verify if password are correct *//
void verify_password(GtkWidget *widget, gpointer data)
{
	char *pass1 = (char*)gtk_entry_get_text(GTK_ENTRY(widget));
	char *pass2 = (char*)gtk_entry_get_text(GTK_ENTRY(data));
	if(!strcmp(pass1, pass2))
		set_page_completed();
	else
		set_page_incompleted();
}
		

GtkWidget *load_gtk_widget()
{
	GtkWidget *hbox, *vboxp, *button, *info;	
	//* For root entry end of page *//
	GtkWidget *hboxroot1, *hboxroot2;
	GtkWidget *rootlabel, *verifylabel;
	
	userorigimg = gtk_image_new_from_file("images/user.png");

	GtkListStore *store;
	GtkTreeModel *model;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;
	GtkWidget *view;
	GtkWidget *pScrollbar;
	GtkTreeSelection *selection;
	
	// main vbox
	vboxp = gtk_vbox_new(FALSE, 5);
	hbox = gtk_hbox_new(FALSE, 5);	

	info = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(info), _("<span face=\"Courier New\"><b>Creating user accounts and set root password</b></span>\n"));	

	store = gtk_list_store_new(6, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_PIXBUF);
	model = GTK_TREE_MODEL(store);
	
	view = gtk_tree_view_new_with_model(model);
	g_object_unref (model);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(view), TRUE);

	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "pixbuf", COLUMN_USR_IMAGE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	
	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", COLUMN_USR_NAME, NULL);
	gtk_tree_view_column_set_title(col, _("User"));
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	
	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", COLUMN_USR_FULLNAME, NULL);
	gtk_tree_view_column_set_title(col, _("Full Name"));
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", COLUMN_USR_SHELL, NULL);
	gtk_tree_view_column_set_title(col, _("Shell"));
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", COLUMN_USR_HOME, NULL);
	gtk_tree_view_column_set_title(col, _("Home"));
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(col, renderer, FALSE);
	gtk_tree_view_column_set_attributes(col, renderer, "pixbuf", COLUMN_USR_PASS, NULL);
	gtk_tree_view_column_set_title(col, _("Password"));
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
        gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	
	pScrollbar = gtk_scrolled_window_new(NULL, NULL);
	
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pScrollbar), view);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pScrollbar), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);

	gtk_box_pack_start (GTK_BOX (hbox), pScrollbar, TRUE, TRUE, 5);

	button = gtk_button_new_from_stock (GTK_STOCK_ADD);
        g_signal_connect (button, "clicked",
                        G_CALLBACK (add_user), view);
        gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 5);

        button = gtk_button_new_from_stock (GTK_STOCK_REMOVE);
        g_signal_connect (button, "clicked",
                        G_CALLBACK (remove_user), view);
        gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 5);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 5);
	gtk_box_pack_start (GTK_BOX (vboxp), info, FALSE, FALSE, 5);
	gtk_box_pack_start (GTK_BOX (vboxp), hbox, TRUE, TRUE, 5);	
	
	hboxroot1 = gtk_hbox_new(FALSE, 5);
	hboxroot2 = gtk_hbox_new(FALSE, 5);
	rootlabel = gtk_label_new(_("Enter root password :  "));
	verifylabel = gtk_label_new(_("Re-enter root password :  "));	
	
	rootpass = gtk_entry_new();
	gtk_entry_set_visibility(GTK_ENTRY(rootpass), FALSE);
	rootverify = gtk_entry_new();
	gtk_entry_set_visibility(GTK_ENTRY(rootverify), FALSE);
	g_signal_connect (rootverify, "changed",
                        G_CALLBACK (verify_password), rootpass);
	g_signal_connect (rootpass, "changed",
                        G_CALLBACK (verify_password), rootverify);

	gtk_box_pack_start (GTK_BOX (hboxroot1), rootlabel, FALSE, FALSE, 5);
	gtk_box_pack_start (GTK_BOX (hboxroot2), verifylabel, FALSE, FALSE, 5);
	gtk_box_pack_start (GTK_BOX (hboxroot1), rootpass, FALSE, FALSE, 5);
	gtk_box_pack_start (GTK_BOX (hboxroot2), rootverify, FALSE, FALSE, 5);	
	
	gtk_box_pack_start (GTK_BOX (vboxp), hboxroot1, FALSE, FALSE, 5);
	gtk_box_pack_start (GTK_BOX (vboxp), hboxroot2, FALSE, FALSE, 5);
	
	return vboxp;
}

int run(GList **config)
{
	char *ptr = NULL, *pass;
	
	//* Set root password *//
	pass = strdup((char*)gtk_entry_get_text(GTK_ENTRY(rootpass)));
	if(strlen(pass))
	{
		ptr = g_strdup_printf("echo '%s:%s' |chroot %s /usr/sbin/chpasswd", "root", pass, TARGETDIR);
		fw_system(ptr);
		FREE(ptr);
	}
	FREE(pass);
	return 0;
}
 
