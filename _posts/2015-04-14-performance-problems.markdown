---
layout: post
title:  "Performance problems"
date:   2015-04-14 18:53:11
categories: gsequencer announcements
---
Since we have the problem that the GUI slows down the engine. Me decided to run its thread in his very own main loop. This should fix this issue. There are some other feature's that to be fixed like muting channels and playback in AgsDrumInputPad.

So there following at least 2-3 more fix releases. First me targets the perfomance issue and later missing features.

I really enjoy the occasion and hope me can keep my promises of giving you a first production environment.

The really bad throughput was detected as trying to do a screencast. Me recognized exported audio has a much shorter duration as captured screen. So me have to say current versions have problems to stay in realtime.
