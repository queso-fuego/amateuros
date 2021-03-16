Overview/Goals:
---------------
- Homemade x86(_64?) operating system, as from scratch as possible, more over time. Currently 32bit protected mode, but will eventually try 64bit long mode. 
Current medium term goal is to develop the OS from within itself, including a homemade toolchain: assembler, compiler, programming language(s), etc. Will later try to include 
GUIs, applications, window managers, and other things. Planning on hardware support for (at minimum? at most?) a thinkpad x60, if possible.

- Initial reasoning & motivation for this project was to learn x86 16bit assembly; This is done for the most part, as the project is now primarily in C.
The only assembly should be touches of inline asm as needed going forward. However, if it's fun or interesting to me, I'll still develop some things as standalone assembly code,
probably using Netwide Assembler (NASM) as it seems to be more portable for my systems than flat assembler (FASM), at least for an eventual 64bit change and using an OpenBSD
development environment. 

- The other main reason was to learn OS development as a newbie with no prior knowledge, and no formal CS eduacation. I have a weird fascination with from-scratch tools and
programming, and in an ideal world with enough time, I could have a full computer with self-made hardware/software stack, from transistors to internet browser and games, as 
simple as possible, for 1 person to maintain.
  This is an attempt to start that, and could grow into the other areas over time. This is also the largest personal project and learning/research experience I've done
so far in my life, and it's really an amateur trying to make something that will take years, for his own self-interest, and not for any others really. It might look simple,
amatuerish, and incomplete. Because it is! Keep that in mind [|:^) 

- Feel free to fork or make your own changes to your own repos, the license is effectively public domain. Suggestions or improvements are welcome, but they will be covered 
in a video if used (and will credit you, unless you say otherwise). I might open up this repo to the public in the future, but currently lack sufficient time to manage that. 

Current Standing:
-----------------
- ASM -> C conversion is complete (sans the bootsector/2ndstage bootloader). The OS from initial kernel boot onward is in C, and further development should continue to be so.
- 32bit protected mode, all ring 0, no paging (yet). Will probably stick to ring 0 only when/if it's set up, and plan on paging and memory management in the nearish future.
- No interrupts (yet). This is really a basic almost functioning shell of a start of an OS, for now.
- Vesa Bios Extensions used, for 1920x1080 32bpp mode. This is set up in 2ndstage.asm, and can be changed as needed, if a desired resolution/bpp is found on your qemu setup.
All text printing is done assuming 1080p 32bpp however, so until that's made generic, you'd need to change some hardcoded values in places.
- Barely functioning text/hex editor for 512 byte files, and a calculator. It's mainly write once, editing previous files isn't guranteed to work at all for text files, yet. 
More programs to come in the future.
- Ability to save and load text or binary (hex) files. Bin files can be run from the hex editor or main kernel command line, assuming they're valid x86 32bit code, and fit
within 512 bytes. Bin files are auto-ended with a '0xCB', or far return. That isn't guaranteed to work, and will be changed when a 'more proper' memory manager and program loader
is developed.

TODO (There's ALWAYS more to do, this list may not get updated as much as I'd like):
------------------------------------------------------------------------------------
In no particular order:
- Physical memory manager, after this, paging for 32bit protected mode, then a virtual memory manager. Then change to 64bit long mode, maybe with PAE or other RAM extending stuff.
- Task scheduling? Not sure yet, may start with round-robin or another simple way to do processes. Would need reliable time tracking, millisecond level at least.
- Interrupts, with an interrupt descriptor table / IDT, for the keyboard and disk reading/loading at minimum.
- Somehow convert the bootsector and bootloader to C. The bootsector is fine, if enough code is moved elsewhere so that it fits in 512 bytes with the AA55h signature, 
but I have had no luck so far with structures and intermediate (to me) level C code working with inline asm and 16bit, to allow the bootloader to work effectively.
- Other/better device drivers, USB, something for SATA or SSD storage, mouse, etc.
- Graphics primitives - lines/polygons, circles, triangles, drawRect() routines and the like
- A C standard library: string abstractions, type conversions, other stuff to help out with C code development.
- Assembler and Compiler for x86 32bit code, with a C-like language. Possibly making a forth or other languages later on too.
- Games, or other graphical things. Or text based games too
- Read and use other fonts, font standards. Such as PC screen font or some types of regular bitmapped fonts
- Font editor program, for homemade fonts at least
- Implement options/flags for the kernel "shell", and other shell commands. Also a way for a user to add shell commands and aliases
- Fix up/improve editor to allow arbitrary saving/updating of files, and backspace/delete functionality
- A windowing system? If the task scheduling and process creation/management gets done
- Get this thing to run on me old thinkpad, just to say that I can and that it can run on actual hardware

