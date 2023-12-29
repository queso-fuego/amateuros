#-----------------------------------------
# MakeFile should be in <OS>/build/ folder
#-----------------------------------------
# Make variables
CC = gcc
#CC = clang

# TODO: Add flag -Wshadow to check for shadowed variables
# TODO: Add -pedantic or -Wpedantic for extra warnings for non C std compliance
CFLAGS = -std=c17 -m32 -march=i386 -mgeneral-regs-only -ffreestanding -fno-stack-protector \
	-fno-builtin -nostdinc -fno-pie \
	-Wall -Wextra -Wno-pointer-sign -Wno-interrupt-service-routine -I../include/

EXE_CFLAGS = -std=c17 -m32 -march=i386 -mgeneral-regs-only -ffreestanding -fno-stack-protector \
	-fno-builtin -nostdinc -fPIE \
	-Wall -Wextra -Wno-pointer-sign -Wno-interrupt-service-routine -I../include/
	
SRCDIR = ../src/
BINDIR = ../bin/
FONTDIR = ../src/fonts/
C_FILES != find $(SRCDIR) -maxdepth 1 -name "*.c" ! -name "calculator.c" ! -name "editor.c" -exec basename -s ".c" {} \;
EXES = calculator editor
FONTS_BDF != find $(FONTDIR) -name "*.bdf" -exec basename -s ".bdf" {} \;

# Make final OS.bin binary - padding out to full 1.44MB "floppy" img
OS: $(FONTS_BDF) asm_files $(C_FILES) $(EXES)
	@mv $(BINDIR)[!OS]*.bin $(BINDIR)fs_root/sys/bin
	@$(CC) ./make_disk.c -o ./make_disk.bin -I../include	
	@./make_disk.bin

# Assemble assembly source files into binary files,
#   as fonts or other targets can make additional .asm files, need to process at runtime
asm_files: 
	@for file in `find $(SRCDIR) -maxdepth 1 -name "*.asm" -exec basename -s ".asm" {} \;`; do \
		nasm -f bin -o $(BINDIR)$$file.bin $(SRCDIR)$$file.asm 1>/dev/null;\
		size=$$(($$(wc -c < $(BINDIR)$$file.bin)));\
		echo "$$file" "$$size ($$(printf '0x%02X' $$((size / 512))) sectors)";\
	done

# Compile C source files into PIE executable files, and pad out their size to next 512 byte sector 
$(EXES):
	@$(CC) -c $(EXE_CFLAGS) -o $@.o $(SRCDIR)$@.c
	@ld -melf_i386 -e main -pie $@.o -o $(BINDIR)$@.bin
	@rm -f $@.o
	@size=$$(($$(wc -c < $(BINDIR)$@.bin)));\
	newsize=$$((size - $$((size % 512)) + 512));\
	echo "$@" "$$size ($$(printf '0x%02X' $$((size / 512))) sectors) ->" "$$newsize ($$(printf '0x%02X' $$((newsize / 512))) sectors)";\
	dd if=/dev/zero of=$(BINDIR)$@.bin bs=1 seek=$$size count=$$((newsize - size)) status=none;

# Compile C source files into binary files, and pad out their size to next 512 byte sector 
$(C_FILES):
	@$(CC) -c $(CFLAGS) -o $@.o $(SRCDIR)$@.c
	@ld -melf_i386 -T$@.ld $@.o --oformat binary -o $(BINDIR)$@.bin
	@rm -f $@.o
	@size=$$(($$(wc -c < $(BINDIR)$@.bin)));\
	newsize=$$((size - $$((size % 512)) + 512));\
	echo "$@" "$$size ($$(printf '0x%02X' $$((size / 512))) sectors) ->" "$$newsize ($$(printf '0x%02X' $$((newsize / 512))) sectors)";\
	dd if=/dev/zero of=$(BINDIR)$@.bin bs=1 seek=$$size count=$$((newsize - size)) status=none;

# Launch OS through qemu 
# NOTE: Remove driftfix=slew if not needed
# NOTE: -enable-kvm makes RTC and disk accesses slow for me, but can be better accuracy
# NOTE: Change or remove audiodev and related audio lines depending on platform and sound setup
run:
	qemu-system-i386 \
	-monitor stdio \
	-m 128M \
	-drive format=raw,file=$(BINDIR)OS.bin,if=ide,index=0,media=disk \
	-rtc base=localtime,clock=host,driftfix=slew \
	-audiodev id=alsa,driver=alsa \
	-machine pcspk-audiodev=alsa
	#-enable-kvm \

# Launch OS through bochs
bochs:
	bochs -qf $(BINDIR).bochsrc

clean:
	rm -f $(BINDIR)*.bin ./*.o 

# Make program to convert fonts
make_font_pgm:
	@make -C $(FONTDIR) 1>/dev/null

# Convert bdf fonts to OS asm font files
$(FONTS_BDF): make_font_pgm
	@$(FONTDIR)bdf_to_asm.bin $(FONTDIR)$@.bdf
	@mv $(FONTDIR)$@.asm $(SRCDIR)

