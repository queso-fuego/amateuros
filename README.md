Overview/Goals:
---
* Homemade x86(_64?) operating system, as from scratch as possible. 
Currently 32bit protected mode using legacy boot, may eventually try 64bit long mode and UEFI. 

NOTE: I'm looking at making a newer, hopefully simpler or more streamlined, OS that would support multiple architectures and machines.
It'd be more of a microkernel with some form of message passing or protocol for kernel <-> userspace and userspace <-> userspace processes,
both to have a simpler kernel and to make most development modular and in userspace/ring 3.
This repo will probably be mostly dead if/when that happens, or as life happens in general.

* The current medium-term goal is to develop the OS from within itself, editing its own source files using a homemade toolchain: 
editor, assembler, compiler, programming language(s), etc. 
Will later try to include GUIs, applications, window managers, and other things. 
Planning on hardware support for (at minimum? at most?) a thinkpad x60, if possible. 
If reached, then a 2019 dell xps, or maybe a desktop PC.
Currently this runs in qemu and sometimes on hardware.

* The main goal was learning from a bottom-up perspective, and doing everything homemade, originally starting from a 512byte empty bootsector. 
Little to no external dependencies needed, other than a recent C compiler, an x86 assembler, and make. 
And an emulator for testing/developing. 
It assumes a linux/unix environment for building, and I do not (yet) provide pre-built binaries/ISOs.

* Initial reasoning & motivation was to learn x86 16 bit real mode assembly; nowadays it's developed in C with spatterings of standalone and inline asm.
When needed or if it's fun/interesting, I'll still develop some things in assembly.
The other main reason was to learn OS development and systems programming as a newbie with no prior knowledge and no formal CS education. 
I'm interested in from-scratch tools and programming, and in an ideal world with enough time I'd have a full homemade hardware/software stack, 
from transistors to internet browsers and games, as simple as possible, for 1 person to understand and maintain.

This OS was an attempt to start that, and could grow into other non-OS areas over time. 
It's also an attempt to get better at explaining my thought processes and how I work, and improving my speaking in general through development videos. 
I've been learning bits of programming and computer science from slowly going through video lectures, various books, articles, forums, etc.
so hopefully code and knowledge improve tangibly over time.

This is my largest personal project and learning/research experience so far, and am trying to make something that will take a
long time for my own self-interest. 
It will look simple, amateurish, and incomplete. Because it is! [|:^) 

