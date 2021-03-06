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

#include <ags/X/ags_menu_bar_callbacks.h>

#include <ags/object/ags_application_context.h>
#include <ags/object/ags_connectable.h>
#include <ags/object/ags_applicable.h>
#include <ags/object/ags_soundcard.h>

#include <ags/file/ags_file.h>

#ifdef AGS_USE_LINUX_THREADS
#include <ags/thread/ags_thread-kthreads.h>
#else
#include <ags/thread/ags_thread-posix.h>
#endif 
#include <ags/thread/ags_task_thread.h>

#include <ags/audio/ags_input.h>
#include <ags/audio/ags_output.h>

#include <ags/file/task/ags_save_file.h>
#include <ags/audio/task/ags_add_audio.h>

#include <ags/X/ags_window.h>
#include <ags/X/ags_export_window.h>

#include <ags/X/machine/ags_panel.h>
#include <ags/X/machine/ags_mixer.h>
#include <ags/X/machine/ags_drum.h>
#include <ags/X/machine/ags_matrix.h>
#include <ags/X/machine/ags_synth.h>
#include <ags/X/machine/ags_ffplayer.h>

#include <ags/X/machine/ags_replicator_bridge.h>

#include <ags/X/machine/ags_ladspa_bridge.h>
#include <ags/X/machine/ags_lv2_bridge.h>

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>

#include <X11/Xlib.h>

void ags_menu_bar_open_ok_callback(GtkWidget *widget, AgsMenuBar *menu_bar);
void ags_menu_bar_open_cancel_callback(GtkWidget *widget, AgsMenuBar *menu_bar);

gboolean
ags_menu_bar_destroy_callback(GtkObject *object, AgsMenuBar *menu_bar)
{
  ags_menu_bar_destroy(object);

  return(TRUE);
}

void
ags_menu_bar_show_callback(GtkWidget *widget, AgsMenuBar *menu_bar)
{
  ags_menu_bar_show(widget);
}

void
ags_menu_bar_open_callback(GtkWidget *menu_item, AgsMenuBar *menu_bar)
{
  GtkFileSelection *file_selection;

  file_selection = (GtkFileSelection *) gtk_file_selection_new(g_strdup("open file\0"));
  gtk_file_selection_set_select_multiple(file_selection, FALSE);

  gtk_widget_show_all((GtkWidget *) file_selection);

  g_signal_connect((GObject *) file_selection->ok_button, "clicked\0",
		   G_CALLBACK(ags_menu_bar_open_ok_callback), menu_bar);
  g_signal_connect((GObject *) file_selection->cancel_button, "clicked\0",
		   G_CALLBACK(ags_menu_bar_open_cancel_callback), menu_bar);
}

void
ags_menu_bar_open_ok_callback(GtkWidget *widget, AgsMenuBar *menu_bar)
{
  AgsWindow *window;
  GtkFileSelection *file_selection;
  char *filename;
  gchar **argv;
  GError *error;

  window = (AgsWindow *) gtk_widget_get_toplevel((GtkWidget *) menu_bar);
  file_selection = (GtkFileSelection *) gtk_widget_get_ancestor(widget, GTK_TYPE_DIALOG);

  filename = g_strdup(gtk_file_selection_get_filename(file_selection));

  error = NULL;

  g_spawn_command_line_async(g_strdup_printf("./ags --filename %s\0",
					     filename),
			     &error);

  gtk_widget_destroy(file_selection);

  g_free(filename);
}

void
ags_menu_bar_open_cancel_callback(GtkWidget *widget, AgsMenuBar *menu_bar)
{
  GtkFileSelection *file_selection;

  file_selection = (GtkFileSelection *) gtk_widget_get_ancestor(widget, GTK_TYPE_DIALOG);
  gtk_widget_destroy((GtkWidget *) file_selection);
}

void
ags_menu_bar_save_callback(GtkWidget *menu_item, AgsMenuBar *menu_bar)
{
  AgsWindow *window;
  AgsFile *file;

  window = (AgsWindow *) gtk_widget_get_toplevel((GtkWidget *) menu_bar);

  //TODO:JK: revise me
  file = (AgsFile *) g_object_new(AGS_TYPE_FILE,
				  "application-context\0", window->application_context,
				  "filename\0", g_strdup(window->name),
				  NULL);
  ags_file_write(file);
  g_object_unref(G_OBJECT(file));
}

