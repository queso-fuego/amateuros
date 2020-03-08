Overview/Goals:
---------------
- Homemade x86(_64?) operating system, as from scratch as possible, more over time. Will eventually try to do 32/64bit protected mode, 
and develop the OS from within itself, including homemade assembler, compiler, programming languages, GUIs, applications, window managers, etc.
- Initial reasoning & motivation for this project was to learn x86 16bit assembly; as such, this will be the primary language until further from-scratch
developments are made or interests change to C or other languages

Videos / Documenting Progress:
------------------------------
All progress, or as much as (hopefully!) every new/changed line of code, will be documented on video in a youtube playlist on my main channel here:
https://www.youtube.com/QuesoFuego

Playlist link:
https://www.youtube.com/playlist?list=PLT7NbkyNWaqajsw8Xh7SP9KJwjfpP8TNX

- All development is currently done on "live" recordings, and will probably be arduous or boring for most people to watch, it is later edited down before uploading

*Suggestions or comments regarding videos should be handled in either video comments or through email - fuegoqueso at gmail dot com*

** The rollout of these will most likely be slow (weeks to months). I have a full time job and lack time/energy/motivation most days to do too much 
    outside of research.**
   
** Any updates to this repo will occur sometime after recording a video and uploading to youtube**

Tools used for these videos:
- recording: OBS Studio
- video editor: Davinci Resolve
- audio separating/light edits as needed: Audacity
- OS: Windows 10
- microphone: Audio Technica AT2005USB
- webcam: Logitech C920

Development:
------------
Right now, subject to change, this OS is developed with: 
- openBSD vm using VMware Workstation Player
- emacs
- bochs emulator
- fasm assembler

But this may change later on to more fully develop the OS within itself, that is, running the OS.bin binary and editing the binary during runtime from within itself.
In this event, changes to the binary would be uploaded to this gitlab (or other) repo as time allows, but relevant changes would not necessarily be seen in source 
files. I'm assuming only the binary file would be changing at that point, so may take a different approach at that time to better document changes.
