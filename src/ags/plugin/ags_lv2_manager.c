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

#include <ags/plugin/ags_lv2_manager.h>

#include <ags/object/ags_marshal.h>

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

void ags_lv2_manager_class_init(AgsLv2ManagerClass *lv2_manager);
void ags_lv2_manager_init (AgsLv2Manager *lv2_manager);
void ags_lv2_manager_set_property(GObject *gobject,
				  guint prop_id,
				  const GValue *value,
				  GParamSpec *param_spec);
void ags_lv2_manager_get_property(GObject *gobject,
				  guint prop_id,
				  GValue *value,
				  GParamSpec *param_spec);
void ags_lv2_manager_finalize(GObject *gobject);

/**
 * SECTION:ags_lv2_manager
 * @short_description: Singleton pattern to organize LV2
 * @title: AgsLv2Manager
 * @section_id:
 * @include: ags/object/ags_lv2_manager.h
 *
 * The #AgsLv2Manager loads/unloads LV2 plugins.
 */

enum{
  PROP_0,
  PROP_LOCALE,
};

static gpointer ags_lv2_manager_parent_class = NULL;

AgsLv2Manager *ags_lv2_manager = NULL;
static const gchar *ags_lv2_default_path = "/usr/lib/lv2\0";

GType
ags_lv2_manager_get_type (void)
{
  static GType ags_type_lv2_manager = 0;

  if(!ags_type_lv2_manager){
    static const GTypeInfo ags_lv2_manager_info = {
      sizeof (AgsLv2ManagerClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) ags_lv2_manager_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (AgsLv2Manager),
      0,    /* n_preallocs */
      (GInstanceInitFunc) ags_lv2_manager_init,
    };

    ags_type_lv2_manager = g_type_register_static(G_TYPE_OBJECT,
						  "AgsLv2Manager\0",
						  &ags_lv2_manager_info,
						  0);
  }

  return (ags_type_lv2_manager);
}

void
ags_lv2_manager_class_init(AgsLv2ManagerClass *lv2_manager)
{
  GObjectClass *gobject;
  GParamSpec *param_spec;
  
  ags_lv2_manager_parent_class = g_type_class_peek_parent(lv2_manager);

  /* GObjectClass */
  gobject = (GObjectClass *) lv2_manager;

  gobject->set_property = ags_lv2_manager_set_property;
  gobject->get_property = ags_lv2_manager_get_property;

  gobject->finalize = ags_lv2_manager_finalize;

  /* properties */
  /**
   * AgsLv2Manager:locale:
   *
   * The assigned locale.
   * 
   * Since: 0.4.3
   */
  param_spec = g_param_spec_string("locale\0",
				   "locale of lv2 manager\0",
				   "The locale this lv2 manager is assigned to\0",
				   NULL,
				   G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_LOCALE,
				  param_spec);
}

void
ags_lv2_manager_init(AgsLv2Manager *lv2_manager)
{
  lv2_manager->lv2_plugin = NULL;
}

void
ags_lv2_manager_set_property(GObject *gobject,
			     guint prop_id,
			     const GValue *value,
			     GParamSpec *param_spec)
{
  AgsLv2Manager *lv2_manager;

  lv2_manager = AGS_LV2_MANAGER(gobject);

  switch(prop_id){
  case PROP_LOCALE:
    {
      gchar *locale;

      locale = (gchar *) g_value_get_string(value);

      if(lv2_manager->locale == locale){
	return;
      }
      
      if(lv2_manager->locale != NULL){
	g_free(lv2_manager->locale);
      }

      lv2_manager->locale = g_strdup(locale);
    }
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, param_spec);
    break;
  }
}

void
ags_lv2_manager_get_property(GObject *gobject,
			     guint prop_id,
			     GValue *value,
			     GParamSpec *param_spec)
{
  AgsLv2Manager *lv2_manager;

  lv2_manager = AGS_LV2_MANAGER(gobject);

  switch(prop_id){
  case PROP_LOCALE:
    g_value_set_string(value, lv2_manager->locale);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, param_spec);
    break;
  }
}

void
ags_lv2_manager_finalize(GObject *gobject)
{
  AgsLv2Manager *lv2_manager;
  GList *lv2_plugin;

  lv2_manager = AGS_LV2_MANAGER(gobject);

  lv2_plugin = lv2_manager->lv2_plugin;

  g_list_free_full(lv2_plugin,
		   ags_lv2_plugin_free);
}

/**
 * ags_lv2_plugin_alloc:
 * 
 * Alloc the #AgsLv2Plugin-struct
 *
 * Returns: the #AgsLv2Plugin-struct
 *
 * Since: 0.4.3
 */
AgsLv2Plugin*
ags_lv2_plugin_alloc()
{
  AgsLv2Plugin *lv2_plugin;

  lv2_plugin = (AgsLv2Plugin *) malloc(sizeof(AgsLv2Plugin));

  lv2_plugin->flags = 0;

  lv2_plugin->turtle = NULL;

  lv2_plugin->filename = NULL;
  lv2_plugin->plugin_so = NULL;

  return(lv2_plugin);
}

