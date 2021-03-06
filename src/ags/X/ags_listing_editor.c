/* AGS - Advanced GTK Sequencer
 * Copyright (C) 2005-2011 Joël Krähemann
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

#include <ags/X/ags_listing_editor.h>
#include <ags/X/ags_listing_editor_callbacks.h>

#include <ags/object/ags_connectable.h>
#include <ags/object/ags_applicable.h>

#include <ags/audio/ags_channel.h>
#include <ags/audio/ags_output.h>
#include <ags/audio/ags_input.h>

#include <ags/X/ags_machine_editor.h>
#include <ags/X/ags_pad_editor.h>

void ags_listing_editor_class_init(AgsListingEditorClass *listing_editor);
void ags_listing_editor_connectable_interface_init(AgsConnectableInterface *connectable);
void ags_listing_editor_applicable_interface_init(AgsApplicableInterface *applicable);
void ags_listing_editor_init(AgsListingEditor *listing_editor);
void ags_listing_editor_connect(AgsConnectable *connectable);
void ags_listing_editor_disconnect(AgsConnectable *connectable);
void ags_listing_editor_set_update(AgsApplicable *applicable, gboolean update);
void ags_listing_editor_apply(AgsApplicable *applicable);
void ags_listing_editor_reset(AgsApplicable *applicable);
void ags_listing_editor_destroy(GtkObject *object);
void ags_listing_editor_show(GtkWidget *widget);

/**
 * SECTION:ags_listing_editor
 * @short_description: pack pad editors.
 * @title: AgsListingEditor
 * @section_id:
 * @include: ags/X/ags_listing_editor.h
 *
 * #AgsListingEditor is a composite widget to pack #AgsPadEditor.
 */

static gpointer ags_listing_editor_parent_class = NULL;
static AgsConnectableInterface* ags_listing_editor_parent_connectable_interface;

GType
ags_listing_editor_get_type(void)
{
  static GType ags_type_listing_editor = 0;

  if(!ags_type_listing_editor){
    static const GTypeInfo ags_listing_editor_info = {
      sizeof (AgsListingEditorClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) ags_listing_editor_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (AgsListingEditor),
      0,    /* n_preallocs */
      (GInstanceInitFunc) ags_listing_editor_init,
    };

    static const GInterfaceInfo ags_connectable_interface_info = {
      (GInterfaceInitFunc) ags_listing_editor_connectable_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };

    static const GInterfaceInfo ags_applicable_interface_info = {
      (GInterfaceInitFunc) ags_listing_editor_applicable_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };

    ags_type_listing_editor = g_type_register_static(AGS_TYPE_PROPERTY_LISTING_EDITOR,
						     "AgsListingEditor\0",
						     &ags_listing_editor_info,
						     0);
    
    g_type_add_interface_static(ags_type_listing_editor,
				AGS_TYPE_CONNECTABLE,
				&ags_connectable_interface_info);

    g_type_add_interface_static(ags_type_listing_editor,
				AGS_TYPE_APPLICABLE,
				&ags_applicable_interface_info);
  }

  return(ags_type_listing_editor);
}

void
ags_listing_editor_class_init(AgsListingEditorClass *listing_editor)
{
}

void
ags_listing_editor_connectable_interface_init(AgsConnectableInterface *connectable)
{
  ags_listing_editor_parent_connectable_interface = g_type_interface_peek_parent(connectable);

  connectable->is_ready = NULL;
  connectable->is_connected = NULL;
  connectable->connect = ags_listing_editor_connect;
  connectable->disconnect = ags_listing_editor_disconnect;
}

void
ags_listing_editor_applicable_interface_init(AgsApplicableInterface *applicable)
{
  applicable->set_update = ags_listing_editor_set_update;
  applicable->apply = ags_listing_editor_apply;
  applicable->reset = ags_listing_editor_reset;
}

void
ags_listing_editor_init(AgsListingEditor *listing_editor)
{
  g_signal_connect_after(G_OBJECT(listing_editor), "parent_set\0",
			 G_CALLBACK(ags_listing_editor_parent_set_callback), listing_editor);

  listing_editor->child = NULL;
}

void
ags_listing_editor_connect(AgsConnectable *connectable)
{
  AgsMachineEditor *machine_editor;
  AgsListingEditor *listing_editor;
  GList *pad_editor, *pad_editor_start;

  ags_listing_editor_parent_connectable_interface->connect(connectable);

  listing_editor = AGS_LISTING_EDITOR(connectable);

  machine_editor = (AgsMachineEditor *) gtk_widget_get_ancestor(GTK_WIDGET(listing_editor),
								AGS_TYPE_MACHINE_EDITOR);

  /* AgsPadEditor */
  pad_editor_start = 
    pad_editor = gtk_container_get_children(GTK_CONTAINER(listing_editor->child));

  while(pad_editor != NULL){
    ags_connectable_connect(AGS_CONNECTABLE(pad_editor->data));

    pad_editor = pad_editor->next;
  }
  
  g_list_free(pad_editor_start);
}

void
ags_listing_editor_disconnect(AgsConnectable *connectable)
{
  /* empty */
}

