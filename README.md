Overview/Goals:
---------------
- Homemade x86(_64?) operating system, as from scratch as possible, more over time. Will eventually try to do 32/64bit protected mode/long mode, 
and develop the OS from within itself, including homemade assembler, compiler, programming languages, GUIs, applications, window managers, etc.
- Initial reasoning & motivation for this project was to learn x86 16bit assembly; as such, this will be the primary language until further from-scratch
developments are made or interests change to C or other languages
- Feel free to fork or make your own changes to your own repos, the license is effectively public domain. Suggestions or improvements are welcome, but they will be covered in a video if used. I might open up this repo in the future, but currently lack sufficient time to manage that. 

Videos / Documenting Progress:
------------------------------
All progress, or as much as (hopefully!) every new/changed line of code, will be documented on video in a youtube playlist on my main channel here:
https://www.youtube.com/QuesoFuego

Playlist link:
https://www.youtube.com/playlist?list=PLT7NbkyNWaqajsw8Xh7SP9KJwjfpP8TNX

- All development is currently done on "live" recordings, and would probably be arduous or boring for most people to watch unedited. Footage is edited down before uploading to cut out long pauses, gratuitous ums and ahs, redundant info, off-topic ramblings, etc.

*Suggestions or comments regarding videos should be handled in either video comments, twitter @Queso_Fuego, or through email - fuegoqueso at gmail dot com*

** The rollout of these will most likely be slow (weeks to months). I have a full time job and lack time/energy/motivation most days to do too much 
    outside of research.**
   
** Any updates to this repo will not necessarily occur at the same time as recording a video or uploading to youtube, it could be before or after**

Tools used for these videos:
- recording: OBS Studio
- video editor: Davinci Resolve
- audio separating/light edits as needed: Audacity
- OS: Windows 10 LTSC
- microphone: Audio Technica AT2005USB
- webcam: Logitech C920

Development:
------------
Right now, subject to change, this OS is developed with: 
- openBSD vm using VMware Workstation Player
- vim
- qemu emulator (sometimes bochs)
- fasm assembler

But this may change later on if I more fully develop the OS within itself, that is, running the OS binary and editing the binary during runtime from within itself.
In this event, changes to the binary would be uploaded to this gitlab (or other) repo as time allows, but relevant changes would not necessarily be seen in source 
files. I'm assuming only the binary file would be changing at that point, so I may take a different approach at that time to better document changes.

Screenshots:
------------
![Basic Screenshot showing 'dir' command inside dev environment](https://gitlab.com/queso_fuego/quesos/-/blob/master/OS_Dev_1_2020_03_08.PNG "Basic Screenshot showing 'dir' command inside dev environment")

More screenshots will be added in future

How to Build:
-------------
- Download & install bochs http://bochs.sourceforge.net/ or qemu https://www.qemu.org/download/ (or get either from your distro's package manager)
- Download & install make or ensure you have 'make' installed (bsd and gnu make should both work I think, though this is mainly tested with bsdmake)
- Download & install fasm/flat assembler https://flatassembler.net/download.php
- clone and cd to this repo's /build folder
- Ensure you have the .bochsrc file (if using bochs) in /bin, and the makefile in /build
- Run 'make OS' or 'make' from command line to build the OS.bin binary file in /bin
- For bochs: In the /bin folder, run 'bochs' or 'bochs -q', or some other way of starting bochs that you prefer
- For qemu: In the /build folder run 'make run', or in the /bin folder run 'qemu-system-i386 -fda OS.bin'. This will generate a warning message by default, but that is harmless
 
Note: Qemu seems to run and act better than bochs, so I have switched to using it full time instead. If anything is broken on bochs and not
  on qemu, let me know. 

