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

#include <ags/audio/ags_recycling.h>

#include <ags/lib/ags_list.h>

#include <ags/object/ags_marshal.h>
#include <ags/object/ags_connectable.h>
#include <ags/object/ags_soundcard.h>

#include <ags/audio/ags_audio.h>
#include <ags/audio/ags_channel.h>

#include <string.h>
#include <math.h>

void ags_recycling_class_init(AgsRecyclingClass *recycling_class);
void ags_recycling_connectable_interface_init(AgsConnectableInterface *connectable);
void ags_recycling_set_property(GObject *gobject,
				guint prop_id,
				const GValue *value,
				GParamSpec *param_spec);
void ags_recycling_get_property(GObject *gobject,
				guint prop_id,
				GValue *value,
				GParamSpec *param_spec);
void ags_recycling_init(AgsRecycling *recycling);
void ags_recycling_connect(AgsConnectable *connectable);
void ags_recycling_disconnect(AgsConnectable *connectable);
void ags_recycling_finalize(GObject *gobject);

void ags_recycling_real_add_audio_signal(AgsRecycling *recycling,
					 AgsAudioSignal *audio_signal);

void ags_recycling_real_remove_audio_signal(AgsRecycling *recycling,
					    AgsAudioSignal *audio_signal);

/**
 * SECTION:ags_recycling
 * @short_description: A container of audio signals
 * @title: AgsRecycling
 * @section_id:
 * @include: ags/audio/ags_recycling.h
 *
 * #AgsRecycling forms the nested tree of AgsChannel. Ever channel
 * having own audio signal contains therefor an #AgsRecycling
 */

enum{
  PROP_0,
  PROP_SOUNDCARD,
  PROP_CHANNEL,
  PROP_PARENT,
  PROP_NEXT,
  PROP_PREV,
  PROP_AUDIO_SIGNAL,
};

enum{
  ADD_AUDIO_SIGNAL,
  REMOVE_AUDIO_SIGNAL,
  LAST_SIGNAL,
};

static gpointer ags_recycling_parent_class = NULL;
static guint recycling_signals[LAST_SIGNAL];

//extern void ags_audio_signal_copy_buffer_to_buffer(signed short *destination, guint dchannels,
//						   signed short *source, guint schannels, guint size)
//  __attribute__ ((hot))
//  __attribute__ ((fastcall));

GType
ags_recycling_get_type (void)
{
  static GType ags_type_recycling = 0;

  if(!ags_type_recycling){
    static const GTypeInfo ags_recycling_info = {
      sizeof (AgsRecyclingClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) ags_recycling_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (AgsRecycling),
      0,    /* n_preallocs */
      (GInstanceInitFunc) ags_recycling_init,
    };

    static const GInterfaceInfo ags_connectable_interface_info = {
      (GInterfaceInitFunc) ags_recycling_connectable_interface_init,
      NULL, /* interface_finalize */
      NULL, /* interface_data */
    };

    ags_type_recycling = g_type_register_static(G_TYPE_OBJECT,
						"AgsRecycling\0",
						&ags_recycling_info, 0);

    g_type_add_interface_static(ags_type_recycling,
				AGS_TYPE_CONNECTABLE,
				&ags_connectable_interface_info);
  }

  return(ags_type_recycling);
}

