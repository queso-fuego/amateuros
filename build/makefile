# MakeFile is located in quesos/bin/
#-----------------------------------
# Create the OS.bin file (use 'Make OS') - padding out to full 1.44MB "floppy" bochs img
OS:
	./assemble_sources.sh;\
	mv ../src/*.bin ../bin;\
	cat ../bin/bootSect.bin ../bin/2ndstage.bin ../bin/testfont.bin ../bin/kernel.bin ../bin/fileTable.bin ../bin/calculator.bin ../bin/editor.bin > ../bin/temp.bin;\
	dd if=/dev/zero of=../bin/OS.bin bs=512 count=2880;\
	dd if=../bin/temp.bin of=../bin/OS.bin conv=notrunc;\
	rm ../bin/*[!OS].bin

run:
	qemu-system-i386 -drive format=raw,file=../bin/OS.bin,if=ide,index=0,media=disk

clean:
	rm -f ../bin/*.bin
