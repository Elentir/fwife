/*
 *  netconf.c for Fwife
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

#include <gtk/gtk.h>
#include <stdio.h>
#include <net/if.h>
#include <glib.h>
#include <libfwutil.h>
#include <libfwnetconfig.h>
#include <getopt.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libintl.h>

#include "common.h"
#include "../util.h"

GtkWidget *intercombo;
GtkWidget *viewif;
GList *iflist=NULL;
GList *interfaceslist=NULL;

enum
{
	COLUMN_NET_IMAGE,
	COLUMN_NET_NAME,
	COLUMN_NET_IP,
	COLUMN_NET_NETMASK,
	COLUMN_NET_GATEWAY,
	COLUMN_NET_NAMESERV
};

plugin_t plugin =
{
	"netconf",
	desc,
	65,
	load_gtk_widget,
	GTK_ASSISTANT_PAGE_CONTENT,
	TRUE,
	NULL,
	prerun,
	run,
	NULL // dlopen handle
};

char *desc()
{
	return _("Configure network");
}

plugin_t *info()
{
	return &plugin;
}

void change_interface(GtkComboBox *combo, gpointer labelinfo)
{
	char *selected;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gtk_combo_box_get_active_iter(combo, &iter);
	model = gtk_combo_box_get_model(combo);
	gtk_tree_model_get (model, &iter, 0, &selected, -1);
	GList * elem = g_list_find_custom(iflist, (gconstpointer) selected, cmp_str);
	int i = g_list_position(iflist, elem);
	gtk_label_set_label(GTK_LABEL(labelinfo), (char*)g_list_nth_data(iflist, i+1));		
}

GtkWidget *getNettypeCombo()
{
	GtkWidget *combo;
	GtkTreeIter iter;
	GtkListStore *store;
	gint i;

	char *types[] =
	{
		"dhcp", _("Use a DHCP server"),
		"static", _("Use a static IP address"),
		"dsl", _("DSL connection"),
		"lo", _("No network - loopback connection only")
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
	

	for (i = 0; i < 8; i+=2)
	{
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, types[i], 1, types[i+1], -1);
	}

	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
	
	return combo;
}

char *ask_nettype()
{
	char *str = NULL;
	extern GtkWidget *assistant;
	GtkTreeIter iter;
	GtkTreeModel *model;
	
	GtkWidget *pBoite = gtk_dialog_new_with_buttons("Select network type",
			GTK_WINDOW(assistant),
			GTK_DIALOG_MODAL,
      			GTK_STOCK_OK,GTK_RESPONSE_OK,
       			NULL);

	GtkWidget *labelinfo = gtk_label_new(_("Now we need to know how your machine connects to the network.\n"
			"If you have an internal network card and an assigned IP address, gateway, and DNS, use 'static' "
			"to enter these values.\n You may also use this if you have a DSL connection.\n"
			"If your IP address is assigned by a DHCP server (commonly used by cable modem services), select 'dhcp'. \n"
			"If you just have a network card to connect directly to a DSL modem, then select 'dsl'. \n"
			"Finally, if you do not have a network card, select the 'lo' choice. \n"));
	
	GtkWidget *combotype = getNettypeCombo();
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), labelinfo, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), combotype, FALSE, FALSE, 5);

	/* Affichage des elements de la boite de dialogue */
	gtk_widget_show_all(GTK_DIALOG(pBoite)->vbox);

	/* On lance la boite de dialogue et on recupere la reponse */
	switch (gtk_dialog_run(GTK_DIALOG(pBoite)))
	{
		/* L utilisateur valide */
		case GTK_RESPONSE_OK:			
			gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combotype), &iter);
			model = gtk_combo_box_get_model(GTK_COMBO_BOX(combotype));
			gtk_tree_model_get (model, &iter, 0, &str, -1);
			break;
			/* L utilisateur annule */
		case GTK_RESPONSE_CANCEL:
		case GTK_RESPONSE_NONE:
		default:
			break;
	}

	/* Destruction de la boite de dialogue */
	gtk_widget_destroy(pBoite);
	return str;
}