void
ags_recycling_class_init(AgsRecyclingClass *recycling)
{
  GObjectClass *gobject;
  GParamSpec *param_spec;

  ags_recycling_parent_class = g_type_class_peek_parent(recycling);

  /* GObjectClass */
  gobject = (GObjectClass *) recycling;

  gobject->set_property = ags_recycling_set_property;
  gobject->get_property = ags_recycling_get_property;

  gobject->finalize = ags_recycling_finalize;

  /* properties */
  /**
   * AgsRecycling:soundcard:
   *
   * The assigned #AgsSoundcard acting as default sink.
   * 
   * Since: 0.4.0
   */
  param_spec = g_param_spec_object("soundcard\0",
				   "assigned soundcard\0",
				   "The soundcard it is assigned with\0",
				   G_TYPE_OBJECT,
				   G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_SOUNDCARD,
				  param_spec);


  /**
   * AgsRecycling:channel:
   *
   * The assigned #AgsChannel.
   * 
   * Since: 0.4.3
   */
  param_spec = g_param_spec_object("channel\0",
				   "assigned channel\0",
				   "The channel it is assigned with\0",
				   AGS_TYPE_CHANNEL,
				   G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_CHANNEL,
				  param_spec);

  /**
   * AgsRecycling:parent:
   *
   * The assigned parent #AgsRecycling.
   * 
   * Since: 0.4.3
   */
  param_spec = g_param_spec_object("parent\0",
				   "assigned parent\0",
				   "The parent it is assigned with\0",
				   AGS_TYPE_RECYCLING,
				   G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_PARENT,
				  param_spec);

  /**
   * AgsRecycling:prev:
   *
   * The assigned prev #AgsRecycling.
   * 
   * Since: 0.4.3
   */
  param_spec = g_param_spec_object("prev\0",
				   "assigned prev\0",
				   "The prev it is assigned with\0",
				   AGS_TYPE_RECYCLING,
				   G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_PREV,
				  param_spec);

  /**
   * AgsRecycling:next:
   *
   * The assigned next #AgsRecycling.
   * 
   * Since: 0.4.3
   */
  param_spec = g_param_spec_object("next\0",
				   "assigned next\0",
				   "The next it is assigned with\0",
				   AGS_TYPE_RECYCLING,
				   G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_NEXT,
				  param_spec);

  /**
   * AgsRecycling:audio-signal:
   *
   * The containing  #AgsAudioSignal.
   * 
   * Since: 0.4.3
   */
  param_spec = g_param_spec_object("audio-signal\0",
				   "containing audio signal\0",
				   "The audio signal it contains\0",
				   AGS_TYPE_AUDIO_SIGNAL,
				   G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(gobject,
				  PROP_AUDIO_SIGNAL,
				  param_spec);

  /*  */
  recycling->add_audio_signal = ags_recycling_real_add_audio_signal;
  recycling->remove_audio_signal = ags_recycling_real_remove_audio_signal;

  /**
   * AgsRecycling::add-audio-signal
   * @recycling: an #AgsRecycling
   * @audio_signal: the #AgsAudioSignal to add
   *
   * The ::add-audio-signal signal is emited as adding #AgsAudioSignal
   */
  recycling_signals[ADD_AUDIO_SIGNAL] =
    g_signal_new("add-audio-signal\0",
		 G_TYPE_FROM_CLASS (recycling),
		 G_SIGNAL_RUN_LAST,
		 G_STRUCT_OFFSET (AgsRecyclingClass, add_audio_signal),
		 NULL, NULL,
		 g_cclosure_marshal_VOID__OBJECT,
		 G_TYPE_NONE, 1,
		 G_TYPE_OBJECT);

  /**
   * AgsRecycling::remove-audio-signal:
   * @recycling: an #AgsRecycling
   * @audio_signal: the #AgsAudioSignal to remove
   *
   * The ::remove-audio-signal signal is emited as removing #AgsAudioSignal
   */
  recycling_signals[REMOVE_AUDIO_SIGNAL] =
    g_signal_new("remove-audio-signal\0",
		 G_TYPE_FROM_CLASS (recycling),
		 G_SIGNAL_RUN_LAST,
		 G_STRUCT_OFFSET (AgsRecyclingClass, remove_audio_signal),
		 NULL, NULL,
		 g_cclosure_marshal_VOID__OBJECT,
		 G_TYPE_NONE, 1,
		 G_TYPE_OBJECT);
}

void
ags_recycling_connectable_interface_init(AgsConnectableInterface *connectable)
{
  connectable->is_ready = NULL;
  connectable->is_connected = NULL;
  connectable->connect = ags_recycling_connect;
  connectable->disconnect = ags_recycling_disconnect;
}

void
ags_recycling_init(AgsRecycling *recycling)
{
  recycling->flags = 0;

  recycling->soundcard = NULL;
  recycling->channel = NULL;

  recycling->parent = NULL;

  recycling->next = NULL;
  recycling->prev = NULL;

  recycling->audio_signal = NULL;
}

void
ags_recycling_set_property(GObject *gobject,
			   guint prop_id,
			   const GValue *value,
			   GParamSpec *param_spec)
{
  AgsRecycling *recycling;

  recycling = AGS_RECYCLING(gobject);

  switch(prop_id){
  case PROP_SOUNDCARD:
    {
      GObject *soundcard;

      soundcard = (GObject *) g_value_get_object(value);

      ags_recycling_set_soundcard(recycling, (GObject *) soundcard);
    }
    break;
  case PROP_CHANNEL:
    {
      AgsChannel *channel;

      channel = (AgsChannel *) g_value_get_object(value);

      if(recycling->channel == channel){
	return;
      }

      if(recycling->channel != NULL){
	g_object_unref(recycling->channel);
      }

      if(channel != NULL){
	g_object_ref(channel);
      }

      recycling->channel = channel;
    }
    break;
  case PROP_PARENT:
    {
      AgsRecycling *recycling;

      recycling = (AgsRecycling *) g_value_get_object(value);

      if(recycling->parent == recycling){
	return;
      }

      if(recycling->parent != NULL){
	g_object_unref(recycling->parent);
      }

      if(recycling != NULL){
	g_object_ref(recycling);
      }

      recycling->parent = recycling;
    }
    break;
  case PROP_NEXT:
    {
      AgsRecycling *recycling;

      recycling = (AgsRecycling *) g_value_get_object(value);

      if(recycling->next == recycling){
	return;
      }

      if(recycling->next != NULL){
	g_object_unref(recycling->next);
      }

      if(recycling != NULL){
	g_object_ref(recycling);
      }

      recycling->next = recycling;
    }
    break;
  case PROP_PREV:
    {
      AgsRecycling *recycling;

      recycling = (AgsRecycling *) g_value_get_object(value);

      if(recycling->prev == recycling){
	return;
      }

      if(recycling->prev != NULL){
	g_object_unref(recycling->prev);
      }

      if(recycling != NULL){
	g_object_ref(recycling);
      }

      recycling->prev = recycling;
    }
    break;
  case PROP_AUDIO_SIGNAL:
    {
      AgsAudioSignal *audio_signal;

      audio_signal = g_value_get_object(value);

      if(audio_signal == NULL ||
	 g_list_find(recycling->audio_signal, audio_signal) != NULL){
	return;
      }

      ags_recycling_add_audio_signal(recycling,
				     audio_signal);
    }
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, param_spec);
    break;
  }
}

