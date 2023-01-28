Overview/Goals:
---
* Homemade x86(_64?) operating system, as from scratch as possible, more over time. Currently 32bit protected mode using legacy boot, but will eventually try 64bit long mode and UEFI. 
The current medium-term goal is to develop the OS from within itself, editing its own source files using a homemade toolchain: editor, assembler, compiler, programming language(s), etc. Will later try to include 
GUIs, applications, window managers, and other things. Planning on hardware support for (at minimum? at most?) a thinkpad x60, if possible. If reached, then a 2019 dell xps, or maybe a desktop PC.
Currently runs in qemu and bochs. 
The main goal is learning from a bottom-up perspective, and doing everything homemade, originally starting from a 512byte empty bootsector. Little to no external dependencies needed, other than a recent
C compiler, an x86 assembler, and make. And an emulator for testing/developing. I assume a linux/unix environment for building, and do not (yet) provide any pre-built binaries/ISOs.

* Initial reasoning & motivation for this project was to learn x86 16 bit real mode assembly; This is mostly done, as it's now developed in C with spatterings of standalone and inline asm.
However, if/when it's fun or interesting, I'll still develop some things in assembly.

* The other main reason was to learn OS development and systems programming as a newbie with no prior knowledge and no formal CS education. I'm interested in from-scratch tools and
programming, and in an ideal world with enough time I could have a full computer with self-made hardware/software stack, from transistors to internet browser and games, as 
simple as possible, for 1 person to maintain.
  This OS is an attempt to start that, and could grow into other non-OS areas over time. It's also an attempt to get better at explaining my
thought processes and how I work through things, and improving my speaking in general, through development videos. 
I'm learning bits of computer science from slowly going through MIT lectures and various books, so hopefully the code and knowledge improves tangibly over time.
This is my largest personal project and learning/research experience so far, and I'm a newbie trying to make something that will take a
long time for his own self-interest. It might look simple, amateurish, and incomplete. Because it is! Keep that in mind [|:^) 

