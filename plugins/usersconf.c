#include <stdio.h>
#include <gtk/gtk.h>
#include<string.h>

#include "common.h"
#include "../util.h"

GList *userslist=NULL;
GtkWidget *rootpass, *rootverify;
GtkWidget *userorigimg;

plugin_t plugin =
{
	"usersconf",
	desc,
	55,
	load_gtk_widget,
	GTK_ASSISTANT_PAGE_CONTENT,
	TRUE,
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

int cmp_users(gconstpointer a, gconstpointer b)
{
	return(strcmp(a, b));
}

void remove_user (GtkWidget *widget, gpointer data)
{
	GtkTreeIter iter;
  	GtkTreeView *treeview = (GtkTreeView *)data;
  	GtkTreeModel *model = gtk_tree_view_get_model (treeview);
  	GtkTreeSelection *selection = gtk_tree_view_get_selection (treeview);
	char *old_name;

  	if (gtk_tree_selection_get_selected (selection, NULL, &iter))
        {
  	  gtk_tree_model_get (model, &iter, 0, &old_name, -1);
	  GList * elem = g_list_find_custom(userslist, (gconstpointer) old_name, cmp_users);
    	  gint i = g_list_position(userslist, elem); 
	  userslist =  g_list_delete_link (userslist, g_list_nth(userslist, i));
	  userslist =  g_list_delete_link (userslist, g_list_nth(userslist, i));
	  userslist =  g_list_delete_link (userslist, g_list_nth(userslist, i));
	  userslist =  g_list_delete_link (userslist, g_list_nth(userslist, i));
     	  gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
    	}
    return;
}

void add_user (GtkWidget *widget, gpointer data)
{
	GtkWidget* pBoite;
  	GtkWidget *pEntryPass, *pVerifyPass, *pEntryName, *pEntryHome, *pEntryShell;
  	char* sName, *sPass, *sVerify, *sShell, *sHome;

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
    	pFrame = gtk_frame_new("General Configuration");
    gtk_box_pack_start(GTK_BOX(pVBox), pFrame, TRUE, FALSE, 0);

    /* Creation et insertion d une boite pour le premier GtkFrame */
    pVBoxFrame = gtk_vbox_new(TRUE, 0);
    gtk_container_add(GTK_CONTAINER(pFrame), pVBoxFrame);   

    /* Creation et insertion des elements contenus dans le premier GtkFrame */
    pLabel = gtk_label_new("Name :");
    gtk_box_pack_start(GTK_BOX(pVBoxFrame), pLabel, TRUE, FALSE, 0);
    pEntryName = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(pVBoxFrame), pEntryName, TRUE, FALSE, 0);

    /* Creation du deuxieme GtkFrame */
    pFrame = gtk_frame_new("Password");
    gtk_box_pack_start(GTK_BOX(pVBox), pFrame, TRUE, FALSE, 0);

    /* Creation et insertion d une boite pour le deuxieme GtkFrame */
    pVBoxFrame = gtk_vbox_new(TRUE, 0);
    gtk_container_add(GTK_CONTAINER(pFrame), pVBoxFrame);

    /* Creation et insertion des elements contenus dans le deuxieme GtkFrame */
    pLabel = gtk_label_new("Enter password :");
    gtk_box_pack_start(GTK_BOX(pVBoxFrame), pLabel, TRUE, FALSE, 0);
    pEntryPass = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(pEntryPass), FALSE);
    gtk_box_pack_start(GTK_BOX(pVBoxFrame), pEntryPass, TRUE, FALSE, 0);

    pLabel = gtk_label_new("Re-enter password :");
    gtk_box_pack_start(GTK_BOX(pVBoxFrame), pLabel, TRUE, FALSE, 0);
    pVerifyPass = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(pVerifyPass), FALSE);
    gtk_box_pack_start(GTK_BOX(pVBoxFrame), pVerifyPass, TRUE, FALSE, 0);

    /* Creation du troisieme GtkFrame */
    pFrame = gtk_frame_new("Other (optionnal)");
    gtk_box_pack_start(GTK_BOX(pVBox), pFrame, TRUE, FALSE, 5);

    /* Creation et insertion d une boite pour le troisieme GtkFrame */
    pVBoxFrame = gtk_vbox_new(TRUE, 0);
    gtk_container_add(GTK_CONTAINER(pFrame), pVBoxFrame);

    /* Creation et insertion des elements contenus dans le troisieme GtkFrame */
    pLabel = gtk_label_new("Shell");
    gtk_box_pack_start(GTK_BOX(pVBoxFrame), pLabel, TRUE, FALSE, 0);
    pEntryShell = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(pVBoxFrame), pEntryShell, TRUE, FALSE, 0);

    pLabel = gtk_label_new("Home directory");
    gtk_box_pack_start(GTK_BOX(pVBoxFrame), pLabel, TRUE, FALSE, 0);
    pEntryHome = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(pVBoxFrame), pEntryHome, TRUE, FALSE, 0);
    
    gtk_widget_show_all(pBoite);

    switch (gtk_dialog_run(GTK_DIALOG(pBoite)))
    {
        case GTK_RESPONSE_OK:
            sName = (char*)gtk_entry_get_text(GTK_ENTRY(pEntryName));
	    sPass = (char*)gtk_entry_get_text(GTK_ENTRY(pEntryPass));
	    sVerify = (char*)gtk_entry_get_text(GTK_ENTRY(pVerifyPass));
	    sShell = (char*)gtk_entry_get_text(GTK_ENTRY(pEntryShell));
	    sHome = (char*)gtk_entry_get_text(GTK_ENTRY(pEntryHome));
	    if(strcmp(sPass, sVerify))
	    {
		    gtk_widget_destroy(pBoite);
		    fwife_error(_("Password verification incorrect"));
		    return;
	    }
	    if(g_list_find_custom(userslist, (gconstpointer) sName, cmp_users) != NULL)
	    {
		    gtk_widget_destroy(pBoite);
		    fwife_error(_("A user get the same name"));
		    return;
	    }
            gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	    userimg = gtk_image_get_pixbuf (GTK_IMAGE(userorigimg));
	    cellview = gtk_cell_view_new ();
	    passimg = gtk_widget_render_icon (cellview, GTK_STOCK_DIALOG_AUTHENTICATION,
					GTK_ICON_SIZE_BUTTON, NULL);
	    gtk_widget_destroy (cellview);
	    if(!strcmp(sPass, ""))
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                     0, userimg, 1, sName, 2, sShell, 3, sHome, 4, NULL, -1);
	    else
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                      0, userimg, 1, sName, 2, sShell, 3, sHome, 4, passimg, -1);
	   g_object_unref (passimg);
	   g_object_unref (userimg);
	   userslist = g_list_append(userslist, strdup(sName));
	   userslist = g_list_append(userslist, strdup(sShell));
	   userslist = g_list_append(userslist, strdup(sHome));
	   userslist = g_list_append(userslist, strdup(sPass));	
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
		