void
ags_menu_bar_save_as_callback(GtkWidget *menu_item, AgsMenuBar *menu_bar)
{
  AgsWindow *window;
  GtkFileChooserDialog *file_chooser;
  gint response;

  window = (AgsWindow *) gtk_widget_get_toplevel((GtkWidget *) menu_bar);

  file_chooser = (GtkFileChooserDialog *) gtk_file_chooser_dialog_new("save file as\0",
								      (GtkWindow *) window,
								      GTK_FILE_CHOOSER_ACTION_SAVE,
								      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
								      GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
								      NULL);
  gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(file_chooser), FALSE);
  gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(file_chooser), TRUE);
  gtk_widget_show_all((GtkWidget *) file_chooser);

  response = gtk_dialog_run(GTK_DIALOG(file_chooser));

  if(response == GTK_RESPONSE_ACCEPT){
    AgsSaveFile *save_file;

    AgsThread *main_loop;
    AgsTaskThread *task_thread;

    AgsApplicationContext *application_context;

    AgsFile *file;
    char *filename;

    application_context = window->application_context;
    
    main_loop = application_context->main_loop;
    
    task_thread = ags_thread_find_type(main_loop,
				       AGS_TYPE_TASK_THREAD);

    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser));
    window->name = filename;
    
    file = (AgsFile *) g_object_new(AGS_TYPE_FILE,
				    "application-context\0", application_context,
				    "filename\0", filename,
				    NULL);

    save_file = ags_save_file_new(file);
    ags_task_thread_append_task(task_thread,
				AGS_TASK(save_file));
  }

  gtk_widget_destroy((GtkWidget *) file_chooser);
}

void
ags_menu_bar_export_callback(GtkWidget *menu_item, AgsMenuBar *menu_bar)
{
  AgsWindow *window;

  window = (AgsWindow *) gtk_widget_get_toplevel((GtkWidget *) menu_bar);

  gtk_widget_show_all(window->export_window);
}

void
ags_menu_bar_import_midi_callback(GtkWidget *menu_item, AgsMenuBar *menu_bar)
{
  AgsWindow *window;

  window = (AgsWindow *) gtk_widget_get_toplevel((GtkWidget *) menu_bar);

  gtk_widget_show_all(window->import_window);
}

void
ags_menu_bar_quit_callback(GtkWidget *menu_item, AgsMenuBar *menu_bar)
{
  AgsWindow *window;
  GtkDialog *dialog;
  GtkWidget *cancel_button;
  AgsApplicationContext *application_context;
  gint response;

  window = (AgsWindow *) gtk_widget_get_toplevel((GtkWidget *) menu_bar);

  application_context = window->application_context;
  
  /* ask the user if he wants save to a file */
  dialog = (GtkDialog *) gtk_message_dialog_new(GTK_WINDOW(window),
						GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_MESSAGE_QUESTION,
						GTK_BUTTONS_YES_NO,
						"Do you want to save '%s'?\0", window->name);
  cancel_button = gtk_dialog_add_button(dialog,
					GTK_STOCK_CANCEL,
					GTK_RESPONSE_CANCEL);
  gtk_widget_grab_focus(cancel_button);

  response = gtk_dialog_run(dialog);

  if(response == GTK_RESPONSE_YES){
    AgsFile *file;

    //TODO:JK: revise me
    file = (AgsFile *) g_object_new(AGS_TYPE_FILE,
				    "main\0", application_context,
				    "filename\0", g_strdup(window->name),
				    NULL);

    ags_file_write(file);
    g_object_unref(G_OBJECT(file));
  }

  if(response != GTK_RESPONSE_CANCEL){
    ags_main_quit(application_context);
  }else{
    gtk_widget_destroy(GTK_WIDGET(dialog));
  }
}


void
ags_menu_bar_add_callback(GtkWidget *menu_item, AgsMenuBar *menu_bar)
{
}


