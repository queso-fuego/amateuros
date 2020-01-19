# MakeFile is located in quesos/bin/
#-----------------------------------

# Create the OS.bin file (use 'Make OS')
OS: bootSect.bin fileTable.bin kernel.bin calculator.bin
	cat bootSect.bin fileTable.bin kernel.bin calculator.bin > OS.bin

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

# Clean up binary files (use 'Make clean')
clean:
	rm *.bin