GtkWidget *load_gtk_widget(GtkWidget *assist)
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

	vboxp = gtk_vbox_new(FALSE, 5);
	hbox = gtk_hbox_new(FALSE, 5);	

	info = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(info), "<span face=\"Courier New\"><b>A litle bit help</b></span>\n");	

	store = gtk_list_store_new(5, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_PIXBUF);
	model = GTK_TREE_MODEL(store);
	
	view = gtk_tree_view_new_with_model(model);
	g_object_unref (model);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(view), TRUE);

	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "pixbuf", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	
	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", 1, NULL);
	gtk_tree_view_column_set_title(col, "User");
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", 2, NULL);
	gtk_tree_view_column_set_title(col, "Shell");
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", 3, NULL);
	gtk_tree_view_column_set_title(col, "Home");
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(col, renderer, FALSE);
	gtk_tree_view_column_set_attributes(col, renderer, "pixbuf", 4, NULL);
	gtk_tree_view_column_set_title(col, "Password");
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
        gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	
	pScrollbar = gtk_scrolled_window_new(NULL, NULL);
	
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pScrollbar), view);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pScrollbar), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	GtkWidget *vbox = gtk_vbox_new(FALSE, 5);

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
	rootlabel = gtk_label_new("Enter root password :  ");
	verifylabel = gtk_label_new("Re-enter root password :  ");	
	
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
	int i = 0;
	char *user, *shell, *home, *pass, *ptr;
	for(i=0; i<g_list_length(userslist); i+=4)
	{
		user = (char*)g_list_nth_data(userslist, i);
		shell = (char*)g_list_nth_data(userslist, i+1);
		home = (char*)g_list_nth_data(userslist, i+2);
		pass = (char*)g_list_nth_data(userslist, i+3);
		if(strlen(pass))
		{
			if(!strlen(shell) && !strlen(home))
				ptr = g_strdup_printf("chroot ./ /usr/sbin/useradd -p%s %s", pass, user);
			else if(!strlen(shell))
				ptr = g_strdup_printf("chroot ./ /usr/sbin/useradd -d %s -p%s -m %s", home, pass, user);
			else if(!strlen(home))
				ptr = g_strdup_printf("chroot ./ /usr/sbin/useradd -s %s -p%s -m %s", shell, pass, user);
			else
				ptr = g_strdup_printf("chroot ./ /usr/sbin/useradd -d %s -s %s -p%s -m %s", home, shell, pass, user);
		}
		else
		{
			if(!strlen(shell) && !strlen(home))
				ptr = g_strdup_printf("chroot ./ /usr/sbin/useradd %s", user);
			else if(!strlen(shell))
				ptr = g_strdup_printf("chroot ./ /usr/sbin/useradd -d %s -m %s", home, user);
			else if(!strlen(home))
				ptr = g_strdup_printf("chroot ./ /usr/sbin/useradd -s %s -m %s", shell, user);
			else
				ptr = g_strdup_printf("chroot ./ /usr/sbin/useradd -d %s -s %s -m %s", home, shell, user);
		}
		if(fw_system(ptr) != 0)
			fwife_error(_("A user can be added"));
		FREE(ptr);		
	}
		
	pass = (char*)gtk_entry_get_text(GTK_ENTRY(rootpass));
	if(!strcmp(pass, ""))
		ptr = g_strdup_printf("echo %s:%s |chroot ./ /usr/sbin/chpasswd", "root", pass);
	fw_system(ptr);
	FREE(ptr);	
	return 0;
}
 