void
ags_menu_bar_add_panel_callback(GtkWidget *menu_item, AgsMenuBar *menu_bar)
{
  AgsWindow *window;
  AgsPanel *panel;
  AgsAddAudio *add_audio;
  AgsThread *main_loop;
  AgsTaskThread *task_thread;
  AgsApplicationContext *application_context;

  window = (AgsWindow *) gtk_widget_get_ancestor((GtkWidget *) menu_bar, AGS_TYPE_WINDOW);

  application_context = window->application_context;
  
  main_loop = application_context->main_loop;
  task_thread = ags_thread_find_type(main_loop,
				     AGS_TYPE_TASK_THREAD);

  panel = ags_panel_new(G_OBJECT(window->soundcard));

  add_audio = ags_add_audio_new(window->soundcard,
				AGS_MACHINE(panel)->audio);
  ags_task_thread_append_task(task_thread,
			      AGS_TASK(add_audio));

  gtk_box_pack_start((GtkBox *) window->machines,
		     GTK_WIDGET(panel),
		     FALSE, FALSE, 0);

  ags_connectable_connect(AGS_CONNECTABLE(panel));

  gtk_widget_show_all(GTK_WIDGET(panel));

  AGS_MACHINE(panel)->audio->audio_channels = 2;
  ags_audio_set_pads(AGS_MACHINE(panel)->audio,
		     AGS_TYPE_INPUT, 1);
  ags_audio_set_pads(AGS_MACHINE(panel)->audio,
		     AGS_TYPE_OUTPUT, 1);

  ags_machine_find_port(AGS_MACHINE(panel));

  gtk_widget_show_all(panel->vbox);
}

void
ags_menu_bar_add_mixer_callback(GtkWidget *menu_item, AgsMenuBar *menu_bar)
{
  AgsWindow *window;
  AgsMixer *mixer;
  AgsAddAudio *add_audio;
  AgsThread *main_loop;
  AgsTaskThread *task_thread;
  AgsApplicationContext *application_context;

  window = (AgsWindow *) gtk_widget_get_ancestor((GtkWidget *) menu_bar, AGS_TYPE_WINDOW);

  application_context = window->application_context;
  
  main_loop = application_context->main_loop;
  task_thread = ags_thread_find_type(main_loop,
				     AGS_TYPE_TASK_THREAD);

  mixer = ags_mixer_new(G_OBJECT(window->soundcard));

  add_audio = ags_add_audio_new(window->soundcard,
				AGS_MACHINE(mixer)->audio);
  ags_task_thread_append_task(task_thread,
			      AGS_TASK(add_audio));

  gtk_box_pack_start((GtkBox *) window->machines,
		     GTK_WIDGET(mixer),
		     FALSE, FALSE, 0);

  ags_connectable_connect(AGS_CONNECTABLE(mixer));

  gtk_widget_show_all(GTK_WIDGET(mixer));

  mixer->machine.audio->audio_channels = 2;
  ags_audio_set_pads(mixer->machine.audio,
		     AGS_TYPE_INPUT, 8);
  ags_audio_set_pads(mixer->machine.audio,
		     AGS_TYPE_OUTPUT, 1);

  ags_machine_find_port(AGS_MACHINE(mixer));

  gtk_widget_show_all(mixer->input_pad);
}

void
ags_menu_bar_add_drum_callback(GtkWidget *menu_item, AgsMenuBar *menu_bar)
{
  AgsWindow *window;
  AgsDrum *drum;

  AgsAddAudio *add_audio;

  AgsThread *main_loop;
  AgsTaskThread *task_thread;

  AgsApplicationContext *application_context;

  window = (AgsWindow *) gtk_widget_get_ancestor((GtkWidget *) menu_bar, AGS_TYPE_WINDOW);

  application_context = window->application_context;
  
  main_loop = application_context->main_loop;
  task_thread = ags_thread_find_type(main_loop,
				     AGS_TYPE_TASK_THREAD);

  drum = ags_drum_new(G_OBJECT(window->soundcard));

  add_audio = ags_add_audio_new(window->soundcard,
				AGS_MACHINE(drum)->audio);
  ags_task_thread_append_task(task_thread,
			      AGS_TASK(add_audio));
  
  gtk_box_pack_start((GtkBox *) window->machines,
		     GTK_WIDGET(drum),
		     FALSE, FALSE, 0);

  /* connect everything */
  ags_connectable_connect(AGS_CONNECTABLE(drum));

  /* */
  gtk_widget_show_all(GTK_WIDGET(drum));

  /* */
  drum->machine.audio->audio_channels = 2;

  /* AgsDrumInputPad */
  ags_audio_set_pads(drum->machine.audio, AGS_TYPE_INPUT, 8);
  ags_audio_set_pads(drum->machine.audio, AGS_TYPE_OUTPUT, 1);

  ags_machine_find_port(AGS_MACHINE(drum));

  gtk_widget_show_all(drum->output_pad);
  gtk_widget_show_all(drum->input_pad);
}

