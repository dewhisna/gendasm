;
; Disassembly Control File for:
;
;   Buffalo v3.4_gup
;

load 0 buf34_gup.s19 s19
output disassembly buf34_gup.dis
output functions buf34_gup.fnc

memmap ROM E000 2000
memmap EE B600 0200
memmap RAM 0000 1000
memmap IO 1000 0040

tabs off
addresses on
ascii off
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
entry fb77 XIRQIN
entry fa4b SWIIN

; Add indirects for the Command Table
indirect e574 ASSEM
indirect e57c BREAK
indirect e583 BULK
indirect e58d BULKALL
indirect e594 CALL
indirect e59b DUMP
indirect e5a3 EEMOD
indirect e5aa FILL
indirect e5af GO
indirect e5b6 HELP
indirect e5bd HOST
indirect e5c4 LOAD
indirect e5cd MEMORY
indirect e5d4 MOVE
indirect e5dd OFFSET
indirect e5e7 PROCEED
indirect e5f2 REGISTER
indirect e5fb STOPAT
indirect e603 TRACE
indirect e60c VERIFY
indirect e610 ; Alias for HELP
indirect e618 BOOT
indirect e61c TILDE
; ----
indirect e622 ; Alias for ASSEM
indirect e627 ; Alias for FILL
indirect e62e ; Alias for MOVE
indirect e636 ; Alias for BULK
indirect e63b ; Alias for DUMP
indirect e640 ; Alias for MEMORY
indirect e645 ; Alias for REGISTER
indirect e64a ; Alias for REGISTER
indirect e651 ; Alias for MOVE
indirect e656 ; Alias for HOST
indirect e65d EVBTEST

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