void
ags_recycling_get_property(GObject *gobject,
			   guint prop_id,
			   GValue *value,
			   GParamSpec *param_spec)
{
  AgsRecycling *recycling;

  recycling = AGS_RECYCLING(gobject);

  switch(prop_id){
  case PROP_SOUNDCARD:
    {
      g_value_set_object(value, recycling->soundcard);
    }
    break;
  case PROP_CHANNEL:
    {
      g_value_set_object(value, recycling->channel);
    }
    break;
  case PROP_PARENT:
    {
      g_value_set_object(value, recycling->parent);
    }
    break;
  case PROP_NEXT:
    {
      g_value_set_object(value, recycling->next);
    }
    break;
  case PROP_PREV:
    {
      g_value_set_object(value, recycling->prev);
    }
    break;
  case PROP_AUDIO_SIGNAL:
    {
      g_value_set_pointer(value, g_list_copy(recycling->audio_signal));
    }
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, param_spec);
    break;
  }
}

void
ags_recycling_connect(AgsConnectable *connectable)
{
}

void
ags_recycling_disconnect(AgsConnectable *connectable)
{
}

void
ags_recycling_finalize(GObject *gobject)
{
  AgsRecycling *recycling;
  GList *list, *list_next;

  g_warning("ags_recycling_finalize\0");

  recycling = AGS_RECYCLING(gobject);

  /* AgsAudioSignal */
  ags_list_free_and_unref_link(recycling->audio_signal);

  /* call parent */
  G_OBJECT_CLASS(ags_recycling_parent_class)->finalize(gobject);
}

/**
 * ags_recycling_set_soundcard:
 * @recycling:  an #AgsRecycling
 * @soundcard: the #AgsSoundcard to set
 *
 * Sets #AgsSoundcard to recycling.
 *
 * Since: 0.3
 */
void
ags_recycling_set_soundcard(AgsRecycling *recycling, GObject *soundcard)
{
  /* recycling */
  if(recycling->soundcard == soundcard)
    return;

  if(recycling->soundcard != NULL)
    g_object_unref(recycling->soundcard);

  if(soundcard != NULL)
    g_object_ref(soundcard);

  recycling->soundcard = (GObject *) soundcard;
}

/**
 * ags_recycling_add_audio_signal:
 * @recycling:  an #AgsRecycling
 * @audio_signal: the #AgsAudioSignal to add
 *
 * Add #AgsAudioSignal to recycling.
 *
 * Since: 0.3
 */
void
ags_recycling_add_audio_signal(AgsRecycling *recycling,
			       AgsAudioSignal *audio_signal)
{
  g_return_if_fail(AGS_IS_RECYCLING(recycling));
  g_return_if_fail(AGS_IS_AUDIO_SIGNAL(audio_signal));

  g_object_ref(G_OBJECT(recycling));
  g_signal_emit(G_OBJECT(recycling),
		recycling_signals[ADD_AUDIO_SIGNAL], 0,
		audio_signal);
  g_object_unref(G_OBJECT(recycling));
}

