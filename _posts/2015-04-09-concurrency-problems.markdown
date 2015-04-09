---
layout: post
title:  "Concurrency problems with Advanced Gtk+ Sequencer"
date:   2015-04-09 17:37:44
categories: gsequencer specifications
---
As you won't be benefit of reliability while doing concurrent operations with ordinary computer's - except those with Error Correction Code (abbr. ECC). There will be added to Advanced Gtk+ Sequencer version 0.4.2 an additional object called AgsMutexManager.
It basically provides a object to mutex mapping. Note it won't replace global variable ags_application_mutex and later in gsequencer application_mutex field of AgsApplicationContext. Rather you have to lock it prior accessing the manager.

That's why me decided to still set the 0.4.2 release under active development. Thus it was imported as a git branch as well prior 0.4.x series. The API is not going to change more additional code is in needed in order to access fields of audio layer by using those mapped mutices. Me promises by these changes more reliability of the engine on ordinary computers.

The nature of concurrency problem is that it can happen that simultaneous reads can occure. This is very bad because this crashes Advanced Gtk+ Sequencer.

Conclusion `ags` doesn't contain concurrent write access, hope so :)