- Whatever else comes up...

Videos / Documenting Progress:
------------------------------
All progress, or as much as (hopefully!) every new/changed line of code, will be documented on video in a youtube playlist on my main channel here:
https://www.youtube.com/QuesoFuego

Playlist link:
https://www.youtube.com/playlist?list=PLT7NbkyNWaqajsw8Xh7SP9KJwjfpP8TNX

- All development is currently done on "live" recordings, and would probably be arduous or boring for most people to watch unedited. Footage is edited down before uploading to cut out long pauses, gratuitous ums and ahs, redundant info, off-topic ramblings, etc.

*Suggestions or comments regarding videos should be handled in either video comments, twitter @Queso_Fuego, or through email - fuegoqueso at gmail dot com*

** The rollout of these will most likely be slow (weeks to months). I have a full time job and lack time/energy/motivation most days to do too much 
    outside of research. I can and will respond to messages made in either Youtube video comments, twitter, or by email however.**
   
** Any updates to this repo will not necessarily occur at the same time as recording a video or uploading to youtube, it could be before or after**

Tools used for these videos:
- recording: OBS Studio
- video editor: Davinci Resolve
- audio separating/light edits as needed: Audacity
- OS: Windows 10 Enterprise
- microphone: Audio Technica AT2005USB
- webcam/camera: Sony ZV-1
- mouse: Logitech G502
- keyboard: previously a Leopold FC900R w/cherry black switches. Currently, an HHKB professional hybrid type S. It is not worth the price, but it is quite nice.

Development:
------------
Right now, subject to change, this OS is developed with: 
- openBSD vm using VMware Workstation Player (32bit, will move to a 64bit vm eventually)
- vim (will move to neovim eventually)
- qemu emulator (sometimes bochs)
- fasm assembler (as needed, will switch to NASM for future asm development)

This may change later on if I more fully develop the OS within itself; that is, running the OS binary and editing the binary during runtime from within itself, using the OS's
own editors, languages, and toolchains.
In this event, changes to the binary would still be uploaded to this (or other) repo as time allows, but relevant changes would not necessarily be seen in source 
files. I'm assuming only the binary file would be changing at that point, so I may take a different approach at that time to better document changes.

Screenshots:
------------
![Showing 'dir' command inside bochs dev environment](https://gitlab.com/queso_fuego/quesos/-/raw/master/screenshots/OS_Dev_1_2020_03_08.PNG "Basic Screenshot showing 'dir' command inside dev environment")
![Showing 'editor' program in hex editor mode, loading its own bootsector](https://gitlab.com/queso_fuego/quesos/-/raw/master/screenshots/OS_Dev_2020_07-17.PNG "Basic Screenshot showing 'editor' program loading its own bootsector")

These are out of date and more screenshots will be added in future

How to Build:
-------------
- Download & install bochs http://bochs.sourceforge.net/ or qemu https://www.qemu.org/download/ (or get either from your distro's package manager)
- Download & install make or ensure you have 'make' installed (bsd and gnu make should both work I think, though this is mainly tested with bsdmake)
- Download & install fasm/flat assembler https://flatassembler.net/download.php (nasm should work as well, though you will need minor changes to any pure .asm files)
- clone and cd to this repo's /build folder
- Ensure you have the .bochsrc file (if using bochs) in /bin, and the makefile in /build
- Run 'make OS' or 'make' from command line to build the OS.bin binary file in /bin
- For bochs: In the /bin folder, run 'bochs' or 'bochs -q', or some other way of starting bochs that you prefer (the .bochsrc file needs updating, use qemu if possible)
- For qemu: In the /build folder run 'make run', or in the /bin folder run 'qemu-system-i386 -drive format=raw,file=OS.bin,if=ide,index=0,media=disk'.
 
Note: Qemu seems to run and act better than bochs, so I have switched to using it full time. If anything is broken on bochs and not
  on qemu, let me know. The .bochsrc file does need to updated for an IDE or regular harddrive instead of a floppy, probably using ata0-master or something like that, so
  using qemu is recommended at this time.

