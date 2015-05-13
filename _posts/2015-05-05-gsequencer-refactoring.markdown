---
layout: post
title:  "GSequencer refactoring"
date:   2015-05-5 01:38:51
categories: gsequencer announcements
---
There many changes fulfilled me really appreciate. Like introduction of AgsApplicationContext and alike replacing AgsMain. The hardest refactoring was subsituting AgsDevout by AgsSoundcard, thought.
This evening me dealt with proper implementation of ags_channel_add_effect(), ags_line_add_effect() and ags_effect_line_add_effect() signals. Thus added the AgsAddEffect task. Additionally the DTD
definition got some changes and me implemented them immediately.

To separate GSequencer into self containing layers wasn't a big job because this was always kept in mind. So me removed Gtk+ includes of libags_audio.

There is one refactoring outstanding replacing mutex allocation within struct by dynamic allocation. Since me get really annoyed about helgrind warnings.

AgsMidiParser was added as well, it is capable of parsing midi files. So what remains for oncomming release 0.4.3 is implementing following:

* lv2 plugin support
* ladspa bridge
* replicator bridge
* midi in-/output
* automation editor
* implementing some recalls needed for above

Whereas the automation editor is almost done and the first four issues need a bit more work.

Me hope you like the new features and enriched GUI.