void
ags_menu_bar_add_matrix_callback(GtkWidget *menu_item, AgsMenuBar *menu_bar)
{
  AgsWindow *window;
  AgsMatrix *matrix;
  AgsAddAudio *add_audio;
  AgsThread *main_loop;
  AgsTaskThread *task_thread;
  AgsApplicationContext *application_context;

  window = (AgsWindow *) gtk_widget_get_ancestor((GtkWidget *) menu_bar, AGS_TYPE_WINDOW);

  application_context = window->application_context;
  
  main_loop = application_context->main_loop;
  task_thread = ags_thread_find_type(main_loop,
				     AGS_TYPE_TASK_THREAD);

  matrix = ags_matrix_new(G_OBJECT(window->soundcard));

  add_audio = ags_add_audio_new(window->soundcard,
				AGS_MACHINE(matrix)->audio);
  ags_task_thread_append_task(task_thread,
			      AGS_TASK(add_audio));
  
  gtk_box_pack_start((GtkBox *) window->machines,
		     GTK_WIDGET(matrix),
		     FALSE, FALSE, 0);

  /* connect everything */
  ags_connectable_connect(AGS_CONNECTABLE(matrix));

  /* */
  gtk_widget_show_all(GTK_WIDGET(matrix));

  /* */
  matrix->machine.audio->audio_channels = 1;

  /* AgsMatrixInputPad */
  ags_audio_set_pads(matrix->machine.audio, AGS_TYPE_INPUT, 78);
  ags_audio_set_pads(matrix->machine.audio, AGS_TYPE_OUTPUT, 1);

  ags_machine_find_port(AGS_MACHINE(matrix));
}

void
ags_menu_bar_add_synth_callback(GtkWidget *menu_item, AgsMenuBar *menu_bar)
{
  AgsWindow *window;
  AgsSynth *synth;
  AgsAddAudio *add_audio;
  AgsThread *main_loop;
  AgsTaskThread *task_thread;
  AgsApplicationContext *application_context;

  window = (AgsWindow *) gtk_widget_get_ancestor((GtkWidget *) menu_bar, AGS_TYPE_WINDOW);

  application_context = window->application_context;
  
  main_loop = application_context->main_loop;
  task_thread = ags_thread_find_type(main_loop,
				     AGS_TYPE_TASK_THREAD);

  synth = ags_synth_new(G_OBJECT(window->soundcard));

  add_audio = ags_add_audio_new(window->soundcard,
				AGS_MACHINE(synth)->audio);
  ags_task_thread_append_task(task_thread,
			      AGS_TASK(add_audio));

  gtk_box_pack_start((GtkBox *) window->machines,
		     (GtkWidget *) synth,
		     FALSE, FALSE, 0);

  ags_connectable_connect(AGS_CONNECTABLE(synth));

  synth->machine.audio->audio_channels = 1;
  ags_audio_set_pads((AgsAudio*) synth->machine.audio, AGS_TYPE_INPUT, 2);
  ags_audio_set_pads((AgsAudio*) synth->machine.audio, AGS_TYPE_OUTPUT, 78);

  ags_machine_find_port(AGS_MACHINE(synth));

  gtk_widget_show_all((GtkWidget *) synth);
}

void
ags_menu_bar_add_ffplayer_callback(GtkWidget *menu_item, AgsMenuBar *menu_bar)
{
  AgsWindow *window;
  AgsFFPlayer *ffplayer;
  AgsAddAudio *add_audio;
  AgsThread *main_loop;
  AgsTaskThread *task_thread;
  AgsApplicationContext *application_context;

  window = (AgsWindow *) gtk_widget_get_ancestor((GtkWidget *) menu_bar, AGS_TYPE_WINDOW);

  application_context = window->application_context;
  
  main_loop = application_context->main_loop;
  task_thread = ags_thread_find_type(main_loop,
				     AGS_TYPE_TASK_THREAD);

  ffplayer = ags_ffplayer_new(G_OBJECT(window->soundcard));

  add_audio = ags_add_audio_new(window->soundcard,
				AGS_MACHINE(ffplayer)->audio);
  ags_task_thread_append_task(task_thread,
			      AGS_TASK(add_audio));

  gtk_box_pack_start((GtkBox *) window->machines,
		     (GtkWidget *) ffplayer,
		     FALSE, FALSE, 0);

  ags_connectable_connect(AGS_CONNECTABLE(ffplayer));

  //  ffplayer->machine.audio->frequence = ;
  ffplayer->machine.audio->audio_channels = 2;
  ags_audio_set_pads(AGS_MACHINE(ffplayer)->audio, AGS_TYPE_INPUT, 78);
  ags_audio_set_pads(AGS_MACHINE(ffplayer)->audio, AGS_TYPE_OUTPUT, 1);

  ags_machine_find_port(AGS_MACHINE(ffplayer));

  gtk_widget_show_all((GtkWidget *) ffplayer);
}

