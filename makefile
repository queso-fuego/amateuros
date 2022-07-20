#-----------------------------------------
# MakeFile should be in <OS>/build/ folder
#-----------------------------------------
# Make variables
SRCDIR   = src/
BINDIR   = bin/
BUILDDIR = build/
INCDIR   = include/
C_FILES != find $(SRCDIR) -name *.c -exec basename -s ".c" {} \;
ASM_FILES != find $(SRCDIR) -name *.asm ! -name bootSect.asm -exec basename -s ".asm" {} \;
TMPFILES = $(BINDIR)bootSect $(BINDIR)fileTable $(ASM_FILES) $(C_FILES)
BINFILES = $(TMPFILES:%=$(BINDIR)%.bin)

CFLAGS = -std=c17 -m32 -march=i386 -ffreestanding -fno-builtin -nostdinc -fPIE -Os -fno-stack-protector	-Wno-pointer-sign\
	-I$(INCDIR)


.PHONY: OS reset_filetable run bochs clean

# Make final OS.bin binary - padding out to full 1.44MB "floppy" bochs img
OS: reset_filetable $(ASM_FILES) $(C_FILES)
	@size=$$(($$(wc -c < $(BINDIR)fileTable.bin)));\
	newsize=$$((size - $$((size % 512)) + 512));\
	dd if=/dev/zero of=$(BINDIR)fileTable.bin bs=1 seek=$$size count=$$((newsize - size)) status=none
	@cat $(BINFILES) > $(BINDIR)temp.bin
	@dd if=/dev/zero of=$(BINDIR)OS.bin bs=512 count=2880 status=none
	@dd if=$(BINDIR)temp.bin of=$(BINDIR)OS.bin conv=notrunc status=none
	@rm $(BINDIR)*[!OS].bin
	@rm $(BINDIR)sector_num.txt

reset_filetable:
	@echo -n 1 > $(BINDIR)sector_num.txt
	@nasm -f bin -o $(BINDIR)bootSect.bin $(SRCDIR)bootSect.asm 1>/dev/null
	@$(BUILDDIR)add_filetable_entry.sh $(BINDIR) bootSect 1
	@$(BUILDDIR)add_filetable_entry.sh $(BINDIR) fileTable 1

# Assemble assembly source files into binary files
$(ASM_FILES):
	@nasm -f bin -o $(BINDIR)$@.bin $(SRCDIR)$@.asm 1>/dev/null
	@size=$$(($$(wc -c < $(BINDIR)$@.bin)));\
	echo "$@" "$$size ($$(printf '0x%02X' $$((size / 512))) sectors)";\
	$(BUILDDIR)add_filetable_entry.sh $(BINDIR) $@ $$((size / 512))

# Compile C source files into binary files, and pad out their size to next 512 byte sector size
$(C_FILES):
	@$(CC) -c $(CFLAGS) -o $@.o $(SRCDIR)$@.c
	@ld -T$(BUILDDIR)$@.ld $@.o -z notext --oformat binary -o $(BINDIR)$@.bin
	@rm -f $@.o
	@size=$$(($$(wc -c < $(BINDIR)$@.bin)));\
	newsize=$$((size - $$((size % 512)) + 512));\
	echo "$@" "$$size ($$(printf '0x%02X' $$((size / 512))) sectors) ->" "$$newsize ($$(printf '0x%02X' $$((newsize / 512))) sectors)";\
	dd if=/dev/zero of=$(BINDIR)$@.bin bs=1 seek=$$size count=$$((newsize - size)) status=none;\
	$(BUILDDIR)add_filetable_entry.sh $(BINDIR) $@ $$((newsize / 512))

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
	rm -f $(BINDIR)sector_num.txt
