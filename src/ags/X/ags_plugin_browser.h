/* AGS - Advanced GTK Sequencer
 * Copyright (C) 2015 Joël Krähemann
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __AGS_PLUGIN_BROWSER_H__
#define __AGS_PLUGIN_BROWSER_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#define AGS_TYPE_PLUGIN_BROWSER                (ags_plugin_browser_get_type())
#define AGS_PLUGIN_BROWSER(obj)                (G_TYPE_CHECK_INSTANCE_CAST(obj, AGS_TYPE_PLUGIN_BROWSER, AgsPluginBrowser))
#define AGS_PLUGIN_BROWSER_CLASS(class)        (G_TYPE_CHECK_CLASS_CAST(class, AGS_TYPE_PLUGIN_BROWSER, AgsPluginBrowserClass))
#define AGS_IS_PLUGIN_BROWSER(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AGS_TYPE_PLUGIN_BROWSER))
#define AGS_IS_PLUGIN_BROWSER_CLASS(class)     (G_TYPE_CHECK_CLASS_TYPE ((class), AGS_TYPE_PLUGIN_BROWSER))
#define AGS_PLUGIN_BROWSER_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), AGS_TYPE_PLUGIN_BROWSER, AgsPluginBrowserClass))

typedef struct _AgsPluginBrowser AgsPluginBrowser;
typedef struct _AgsPluginBrowserClass AgsPluginBrowserClass;

struct _AgsPluginBrowser
{
  GtkDialog dialog;

  GtkWidget *parent;

  GtkComboBoxText *plugin_type;

  GtkWidget *active_browser;
  
  GtkWidget *lv2_browser;
  GtkWidget *ladspa_browser;
  
  GtkButton *apply;
  GtkButton *ok;
  GtkButton *cancel;
};

struct _AgsPluginBrowserClass
{
  GtkDialogClass dialog;
};

GType ags_plugin_browser_get_type(void);

gchar* ags_plugin_browser_get_plugin_filename(AgsPluginBrowser *plugin_browser);
gchar* ags_plugin_browser_get_plugin_effect(AgsPluginBrowser *plugin_browser);

AgsPluginBrowser* ags_plugin_browser_new(GtkWidget *parent);

#endif /*__AGS_PLUGIN_BROWSER_H__*/