void
ags_recycling_real_add_audio_signal(AgsRecycling *recycling,
				    AgsAudioSignal *audio_signal)
{
  recycling->audio_signal = g_list_prepend(recycling->audio_signal, (gpointer) audio_signal);
  audio_signal->recycling = (GObject *) recycling;
  g_object_ref(audio_signal);
}

/**
 * ags_recycling_remove_audio_signal:
 * @recycling:  an #AgsRecycling
 * @audio_signal: the #AgsAudioSignal to remove
 *
 * Remove #AgsAudioSignal of recycling.
 *
 * Since: 0.3
 */
void
ags_recycling_remove_audio_signal(AgsRecycling *recycling,
				  AgsAudioSignal *audio_signal)
{
  g_return_if_fail(AGS_IS_RECYCLING(recycling));

  g_object_ref((GObject *) recycling);
  g_object_ref((GObject *) audio_signal);
  g_signal_emit(G_OBJECT(recycling),
		recycling_signals[REMOVE_AUDIO_SIGNAL], 0,
		audio_signal);
  g_object_unref((GObject *) audio_signal);
  g_object_unref((GObject *) recycling);
}

void
ags_recycling_real_remove_audio_signal(AgsRecycling *recycling,
				       AgsAudioSignal *audio_signal)
{
  recycling->audio_signal = g_list_remove(recycling->audio_signal,
					  (gpointer) audio_signal);
  g_object_set(audio_signal,
	       "recycling\0", NULL,
	       NULL);
}

/**
 * ags_recycling_create_audio_signal_with_defaults:
 * @recycling: an #AgsRecycling
 * @audio_signal: the #AgsAudioSignal to apply defaults 
 * @delay: 
 * @attack: 
 *
 * Create audio signal with defaults.
 *
 * Since: 0.4
 */
void
ags_recycling_create_audio_signal_with_defaults(AgsRecycling *recycling,
						AgsAudioSignal *audio_signal,
						gdouble delay, guint attack)
{
  AgsAudioSignal *template;

  template = ags_audio_signal_get_template(recycling->audio_signal);

  audio_signal->delay = delay;
  audio_signal->attack = attack;

  if(template == NULL){
    ags_audio_signal_stream_resize(audio_signal,
				   0);
    return;
  }

  audio_signal->soundcard = template->soundcard;

  audio_signal->recycling = (GObject *) recycling;

  audio_signal->samplerate = template->samplerate;
  audio_signal->buffer_size = template->buffer_size;
  audio_signal->format = template->format;

  audio_signal->last_frame = (((guint)(delay *
				       template->buffer_size) +
			       attack +
			       template->last_frame) %
			      template->buffer_size);
  audio_signal->loop_start = (((guint) (delay *
					template->buffer_size) +
			       attack +
			       template->loop_start) %
			      template->buffer_size);
  audio_signal->loop_end = (((guint)(delay *
				     template->buffer_size) +
			     attack +
			     template->loop_end) %
			    template->buffer_size);

  ags_audio_signal_stream_resize(audio_signal,
				 template->length);

  ags_audio_signal_duplicate_stream(audio_signal,
				    template);
}

/**
 * ags_recycling_create_audio_signal_with_frame_count:
 * @recycling: an #AgsRecycling
 * @audio_signal: the #AgsAudioSignal to apply defaults 
 * @frame_count: the audio data size
 * @delay:
 * @attack: 
 *
 * Create audio signal with frame count.
 *
 * Since: 0.4
 */
