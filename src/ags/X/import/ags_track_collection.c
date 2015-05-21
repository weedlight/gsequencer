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

#include <ags/X/import/ags_track_collection.h>
#include <ags/X/import/ags_track_collection_callbacks.h>

#include <ags/object/ags_connectable.h>
#include <ags/object/ags_applicable.h>

#include <ags/X/import/ags_track_collection_mapper.h>

#include <libxml/parser.h>
#include <libxml/xlink.h>
#include <libxml/xpath.h>

void ags_track_collection_class_init(AgsTrackCollectionClass *track_collection);
void ags_track_collection_connectable_interface_init(AgsConnectableInterface *connectable);
void ags_track_collection_applicable_interface_init(AgsApplicableInterface *applicable);
void ags_track_collection_init(AgsTrackCollection *track_collection);
void ags_track_collection_set_property(GObject *gobject,
				       guint prop_id,
				       const GValue *value,
				       GParamSpec *param_spec);
void ags_track_collection_get_property(GObject *gobject,
				       guint prop_id,
				       GValue *value,
				       GParamSpec *param_spec);
void ags_track_collection_connect(AgsConnectable *connectable);
void ags_track_collection_disconnect(AgsConnectable *connectable);
void ags_track_collection_set_update(AgsApplicable *applicable, gboolean update);
void ags_track_collection_apply(AgsApplicable *applicable);
void ags_track_collection_reset(AgsApplicable *applicable);

/**
 * SECTION:ags_track_collection
 * @short_description: Property of audio, channels or pads in bulk mode.
 * @title: AgsTrackCollection
 * @section_id:
 * @include: ags/X/ags_track_collection.h
 *
 * #AgsTrackCollection is a composite widget to property. A property editor 
 * should be packed by a #AgsPropertyCollectionEditor.
 */

enum{
  PROP_0,
  PROP_MIDI_DOCUMENT,
};

static gpointer ags_track_collection_parent_class = NULL;

GType
ags_track_collection_get_type(void)
{
  static GType ags_type_track_collection = 0;

  if(!ags_type_track_collection){
    static const GTypeInfo ags_track_collection_info = {
      sizeof (AgsTrackCollectionClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) ags_track_collection_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (AgsTrackCollection),
      0,    /* n_preallocs */
      (GInstanceInitFunc) ags_track_collection_init,
    };

    static const GInterfaceInfo ags_connectable_interface_info = {
      (GInterfaceInitFunc) ags_track_collection_connectable_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };

    static const GInterfaceInfo ags_applicable_interface_info = {
      (GInterfaceInitFunc) ags_track_collection_applicable_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };

    ags_type_track_collection = g_type_register_static(GTK_TYPE_VBOX,
						       "AgsTrackCollection\0", &ags_track_collection_info,
						       0);
    
    g_type_add_interface_static(ags_type_track_collection,
				AGS_TYPE_CONNECTABLE,
				&ags_connectable_interface_info);

    g_type_add_interface_static(ags_type_track_collection,
				AGS_TYPE_APPLICABLE,
				&ags_applicable_interface_info);
  }

  return(ags_type_track_collection);
}

void
ags_track_collection_class_init(AgsTrackCollectionClass *track_collection)
{
  GObjectClass *gobject;
  GParamSpec *param_spec;
  
  ags_track_collection_parent_class = g_type_class_peek_parent(track_collection);

  /* GObjectClass */
  gobject = (GObjectClass *) track_collection;

  gobject->set_property = ags_track_collection_set_property;
  gobject->get_property = ags_track_collection_get_property;

  /* properties */
  /**
   * AgsTrackCollection:midi-document:
   *
   * The assigned midi document.
   * 
   * Since: 0.4.3
   */
  param_spec = g_param_spec_pointer("midi-document\0",
				    "midi document of track collection\0",
				    "The midi document this track collection is assigned to\0",
				    G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_MIDI_DOCUMENT,
				  param_spec);
}

void
ags_track_collection_connectable_interface_init(AgsConnectableInterface *connectable)
{
  connectable->is_ready = NULL;
  connectable->is_connected = NULL;
  connectable->connect = ags_track_collection_connect;
  connectable->disconnect = ags_track_collection_disconnect;
}

void
ags_track_collection_applicable_interface_init(AgsApplicableInterface *applicable)
{
  applicable->set_update = ags_track_collection_set_update;
  applicable->apply = ags_track_collection_apply;
  applicable->reset = ags_track_collection_reset;
}

void
ags_track_collection_init(AgsTrackCollection *track_collection)
{
  GtkAlignment *alignment;

  track_collection->midi_doc = NULL;
  
  track_collection->child = (GtkVBox *) gtk_vbox_new(FALSE,
						     0);
  gtk_box_pack_start(GTK_BOX(track_collection),
		     track_collection->child,
		     TRUE, TRUE,
		     0);
  
  alignment = (GtkAlignment *) gtk_alignment_new(0.0, 0.0,
						 0.0, 0.0);
  gtk_box_pack_start(GTK_BOX(track_collection),
		     alignment,
		     FALSE, FALSE,
		     0);
}