void
ags_menu_bar_add_replicator_bridge_callback(GtkWidget *menu_item, AgsMenuBar *menu_bar)
{
  AgsWindow *window;
  AgsReplicatorBridge *replicator_bridge;
  AgsAddAudio *add_audio;
  AgsThread *main_loop;
  AgsTaskThread *task_thread;
  AgsApplicationContext *application_context;

  window = (AgsWindow *) gtk_widget_get_ancestor((GtkWidget *) menu_bar, AGS_TYPE_WINDOW);

  application_context = window->application_context;
  
  main_loop = application_context->main_loop;
  task_thread = ags_thread_find_type(main_loop,
				     AGS_TYPE_TASK_THREAD);

  replicator_bridge = ags_replicator_bridge_new(G_OBJECT(window->soundcard));

  add_audio = ags_add_audio_new(window->soundcard,
				AGS_MACHINE(replicator_bridge)->audio);
  ags_task_thread_append_task(task_thread,
			      AGS_TASK(add_audio));

  gtk_box_pack_start((GtkBox *) window->machines,
		     (GtkWidget *) replicator_bridge,
		     FALSE, FALSE, 0);
  
  ags_connectable_connect(AGS_CONNECTABLE(replicator_bridge));

  //  replicator_bridge->machine.audio->frequence = ;
  replicator_bridge->machine.audio->audio_channels = 2;
  ags_audio_set_pads(AGS_MACHINE(replicator_bridge)->audio, AGS_TYPE_INPUT, 1);
  ags_audio_set_pads(AGS_MACHINE(replicator_bridge)->audio, AGS_TYPE_OUTPUT, 1);

  ags_machine_find_port(AGS_MACHINE(replicator_bridge));

  gtk_widget_show_all((GtkWidget *) replicator_bridge);
}

void
ags_menu_bar_add_ladspa_bridge_callback(GtkWidget *menu_item, AgsMenuBar *menu_bar)
{
  AgsWindow *window;
  AgsLadspaBridge *ladspa_bridge;
  AgsAddAudio *add_audio;
  AgsThread *main_loop;
  AgsTaskThread *task_thread;
  AgsApplicationContext *application_context;

  window = (AgsWindow *) gtk_widget_get_ancestor((GtkWidget *) menu_bar, AGS_TYPE_WINDOW);

  application_context = window->application_context;
  
  main_loop = application_context->main_loop;
  task_thread = ags_thread_find_type(main_loop,
				     AGS_TYPE_TASK_THREAD);

  ladspa_bridge = ags_ladspa_bridge_new(G_OBJECT(window->soundcard),
					g_object_get_data(menu_item,
							  AGS_MENU_BAR_LADSPA_FILENAME_KEY),
					g_object_get_data(menu_item,
							  AGS_MENU_BAR_LADSPA_EFFECT_KEY));

  add_audio = ags_add_audio_new(window->soundcard,
				AGS_MACHINE(ladspa_bridge)->audio);
  ags_task_thread_append_task(task_thread,
			      AGS_TASK(add_audio));

  gtk_box_pack_start((GtkBox *) window->machines,
		     (GtkWidget *) ladspa_bridge,
		     FALSE, FALSE, 0);
  ags_ladspa_bridge_load(ladspa_bridge);
  
  ags_connectable_connect(AGS_CONNECTABLE(ladspa_bridge));

  //  ladspa_bridge->machine.audio->frequence = ;
  ladspa_bridge->machine.audio->audio_channels = 2;
  ags_audio_set_pads(AGS_MACHINE(ladspa_bridge)->audio, AGS_TYPE_INPUT, 1);
  ags_audio_set_pads(AGS_MACHINE(ladspa_bridge)->audio, AGS_TYPE_OUTPUT, 1);

  ags_machine_find_port(AGS_MACHINE(ladspa_bridge));

  gtk_widget_show_all((GtkWidget *) ladspa_bridge);
}

