---
layout: post
title:  "Refactoring GSequencer libraries"
date:   2015-04-06 12:27:26
categories: gsequencer specifications
---
GSequencer development tree just passed a big refactoring. The AgsMain struct disappeared from code base. It was superseeded by AgsApplicationContext and derivates. Note each library has it's very own AgsApplicationContext and they are connected by its sibling field.

Due the refactoring didn't pass entirely we're heading incremental fixes. At this time, there still some linker errors mostly of missing includes. This is going to be fixed very soon. For now, GSequencer knows following contices:

* AgsApplicationContext
* AgsServerApplicationContext
* AgsThreadApplicationContext
* AgsAudioApplicationContext
* AgsXorgApplicationContext

The next days work will be in bringing back programm stability as known of previous stable release 0.4.2-44. The AgsFile object got its refactoring, too. The Doctype Definition got some incompatible changes so we need support for 0.4.2 files in 0.4.3 release. This can be done easily by using Xml Stylesheet Language Transformation (abbr. XSLT).

This is important to implement to support previous file formats notably pre 0.4.0 version. Me expect the refactoring being done in 1-2 days. At this point me want to move the following tasks to directory src/ags/X/task:

* src/ags/audio/task/ags_add_line_member.c
* src/ags/audio/task/ags_add_bulk_member.c
* src/ags/audio/task/ags_update_bulk_member.c
* src/ags/audio/task/ags_add_point_to_selection.c
* src/ags/audio/task/ags_add_region_to_selection.c
* src/ags/audio/task/ags_change_indicator.c
* src/ags/audio/task/ags_change_tact.c	
* src/ags/audio/task/ags_display_tact.c	
* src/ags/audio/task/ags_free_selection.c
* src/ags/audio/task/ags_remove_point_from_selection.c
* src/ags/audio/task/ags_save_file.c	
* src/ags/audio/task/ags_scroll_on_play.c
* src/ags/audio/task/ags_toggle_led.c
* src/ags/audio/task/ags_toggle_pattern_bit.c

Further the AgsTask class is moved to src/ags/thread. And the the file IO is moved to it's specified dependency directory. The AgsFile as well AgsConfig class but is packaged with libags. Whereby ags_file.[ch] and related remains in its directory.

ags_config_set() and ags_config_get() was renamed to ags_config_get_value() and ags_config_set_value().

In front of these changes the GSequencer library is completely modular and reusable.