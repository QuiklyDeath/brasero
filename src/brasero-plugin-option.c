/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * brasero
 * Copyright (C) Philippe Rouquier 2007 <bonfire-app@wanadoo.fr>
 * 
 * brasero is free software.
 * 
 * You may redistribute it and/or modify it under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * brasero is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with brasero.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>

#include <gtk/gtkdialog.h>
#include <gtk/gtkbox.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkspinbutton.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkcheckbutton.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkframe.h>

#include <gconf/gconf-client.h>

#include "burn-plugin.h"
#include "burn-plugin-private.h"
#include "brasero-plugin-option.h"

typedef struct _BraseroPluginOptionPrivate BraseroPluginOptionPrivate;
struct _BraseroPluginOptionPrivate
{
	GSList *widgets;
	GtkWidget *title;
	GtkWidget *vbox;
};

struct _BraseroPluginOptionWidget {
	GtkWidget *widget;
	GtkWidget *sensitive;
	BraseroPluginConfOption *option;

	GSList *suboptions;
};
typedef struct _BraseroPluginOptionWidget BraseroPluginOptionWidget;

#define BRASERO_PLUGIN_OPTION_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BRASERO_TYPE_PLUGIN_OPTION, BraseroPluginOptionPrivate))

G_DEFINE_TYPE (BraseroPluginOption, brasero_plugin_option, GTK_TYPE_DIALOG);

void
brasero_plugin_option_save_settings (BraseroPluginOption *self)
{
	GSList *iter;
	GConfClient *client;
	BraseroPluginOptionPrivate *priv;

	priv = BRASERO_PLUGIN_OPTION_PRIVATE (self);

	client = gconf_client_get_default ();

	for (iter = priv->widgets; iter; iter = iter->next) {
		BraseroPluginOptionWidget *widget;
		BraseroPluginConfOptionType type;
		const gchar *value_str;
		gboolean value_bool;
		gint value_int;
		gchar *key;

		widget = iter->data;
		if (!GTK_WIDGET_IS_SENSITIVE (widget->widget))
			continue;

		brasero_plugin_conf_option_get_info (widget->option,
						     &key,
						     NULL,
						     &type);
		switch (type) {
		case BRASERO_PLUGIN_OPTION_BOOL:
			value_bool = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget->widget));
			gconf_client_set_bool (client, key, value_bool, NULL);
			break;

		case BRASERO_PLUGIN_OPTION_INT:
			value_int = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (widget->widget));
			gconf_client_set_int (client, key, value_int, NULL);
			break;

		case BRASERO_PLUGIN_OPTION_STRING:
			value_str = gtk_entry_get_text (GTK_ENTRY (widget->widget));
			gconf_client_set_string (client, key, value_str, NULL);
			break;

		default:
			break;
		}

		g_free (key);
	}

	g_object_unref (client);
}

static void
brasero_plugin_option_set_toggle_slaves (BraseroPluginOption *self,
					 BraseroPluginOptionWidget *option,
					 gboolean active)
{
	BraseroPluginOptionPrivate *priv;
	GSList *iter;

	priv = BRASERO_PLUGIN_OPTION_PRIVATE (self);

	for (iter = option->suboptions; iter; iter = iter->next) {
		BraseroPluginOptionWidget *suboption;

		suboption = iter->data;
		gtk_widget_set_sensitive (suboption->sensitive, active);
	}
}

static void
brasero_plugin_option_toggled_changed (GtkWidget *button,
				       BraseroPluginOption *self)
{
	BraseroPluginOptionPrivate *priv;
	gboolean active;
	GSList *iter;

	priv = BRASERO_PLUGIN_OPTION_PRIVATE (self);

	active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
	for (iter = priv->widgets; iter; iter = iter->next) {
		BraseroPluginOptionWidget *option;

		option = iter->data;
		if (option->widget == button) {
			brasero_plugin_option_set_toggle_slaves (self,
								 option,
								 active);
			break;
		}
	}
}

