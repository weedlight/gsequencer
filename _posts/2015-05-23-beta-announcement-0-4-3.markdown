---
layout: post
title:  "Announcement of 0.4.3-beta - soon come"
date:   2015-05-23 11:04:11
categories: gsequencer announcements
---
GSequencer 0.4.3-beta comes soon, it takes probably 2-3 days until you get it. There are only a few tasks remaining me still want to do therefor. Like the automation editor to implement and do some basic testing. Don't forget XML Input/Output to save your changes and restore.
Following changes after beta release may not significantly involve the users but developers. The ABI changes definitly since full AgsConnectabl::disconnect() is intended to be added.

What me did so far is Lv2ui support and midi import wizard. And lots of new machines representing LADSPA or LV2 plugins using AgsEffectBulk object.
AgsMatrix, AgsSynth and AgsFFPlayer may for now include effect processors with GUI control as well.

