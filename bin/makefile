# MakeFile is located in quesos/bin/
#-----------------------------------

# Create the OS.bin file (use 'Make OS') - padding out to full 1.44MB "floppy" bochs img
OS: clean bootSect.bin kernel.bin fileTable.bin calculator.bin editor.bin
	cat bootSect.bin kernel.bin fileTable.bin calculator.bin editor.bin > temp.bin;\
	dd if=/dev/zero of=OS.bin bs=512 count=2880;\
	dd if=temp.bin of=OS.bin conv=notrunc;\
	rm *[!OS].bin

# Create bootsector binary file
bootSect.bin: 
	fasm ../src/asm/bootSect.asm
	mv ../src/asm/bootSect.bin ./

# Create file table binary file
fileTable.bin:
	fasm ../src/asm/fileTable.asm
	mv ../src/asm/fileTable.bin ./

# Create kernel binary file
kernel.bin:
	fasm ../src/asm/kernel.asm
	mv ../src/asm/kernel.bin ./

calculator.bin:
	fasm ../src/asm/calculator.asm
	mv ../src/asm/calculator.bin ./

editor.bin:
	fasm ../src/asm/editor.asm
	mv ../src/asm/editor.bin ./

# Run OS through bochs (use 'Make run')
run:
	bochs -q

# Clean up binary files (use 'Make clean')
clean:
	rm -f *.bin