static BraseroPluginOptionWidget *
brasero_plugin_option_add_conf_widget (BraseroPluginOption *self,
				       BraseroPluginConfOption *option,
				       GtkBox *container)
{
	BraseroPluginOptionPrivate *priv;
	BraseroPluginOptionWidget *info;
	GSList *suboptionsw = NULL;
	GConfClient *client;
	gboolean value_bool;
	gchar *value_str;
	gint value_int;
	GSList *suboptions;
	GtkWidget *widget;
	GtkWidget *label;
	GtkWidget *hbox;
	GtkWidget *box;
	BraseroPluginConfOptionType type;
	gchar *description;
	gchar *key;

	priv = BRASERO_PLUGIN_OPTION_PRIVATE (self);
	client = gconf_client_get_default ();

	brasero_plugin_conf_option_get_info (option,
					     &key,
					     &description,
					     &type);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox);
	gtk_box_pack_start (container, hbox, FALSE, FALSE, 0);

	label = gtk_label_new ("\t");
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	box = NULL;
	switch (type) {
	case BRASERO_PLUGIN_OPTION_BOOL:
		widget = gtk_check_button_new_with_label (description);

		gtk_widget_show (widget);

		value_bool = gconf_client_get_bool (client, key, NULL);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), value_bool);

		suboptions = brasero_plugin_conf_option_bool_get_suboptions (option);
		if (suboptions) {
			box = gtk_vbox_new (FALSE, 0);
			gtk_widget_show (box);

			gtk_box_pack_start (GTK_BOX (box),
					    widget,
					    FALSE,
					    FALSE,
					    0);
			gtk_box_pack_start (GTK_BOX (hbox),
					    box,
					    FALSE,
					    FALSE,
					    0);

			for (; suboptions; suboptions = suboptions->next) {
				BraseroPluginConfOption *suboption;

				suboption = suboptions->data;

				/* first create the slaves then set state */
				info = brasero_plugin_option_add_conf_widget (self, suboption, GTK_BOX (box));
				gtk_widget_set_sensitive (info->sensitive, value_bool);

				suboptionsw = g_slist_prepend (suboptionsw, info);
			}

			g_signal_connect (widget,
					  "toggled",
					  G_CALLBACK (brasero_plugin_option_toggled_changed),
					  self);
		}
		else
			gtk_box_pack_start (GTK_BOX (hbox),
					    widget,
					    FALSE,
					    FALSE,
					    0);
		break;

	case BRASERO_PLUGIN_OPTION_INT:
		box = gtk_hbox_new (FALSE, 6);

		label = gtk_label_new (description);
		gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
		gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);

		widget = gtk_spin_button_new_with_range (1.0, 500.0, 1.0);
		gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);

		gtk_widget_show_all (box);
		gtk_box_pack_start (GTK_BOX (hbox),
				    box,
				    FALSE,
				    FALSE,
				    0);

		value_int = gconf_client_get_int (client, key, NULL);

		if (brasero_plugin_conf_option_int_get_min (option) > value_int)
			value_int = brasero_plugin_conf_option_int_get_min (option);

		if (brasero_plugin_conf_option_int_get_max (option) > value_int)
			value_int = brasero_plugin_conf_option_int_get_max (option);

		gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), value_int);
		break;

	case BRASERO_PLUGIN_OPTION_STRING:
		box = gtk_hbox_new (FALSE, 6);

		label = gtk_label_new (description);
		gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
		gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);

		widget = gtk_entry_new ();
		gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 0);

		gtk_widget_show_all (box);
		gtk_box_pack_start (GTK_BOX (hbox),
				    box,
				    FALSE,
				    FALSE,
				    0);

		value_str = gconf_client_get_string (client, key, NULL);
		gtk_entry_set_text (GTK_ENTRY (widget), value_str);
		g_free (value_str);
		break;

	default:
		widget = NULL;
		break;
	}

	info = g_new0 (BraseroPluginOptionWidget, 1);
	info->widget = widget;
	info->option = option;
	info->suboptions = suboptionsw;
	info->sensitive = box;

	priv->widgets = g_slist_prepend (priv->widgets, info);

	g_free (key);
	g_free (description);

	g_object_unref (client);
	return info;
}

void
brasero_plugin_option_set_plugin (BraseroPluginOption *self,
				  BraseroPlugin *plugin)
{
	BraseroPluginOptionPrivate *priv;
	BraseroPluginConfOption *option; 
	gchar *string;

	priv = BRASERO_PLUGIN_OPTION_PRIVATE (self);

	string = g_strdup_printf ("<b>Options for plugin %s</b>", brasero_plugin_get_name (plugin));
	gtk_label_set_markup (GTK_LABEL (priv->title), string);
	g_free (string);

	option = brasero_plugin_get_next_conf_option (plugin, NULL);
	for (; option; option = brasero_plugin_get_next_conf_option (plugin, option))
		brasero_plugin_option_add_conf_widget (self,
						       option,
						       GTK_BOX (priv->vbox));
}

static void
brasero_plugin_option_init (BraseroPluginOption *object)
{
	BraseroPluginOptionPrivate *priv;
	GtkWidget *frame;

	priv = BRASERO_PLUGIN_OPTION_PRIVATE (object);

	frame = gtk_frame_new ("");
	gtk_container_set_border_width (GTK_CONTAINER (frame), 8);
	gtk_widget_show (frame);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);

	priv->title = gtk_frame_get_label_widget (GTK_FRAME (frame));
	gtk_label_set_use_markup (GTK_LABEL (priv->title), TRUE);

	priv->vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (priv->vbox);
	gtk_container_set_border_width (GTK_CONTAINER (priv->vbox), 8);
	gtk_container_add (GTK_CONTAINER (frame), priv->vbox);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (object)->vbox),
			    frame,
			    FALSE,
			    FALSE,
			    0);

	gtk_dialog_set_has_separator (GTK_DIALOG (object), FALSE);
	gtk_dialog_add_button (GTK_DIALOG (object),
			       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button (GTK_DIALOG (object),
			       GTK_STOCK_APPLY, GTK_RESPONSE_OK);
}

static void
brasero_plugin_option_widget_free (BraseroPluginOptionWidget *option)
{
	g_slist_free (option->suboptions);
	g_free (option);
}

static void
brasero_plugin_option_finalize (GObject *object)
{
	BraseroPluginOptionPrivate *priv;

	priv = BRASERO_PLUGIN_OPTION_PRIVATE (object);

	g_slist_foreach (priv->widgets, (GFunc) brasero_plugin_option_widget_free, NULL);
	g_slist_free (priv->widgets);
	priv->widgets = NULL;

	G_OBJECT_CLASS (brasero_plugin_option_parent_class)->finalize (object);
}

static void
brasero_plugin_option_class_init (BraseroPluginOptionClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BraseroPluginOptionPrivate));
	object_class->finalize = brasero_plugin_option_finalize;
}

GtkWidget *
brasero_plugin_option_new (void)
{
	return g_object_new (BRASERO_TYPE_PLUGIN_OPTION, NULL);
}