/**
 * ags_lv2_plugin_free:
 * @lv2_plugin: the #AgsLv2Plugin-struct
 * 
 * Free the #AgsLv2Plugin-struct
 *
 * Since: 0.4.3
 */
void
ags_lv2_plugin_free(AgsLv2Plugin *lv2_plugin)
{
  if(lv2_plugin->plugin_so != NULL){
    dlclose(lv2_plugin->plugin_so);
  }

  free(lv2_plugin->filename);
  g_object_unref(lv2_plugin->turtle);
  free(lv2_plugin);
}

/**
 * ags_lv2_manager_get_filenames:
 * 
 * Retrieve all filenames
 *
 * Returns: a %NULL-terminated array of filenames
 *
 * Since: 0.4.3
 */
gchar**
ags_lv2_manager_get_filenames()
{
  AgsLv2Manager *lv2_manager;
  GList *lv2_plugin;
  gchar **filenames;
  guint length;
  guint i;

  lv2_manager = ags_lv2_manager_get_instance();
  length = g_list_length(lv2_manager->lv2_plugin);

  lv2_plugin = lv2_manager->lv2_plugin;
  filenames = (gchar **) malloc((length + 1) * sizeof(gchar *));

  for(i = 0; i < length; i++){
    filenames[i] = AGS_LV2_PLUGIN(lv2_plugin->data)->filename;
    lv2_plugin = lv2_plugin->next;
  }

  filenames[i] = NULL;

  return(filenames);
}

/**
 * ags_lv2_manager_find_lv2_plugin:
 * @filename: the filename of the plugin
 *
 * Lookup filename in loaded plugins.
 *
 * Returns: the #AgsLv2Plugin-struct
 *
 * Since: 0.4.3
 */
AgsLv2Plugin*
ags_lv2_manager_find_lv2_plugin(gchar *filename)
{
  AgsLv2Manager *lv2_manager;
  AgsLv2Plugin *lv2_plugin;
  GList *list;

  lv2_manager = ags_lv2_manager_get_instance();

  list = lv2_manager->lv2_plugin;

  while(list != NULL){
    lv2_plugin = AGS_LV2_PLUGIN(list->data);
    if(!g_strcmp0(lv2_plugin->filename,
		  filename)){
      return(lv2_plugin);
    }

    list = list->next;
  }

  return(NULL);
}

/**
 * ags_lv2_manager_load_file:
 * @filename: the filename of the plugin
 *
 * Load @filename specified plugin.
 *
 * Since: 0.4.3
 */
void
ags_lv2_manager_load_file(AgsTurtle *turtle,
			  gchar *filename)
{
  AgsLv2Manager *lv2_manager;
  AgsLv2Plugin *lv2_plugin;

  gchar *turtle_path;

  GError *error;

  static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

  if(filename == NULL){
    return;
  }

  lv2_manager = ags_lv2_manager_get_instance();
  
  /* load plugin */
  pthread_mutex_lock(&(mutex));

  lv2_plugin = ags_lv2_manager_find_lv2_plugin(filename);
  g_message("loading: %s\0", filename);

  if(lv2_plugin == NULL){
    lv2_plugin = ags_lv2_plugin_alloc();

    /* set turtle */
    lv2_plugin->turtle = turtle;

    /* set filename and plugin file */
    lv2_plugin->filename = g_strdup(filename);
    
    lv2_manager->lv2_plugin = g_list_prepend(lv2_manager->lv2_plugin,
					     lv2_plugin);
  }

  pthread_mutex_unlock(&(mutex));
}

/**
 * ags_lv2_manager_load_default_directory:
 * 
 * Loads all available plugins.
 *
 * Since: 0.4.3
 */