int configure_wireless(fwnet_interface_t *interface)
{
	extern GtkWidget *assistant;
	GtkWidget *phboxtemp, *labeltemp;

	GtkWidget *pBoite = gtk_dialog_new_with_buttons("Configure Wireless",
			GTK_WINDOW(assistant),
			GTK_DIALOG_MODAL,
      			GTK_STOCK_OK,GTK_RESPONSE_OK,
       			NULL);

	GtkWidget *labelinfo = gtk_label_new(_("Now we need to know how your machine connects to the network.\n"
			"If you have an internal network card and an assigned IP address, gateway, and DNS, use 'static' "
			"to enter these values.\n You may also use this if you have a DSL connection.\n"
			"If your IP address is assigned by a DHCP server (commonly used by cable modem services), select 'dhcp'. \n"
			"If you just have a network card to connect directly to a DSL modem, then select 'dsl'. \n"
			"Finally, if you do not have a network card, select the 'lo' choice. \n"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), labelinfo, FALSE, FALSE, 5);
	
	phboxtemp = gtk_hbox_new(FALSE, 0);
	labeltemp = gtk_label_new("Essid : ");
	gtk_box_pack_start(GTK_BOX(phboxtemp), labeltemp, FALSE, FALSE, 5);
	GtkWidget *pEntryEssid = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(phboxtemp), pEntryEssid, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), phboxtemp, FALSE, FALSE, 5);

	phboxtemp = gtk_hbox_new(FALSE, 0);
	labeltemp = gtk_label_new("WEP encryption key : ");
	gtk_box_pack_start(GTK_BOX(phboxtemp), labeltemp, FALSE, FALSE, 5);
	GtkWidget *pEntryWepKey = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(phboxtemp), pEntryWepKey, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), phboxtemp, FALSE, FALSE, 5);

	phboxtemp = gtk_hbox_new(FALSE, 0);
	labeltemp = gtk_label_new("WPA encryption passphrase : ");
	gtk_box_pack_start(GTK_BOX(phboxtemp), labeltemp, FALSE, FALSE, 5);
	GtkWidget *pEntryWpaPass = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(phboxtemp), pEntryWpaPass, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), phboxtemp, FALSE, FALSE, 5);

	phboxtemp = gtk_hbox_new(FALSE, 0);
	labeltemp = gtk_label_new("WPA Driver : ");
	gtk_box_pack_start(GTK_BOX(phboxtemp), labeltemp, FALSE, FALSE, 5);
	GtkWidget *pEntryWpaDriver = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(phboxtemp), pEntryWpaDriver, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), phboxtemp, FALSE, FALSE, 5);
	
	/* Affichage des elements de la boite de dialogue */
	gtk_widget_show_all(GTK_DIALOG(pBoite)->vbox);

	/* On lance la boite de dialogue et on recupere la reponse */
	switch (gtk_dialog_run(GTK_DIALOG(pBoite)))
	{
		/* L utilisateur valide */
		case GTK_RESPONSE_OK:
			snprintf(interface->essid, FWNET_ESSID_MAX_SIZE, (char*)gtk_entry_get_text(GTK_ENTRY(pEntryEssid)));
			snprintf(interface->key, FWNET_ENCODING_TOKEN_MAX, (char*)gtk_entry_get_text(GTK_ENTRY(pEntryWepKey)));
			snprintf(interface->wpa_psk, PATH_MAX, (char*)gtk_entry_get_text(GTK_ENTRY(pEntryWpaPass)));
			snprintf(interface->wpa_driver, PATH_MAX, (char*)gtk_entry_get_text(GTK_ENTRY(pEntryWpaDriver)));
			break;
			/* L utilisateur annule */
		case GTK_RESPONSE_CANCEL:
		case GTK_RESPONSE_NONE:
		default:
			break;
	}

	/* Destruction de la boite de dialogue */
	gtk_widget_destroy(pBoite);
	return 0;
}

