;;;
;;; ../src/fonts/ter-u32n.asm
;;;

;; Font width, height
db 16, 32

;; Initial padding
times 31*64 - 2 db 0

;; space 32
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; exclam 33
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; quotedbl 34
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0C30
dw 0x0C30
dw 0x0C30
dw 0x0C30
dw 0x0C30
dw 0x0C30
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; numbersign 35
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0C30
dw 0x0C30
dw 0x0C30
dw 0x0C30
dw 0x0C30
dw 0x3FFC
dw 0x3FFC
dw 0x0C30
dw 0x0C30
dw 0x0C30
dw 0x0C30
dw 0x0C30
dw 0x0C30
dw 0x3FFC
dw 0x3FFC
dw 0x0C30
dw 0x0C30
dw 0x0C30
dw 0x0C30
dw 0x0C30
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; dollar 36
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0FF0
dw 0x1FF8
dw 0x399C
dw 0x318C
dw 0x3180
dw 0x3180
dw 0x3180
dw 0x3980
dw 0x1FF0
dw 0x0FF8
dw 0x019C
dw 0x018C
dw 0x018C
dw 0x018C
dw 0x318C
dw 0x399C
dw 0x1FF8
dw 0x0FF0
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; percent 37
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x1E18
dw 0x3F18
dw 0x3330
dw 0x3330
dw 0x3F60
dw 0x1E60
dw 0x00C0
dw 0x00C0
dw 0x0180
dw 0x0180
dw 0x0300
dw 0x0300
dw 0x0600
dw 0x0600
dw 0x0CF0
dw 0x0DF8
dw 0x1998
dw 0x1998
dw 0x31F8
dw 0x30F0
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; ampersand 38
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0F80
dw 0x1FC0
dw 0x38E0
dw 0x3060
dw 0x3060
dw 0x3060
dw 0x38E0
dw 0x1DC0
dw 0x0F80
dw 0x0F00
dw 0x1F8C
dw 0x39DC
dw 0x70F8
dw 0x6070
dw 0x6030
dw 0x6030
dw 0x6070
dw 0x70F8
dw 0x3FDC
dw 0x1F8C
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; quotesingle 39
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; parenleft 40
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0060
dw 0x00E0
dw 0x01C0
dw 0x0380
dw 0x0300
dw 0x0700
dw 0x0600
dw 0x0600
dw 0x0600
dw 0x0600
dw 0x0600
dw 0x0600
dw 0x0600
dw 0x0600
dw 0x0700
dw 0x0300
dw 0x0380
dw 0x01C0
dw 0x00E0
dw 0x0060
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; parenright 41
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0600
dw 0x0700
dw 0x0380
dw 0x01C0
dw 0x00C0
dw 0x00E0
dw 0x0060
dw 0x0060
dw 0x0060
dw 0x0060
dw 0x0060
dw 0x0060
dw 0x0060
dw 0x0060
dw 0x00E0
dw 0x00C0
dw 0x01C0
dw 0x0380
dw 0x0700
dw 0x0600
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; asterisk 42
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x3838
dw 0x1C70
dw 0x0EE0
dw 0x07C0
dw 0x0380
dw 0x7FFC
dw 0x7FFC
dw 0x0380
dw 0x07C0
dw 0x0EE0
dw 0x1C70
dw 0x3838
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; plus 43
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x3FFC
dw 0x3FFC
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; comma 44
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0380
dw 0x0300
dw 0x0600
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; hyphen 45
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x3FFC
dw 0x3FFC
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; period 46
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; slash 47
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0018
dw 0x0018
dw 0x0030
dw 0x0030
dw 0x0060
dw 0x0060
dw 0x00C0
dw 0x00C0
dw 0x0180
dw 0x0180
dw 0x0300
dw 0x0300
dw 0x0600
dw 0x0600
dw 0x0C00
dw 0x0C00
dw 0x1800
dw 0x1800
dw 0x3000
dw 0x3000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; zero 48
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0FF0
dw 0x1FF8
dw 0x381C
dw 0x300C
dw 0x300C
dw 0x301C
dw 0x303C
dw 0x307C
dw 0x30EC
dw 0x31CC
dw 0x338C
dw 0x370C
dw 0x3E0C
dw 0x3C0C
dw 0x380C
dw 0x300C
dw 0x300C
dw 0x381C
dw 0x1FF8
dw 0x0FF0
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; one 49
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0180
dw 0x0380
dw 0x0780
dw 0x0F80
dw 0x0D80
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0FF0
dw 0x0FF0
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; two 50
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0FF0
dw 0x1FF8
dw 0x381C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x000C
dw 0x001C
dw 0x0038
dw 0x0070
dw 0x00E0
dw 0x01C0
dw 0x0380
dw 0x0700
dw 0x0E00
dw 0x1C00
dw 0x3800
dw 0x3FFC
dw 0x3FFC
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; three 51
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0FF0
dw 0x1FF8
dw 0x381C
dw 0x300C
dw 0x300C
dw 0x000C
dw 0x000C
dw 0x000C
dw 0x001C
dw 0x07F8
dw 0x07F8
dw 0x001C
dw 0x000C
dw 0x000C
dw 0x000C
dw 0x300C
dw 0x300C
dw 0x381C
dw 0x1FF8
dw 0x0FF0
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; four 52
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x000C
dw 0x001C
dw 0x003C
dw 0x007C
dw 0x00EC
dw 0x01CC
dw 0x038C
dw 0x070C
dw 0x0E0C
dw 0x1C0C
dw 0x380C
dw 0x300C
dw 0x300C
dw 0x3FFC
dw 0x3FFC
dw 0x000C
dw 0x000C
dw 0x000C
dw 0x000C
dw 0x000C
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; five 53
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x3FFC
dw 0x3FFC
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3FF0
dw 0x3FF8
dw 0x001C
dw 0x000C
dw 0x000C
dw 0x000C
dw 0x000C
dw 0x000C
dw 0x300C
dw 0x380C
dw 0x1FF8
dw 0x0FF0
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; six 54
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0FF8
dw 0x1FF8
dw 0x3800
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3FF0
dw 0x3FF8
dw 0x301C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x381C
dw 0x1FF8
dw 0x0FF0
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; seven 55
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x3FFC
dw 0x3FFC
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x3018
dw 0x0018
dw 0x0030
dw 0x0030
dw 0x0060
dw 0x0060
dw 0x00C0
dw 0x00C0
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; eight 56
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0FF0
dw 0x1FF8
dw 0x381C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x381C
dw 0x1FF8
dw 0x1FF8
dw 0x381C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x381C
dw 0x1FF8
dw 0x0FF0
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; nine 57
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0FF0
dw 0x1FF8
dw 0x381C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x380C
dw 0x1FFC
dw 0x0FFC
dw 0x000C
dw 0x000C
dw 0x000C
dw 0x000C
dw 0x000C
dw 0x001C
dw 0x1FF8
dw 0x1FF0
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; colon 58
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; semicolon 59
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0380
dw 0x0300
dw 0x0600
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; less 60
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x001C
dw 0x0038
dw 0x0070
dw 0x00E0
dw 0x01C0
dw 0x0380
dw 0x0700
dw 0x0E00
dw 0x1C00
dw 0x3800
dw 0x3800
dw 0x1C00
dw 0x0E00
dw 0x0700
dw 0x0380
dw 0x01C0
dw 0x00E0
dw 0x0070
dw 0x0038
dw 0x001C
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; equal 61
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x3FFC
dw 0x3FFC
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x3FFC
dw 0x3FFC
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; greater 62
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x3800
dw 0x1C00
dw 0x0E00
dw 0x0700
dw 0x0380
dw 0x01C0
dw 0x00E0
dw 0x0070
dw 0x0038
dw 0x001C
dw 0x001C
dw 0x0038
dw 0x0070
dw 0x00E0
dw 0x01C0
dw 0x0380
dw 0x0700
dw 0x0E00
dw 0x1C00
dw 0x3800
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; question 63
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0FF0
dw 0x1FF8
dw 0x381C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x001C
dw 0x0038
dw 0x0070
dw 0x00E0
dw 0x01C0
dw 0x0180
dw 0x0180
dw 0x0000
dw 0x0000
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; at 64
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x1FF0
dw 0x3FF8
dw 0x701C
dw 0x600C
dw 0x61FC
dw 0x63FC
dw 0x670C
dw 0x660C
dw 0x660C
dw 0x660C
dw 0x660C
dw 0x660C
dw 0x660C
dw 0x671C
dw 0x63FC
dw 0x61EC
dw 0x6000
dw 0x7000
dw 0x3FFC
dw 0x1FFC
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; A 65
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0FF0
dw 0x1FF8
dw 0x381C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x3FFC
dw 0x3FFC
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; B 66
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x3FF0
dw 0x3FF8
dw 0x301C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x3018
dw 0x3FF0
dw 0x3FF0
dw 0x3038
dw 0x301C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x301C
dw 0x3FF8
dw 0x3FF0
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; C 67
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0FF0
dw 0x1FF8
dw 0x381C
dw 0x300C
dw 0x300C
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x300C
dw 0x300C
dw 0x381C
dw 0x1FF8
dw 0x0FF0
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; D 68
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x3FC0
dw 0x3FF0
dw 0x3038
dw 0x3018
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x3018
dw 0x3038
dw 0x3FF0
dw 0x3FC0
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; E 69
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x3FFC
dw 0x3FFC
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3FE0
dw 0x3FE0
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3FFC
dw 0x3FFC
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; F 70
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x3FFC
dw 0x3FFC
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3FE0
dw 0x3FE0
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; G 71
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0FF0
dw 0x1FF8
dw 0x381C
dw 0x300C
dw 0x300C
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x30FC
dw 0x30FC
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x381C
dw 0x1FF8
dw 0x0FF0
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; H 72
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x3FFC
dw 0x3FFC
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; I 73
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x07E0
dw 0x07E0
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x07E0
dw 0x07E0
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; J 74
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x007E
dw 0x007E
dw 0x0018
dw 0x0018
dw 0x0018
dw 0x0018
dw 0x0018
dw 0x0018
dw 0x0018
dw 0x0018
dw 0x0018
dw 0x0018
dw 0x0018
dw 0x0018
dw 0x3018
dw 0x3018
dw 0x3018
dw 0x3838
dw 0x1FF0
dw 0x0FE0
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; K 75
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x300C
dw 0x301C
dw 0x3038
dw 0x3070
dw 0x30E0
dw 0x31C0
dw 0x3380
dw 0x3700
dw 0x3E00
dw 0x3C00
dw 0x3C00
dw 0x3E00
dw 0x3700
dw 0x3380
dw 0x31C0
dw 0x30E0
dw 0x3070
dw 0x3038
dw 0x301C
dw 0x300C
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; L 76
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3FFC
dw 0x3FFC
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; M 77
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x600C
dw 0x600C
dw 0x701C
dw 0x783C
dw 0x6C6C
dw 0x6C6C
dw 0x67CC
dw 0x638C
dw 0x638C
dw 0x610C
dw 0x600C
dw 0x600C
dw 0x600C
dw 0x600C
dw 0x600C
dw 0x600C
dw 0x600C
dw 0x600C
dw 0x600C
dw 0x600C
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; N 78
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x380C
dw 0x3C0C
dw 0x3E0C
dw 0x370C
dw 0x338C
dw 0x31CC
dw 0x30EC
dw 0x307C
dw 0x303C
dw 0x301C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; O 79
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0FF0
dw 0x1FF8
dw 0x381C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x381C
dw 0x1FF8
dw 0x0FF0
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; P 80
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x3FF0
dw 0x3FF8
dw 0x301C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x301C
dw 0x3FF8
dw 0x3FF0
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; Q 81
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0FF0
dw 0x1FF8
dw 0x381C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x31CC
dw 0x38FC
dw 0x1FF8
dw 0x0FF8
dw 0x001C
dw 0x000E
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; R 82
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x3FF0
dw 0x3FF8
dw 0x301C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x301C
dw 0x3FF8
dw 0x3FF0
dw 0x3700
dw 0x3380
dw 0x31C0
dw 0x30E0
dw 0x3070
dw 0x3038
dw 0x301C
dw 0x300C
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; S 83
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0FF0
dw 0x1FF8
dw 0x381C
dw 0x300C
dw 0x300C
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3800
dw 0x1FF0
dw 0x0FF8
dw 0x001C
dw 0x000C
dw 0x000C
dw 0x000C
dw 0x300C
dw 0x300C
dw 0x381C
dw 0x1FF8
dw 0x0FF0
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; T 84
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x3FFC
dw 0x3FFC
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; U 85
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x381C
dw 0x1FF8
dw 0x0FF0
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; V 86
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x1818
dw 0x1818
dw 0x1818
dw 0x1818
dw 0x1818
dw 0x0C30
dw 0x0C30
dw 0x0C30
dw 0x0C30
dw 0x0660
dw 0x0660
dw 0x0660
dw 0x03C0
dw 0x03C0
dw 0x03C0
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; W 87
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x600C
dw 0x600C
dw 0x600C
dw 0x600C
dw 0x600C
dw 0x600C
dw 0x600C
dw 0x600C
dw 0x600C
dw 0x600C
dw 0x610C
dw 0x638C
dw 0x638C
dw 0x67CC
dw 0x6C6C
dw 0x6C6C
dw 0x783C
dw 0x701C
dw 0x600C
dw 0x600C
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; X 88
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x300C
dw 0x300C
dw 0x1818
dw 0x1818
dw 0x0C30
dw 0x0C30
dw 0x0660
dw 0x0660
dw 0x03C0
dw 0x03C0
dw 0x03C0
dw 0x03C0
dw 0x0660
dw 0x0660
dw 0x0C30
dw 0x0C30
dw 0x1818
dw 0x1818
dw 0x300C
dw 0x300C
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; Y 89
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x1818
dw 0x1818
dw 0x0C30
dw 0x0C30
dw 0x0660
dw 0x0660
dw 0x03C0
dw 0x03C0
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; Z 90
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x3FFC
dw 0x3FFC
dw 0x000C
dw 0x000C
dw 0x000C
dw 0x001C
dw 0x0038
dw 0x0070
dw 0x00E0
dw 0x01C0
dw 0x0380
dw 0x0700
dw 0x0E00
dw 0x1C00
dw 0x3800
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3FFC
dw 0x3FFC
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; bracketleft 91
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0FE0
dw 0x0FE0
dw 0x0C00
dw 0x0C00
dw 0x0C00
dw 0x0C00
dw 0x0C00
dw 0x0C00
dw 0x0C00
dw 0x0C00
dw 0x0C00
dw 0x0C00
dw 0x0C00
dw 0x0C00
dw 0x0C00
dw 0x0C00
dw 0x0C00
dw 0x0C00
dw 0x0FE0
dw 0x0FE0
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; backslash 92
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x3000
dw 0x3000
dw 0x1800
dw 0x1800
dw 0x0C00
dw 0x0C00
dw 0x0600
dw 0x0600
dw 0x0300
dw 0x0300
dw 0x0180
dw 0x0180
dw 0x00C0
dw 0x00C0
dw 0x0060
dw 0x0060
dw 0x0030
dw 0x0030
dw 0x0018
dw 0x0018
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; bracketright 93
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0FE0
dw 0x0FE0
dw 0x0060
dw 0x0060
dw 0x0060
dw 0x0060
dw 0x0060
dw 0x0060
dw 0x0060
dw 0x0060
dw 0x0060
dw 0x0060
dw 0x0060
dw 0x0060
dw 0x0060
dw 0x0060
dw 0x0060
dw 0x0060
dw 0x0FE0
dw 0x0FE0
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; asciicircum 94
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0180
dw 0x03C0
dw 0x07E0
dw 0x0E70
dw 0x1C38
dw 0x381C
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; underscore 95
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x3FFC
dw 0x3FFC
dw 0x0000
dw 0x0000
dw 0x0000
;; grave 96
dw 0x0000
dw 0x0000
dw 0x0E00
dw 0x0700
dw 0x0380
dw 0x01C0
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; a 97
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x1FF0
dw 0x1FF8
dw 0x001C
dw 0x000C
dw 0x000C
dw 0x0FFC
dw 0x1FFC
dw 0x380C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x380C
dw 0x1FFC
dw 0x0FFC
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; b 98
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3FF0
dw 0x3FF8
dw 0x301C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x301C
dw 0x3FF8
dw 0x3FF0
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; c 99
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0FF0
dw 0x1FF8
dw 0x381C
dw 0x300C
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x300C
dw 0x381C
dw 0x1FF8
dw 0x0FF0
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; d 100
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x000C
dw 0x000C
dw 0x000C
dw 0x000C
dw 0x000C
dw 0x000C
dw 0x0FFC
dw 0x1FFC
dw 0x380C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x380C
dw 0x1FFC
dw 0x0FFC
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; e 101
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0FF0
dw 0x1FF8
dw 0x381C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x3FFC
dw 0x3FFC
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x380C
dw 0x1FFC
dw 0x0FF8
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; f 102
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x007E
dw 0x00FE
dw 0x01C0
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x1FF8
dw 0x1FF8
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; g 103
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0FFC
dw 0x1FFC
dw 0x380C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x380C
dw 0x1FFC
dw 0x0FFC
dw 0x000C
dw 0x000C
dw 0x001C
dw 0x1FF8
dw 0x1FF0
dw 0x0000
;; h 104
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3FF0
dw 0x3FF8
dw 0x301C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; i 105
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0000
dw 0x0000
dw 0x0780
dw 0x0780
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x07E0
dw 0x07E0
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; j 106
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0018
dw 0x0018
dw 0x0018
dw 0x0018
dw 0x0000
dw 0x0000
dw 0x0078
dw 0x0078
dw 0x0018
dw 0x0018
dw 0x0018
dw 0x0018
dw 0x0018
dw 0x0018
dw 0x0018
dw 0x0018
dw 0x0018
dw 0x0018
dw 0x0018
dw 0x0018
dw 0x1818
dw 0x1818
dw 0x1C38
dw 0x0FF0
dw 0x07E0
dw 0x0000
;; k 107
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x1800
dw 0x1800
dw 0x1800
dw 0x1800
dw 0x1800
dw 0x1800
dw 0x181C
dw 0x1838
dw 0x1870
dw 0x18E0
dw 0x19C0
dw 0x1B80
dw 0x1F00
dw 0x1F00
dw 0x1B80
dw 0x19C0
dw 0x18E0
dw 0x1870
dw 0x1838
dw 0x181C
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; l 108
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0780
dw 0x0780
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x07E0
dw 0x07E0
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; m 109
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x3FF0
dw 0x3FF8
dw 0x319C
dw 0x318C
dw 0x318C
dw 0x318C
dw 0x318C
dw 0x318C
dw 0x318C
dw 0x318C
dw 0x318C
dw 0x318C
dw 0x318C
dw 0x318C
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; n 110
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x3FF0
dw 0x3FF8
dw 0x301C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; o 111
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0FF0
dw 0x1FF8
dw 0x381C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x381C
dw 0x1FF8
dw 0x0FF0
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; p 112
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x3FF0
dw 0x3FF8
dw 0x301C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x301C
dw 0x3FF8
dw 0x3FF0
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x0000
;; q 113
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0FFC
dw 0x1FFC
dw 0x380C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x380C
dw 0x1FFC
dw 0x0FFC
dw 0x000C
dw 0x000C
dw 0x000C
dw 0x000C
dw 0x000C
dw 0x0000
;; r 114
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x33FC
dw 0x37FC
dw 0x3E00
dw 0x3C00
dw 0x3800
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x3000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; s 115
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0FF0
dw 0x1FF8
dw 0x381C
dw 0x3000
dw 0x3000
dw 0x3800
dw 0x1FF0
dw 0x0FF8
dw 0x001C
dw 0x000C
dw 0x000C
dw 0x381C
dw 0x1FF8
dw 0x0FF0
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; t 116
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0300
dw 0x0300
dw 0x0300
dw 0x0300
dw 0x0300
dw 0x0300
dw 0x3FF0
dw 0x3FF0
dw 0x0300
dw 0x0300
dw 0x0300
dw 0x0300
dw 0x0300
dw 0x0300
dw 0x0300
dw 0x0300
dw 0x0300
dw 0x0380
dw 0x01FC
dw 0x00FC
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; u 117
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x380C
dw 0x1FFC
dw 0x0FFC
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; v 118
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x1818
dw 0x1818
dw 0x1818
dw 0x0C30
dw 0x0C30
dw 0x0C30
dw 0x0660
dw 0x0660
dw 0x03C0
dw 0x03C0
dw 0x03C0
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; w 119
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x318C
dw 0x318C
dw 0x318C
dw 0x318C
dw 0x318C
dw 0x318C
dw 0x318C
dw 0x399C
dw 0x1FF8
dw 0x0FF0
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; x 120
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x300C
dw 0x300C
dw 0x381C
dw 0x1C38
dw 0x0E70
dw 0x07E0
dw 0x03C0
dw 0x03C0
dw 0x07E0
dw 0x0E70
dw 0x1C38
dw 0x381C
dw 0x300C
dw 0x300C
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; y 121
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x300C
dw 0x380C
dw 0x1FFC
dw 0x0FFC
dw 0x000C
dw 0x000C
dw 0x001C
dw 0x1FF8
dw 0x1FF0
dw 0x0000
;; z 122
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x3FFC
dw 0x3FFC
dw 0x001C
dw 0x0038
dw 0x0070
dw 0x00E0
dw 0x01C0
dw 0x0380
dw 0x0700
dw 0x0E00
dw 0x1C00
dw 0x3800
dw 0x3FFC
dw 0x3FFC
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; braceleft 123
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x00F0
dw 0x01F0
dw 0x0380
dw 0x0300
dw 0x0300
dw 0x0300
dw 0x0300
dw 0x0300
dw 0x0300
dw 0x1E00
dw 0x1E00
dw 0x0300
dw 0x0300
dw 0x0300
dw 0x0300
dw 0x0300
dw 0x0300
dw 0x0380
dw 0x01F0
dw 0x00F0
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; bar 124
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; braceright 125
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x1E00
dw 0x1F00
dw 0x0380
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x00F0
dw 0x00F0
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0180
dw 0x0380
dw 0x1F00
dw 0x1E00
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; asciitilde 126
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0E0C
dw 0x1F0C
dw 0x3B8C
dw 0x31DC
dw 0x30F8
dw 0x3070
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
;; Cursor Line 127
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0x0000
dw 0xFFFF

;; Sector padding
times 32256-($-$$) db 0
