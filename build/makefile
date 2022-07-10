#-----------------------------------------
# MakeFile should be in <OS>/build/ folder
#-----------------------------------------
# Make variables
CFLAGS = -std=c17 -m32 -march=i386 -ffreestanding -fno-builtin -nostdinc -fPIE -Os -Wno-pointer-sign\
	-I../include/ 
SRCDIR = ../src/
BINDIR = ../bin/
C_FILES != find $(SRCDIR) -name "*.c" -exec basename -s ".c" {} \;
ASM_FILES != find $(SRCDIR) -name "*.asm" -exec basename -s ".asm" {} \;
TMPFILES = $(ASM_FILES) $(C_FILES)
BINFILES = $(TMPFILES:%=$(BINDIR)%.bin)

# Make final OS.bin binary - padding out to full 1.44MB "floppy" bochs img
OS: $(ASM_FILES) $(C_FILES)
	@$(CC) ./make_disk.c -o ./make_disk -I../include 
	@./make_disk

# Assemble assembly source files into binary files
$(ASM_FILES):
	@nasm -f bin -o $(BINDIR)$@.bin $(SRCDIR)$@.asm 1>/dev/null
	@size=$$(($$(wc -c < $(BINDIR)$@.bin)));\
	echo "$@" "$$size ($$(printf '0x%02X' $$((size / 512))) sectors)";\

# Compile C source files into binary files, and pad out their size to next 512 byte sector size
$(C_FILES):
	@$(CC) -c $(CFLAGS) -o $@.o $(SRCDIR)$@.c
	@ld -T$@.ld $@.o --oformat binary -o $(BINDIR)$@.bin
	@rm -f $@.o
	@size=$$(($$(wc -c < $(BINDIR)$@.bin)));\
	newsize=$$((size - $$((size % 512)) + 512));\
	echo "$@" "$$size ($$(printf '0x%02X' $$((size / 512))) sectors) ->" "$$newsize ($$(printf '0x%02X' $$((newsize / 512))) sectors)";\
	dd if=/dev/zero of=$(BINDIR)$@.bin bs=1 seek=$$size count=$$((newsize - size)) status=none;\

# Launch OS through qemu 
# NOTE: Remove driftfix=slew if not needed
run:
	qemu-system-i386\
	-drive format=raw,file=$(BINDIR)OS.bin,if=ide,index=0,media=disk\
	-rtc base=localtime,clock=host,driftfix=slew\
	-soundhw pcspk

# Launch OS through bochs
bochs:
	bochs -qf $(BINDIR).bochsrc

clean:
	rm -f $(BINDIR)*.bin
	rm -f ./sector_num.txt