int configure_static(fwnet_interface_t *interface, GtkTreeIter iter)
{
	extern GtkWidget *assistant;
	GtkWidget *phboxtemp, *labeltemp;
	char option[50];
	char *ipaddr, *netmask, *gateway;

	GtkWidget *pBoite = gtk_dialog_new_with_buttons("Configure static network",
			GTK_WINDOW(assistant),
			GTK_DIALOG_MODAL,
      			GTK_STOCK_OK,GTK_RESPONSE_OK,
       			NULL);

	GtkWidget *labelinfo = gtk_label_new(_("vnbbbbbbbbbbbbddfgfcf"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), labelinfo, FALSE, FALSE, 5);
	
	phboxtemp = gtk_hbox_new(FALSE, 0);
	labeltemp = gtk_label_new("IP Adress : ");
	gtk_box_pack_start(GTK_BOX(phboxtemp), labeltemp, FALSE, FALSE, 5);
	GtkWidget *pEntryIP = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(phboxtemp), pEntryIP, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), phboxtemp, FALSE, FALSE, 5);

	phboxtemp = gtk_hbox_new(FALSE, 0);
	labeltemp = gtk_label_new("Network Mask : ");
	gtk_box_pack_start(GTK_BOX(phboxtemp), labeltemp, FALSE, FALSE, 5);
	GtkWidget *pEntryNetmask = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(phboxtemp), pEntryNetmask, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), phboxtemp, FALSE, FALSE, 5);

	phboxtemp = gtk_hbox_new(FALSE, 0);
	labeltemp = gtk_label_new("Gateway : ");
	gtk_box_pack_start(GTK_BOX(phboxtemp), labeltemp, FALSE, FALSE, 5);
	GtkWidget *pEntryGateway = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(phboxtemp), pEntryGateway, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), phboxtemp, FALSE, FALSE, 5);

	/* Affichage des elements de la boite de dialogue */
	gtk_widget_show_all(GTK_DIALOG(pBoite)->vbox);

	/* On lance la boite de dialogue et on recupere la reponse */
	switch (gtk_dialog_run(GTK_DIALOG(pBoite)))
	{
		/* L utilisateur valide */
		case GTK_RESPONSE_OK:			
			ipaddr = (char*)gtk_entry_get_text(GTK_ENTRY(pEntryIP));
			netmask = (char*)gtk_entry_get_text(GTK_ENTRY(pEntryNetmask));
			if(strlen(ipaddr))
				snprintf(option, 49, "%s netmask %s", ipaddr, netmask);
			interface->options = g_list_append(interface->options, strdup(option));
			
			gateway = (char*)gtk_entry_get_text(GTK_ENTRY(pEntryGateway));
			if(strlen(gateway))
				snprintf(interface->gateway, FWNET_GW_MAX_SIZE, "default gw %s", gateway);

			GtkTreeView *treeview = (GtkTreeView *)viewif;
  			GtkTreeModel *model = gtk_tree_view_get_model (treeview);
			gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_NET_IP, ipaddr,COLUMN_NET_NETMASK, netmask, COLUMN_NET_GATEWAY, gateway, -1);
			break;
			/* L utilisateur annule */
		case GTK_RESPONSE_CANCEL:
		case GTK_RESPONSE_NONE:
		default:
			break;
	}

	/* Destruction de la boite de dialogue */
	gtk_widget_destroy(pBoite);
	return 0;
}