void
ags_listing_editor_set_update(AgsApplicable *applicable, gboolean update)
{
  AgsListingEditor *listing_editor;
  GList *pad_editor, *pad_editor_start;

  listing_editor = AGS_LISTING_EDITOR(applicable);

  pad_editor_start = 
    pad_editor = gtk_container_get_children(GTK_CONTAINER(listing_editor->child));

  while(pad_editor != NULL){
    ags_applicable_set_update(AGS_APPLICABLE(pad_editor->data), update);

    pad_editor = pad_editor->next;
  }

  g_list_free(pad_editor_start);
}

void
ags_listing_editor_apply(AgsApplicable *applicable)
{

  AgsListingEditor *listing_editor;
  GList *pad_editor, *pad_editor_start;

  listing_editor = AGS_LISTING_EDITOR(applicable);

  if((AGS_PROPERTY_EDITOR_ENABLED & (AGS_PROPERTY_EDITOR(listing_editor)->flags)) == 0)
    return;

  pad_editor_start = 
    pad_editor = gtk_container_get_children(GTK_CONTAINER(listing_editor->child));

  while(pad_editor != NULL){
    ags_applicable_apply(AGS_APPLICABLE(pad_editor->data));

    pad_editor = pad_editor->next;
  }
  
  g_list_free(pad_editor_start);
}

void
ags_listing_editor_reset(AgsApplicable *applicable)
{
  AgsMachineEditor *machine_editor;
  AgsListingEditor *listing_editor;

  AgsAudio *audio;

  GList *pad_editor, *pad_editor_start;

  guint pads_old;
  
  listing_editor = AGS_LISTING_EDITOR(applicable);

  machine_editor = gtk_widget_get_ancestor(listing_editor,
					   AGS_TYPE_MACHINE_EDITOR);
  audio = machine_editor->machine->audio;
  
  pad_editor_start = 
    pad_editor = gtk_container_get_children(GTK_CONTAINER(listing_editor->child));

  pads_old = g_list_length(pad_editor_start);
  
  while(pad_editor != NULL){
    ags_applicable_reset(AGS_APPLICABLE(pad_editor->data));

    pad_editor = pad_editor->next;
  }

  g_list_free(pad_editor_start);

  ags_listing_editor_resize(listing_editor,
			    ((listing_editor->channel_type == AGS_TYPE_INPUT) ? audio->input_pads: audio->output_pads), pads_old);
}

void
ags_listing_editor_destroy(GtkObject *object)
{
  /* empty */
}

void
ags_listing_editor_show(GtkWidget *widget)
{
  /* empty */
}

/**
 * ags_listing_editor_resize:
 * @listing_editor: the #AgsListingEditor to use
 * @pads: the end
 * @pads_old: nth channel to start creation until end
 *
 * Creates new pad editors or destroys them.
 *
 * Since: 0.4.3
 */
void
ags_listing_editor_resize(AgsListingEditor *listing_editor,
			  guint pads, guint pads_old)
{
  if(pads_old < pads){
    AgsMachineEditor *machine_editor;
    AgsPadEditor *pad_editor;
    GtkVBox *vbox;

    AgsAudio *audio;
    AgsChannel *channel;

    guint nth_channel;

    machine_editor = gtk_widget_get_ancestor(listing_editor,
					     AGS_TYPE_MACHINE_EDITOR);
    audio = machine_editor->machine->audio;

    if(audio->audio_channels == 0){
      return;
    }
    
    nth_channel = pads_old * audio->audio_channels;

    if(nth_channel == 0 &&
       listing_editor->child != NULL){
      vbox = listing_editor->child;
      listing_editor->child = NULL;
      gtk_widget_destroy(GTK_WIDGET(vbox));
    }

    if(nth_channel == 0){
      listing_editor->child = (GtkVBox *) gtk_vbox_new(FALSE, 0);
      gtk_box_pack_start(GTK_BOX(listing_editor),
			 GTK_WIDGET(listing_editor->child),
			 FALSE, FALSE,
			 0);
    }

    if(listing_editor->channel_type == AGS_TYPE_OUTPUT)
      channel = ags_channel_nth(audio->output, nth_channel);
    else
      channel = ags_channel_nth(audio->input, nth_channel);

    while(channel != NULL){
      pad_editor = ags_pad_editor_new(channel);
      gtk_box_pack_start(GTK_BOX(listing_editor->child),
			 GTK_WIDGET(pad_editor),
			 FALSE, FALSE,
			 0);

      ags_connectable_connect(AGS_CONNECTABLE(pad_editor));
      gtk_widget_show_all(GTK_WIDGET(pad_editor));

      channel = channel->next_pad;
    }
  }else{
    GList *list, *list_next, *list_start;

    list_start = 
      list = gtk_container_get_children(GTK_CONTAINER(listing_editor->child));
    list = g_list_nth(list, pads);
    
    while(list != NULL){
      list_next = list->next;

      gtk_widget_destroy(GTK_WIDGET(list->data));

      list = list_next;
    }

    g_list_free(list_start);
  }

}

/**
 * ags_listing_editor_new:
 * @channel_type: the channel type to represent
 *
 * Creates an #AgsListingEditor
 *
 * Returns: a new #AgsListingEditor
 *
 * Since: 0.3
 */
AgsListingEditor*
ags_listing_editor_new(GType channel_type)
{
  AgsListingEditor *listing_editor;

  listing_editor = (AgsListingEditor *) g_object_new(AGS_TYPE_LISTING_EDITOR,
						     NULL);
  
  listing_editor->channel_type = channel_type;

  return(listing_editor);
}
