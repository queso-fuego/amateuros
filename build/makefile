.POSIX:
#-----------------------------------------------------
# MakeFile should be in <OS>/build/ folder
# NOTE: Needs gnu make (e.g. gmake) for pattern rules
#   and support for posix style ::= macros
#-----------------------------------------------------
# C files setup
CC ::= gcc
#CC ::= clang

CFLAGS ::= -std=c17 -m32 -march=i386 -mgeneral-regs-only \
		 -ffreestanding -fno-stack-protector -fno-builtin \
		 -fno-asynchronous-unwind-tables \
	     -Wall -Wextra -Wpedantic -Wshadow -Wno-pointer-sign \
		 -nostdinc -nostdlib -static -I../include/ -g

# Directories
SRC_DIR    ::= ../src
BIN_DIR    ::= ../bin
PGM_DIR    ::= $(SRC_DIR)/pgms
FONT_DIR   ::= $(SRC_DIR)/fonts
FS_BIN_DIR ::= $(BIN_DIR)/fs_root/sys/bin

# Source files
FONTS    != ls $(FONT_DIR)/*.bdf
ASM_SRCS != ls $(SRC_DIR)/*.asm
ASM_SRCS += $(FONTS:$(FONT_DIR)%.bdf=$(SRC_DIR)%.asm)  # Fonts will be converted to .asm files 
SYS_SRCS != ls $(SRC_DIR)/*.c
USR_SRCS != ls $(PGM_DIR)/*.c

# Binaries/programs/build targets
FONT_PGM ::= $(FONT_DIR)/bdf_to_asm.bin
DISK_PGM ::= make_disk.bin

UTILS    ::= $(FONT_PGM) $(DISK_PGM)

ASM_BINS ::= $(ASM_SRCS:$(SRC_DIR)%.asm=$(BIN_DIR)%.bin)
SYS_BINS ::= $(SYS_SRCS:$(SRC_DIR)%.c=$(BIN_DIR)%.bin)
USR_BINS ::= $(USR_SRCS:$(PGM_DIR)%.c=$(BIN_DIR)%.bin)

ALL_BINS ::= $(ASM_BINS) $(SYS_BINS) $(USR_BINS)

OS       ::= $(BIN_DIR)/OS.bin

# NOTE: Remove driftfix=slew if not needed
# NOTE: -enable-kvm makes RTC and disk accesses slow for me, but can be better accuracy
# NOTE: Change or remove audiodev and related audio lines depending on platform and sound setup
QEMU     ::= qemu-system-i386 \
             -m 128M \
             -drive format=raw,file=$(OS),if=ide,index=0,media=disk \
             -rtc base=localtime,clock=host,driftfix=slew \
             -audiodev id=alsa,driver=alsa \
             -machine pcspk-audiodev=alsa
             #-enable-kvm \

# Make final OS.bin binary 
all: $(UTILS) make_dirs $(OS)

# If any programs/binaries changed, recreate disk image
$(OS): $(ALL_BINS)
	./$(DISK_PGM)

# Create any programs needed in this directory like make_disk, etc.
./%.bin: ./%.c
	$(CC) -std=c17 -Wall -Wextra -Wpedantic -o $@ -I../include $<

# Make program to convert fonts
$(FONT_DIR)/%.bin: $(FONT_DIR)/%.c
	$(CC) -std=c17 -Wall -Wextra -Wpedantic -o $@ $<

# Create OS filesystem directories if they don't exist yet
make_dirs: $(FS_BIN_DIR)

$(FS_BIN_DIR):
	mkdir -p $@

# Convert .bdf fonts to custom OS .asm files
$(SRC_DIR)/%.asm: $(FONT_DIR)/%.bdf
	$(FONT_PGM) $<
	mv $(FONT_DIR)/$(<:.bdf=.asm) $@

# Assemble .asm files into .bin binaries
$(BIN_DIR)/%.bin: $(SRC_DIR)/%.asm 
	@nasm -f bin -o $@ $< 1>/dev/null; \
	size=$$(($$(wc -c < $@))); \
	echo "$@" "$$size ($$(printf '0x%02X' $$((size / 512))) sectors)";
	@cp $@ $(FS_BIN_DIR)

# Compile user program C source files into PIE executable files, and pad out their size to next 512 byte sector 
$(BIN_DIR)/%.bin: $(PGM_DIR)/%.c 
	@$(CC) $(CFLAGS) -fPIE -Wl,-e,main -o $@ $<
	@size=$$(($$(wc -c < $@)));\
	newsize=$$((size - $$((size % 512)) + 512));\
	echo "$@" "$$newsize ($$(printf '0x%02X' $$((newsize / 512))) sectors)";\
	dd if=/dev/zero of=$@ bs=1 seek=$$size count=$$((newsize - size)) status=none;
	@cp $@ $(FS_BIN_DIR)

# Compile system C source files into binary files, and pad out their size to next 512 byte sector 
$(BIN_DIR)/%.bin: $(SRC_DIR)/%.c 
	@$(CC) $(CFLAGS) -fno-pie -Wl,-T$*.ld,--build-id=none -o $*.elf $<
	@objcopy -O binary --strip-debug $*.elf $@
	@size=$$(($$(wc -c < $@)));\
	newsize=$$((size - $$((size % 512)) + 512));\
	echo "$@" "$$newsize ($$(printf '0x%02X' $$((newsize / 512))) sectors)";\
	dd if=/dev/zero of=$@ bs=1 seek=$$size count=$$((newsize - size)) status=none;
	@cp $@ $(FS_BIN_DIR)

# Launch OS with qemu 
run:
	$(QEMU) -monitor stdio -d cpu_reset

# Use .gdbinit and autoload gdb for debugging
debug:
	$(QEMU) -S -s & gdb

# Launch OS with bochs
bochs:
	bochs -qf $(BIN_DIR).bochsrc

clean:
	rm -f $(BIN_DIR)/*.bin $(FONT_DIR)/*.bin $(FS_BIN_DIR)/*.bin *.bin *.elf *.o 