int add_interface(GtkWidget *button, gpointer data)
{
	fwnet_interface_t *newinterface = NULL;
	GtkWidget *cellview;
	GdkPixbuf *connectimg;
	char *ptr=NULL;
	char *nettype=NULL;
	char *iface=NULL;

	GtkTreeIter iter;
	GtkTreeModel *model;
	gtk_combo_box_get_active_iter(GTK_COMBO_BOX(intercombo), &iter);
	model = gtk_combo_box_get_model(GTK_COMBO_BOX(intercombo));
	gtk_tree_model_get (model, &iter, 0, &iface, -1);
	
	if((newinterface = (fwnet_interface_t*)malloc(sizeof(fwnet_interface_t))) == NULL)
		return(-1);
	memset(newinterface, 0, sizeof(fwnet_interface_t));
	
	snprintf(newinterface->name, IF_NAMESIZE, iface);
	
	nettype = ask_nettype();
	if(nettype == NULL)
		return -1;

	if(strcmp(nettype, "lo"))
	{
		interfaceslist = g_list_append(interfaceslist, strdup(iface));
		interfaceslist = g_list_append(interfaceslist, newinterface);

		GtkTreeView *treeview = (GtkTreeView *)data;
  		model = gtk_tree_view_get_model (treeview);

		cellview = gtk_cell_view_new ();
	    	connectimg = gtk_widget_render_icon (cellview, GTK_STOCK_NETWORK,
					GTK_ICON_SIZE_BUTTON, NULL);
	   	gtk_widget_destroy (cellview);

		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_NET_IMAGE, connectimg, COLUMN_NET_NAME, iface, -1);		
	}

	if(strcmp(nettype, "lo") && fwnet_is_wireless_device(iface))
	{
		switch(fwife_question("Wireless extension detected : Configure wireless?"))
					{
						case GTK_RESPONSE_YES:
							configure_wireless(newinterface);
							break;
						case GTK_RESPONSE_NO:
							break;
					}
	}
	
	if(!strcmp(nettype, "dhcp"))
	{
		ptr = fwife_entry(_("Set DHCP hostname"), _("Some network providers require that the DHCP hostname be\n"
			"set in order to connect. If so, they'll have assigned a hostname to your machine.\n If you were"
			"assigned a DHCP hostname, please enter it below.\n If you do not have a DHCP hostname, just"
			"hit enter."), NULL);
		if(strlen(ptr))
			snprintf(newinterface->dhcp_opts, PATH_MAX, "-t 10 -h %s\n", ptr);
		else
			newinterface->dhcp_opts[0]='\0';
		newinterface->options = g_list_append(newinterface->options, strdup("dhcp"));
		gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_NET_IP, "dhcp", COLUMN_NET_NAMESERV, ptr, -1);
		FREE(ptr);
	}
	else if(!strcmp(nettype, "static"))
	{
		configure_static(newinterface, iter);
	}

	/*if(!strcmp(nettype, "static"))
		dsl_config(newprofile, 1);
	if(!strcmp(nettype, "dsl"))
		dsl_config(newprofile, 0);*/
	
	return 0;
}

void del_interface(GtkWidget *button, gpointer data)
{
	GtkTreeIter iter;
  	GtkTreeView *treeview = (GtkTreeView *)data;
  	GtkTreeModel *model = gtk_tree_view_get_model (treeview);
  	GtkTreeSelection *selection = gtk_tree_view_get_selection (treeview);
	char *nameif;

  	if (gtk_tree_selection_get_selected (selection, NULL, &iter))
        {
  	  gtk_tree_model_get (model, &iter, 1, &nameif, -1);
	  GList * elem = g_list_find_custom(iflist, (gconstpointer) nameif, cmp_str);
    	  gint i = g_list_position(iflist, elem); 
	  iflist =  g_list_delete_link (iflist, g_list_nth(iflist, i));
	  iflist =  g_list_delete_link (iflist, g_list_nth(iflist, i));
	 
     	  gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
    	}	
}