void
ags_track_collection_set_property(GObject *gobject,
				  guint prop_id,
				  const GValue *value,
				  GParamSpec *param_spec)
{
  AgsTrackCollection *track_collection;

  track_collection = AGS_TRACK_COLLECTION(gobject);

  switch(prop_id){
  case PROP_MIDI_DOCUMENT:
    {
      xmlDoc *midi_document;

      midi_document = (gchar *) g_value_get_pointer(value);

      if(track_collection->midi_doc == midi_document){
	return;
      }

      track_collection->midi_doc = midi_document;
    }
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, param_spec);
    break;
  }
}

void
ags_track_collection_get_property(GObject *gobject,
				  guint prop_id,
				  GValue *value,
				  GParamSpec *param_spec)
{
  AgsTrackCollection *track_collection;

  track_collection = AGS_TRACK_COLLECTION(gobject);

  switch(prop_id){
  case PROP_MIDI_DOCUMENT:
    g_value_set_pointer(value, track_collection->midi_doc);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, param_spec);
    break;
  }
}

void
ags_track_collection_connect(AgsConnectable *connectable)
{
  AgsTrackCollection *track_collection;
  GList *list, *list_start;
  
  track_collection = AGS_TRACK_COLLECTION(connectable);

  /* children */
  list_start = 
    list = gtk_container_get_children(track_collection->child);

  while(list != NULL){
    ags_connectable_connect(AGS_CONNECTABLE(list->data));

    list = list->next;
  }

  g_list_free(list_start);
}

void
ags_track_collection_disconnect(AgsConnectable *connectable)
{
  AgsTrackCollection *track_collection;
  GList *list, *list_start;
  
  track_collection = AGS_TRACK_COLLECTION(connectable);

  /* children */
  list_start = 
    list = gtk_container_get_children(track_collection->child);

  while(list != NULL){
    ags_connectable_disconnect(AGS_CONNECTABLE(list->data));

    list = list->next;
  }

  g_list_free(list_start);
}

void
ags_track_collection_set_update(AgsApplicable *applicable, gboolean update)
{
  AgsTrackCollection *track_collection;
  GList *list, *list_start;
  
  track_collection = AGS_TRACK_COLLECTION(applicable);

  list_start = 
    list = gtk_container_get_children(track_collection->child);

  while(list != NULL){
    ags_applicable_set_update(AGS_APPLICABLE(list->data), update);

    list = list->next;
  }

  g_list_free(list_start);
}

void
ags_track_collection_apply(AgsApplicable *applicable)
{
  AgsTrackCollection *track_collection;
  GList *list, *list_start;
  
  track_collection = AGS_TRACK_COLLECTION(applicable);

  list_start = 
    list = gtk_container_get_children(track_collection->child);

  while(list != NULL){
    ags_applicable_apply(AGS_APPLICABLE(list->data));

    list = list->next;
  }

  g_list_free(list_start);
}

void
ags_track_collection_reset(AgsApplicable *applicable)
{
  AgsTrackCollection *track_collection;
  GList *list, *list_start;
  
  track_collection = AGS_TRACK_COLLECTION(applicable);

  list_start = 
    list = gtk_container_get_children(track_collection->child);

  while(list != NULL){
    ags_applicable_reset(AGS_APPLICABLE(list->data));

    list = list->next;
  }

  g_list_free(list_start);
}

void
ags_track_collection_parse(AgsTrackCollection *track_collection)
{
  xmlXPathContext *xpath_context;
  xmlXPathObject *xpath_object;
  xmlNode **node;

  guint i;
  
  xpath_context = xmlXPathNewContext(track_collection->midi_doc);
  xpath_object = xmlXPathEval((xmlChar *) "//midi-tracks/midi-track\0",
			      xpath_context);

  if(xpath_object->nodesetval != NULL){
    node = xpath_object->nodesetval->nodeTab;

    for(i = 0; i < xpath_object->nodesetval->nodeNr; i++){
      if(node[i]->type == XML_ELEMENT_NODE){
	ags_track_collection_add_mapper(track_collection,
					node[i]);
      }
    }
  }
}

void
ags_track_collection_add_mapper(AgsTrackCollection *track_collection,
				xmlNode *track)
{
  AgsTrackCollectionMapper *track_collection_mapper;

  track_collection_mapper = (AgsTrackCollectionMapper *) g_object_newv(track_collection->child_type,
								       track_collection->child_parameter_count,
								       track_collection->child_parameter);
  g_object_set(track_collection_mapper,
	       "track\0", track,
	       NULL);
  gtk_box_pack_start(GTK_BOX(track_collection->child),
		     GTK_WIDGET(track_collection_mapper),
		     FALSE, FALSE,
		     0);
}

/**
 * ags_track_collection_new:
 * @child_type: 
 * @child_parameter_count:
 * @child_parameter:
 *
 * Creates an #AgsTrackCollection
 *
 * Returns: a new #AgsTrackCollection
 *
 * Since: 0.4.3
 */
AgsTrackCollection*
ags_track_collection_new(GType child_type,
			 guint child_parameter_count,
			 GParameter *child_parameter)
{
  AgsTrackCollection *track_collection;

  track_collection = (AgsTrackCollection *) g_object_new(AGS_TYPE_TRACK_COLLECTION,
							 NULL);

  track_collection->child_type = child_type;
  track_collection->child_parameter_count = child_parameter_count;
  track_collection->child_parameter = child_parameter;
  
  return(track_collection);
}