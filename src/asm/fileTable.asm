;;;
;;; fileTable.asm: basic 'file table' made with db, string consists of '{fileName1-sector#, fileName2-sector#, ...fileNameN-sector#}'
;;; 
db '{calculator-06,program2-12}'

        ;; Sector padding magic!
        times 512-($-$$) db 0       ; pad rest of sector out with 0s