GtkWidget *load_gtk_widget()
{
	GtkWidget *pVBox, *pFrame, *pHBoxFrame, *pVBoxFrame, *phboxtemp, *labeltemp;
	GtkWidget *hboxview;
	GtkWidget *labelinfo, *labeldesc;
	
	pVBoxFrame = gtk_vbox_new(FALSE, 0);
	pHBoxFrame = gtk_hbox_new(FALSE, 0);
	pVBox = gtk_vbox_new(FALSE, 0);
	hboxview = gtk_hbox_new(FALSE, 0);
	
	GtkListStore *store;
	GtkTreeModel *model;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;
						
	store = gtk_list_store_new(6, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_PIXBUF);
	model = GTK_TREE_MODEL(store);
	
	viewif = gtk_tree_view_new_with_model(model);
	g_object_unref (model);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(viewif), TRUE);
	
	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "pixbuf", COLUMN_NET_IMAGE, NULL);
	gtk_tree_view_column_set_title(col, "");
	gtk_tree_view_append_column(GTK_TREE_VIEW(viewif), col);

	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", COLUMN_NET_NAME, NULL);
	gtk_tree_view_column_set_title(col, "Device");
	gtk_tree_view_append_column(GTK_TREE_VIEW(viewif), col);
	
	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", COLUMN_NET_IP, NULL);
	gtk_tree_view_column_set_title(col, "Ip Address");
	gtk_tree_view_append_column(GTK_TREE_VIEW(viewif), col);

	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", COLUMN_NET_NETMASK, NULL);
	gtk_tree_view_column_set_title(col, "Netmask");
	gtk_tree_view_append_column(GTK_TREE_VIEW(viewif), col);

	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", COLUMN_NET_GATEWAY, NULL);
	gtk_tree_view_column_set_title(col, "Gateway");
	gtk_tree_view_append_column(GTK_TREE_VIEW(viewif), col);

	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", COLUMN_NET_NAMESERV, NULL);
	gtk_tree_view_column_set_title(col, "DHCP Nameserver");
	gtk_tree_view_append_column(GTK_TREE_VIEW(viewif), col);
	
	gtk_box_pack_start(GTK_BOX(hboxview), viewif, TRUE, TRUE, 10);
	gtk_box_pack_start(GTK_BOX(pVBox), hboxview, TRUE, TRUE, 10);
	
	pFrame = gtk_frame_new("Network configuration");
	gtk_container_add(GTK_CONTAINER(pFrame), pVBoxFrame);
	gtk_box_pack_start(GTK_BOX(pVBox), pFrame, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(pVBoxFrame), pHBoxFrame, FALSE, FALSE, 12);
	
	labeldesc = gtk_label_new("Interface :");
	gtk_box_pack_start(GTK_BOX(pHBoxFrame), labeldesc, FALSE, FALSE, 5);
	
	intercombo = gtk_combo_box_new_text();
	gtk_box_pack_start(GTK_BOX(pHBoxFrame), intercombo, TRUE, TRUE, 5);
	
	labelinfo = gtk_label_new("");
    	gtk_box_pack_start(GTK_BOX(pHBoxFrame), labelinfo, TRUE, TRUE, 0);
	
	phboxtemp = gtk_hbox_new(FALSE, 0);
	labeltemp = gtk_label_new("Network configuration :   ");
	gtk_box_pack_start(GTK_BOX(phboxtemp), labeltemp, FALSE, FALSE, 5);
	GtkWidget *btnsave = gtk_button_new_from_stock (GTK_STOCK_ADD);
	gtk_box_pack_start(GTK_BOX(phboxtemp), btnsave, FALSE, FALSE, 10);
	GtkWidget *btndel = gtk_button_new_from_stock (GTK_STOCK_REMOVE);
	gtk_box_pack_start(GTK_BOX(phboxtemp), btndel, FALSE, FALSE, 10);
	
	gtk_frame_set_label_widget(GTK_FRAME(pFrame), phboxtemp);
	
	g_signal_connect(G_OBJECT(intercombo), "changed", G_CALLBACK(change_interface), labelinfo);
	g_signal_connect(G_OBJECT(btnsave), "clicked", G_CALLBACK(add_interface), viewif);
	g_signal_connect(G_OBJECT(btndel), "clicked", G_CALLBACK(del_interface), viewif);
		
	
	return pVBox;	
}

int prerun(GList **config)
{
	int i;
	fwutil_i18ninit(__FILE__);
	if(iflist == NULL)
	{
		iflist = fwnet_iflist();

		for(i=0; i<g_list_length(iflist); i+=2)
		{
			gtk_combo_box_append_text(GTK_COMBO_BOX(intercombo), (char*)g_list_nth_data(iflist, i));
		}
	}
	
	gtk_combo_box_set_active (GTK_COMBO_BOX (intercombo), 0);
		
	return 0;
}

int run(GList **config)
{
	fwnet_profile_t *newprofile=NULL;
	int i;	
	chroot(TARGETDIR);
	if((newprofile = (fwnet_profile_t*)malloc(sizeof(fwnet_profile_t))) == NULL)
		return(1);
	memset(newprofile, 0, sizeof(fwnet_profile_t));
	
	sprintf(newprofile->name, "essai");
	for(i = 1; i<g_list_length(interfaceslist); i+=2)
	{
		newprofile->interfaces = g_list_append(newprofile->interfaces, (fwnet_interface_t *) g_list_nth_data(interfaceslist, i));
	}
	char *host = fwife_entry("Hostname", "Enter computer hostname :", NULL);
	fwnet_writeconfig(newprofile, host);
	FREE(host);
	return 0;
}
 
