#! /bin/sh

# add_fileetable_entry.sh: auto build fileTable.bin 1 entry at a time
# Inputs:
#  1) Filename
#  2) File size in sectors

# ---------------------------------
# 16 Byte Entries for File Table
# ----		-------
# Byte		Purpose
# ----		-------
# 0-9			File Name
# 10-12		File Extension (txt,exe,etc)
# 13			"Directory Entry" - 0h based # of file table entries
# 14			Starting sector i.e. 6h would be start in sector 6
# 15			File size (in hex digit sectors) - range 00h-FFh/0-255 of 512 byte
# 			  sectors. Max file size for 1 file table entry = 130560 bytes or
# 			  127.5KB; Max file size overall = 255*512*255 bytes or ~32MB
# ---------------------------------
filename="$1"
filesize="$2"

filetable=../bin/fileTable.bin # Output file name to build

# Filename
printf "%s" "$filename" >> $filetable

# Pad out name to 10 with blanks
i=${#filename} # get length of variable
while [ $i -lt 10 ] 
    do
        printf "%c" " " >> $filetable
        i=$((i + 1))
    done

# File extension
case "$filename" in
    fileTable)
        printf "%s" "txt" >> $filetable
        ;;
    testfont|termu16n|termu18n)
        printf "%s" "fnt" >> $filetable
        ;;
    *)
        printf "%s" "bin" >> $filetable
        ;;
esac
        
# Directory entry    
dd if=/dev/zero bs=1 count=1 status=none >> $filetable

# Starting sector
sector=$(cat sector_num.txt)    # Get decimal string
sector=$(printf "%o" $sector)   # Convert decimal to octal string
printf "%b" "\0$sector" >> $filetable # Output octal string as numeric bytes

sector=$(printf "%d" 0$sector) # Convert octal string to decimal string
sector=$((sector + filesize))   # Set next starting sector (current sector + filesize)
printf "%d" $sector > sector_num.txt

# File size
filesize=$(printf "%o" $filesize) # Convert decimal to octal string
printf "%b" "\0$filesize" >> $filetable
