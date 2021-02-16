;;;
;;; fileTable.asm
;;; ---------------------------------
;;; 16 Byte Entries for File Table
;;; ----		-------
;;; Byte		Purpose
;;; ----		-------
;;; 0-9			File Name
;;; 10-12		File Extension (txt,exe,etc)
;;; 13			"Directory Entry" - 0h based # of file table entries
;;; 14			Starting sector i.e. 6h would be start in sector 6
;;; 15			File size (in hex digit sectors) - range 00h-FFh/0-255 of 512 byte
;;; 			  sectors. Max file size for 1 file table entry = 130560 bytes or
;;; 			  127.5KB; Max file size overall = 255*512*255 bytes or ~32MB
;;; ---------------------------------
use32
	db 'bootSect  ','bin',00h,01h,01h,\
    '2ndstage  ','bin',00h,02h,03h,\
    'testfont  ','fnt',00h,5h,04h,\
	'kernel    ','bin',00h,09h,0Eh,\
	'fileTable ','txt',00h,17h,01h,\
	'calculator','bin',00h,18h,05h,\
	'editor    ','bin',00h,1Dh,0Fh

        ;; Sector padding magic!
        times 512-($-$$) db 0       ; pad rest of sector out with 0s
