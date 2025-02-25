target remote localhost:1234
add-symbol-file 3rdstage.elf
add-symbol-file kernel.elf
break kernel_main
tui layout split
continue

