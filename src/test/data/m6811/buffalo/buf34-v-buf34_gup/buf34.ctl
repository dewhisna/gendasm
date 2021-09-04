;
; Disassembly Control File for:
;
;   Buffalo v3.4
;

load 0 buf34.s19 s19
output disassembly buf34.dis
output functions buf34.fnc

memmap ROM E000 2000
memmap EE B600 0200
memmap RAM 0000 1000
memmap IO 1000 0040

tabs off
addresses on
ascii on
asciibytes off

label ffd6 scivect
label ffd8 spivect
label ffda paievect
label ffdc paovect
label ffde tovfvect
label ffe0 ti4o5vect
label ffe2 to4vect
label ffe4 to3vect
label ffe6 to2vect
label ffe8 to1vect
label ffea ti3vect
label ffec ti2vect
label ffee ti1vect
label fff0 rtivect
label fff2 irqvect
label fff4 xirqvect
label fff6 swivect
label fff8 ilopvect
label fffa copvect
label fffc cmonvect
label fffe rstvect

; Note: All but the reset vector point to
;  a RAM vector trampoline
;indirect ffd6 scirtn
;indirect ffd8 spirtn
;indirect ffda paiertn
;indirect ffdc paortn
;indirect ffde tovfrtn
;indirect ffe0 ti4o5rtn
;indirect ffe2 to4rtn
;indirect ffe4 to3rtn
;indirect ffe6 to2rtn
;indirect ffe8 to1rtn
;indirect ffea ti3rtn
;indirect ffec ti2rtn
;indirect ffee ti1rtn
;indirect fff0 rtirtn
;indirect fff2 irqrtn
;indirect fff4 xirqrtn
;indirect fff6 swirtn
;indirect fff8 iloprtn
;indirect fffa coprtn
;indirect fffc cmonrtn
indirect fffe reset

; Add entry points for redirected vectors
entry fba5 XIRQIN
entry fa7a SWIIN

; Add indirects for the Command Table
indirect e554 ASSEM
indirect e55c BREAK
indirect e563 BULK
indirect e56d BULKALL
indirect e574 CALL
indirect e57b DUMP
indirect e583 EEMOD
indirect e58a FILL
indirect e58f GO
indirect e596 HELP
indirect e59d HOST
indirect e5a4 LOAD
indirect e5ad MEMORY
indirect e5b4 MOVE
indirect e5bd OFFSET
indirect e5c7 PROCEED
indirect e5d2 REGISTER
indirect e5db STOPAT
indirect e5e3 TRACE
indirect e5ec VERIFY
indirect e5f0 ; Alias for HELP
indirect e5f8 BOOT
indirect e5fc TILDE
; ----
indirect e602 ; Alias for ASSEM
indirect e607 ; Alias for FILL
indirect e60e ; Alias for MOVE
indirect e616 ; Alias for BULK
indirect e61b ; Alias for DUMP
indirect e620 ; Alias for MEMORY
indirect e625 ; Alias for REGISTER
indirect e62a ; Alias for REGISTER
indirect e631 ; Alias for MOVE
indirect e636 ; Alias for HOST
indirect e63d EVBTEST

; Add entries for the Jump table
entry ff7c _WARMST
entry ff7f _BPCLR
entry ff82 _RPRINT
entry ff85 _HEXBIN
entry ff88 _BUFFAR
entry ff8b _TERMAR
entry ff8e _CHGBYT
entry ff91 _READBU
entry ff94 _INCBUF
entry ff97 _DECBUF
entry ff9a _WSKIP
entry ff9d _CHKABR
; ----
entry ffa0 _UPCASE
entry ffa3 _WCHEK
entry ffa6 _DCHEK
entry ffa9 _INIT
entry ffac _INPUT
entry ffaf _OUTPUT
entry ffb2 _OUTLHL
entry ffb5 _OUTRHL
entry ffb8 _OUTA
entry ffbb _OUT1BY
entry ffbe _OUT1BS
entry ffc1 _OUT2BS
entry ffc4 _OUTCRL
entry ffc7 _OUTSTR
entry ffca _OUTST0
entry ffcd _INCHAR
entry ffd0 _VECINT