* Feel free to fork or make your own changes to your own repos, the license is effectively public domain (0-clause BSD license). Suggestions or improvements are welcome, and will be covered 
in a video if used (and will credit you, if I remember and you don't say otherwise). I might open up this repo to the public in the future, but currently lack 
sufficient time to manage that, and would rather work on things solo for the time being. 

Project Structure:
---
* /bin: intermediate binary files during the build process, and the final OS.bin binary. 
* /build: linker scripts for C source files, a makefile to build the project, and other supporting programs/scripts/files.
* /include: subdirectories containing header/source files to be included in the main /src files. These are mainly .h C files, without .c counterparts, with all the code in the "header". It mostly works ok.
* /src: the main source files used to build the intermediate binary files and final OS.bin file.

Current Standing:
---
* In general: 32 bit protected mode, all ring 0. Virtual memory & paging, no multitasking/multiprocess (yet). May stick to ring 0 only when/if that's set up, not sure yet.
* Initial interrupt support for exceptions, system calls (using int 0x80), regular software interrupts, and the PIC. PIC IRQs 0 & 8 are implemented for PIT channel 0 (at a default rate of ~1000hz for sleep() syscalls) and CMOS Real time clock (at a default rate of 1024hz). 
IRQ1 for keyboard handling, but only a subset of scancode set 1. Currently keystrokes are retrieved from the PS/2 data port 0x60 in a busy loop. You should be able to add your own interrups as desired. 
* Vesa Bios Extensions for graphics modes. On boot you type in desired X resolution, Y resolution, and bits per pixel values, or take a default of 1920x1080 32bpp. 
If trying to run on actual hardware, ensure you know what your hardware supports! Trying to run unsupported modes may damage your hardware!!
* "Generic" bitmap font support for bdf fonts. Add a bdf font to /src/fonts, add the font file with '.asm' extension to the file list in /build/make_disk.c, and use 'chgfont <font name>' at OS shell when booted.
* Barely functioning text/hex editor for 512 byte length files, and a 4 function calculator. More programs to come in the future.
* Ability to save and load text or binary (hex) files. Valid x86 binary files can be run from the hex editor if the file length is <= 1 sector, or from the main kernel command line if set up in the file table.
Binary files written in the hex editor are auto-ended with a '0xCB', or far return. That probably doesn't work, and will change when better memory management and program loading is developed.
* Several commands available for the in-built kernel "shell" such as del, ren, chgcolors, chgfont, etc. A list of available commands is in the kernel.c source, in kernel_main(), where
they're prefixed with "cmd*". Eventually there should be a better implementation like a "help" command to list available commands and descriptions at runtime.

How to Build/Run:
---
* Disclaimer: Mainly tested on (previously) 64 bit OpenBSD, FreeBSD, and (currently) Alpine Linux. Not guaranteeing any other platforms will work for building.

* Dependencies:
    * Emulator: bochs http://bochs.sourceforge.net/ and/or qemu https://www.qemu.org/download/ 
    * make 
    * x86 Assembler: netwide assembler https://www.nasm.us
    * C compiler: clang or gcc. Most versions with C17 support should work, but there could be some different warnings/errors between versions and OSes/platforms/etc.

* Building:
    * clone and cd to this repo's /build folder
    * Run 'make OS' or 'make' from the command line to build the OS.bin binary in /bin

* Running:
    * qemu: In the /build folder, run 'make run'
    * bochs: In the /build folder, run 'make bochs'
 
Note: Qemu seems to run and act better than bochs, so I use it for the most part, with limited testing in bochs for accuracy/stability. If anything is broken on bochs please let me know. 
Also let me know of any ways to simplify the build process/makefile, or how to make the OS more portable for other environments (Windows, POSIX utilities or compliance, etc.)

TODO (There's ALWAYS more to do e.g. lots of TODOs in the source files, this list is updated when/if I remember):
---
In no particular order:
* 64bit long mode? maybe with PAE or other RAM extending stuff? Still mulling it over as a possible "fork" to go with UEFI in the future.
* UEFI, as an alternative to the current bootsector and bootloader. It could be all in C but has it's own ways of handling device discovery and setting video modes and things.
  ** Ultimately, if not too much work, I'd like to maintain separate versions for: 1) 32bit x86 (i386 minimum) BIOS, and 2) 64bit x86 UEFI. 
Any additional versions for other architectures can be worried about later, ARM, PPC, etc., if it gets to that point and better/more general abstractions are in place for any needed asm code. Possibly having separate all assembly versions of these 2 overall OS versions as well, to check size/speed differences, large project structure, etc.
* Task scheduling? Not sure yet on implementations, may do round-robin or another simple way to do processes. Using PIT, HPET, APIC, TSC, other timers?
* Interrupt handler disk reading/loading for the PIC, and more system calls (using int 0x80) for malloc/free and other needed things. Probably using APIC & IOAPIC to replace PIC later on.
* Somehow convert the bootsector and bootloader to C. The bootsector is fine, if enough code is moved elsewhere so that it fits in 512 bytes with the AA55h signature, 
but I have had no luck so far with structures and intermediate (to me) C code working with 16bit inline asm, to allow the bootloader to work effectively. It may be due to only having 32bit addresses and memory instructions available in clang/gcc inline asm.
* Other/better device drivers, PCI(e), USB, something for SATA or SSD storage, mouse, etc.
* More C standard library functions/header files: string abstractions, type conversions, other stuff to help out with C code development.
* Assembler and Compiler for x86/x86_64, for a C-like language or subset. Possibly making a forth or other languages later on.
* Games, or other graphical things. Or text based games.
* Read and use other fonts and font standards, such as PC screen font or bdf or other standardized bitmapped fonts.
* Font editor program, for homemade bitmapped fonts at least.
* Implement options/flags for kernel "shell" commands, if needed. Also a way for the user to add shell commands and aliases.
* Editor fixes/improvements. Eventually edit the OS files & binaries within itself, without rebooting, for self-hosting and dogfooding.
* A windowing/UI system, maybe.
* 16bit real mode emulation (possibly using virtual 8086 mode for the 32bit OS build, but that is unavailable from 64bit long mode) to run bootsector games! Or other things
* Get this thing to run on UEFI for newer computers, but it does currently boot and work from BIOS on a Thinkpad x60!
* Fix up all the TODOs in the code (Ha! who am I kidding...).
* Refactors for less lines of code, clarification, easier interfaces, and overall simplification over time.

* Whatever else comes up...

Videos / Documenting Progress:
---
All progress, or as much as (hopefully!) every new/changed line of code, will be documented on video in a youtube playlist on my main channel here:
https://www.youtube.com/QuesoFuego

Playlist link:
https://www.youtube.com/playlist?list=PLT7NbkyNWaqajsw8Xh7SP9KJwjfpP8TNX

* All development is currently done on "live" recordings, and is probably arduous or boring for most people to watch. Footage is edited down to cut out long pauses, gratuitous ums and ahs, redundant info, off-topic ramblings, and more.

_Suggestions or comments regarding videos can be made in video comments, twitter @Queso_Fuego, or email fuegoqueso@gmail.com_

* The rollout of these videos will be slow (weeks to months between videos). I have a full time job and lack the time/energy/motivation most days to do too much 
outside of research. However, I will respond to messages sent by Youtube video comments, twitter, or email, and I greatly appreciate all those who wait and watch.

Tools used for these videos:
* recording: OBS Studio
* video editor: Davinci Resolve
* OS: Windows 11 Enterprise (blech! but I need it for work, VSTs, Nvidia, and bluetooth)
* microphone: Shure SM7B, Cloudlifter CL-1, Focusrite Scarlett Solo
* camera: Sony ZV-1, Elgato Camlink 4k
* mouse: Logitech M590
* keyboard: HHKB professional hybrid type S

Development:
---
Currently, this OS is developed with: 
* 64 bit Alpine Linux virtual machine using VMware Workstation Player
* neovim 
* qemu emulator (sometimes bochs)
* nasm assembler (as needed, may switch to full clang/gcc 'as' assembly in the future for less dependencies)

This may change later on if I fully develop the OS within itself; that is, running the OS binary and editing that binary during runtime from within itself, using the OS's
own editors, languages, and toolchains.
In this event, changes to the binary would still be uploaded to this repo or others as time and space allows, but relevant changes would not necessarily be seen in source 
files. I'm assuming only the binary file would be changing at that point. So I may take a different approach at that time to better document changes.

Screenshots:
---
![Showing boot screen and example of reading a file to screen](./screenshots/boot_phys_mem_mgr.png "Showing boot screen and example of reading a file to screen")
![Showing 'editor' program updating a text file](./screenshots/editor_test.png "Showing 'editor' program updating a text file")
![Showing 'gfxtst' command showing basic 2D graphics primitives](./screenshots/gfxtst.png "Showing 'gfxtst' command showing basic 2D graphics primitives")

These are way out of date and different screenshots will be used in the future.
