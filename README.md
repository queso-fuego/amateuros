Overview/Goals:
---
* Homemade x86(_64?) operating system, as from scratch as possible, more over time. Currently 32bit protected mode using legacy boot, but will eventually try 64bit long mode and UEFI. 
The current medium-term goal is to develop the OS from within itself, editing its own source files using a homemade toolchain: editor, assembler, compiler, programming language(s), etc. Will later try to include 
GUIs, applications, window managers, and other things. Planning on hardware support for (at minimum? at most?) a thinkpad x60, if possible. If reached, then a 2019 dell xps, or maybe a desktop PC.
Currently runs in qemu and bochs. 
The main goal is learning from a bottom-up perspective, and doing everything homemade, originally starting from a 512byte empty bootsector. Little to no external dependencies needed, other than a recent
C compiler, an x86 assembler, and make. And an emulator for testing/developing. I assume a linux/unix environment for building, and do not (yet) provide any pre-built binaries/ISOs.

* Initial reasoning & motivation for this project was to learn x86 16bit real mode assembly; This is mostly done, as it's now developed in C with spatterings of 32bit asm/inline asm.
However, when it's fun or interesting, I'll still develop some things as fully standalone assembly code.

* The other main reason was to learn OS development and systems programming as a newbie with no prior knowledge and no formal CS education. I have an interest in from-scratch tools and
programming, and in an ideal world with enough time I could have a full computer with self-made hardware/software stack, from transistors to internet browser and games, as 
simple as possible, for 1 person to maintain.
  This is sort of an attempt to start that, and could grow into other non-OS areas over time. It's also an attempt to get better at explaining my
thought processes and how I work through things, as well as improve my speaking in general, through the development videos. 
I'm learning bits of computer science from slowly going through MIT lectures and various books, so hopefully the code and knowledge improves tangibly over time.
This is the largest personal project and learning/research experience I've done so far, and it's a newbie trying to make something that will take a
long time for his own self-interest. It might look simple, amateurish, and incomplete. Because it is! Keep that in mind [|:^) 

