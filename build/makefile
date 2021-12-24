#-----------------------------------------
# MakeFile should be in <OS>/build/ folder
#-----------------------------------------
# Make variables
CFLAGS = -std=c17 -m32 -march=i386 -ffreestanding -fno-builtin -nostdinc -fPIE -Os -fno-stack-protector	
C_FILES = kernel editor calculator
ASM_FILES = 2ndstage testfont termu16n termu18n
SRCDIR = ../src/
BINDIR = ../bin/
TMPFILES = bootSect fileTable $(ASM_FILES) $(C_FILES)
BINFILES = $(TMPFILES:%=$(BINDIR)%.bin)

# Make final OS.bin binary - padding out to full 1.44MB "floppy" bochs img
OS: reset_filetable $(ASM_FILES) $(C_FILES)
	@size=$$(($$(wc -c < $(BINDIR)fileTable.bin)));\
	newsize=$$((size - $$((size % 512)) + 512));\
	dd if=/dev/zero of=$(BINDIR)fileTable.bin bs=1 seek=$$size count=$$((newsize - size)) status=none
	@cat $(BINFILES) > $(BINDIR)temp.bin
	@dd if=/dev/zero of=$(BINDIR)OS.bin bs=512 count=2880 status=none
	@dd if=$(BINDIR)temp.bin of=$(BINDIR)OS.bin conv=notrunc status=none
	@rm $(BINDIR)*[!OS].bin
	@rm ./sector_num.txt

reset_filetable:
	@echo -n 1 > sector_num.txt
	@nasm -f bin -o $(BINDIR)bootSect.bin $(SRCDIR)bootSect.asm 1>/dev/null
	@./add_filetable_entry.sh bootSect 1
	@./add_filetable_entry.sh fileTable 1

# Assemble assembly source files into binary files
$(ASM_FILES):
	@nasm -f bin -o $(BINDIR)$@.bin $(SRCDIR)$@.asm 1>/dev/null
	@size=$$(($$(wc -c < $(BINDIR)$@.bin)));\
	echo "$@" "$$size ($$(printf '0x%02X' $$((size / 512))) sectors)";\
	./add_filetable_entry.sh $@ $$((size / 512))

# Compile C source files into binary files, and pad out their size to next 512 byte sector size
$(C_FILES):
	@$(CC) -c $(CFLAGS) -o $@.o $(SRCDIR)$@.c
	@ld -T$@.ld $@.o -z notext --oformat binary -o $(BINDIR)$@.bin
	@rm -f $@.o
	@size=$$(($$(wc -c < $(BINDIR)$@.bin)));\
	newsize=$$((size - $$((size % 512)) + 512));\
	echo "$@" "$$size ($$(printf '0x%02X' $$((size / 512))) sectors) ->" "$$newsize ($$(printf '0x%02X' $$((newsize / 512))) sectors)";\
	dd if=/dev/zero of=$(BINDIR)$@.bin bs=1 seek=$$size count=$$((newsize - size)) status=none;\
	./add_filetable_entry.sh $@ $$((newsize / 512))

# Launch OS through qemu 
# NOTE: Remove driftfix=slew if not needed, it is needed for my OpenBSD VM setup
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