void
ags_lv2_manager_load_default_directory()
{
  AgsLv2Manager *lv2_manager;
  AgsLv2Plugin *lv2_plugin;

  GDir *dir;

  gchar *path, *plugin_path;
  gchar *str;

  GError *error;

  lv2_manager = ags_lv2_manager_get_instance();

  error = NULL;
  dir = g_dir_open(ags_lv2_default_path,
		   0,
		   &error);

  if(error != NULL){
    g_warning(error->message);
  }

  while((path = g_dir_read_name(dir)) != NULL){
    if(!g_ascii_strncasecmp(path,
			    "..\0",
			    3) ||
       !g_ascii_strncasecmp(path,
			    ".\0",
			    2)){
      continue;
    }
    
    plugin_path = g_strdup_printf("%s/%s\0",
				  ags_lv2_default_path,
				  path);

    if(g_file_test(plugin_path,
		   G_FILE_TEST_IS_DIR)){
      AgsTurtle *manifest, *turtle;
      
      GList *ttl_list, *binary_list;

      gchar *turtle_path, *filename;
      
      manifest = ags_turtle_new(g_strdup_printf("%s/manifest.ttl\0",
						plugin_path));
      ags_turtle_load(manifest);
  
      /* instantiate and load turtle */
      ttl_list = ags_turtle_find_xpath(manifest,
				       "//rdf-triple/rdf-verb[@do=\"rdfs:seeAlso\"]/rdf-list/rdf-value\0");

      /* read binary from turtle */
      binary_list = ags_turtle_find_xpath(manifest,
					  "//rdf-triple/rdf-verb[@do=\"lv2:binary\"]/rdf-list/rdf-value\0");

      /* load */
      if(ttl_list == NULL ||
	 binary_list == NULL){
	continue;
      }
      
      while(ttl_list != NULL &&
	    binary_list != NULL){
	turtle_path = xmlGetProp((xmlNode *) ttl_list->data,
				 "value\0");

	turtle_path = g_strndup(&(turtle_path[1]),
				strlen(turtle_path) - 2);
	
	if(!g_ascii_strncasecmp(turtle_path,
				"http://\0",
				7)){
	  ttl_list = ttl_list->next;
	  continue;
	}
	
	g_message(turtle_path);
	turtle = ags_turtle_new(g_strdup_printf("%s/%s\0",
						plugin_path,
						turtle_path));
	ags_turtle_load(turtle);
	//	xmlSaveFormatFileEnc("-\0", turtle->doc, "UTF-8\0", 1);

	str = xmlGetProp(binary_list->data,
			 "value\0");
	str = g_strndup(&(str[1]),
			strlen(str) - 2);
	filename = g_strdup_printf("%s/%s\0",
				   plugin_path,
				   str);
	free(str);
	
	ags_lv2_manager_load_file(turtle,
				  filename);

	ttl_list = ttl_list->next;
	binary_list = binary_list->next;
      }

      g_object_unref(manifest);
    }
  }
}
  
/**
 * ags_lv2_manager_uri_index:
 * @filename: the plugin.so filename
 * @uri: the uri's name within plugin
 *
 * Retrieve the uri's index within @filename
 *
 * Returns: the index, G_MAXULONG if not found
 *
 * Since: 0.4.3
 */
uint32_t
ags_lv2_manager_uri_index(gchar *filename,
			  gchar *uri)
{
  AgsLv2Plugin *lv2_plugin;
    
  void *plugin_so;
  LV2_Descriptor_Function lv2_descriptor;
  LV2_Descriptor *plugin_descriptor;

  uint32_t uri_index;
  uint32_t i;

  if(filename == NULL ||
     uri == NULL){
    return(G_MAXULONG);
  }
  
  /* load plugin */
  lv2_plugin = ags_lv2_manager_find_lv2_plugin(filename);

  if(lv2_plugin->plugin_so == NULL){
    plugin_so =
      lv2_plugin->plugin_so = dlopen(lv2_plugin->filename,
				     RTLD_NOW);
  }else{
    plugin_so = lv2_plugin->plugin_so;
  }
  
  uri_index = G_MAXULONG;

  if(plugin_so){
    lv2_descriptor = (LV2_Descriptor_Function) dlsym(plugin_so,
						     "lv2_descriptor\0");
    
    if(dlerror() == NULL && lv2_descriptor){
      for(i = 0; (plugin_descriptor = lv2_descriptor(i)) != NULL; i++){
	if(!strncmp(plugin_descriptor->URI,
		    uri,
		    strlen(uri))){
	  uri_index = i;
	  break;
	}
      }
    }
  }
  
  return(uri_index);
}

/**
 * ags_lv2_manager_get_instance:
 *
 * Get instance.
 *
 * Returns: the #AgsLv2Manager
 *
 * Since: 0.4.3
 */
AgsLv2Manager*
ags_lv2_manager_get_instance()
{
  static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

  pthread_mutex_lock(&(mutex));

  if(ags_lv2_manager == NULL){
    ags_lv2_manager = ags_lv2_manager_new(AGS_LV2_MANAGER_DEFAULT_LOCALE);

    pthread_mutex_unlock(&(mutex));

    ags_lv2_manager_load_default_directory();
  }else{
    pthread_mutex_unlock(&(mutex));
  }

  return(ags_lv2_manager);
}

/**
 * ags_lv2_manager_new:
 * @locale: the default locale
 *
 * Creates an #AgsLv2Manager
 *
 * Returns: a new #AgsLv2Manager
 *
 * Since: 0.4.3
 */
AgsLv2Manager*
ags_lv2_manager_new(gchar *locale)
{
  AgsLv2Manager *lv2_manager;

  lv2_manager = (AgsLv2Manager *) g_object_new(AGS_TYPE_LV2_MANAGER,
					       "locale\0", locale,
					       NULL);

  return(lv2_manager);
}