* Feel free to fork or make your own changes to your own repos, the license is effectively public domain (0-clause BSD license). Suggestions or improvements are welcome, and will be covered 
in a video if used (and will credit you, if I remember and you don't say otherwise). I might open up this repo to the public in the future, but currently lack 
sufficient time to manage that, and would rather work on things solo for the time being. 

Project Structure:
---
* /bin holds intermediate binary files during the build process, and the final OS.bin file to run. 
* /build holds linker scripts for C source files, a makefile to build the project, and other supporting scripts/files.
* /include holds subdirectories containing header files to be included in the main source files. These are mainly .h C files, without .c counterparts, all the code in the header. It mostly works ok.
* /src holds the main source files that are used to build the intermediate binary files and final OS.bin file.

Current Standing:
---
* 32bit protected mode, all ring 0, no paging (yet). Will probably stick to ring 0 only when/if it's set up, and plan on paging and memory management in the nearish future.
* Initial interrupt support for exceptions, system calls (using int 0x80), regular software interrupts, and the PIC. Limited to no interrupt handlers are implemented for most of these, but there are places and functions to implement your own as desired. 
* Vesa Bios Extensions for graphics modes. On boot you can type in desired X resolution, Y resolution, and bits per pixel values, or take a default of 1920x1080 32bpp. 
If trying to run on actual hardware, ensure you know what your hardware supports! Trying to run unsupported modes may damage your hardware!!
* "Generic" bitmap font support. Add a font in the style of the current fonts in /src (see testfont or termu* files) with a 2 byte width/height "header", add the filename to ASM_FILES in the makefile, and use 'chgFont <filename>' after booting.
* Barely functioning text/hex editor for 512 byte length files, and a 4 function calculator. More programs to come in the future.
* Ability to save and load text or binary (hex) files. Valid x86 binary files can be run from the hex editor if the file length is <= 1 sector, or from the main kernel command line if set up in the file table.
Binary files written in the hex editor are auto-ended with a '0xCB', or far return. That probably doesn't work, and will chang when better memory management and program loading is developed.
* Several commands available for the in-built kernel "shell" such as del, ren, chgColors, chgFont, etc. A list of available commands is in the kernel.c source, in main(), where
they're prefixed with "cmd". Eventually there should be a better implementation like a "help" command or similar to list available commands and descriptions at runtime.

TODO (There's ALWAYS more to do e.g. lots of TODOs in the source files, this list is updated when/if I remember):
---
In no particular order:
* Paging for 32bit protected mode, and a virtual memory manager. Then a change/addition for 64bit long mode, maybe with PAE or other RAM extending stuff.
* UEFI, as an alternative to the current bootsector and bootloader. It could be all in C but has it's own ways of handling device discovery and setting video modes and things.
  * Ultimately, if not too much work, I'd like to maintain separate versions for: 1) 32bit x86 (i386 minimum) BIOS, 2) 64bit x86 UEFI. 
Any additional versions for other architectures can be worried about later, ARM, PPC, and so on, if it gets to that point and better/more general abstractions are in place for any needed asm code. Possibly having separate all assembly versions of these 2 overall OS versions as well, to check size/speed differences, large project structure, etc.
* Task scheduling? Not sure yet on implementations, may do round-robin or another simple way to do processes. Would need reliable time tracking, at the millisecond level at least (using PIT, HPET, APIC, TSC, other timers?).
* Interrupt handlers for keyboard, timer(s), and disk reading/loading at minimum for the PIC, and system calls (using int 0x80) for malloc/free and other needed things. Probably using APIC & IOAPIC to replace PIC later on.
* Somehow convert the bootsector and bootloader to C. The bootsector is fine, if enough code is moved elsewhere so that it fits in 512 bytes with the AA55h signature, 
but I have had no luck so far with structures and intermediate (to me) C code working with 16bit inline asm, to allow the bootloader to work effectively. It may be due to only having 32bit addresses and memory instructions available in clang/gcc inline asm.
* Other/better device drivers, PCI(e), USB, something for SATA or SSD storage, mouse, etc.
* More C standard library functions/header files: string abstractions, type conversions, other stuff to help out with C code development.
* Assembler and Compiler for x86/x86_64, for a C-like language or subset. Possibly making a forth or other languages later on.
* Games, or other graphical things. Or text based games.
* Read and use other fonts and font standards, such as PC screen font or bdf or other standardized bitmapped fonts.
* Font editor program, for homemade bitmapped fonts at least.
* Implement options/flags for kernel "shell" commands, if needed. Also a way for the user to add shell commands and aliases.
* Editor fixes/improvements. Eventually be able to edit the OS files & binaries within itself, without rebooting.
* A windowing/UI system, maybe.
* 16bit real mode emulation (possibly using virtual 8086 mode for the 32bit OS build, but that is unavailable from 64bit long mode) to run bootsector games! Or other things
* Get this thing to run on me old thinkpad, to prove it can run on actual hardware. Then UEFI for newer computers...
* Fix up all the TODOs in the code (Ha! who am I kidding...).
* Refactors for less lines of code, clarification, easier interfaces, and overall simplification over time.

* Whatever else comes up...

Videos / Documenting Progress:
---
All progress, or as much as (hopefully!) every new/changed line of code, will be documented on video in a youtube playlist on my main channel here:
https://www.youtube.com/QuesoFuego

Playlist link:
https://www.youtube.com/playlist?list=PLT7NbkyNWaqajsw8Xh7SP9KJwjfpP8TNX

* All development is currently done on "live" recordings, and is probably arduous or boring for most people to watch. Footage is edited down before uploading to cut out long pauses, gratuitous ums and ahs, redundant info, off-topic ramblings, and more.

_Suggestions or comments regarding videos can be made in video comments, twitter @Queso_Fuego, or email - fuegoqueso at gmail dot com_

* The rollout of these videos will probably be slow (weeks to months between videos). I have a full time job and lack time/energy/motivation most days to do too much 
outside of research. However, I will respond to messages sent by Youtube video comments, twitter, or email, and I appreciate all those who wait and watch.

Tools used for these videos:
* recording: OBS Studio
* video editor: Davinci Resolve
* OS: Windows 10 Enterprise (blech, but I use it for work...)
* microphone: Shure SM7B, Cloudlifter CL-1, Focusrite Scarlett Solo
* camera: Sony ZV-1, Elgato Camlink 4k
* mouse: Logitech M590
* keyboard: HHKB professional hybrid type S

Development:
---
Currently, this OS is developed with: 
* 64bit OpenBSD (virtual machine using VMware Workstation Player)
* vim (might try out neovim eventually?)
* qemu emulator (sometimes bochs)
* nasm assembler (as needed, may switch to full clang/gcc assembly in the future for more portability/less dependencies)

This may change later on if I fully develop the OS within itself; that is, running the OS binary and editing that binary during runtime from within itself, using the OS's
own editors, languages, and toolchains.
In this event, changes to the binary would still be uploaded to this repo or others as time and space allows, but relevant changes would not necessarily be seen in source 
files. I'm assuming only the binary file would be changing at that point. So I may take a different approach at that time to better document changes.

Screenshots:
---
![Showing boot screen and example of reading a file to screen](https://git.sr.ht/~queso_fuego/quesos/tree/master/item/screenshots/boot_phys_mem_mgr.png "Showing boot screen and example of reading a file to screen")
![Showing 'editor' program updating a text file](https://git.sr.ht/~queso_fuego/quesos/tree/master/item/screenshots/editor_test.png  "Showing 'editor' program updating a text file")
![Showing 'gfxtst' command showing basic 2D graphics primitives](https://git.sr.ht/~queso_fuego/quesos/tree/master/item/screenshots/gfxtst.png  "Showing 'gfxtst' command showing basic 2D graphics primitives")

These are most likely out of date and more or different screenshots will be added in the future

How to Build/Run:
---
* Disclaimer: Mainly tested on 64bit OpenBSD and FreeBSD, not guaranteeing any other platforms will work

* Install Dependencies: 
  * bochs http://bochs.sourceforge.net/ and/or qemu https://www.qemu.org/download/ 
  * make (bsd and gnu make should both work I think, though this is mainly tested with bsdmake)
  * nasm assembler https://www.nasm.us
  * clang (8.0.1 or newer). Eventually I want gcc to work as well, but it needs a different setup for -mno-general-regs at minimum, at least for any ISR code.

* clone and cd to this repo's /build folder
* Run 'make OS' or 'make' from the command line to build the final OS.bin binary in /bin
* For bochs: In the /build folder, run 'make bochs'; or in the /bin folder, run 'bochs' or 'bochs -q'
* For qemu: In the /build folder, run 'make run'; or in the /bin folder run 'qemu-system-i386 -drive format=raw,file=OS.bin,if=ide,index=0,media=disk'.
 
Note: Qemu seems to run and act better than bochs, and I use it for the most part, with some limited testing in bochs for accuracy/stability. If anything is broken on bochs let me know. 
Also let me know of any suggestions to simplify the build process/makefile, or ways to make the OS more portable for other environments (POSIX utilities or compliance, etc.)