void
ags_recycling_create_audio_signal_with_frame_count(AgsRecycling *recycling,
						   AgsAudioSignal *audio_signal,
						   guint frame_count,
						   gdouble delay, guint attack)
{
  AgsSoundcard *soundcard;
  AgsAudioSignal *template;
  GList *stream, *template_stream, *template_loop;
  guint frames_copied;
  guint loop_start, loop_attack;
  gboolean enter_loop, initial_loop;

  /* some init */
  template = ags_audio_signal_get_template(recycling->audio_signal);

  audio_signal->soundcard = template->soundcard;

  soundcard = AGS_SOUNDCARD(audio_signal->soundcard);

  audio_signal->recycling = (GObject *) recycling;

  /* resize */
  ags_audio_signal_stream_resize(audio_signal,
				 (guint) ceil(((double) delay +
					       (double) attack +
					       (double) frame_count) /
					      (double) audio_signal->buffer_size));
  
  if(template->length == 0)
    return;

  audio_signal->last_frame = ((guint) (delay * audio_signal->buffer_size) + frame_count + attack) % audio_signal->buffer_size;

  /* generic copying */
  stream = audio_signal->stream_beginning;
  template_stream = template->stream_beginning;

  frames_copied = 0;

  loop_start = template->loop_start;

  initial_loop = TRUE;

  /* loop related copying */
  if(frame_count >= template->loop_start){
    template_loop = g_list_nth(template->stream_beginning,
			       (guint) floor((double)loop_start / audio_signal->buffer_size));

    enter_loop = TRUE;
  }else{
    template_loop = NULL;
    enter_loop = FALSE;
  }

  /* the copy loops */
  while(stream != NULL && template_stream != NULL && frames_copied < frame_count){
    if(frames_copied + audio_signal->buffer_size < loop_start &&
       frames_copied < frame_count){
      ags_audio_signal_copy_buffer_to_buffer(&(((short *) stream->data)[attack]), 1,
					     (short *) template_stream->data, 1,
					     audio_signal->buffer_size - attack);

      if(stream->next != NULL && attack != 0){
	ags_audio_signal_copy_buffer_to_buffer((short *) stream->next->data, 1,
					       &(((short *) template_stream->data)[audio_signal->buffer_size - attack]), 1,
					       attack);
      }
    }

    if(enter_loop &&
       ((frames_copied > loop_start || frames_copied + audio_signal->buffer_size > loop_start) ||
	(frames_copied < frame_count))){
      if(template_stream == NULL){
	template_stream = template_loop;
      }

      if(initial_loop &&
	 (loop_start % audio_signal->buffer_size) == 0){
	loop_attack = 0;
      }else{
	loop_attack = loop_start % audio_signal->buffer_size;
      }

      initial_loop = FALSE;

      ags_audio_signal_copy_buffer_to_buffer(&(((short *) stream->data)[loop_attack]), 1,
					     &(((short *) template_stream->data)[audio_signal->buffer_size - loop_attack]), 1,
					     audio_signal->buffer_size - loop_attack);
      
      if(loop_attack != 0 && stream->next != NULL){
	ags_audio_signal_copy_buffer_to_buffer(&(((short *) stream->next->data)[loop_attack]), 1,
					       &(((short *) template_stream->data)[audio_signal->buffer_size - loop_attack]), 1,
					       loop_attack);
      }
    }

    stream = stream->next;
    template_stream = template_stream->next;
    frames_copied += audio_signal->buffer_size;
  }
}

/**
 * ags_recycling_find_next_channel:
 * @start_region: boundary start
 * @end_region: boundary end
 * @prev_channel: previous channel
 *
 * Retrieve next recycling with different channel.
 *
 * Returns: Matching recycling.
 *
 * Since: 0.4
 */
AgsRecycling*
ags_recycling_find_next_channel(AgsRecycling *start_region, AgsRecycling *end_region,
				GObject *prev_channel)
{
  AgsRecycling *recycling;

  recycling = start_region;

  while(recycling != end_region->next){
    if(recycling->channel != prev_channel){
      return(recycling);
    }

    recycling = recycling->next;
  }

  return(NULL);
}

/**
 * ags_recycling_position:
 * @start_region: boundary start
 * @end_region: boundary end
 * @recycling: matching recycling
 *
 * Retrieve position of recycling.
 *
 * Returns: position within boundary.
 *
 * Since: 0.4
 */
gint
ags_recycling_position(AgsRecycling *start_recycling, AgsRecycling *end_region,
		       AgsRecycling *recycling)
{
  AgsRecycling *current;
  gint position;

  if(start_recycling == NULL){
    return(-1);
  }

  current = start_recycling;
  position = -1;

  while(current != NULL && current != end_region){
    position++;

    if(current == recycling){
      return(position);
    }

    current = current->next;
  }

  return(-1);
}

/**
 * ags_recycling_new:
 * @soundcard: the #AgsSoundcard
 *
 * Creates a #AgsRecycling, with defaults of @soundcard.
 *
 * Returns: a new #AgsRecycling
 *
 * Since: 0.3
 */
AgsRecycling*
ags_recycling_new(GObject *soundcard)
{
  AgsRecycling *recycling;
  AgsAudioSignal *audio_signal;

  recycling = (AgsRecycling *) g_object_new(AGS_TYPE_RECYCLING, NULL);

  audio_signal = ags_audio_signal_new(soundcard,
				      (GObject *) recycling,
				      NULL);
  audio_signal->flags |= AGS_AUDIO_SIGNAL_TEMPLATE;

  ags_connectable_connect(AGS_CONNECTABLE(audio_signal));

  recycling->audio_signal = g_list_alloc();
  recycling->audio_signal->data = (gpointer) audio_signal;

  return(recycling);
}