* Feel free to fork or make your own changes in your own repos, the license is effectively public domain 
(0-clause BSD license, though I may use the unlicense in the future, which I use in other projects). 
Suggestions or improvements are welcome, and will be covered in a video if used (and will credit you, if I remember and you don't say otherwise). 
I might open this repo more to the public in the future, but currently lack time and energy to manage that, so will mostly work on things solo.

Project Structure:
---
* /bin: intermediate binary files during the build process, the final `OS.bin` binary, and the initial file tree under `bin/fs_root`. 
* /build: linker scripts for C source files, a makefile to build the project, and other supporting programs/scripts/files used for building.
* /include: subdirectories containing header/source files to be included in the main /src files. These are mainly .h C files, without .c counterparts, with all the code in the "header". It mostly works ok.
* /src: the main source files used to build the intermediate binary files and final OS.bin file.

Current Standing:
---
* In general: 32 bit protected mode, ring 0. Virtual memory & paging, but no multitasking/multiprocess (yet). 
May stick to ring 0 only, not sure yet.
* Initial interrupt support for exceptions, system calls using int 0x80, regular software interrupts, and the PIC. 
PIC IRQs 0 & 8 are implemented for PIT channel 0 at a default rate of ~1000hz for sleep() syscalls, and CMOS Real time clock at a default rate of 1024hz. 
IRQ1 for keyboard handling, but only a subset of scancode set 1. 
Currently keystrokes are retrieved from the PS/2 data port 0x60 in a busy loop. 
You should be able to add your own interrupts with the `void set_idt_descriptor_no_err_32(uint8_t entry_number, void (*isr)(int_frame_32_t *), uint8_t flags)`
function in `include/interrupts/idt.h`.
* Vesa Bios Extensions (VBE) for graphics modes. 
On boot you type in the desired X resolution, Y resolution, and bits per pixel values, or take a default of 1920x1080 32bpp. 
This should be changed to save the last set values to not need retyping/skipping on boot.
If trying to run on actual hardware, know what your hardware supports. Trying to run unsupported modes may or may not damage your hardware!
* "Generic" bitmap font support, for bdf fonts. Add a bdf font to /src/fonts, add the font file with a '.asm' extension to the file list in /build/make_disk.c, and use 'chgfont <font name>' at OS shell when booted.
* Broken text/hex editor for 512 byte length files (needs refactored to use new file system functions and/or syscalls), and a 4 function calculator. 
More/other programs may come in the future.
* Commands for the in-built kernel "shell" such as del, ren, chgcolors, chgfont, etc.  
A list of available commands is in `src/kernel.c` source in `kernel_main()`, where commands are prefixed with `cmd*`. 
Eventually there should be a "help" command or similar to list available commands and descriptions at runtime.

How to Build/Run:
---
* Disclaimer: Mainly tested on (previously) 64 bit OpenBSD, FreeBSD, and (currently) Alpine Linux. Not guaranteeing any other platforms will work for building.

* Dependencies:
    * Emulator: bochs http://bochs.sourceforge.net/ and/or qemu https://www.qemu.org/download/ 
    * gnu make 
    * x86 Assembler: netwide assembler https://www.nasm.us
    * C compiler: clang or gcc. Most versions with C17 support should work, but there could be some different warnings/errors between versions and OSes/platforms/etc.

* Building:
    * clone and cd to this repo's /build folder
    * Run `make` from the command line to build the OS.bin binary in /bin

* Running:
    * qemu: In the /build folder, run `make run`
    * bochs: In the /build folder, run `make bochs`
 
Note: Qemu seems to run and act better than bochs, so I use it for the most part, with limited testing in bochs for accuracy/stability. If anything is broken on bochs please let me know. 
Also let me know of any ways to simplify the build process/makefile, or how to make the OS more portable for other environments (Windows, POSIX utilities or compliance, etc.)

TODO:
---
See `ToDos.txt` for a more up-to-date list.

Videos / Documenting Progress:
---
All progress, or every new/changed line of code (as possible and needed), will be documented on 
video in a youtube playlist:
https://www.youtube.com/playlist?list=PLT7NbkyNWaqajsw8Xh7SP9KJwjfpP8TNX

* A lot of development is done in single take recordings, and would be arduous or boring for 
most people. Footage is edited down to cut out long pauses, gratuitous ums and ahs, 
redundant info, off-topic ramblings, mouth noises/clicks, and more.

_Suggestions or comments regarding videos can be made in the video comments, twitter @Queso_Fuego, 
or email fuegoqueso@gmail.com_

* The rollout of these videos will be slow (weeks to months between videos). 
With a job and house and all, I lack the time/energy/motivation most days to do much outside of 
reading, research, and light testing. However, I'll try to respond to messages from Youtube 
comments/emails/discord when able, and I greatly appreciate everyone who watches.

Development:
---
Currently, this OS is developed with: 
* x86_64 Alpine Linux
* vim 
* qemu emulator 
* nasm assembler (may switch to full clang/gcc 'as' assembly in the future)

This may change later if I fully develop the OS within itself; running the OS binary and editing
that binary during runtime from within itself, using the OS's own editors, languages, and
toolchains.
In this event, changes to the binary would still be uploaded to this repo or others as time and 
space allows, but relevant changes would not necessarily be seen in source files. I'm assuming 
only the binary file would be changing at that point. So I may take a different approach at that 
time to better document changes.

Screenshots:
---
![final build output and choosing resolution on boot](./screenshots/os_choose_resolution.png "Final build output and choosing resolution on boot")
![loading and running an ELF32 PIE executable](./screenshots/os_elf_program_example.png "loading and running an ELF32 PIE executable")
![making a new directory and running sample OS tests](./screenshots/os_mkdir_run_tests.png "making a new directory and running sample OS tests")
![boot screen and read file test](./screenshots/os_read_file_test.png "boot screen and read file test")

!['gfxtst' command showing basic 2D graphics primitives](./screenshots/gfxtst.png "'gfxtst' command showing basic 2D graphics primitives")

These are way out of date and different screenshots may be used in the future.