void
ags_menu_bar_add_lv2_bridge_callback(GtkWidget *menu_item, AgsMenuBar *menu_bar)
{
  AgsWindow *window;
  AgsLv2Bridge *lv2_bridge;
  AgsAddAudio *add_audio;
  AgsThread *main_loop;
  AgsTaskThread *task_thread;
  AgsApplicationContext *application_context;

  window = (AgsWindow *) gtk_widget_get_ancestor((GtkWidget *) menu_bar, AGS_TYPE_WINDOW);

  application_context = window->application_context;
  
  main_loop = application_context->main_loop;
  task_thread = ags_thread_find_type(main_loop,
				     AGS_TYPE_TASK_THREAD);

  lv2_bridge = ags_lv2_bridge_new(G_OBJECT(window->soundcard),
				  g_object_get_data(menu_item,
						    AGS_MENU_BAR_LV2_FILENAME_KEY),
				  g_object_get_data(menu_item,
						    AGS_MENU_BAR_LV2_URI_KEY));
  
  add_audio = ags_add_audio_new(window->soundcard,
				AGS_MACHINE(lv2_bridge)->audio);
  ags_task_thread_append_task(task_thread,
			      AGS_TASK(add_audio));

  gtk_box_pack_start((GtkBox *) window->machines,
		     (GtkWidget *) lv2_bridge,
		     FALSE, FALSE, 0);
  ags_lv2_bridge_load(lv2_bridge);
  
  ags_connectable_connect(AGS_CONNECTABLE(lv2_bridge));

  //  lv2_bridge->machine.audio->frequence = ;
  lv2_bridge->machine.audio->audio_channels = 2;
  ags_audio_set_pads(AGS_MACHINE(lv2_bridge)->audio, AGS_TYPE_INPUT, 1);
  ags_audio_set_pads(AGS_MACHINE(lv2_bridge)->audio, AGS_TYPE_OUTPUT, 1);

  ags_machine_find_port(AGS_MACHINE(lv2_bridge));

  gtk_widget_show_all((GtkWidget *) lv2_bridge);
}

void
ags_menu_bar_remove_callback(GtkWidget *menu_item, AgsMenuBar *menu_bar)
{
  //TODO:JK: implement me
}

void
ags_menu_bar_automation_editor_callback(GtkWidget *menu_item, AgsMenuBar *menu_bar)
{
  AgsWindow *window;

  window = (AgsWindow *) gtk_widget_get_ancestor((GtkWidget *) menu_bar, AGS_TYPE_WINDOW);
  
  gtk_widget_show_all(window->automation_editor);
}

void
ags_menu_bar_preferences_callback(GtkWidget *menu_item, AgsMenuBar *menu_bar)
{
  AgsWindow *window;

  window = (AgsWindow *) gtk_widget_get_ancestor((GtkWidget *) menu_bar, AGS_TYPE_WINDOW);

  if(window->preferences != NULL){
    return;
  }

  window->preferences = ags_preferences_new();
  window->preferences->parent = GTK_WINDOW(window);

  ags_applicable_reset(AGS_APPLICABLE(window->preferences));

  ags_connectable_connect(AGS_CONNECTABLE(window->preferences));
  gtk_widget_show_all(GTK_WIDGET(window->preferences));
}

void
ags_menu_bar_about_callback(GtkWidget *menu_item, AgsMenuBar *menu_bar)
{
  static FILE *file = NULL;
  struct stat sb;
  static gchar *license;
  static GdkPixbuf *logo;
  GError *error;

  gchar *authors[] = { "Joël Krähemann\0", NULL }; 

  if(file == NULL){
    file = fopen("./COPYING\0", "r\0");
    stat("./COPYING\0", &sb);
    license = (gchar *) malloc((sb.st_size + 1) * sizeof(gchar));
    fread(license, sizeof(char), sb.st_size, file);
    license[sb.st_size] = '\0';
    fclose(file);

    error = NULL;

    logo = gdk_pixbuf_new_from_file("./doc/images/ags.png\0", &error);
  }

  gtk_show_about_dialog((GtkWindow *) gtk_widget_get_ancestor((GtkWidget *) menu_bar, GTK_TYPE_WINDOW),
			"program-name\0", "ags\0",
			"authors\0", authors,
			"license\0", license,
			"version\0", AGS_VERSION,
			"website\0", "http://ags.sf.net\0",
			"title\0", "Advanced Gtk+ Sequencer\0",
			"logo\0", logo,
			NULL);
}
