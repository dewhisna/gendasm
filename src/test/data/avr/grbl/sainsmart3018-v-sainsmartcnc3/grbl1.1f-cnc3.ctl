;
; AVR GDC Disassembler Control File for:
;
;  SainSmart grbl1.1f-cnc3

mcu m328p

load 0000 grbl1.1f-cnc3.hex intel
output disassembly grbl1.1f-cnc3.dis
output functions grbl1.1f-cnc3.fnc

tabs off
addresses on
ascii on
dataopbytes on
opbytes on

; Jump Table:
indirect code 0068
indirect code 006a
indirect code 006c
indirect code 006e
indirect code 0070
indirect code 0072
indirect code 0074
indirect code 0076
indirect code 0078
indirect code 007a
indirect code 007c
indirect code 007e
indirect code 0080
indirect code 0082
indirect code 0084
indirect code 0086
indirect code 0088
indirect code 008a
indirect code 008c
indirect code 008e
indirect code 0090
indirect code 0092
indirect code 0094
indirect code 0096
indirect code 0098
indirect code 009a
indirect code 009c
indirect code 009e
indirect code 00a0
indirect code 00a2
indirect code 00a4
indirect code 00a6
indirect code 00a8
indirect code 00aa
indirect code 00ac
indirect code 00ae
indirect code 00b0
indirect code 00b2
indirect code 00b4
indirect code 00b6
indirect code 00b8
indirect code 00ba
indirect code 00bc
indirect code 00be
indirect code 00c0
indirect code 00c2
indirect code 00c4
indirect code 00c6
indirect code 00c8
indirect code 00ca
indirect code 00cc
indirect code 00ce
indirect code 00d0
indirect code 00d2
indirect code 00d4
indirect code 00d6
indirect code 00d8
indirect code 00da
indirect code 00dc
indirect code 00de
indirect code 00e0
indirect code 00e2
indirect code 00e4
indirect code 00e6
indirect code 00e8
indirect code 00ea
indirect code 00ec
indirect code 00ee
indirect code 00f0
indirect code 00f2
indirect code 00f4
indirect code 00f6
indirect code 00f8
indirect code 00fa
indirect code 00fc
indirect code 00fe
indirect code 0100
indirect code 0102
indirect code 0104
indirect code 0106
indirect code 0108
indirect code 010a
indirect code 010c

; Data Blocks:
; 010E-0159
datablock 010E 004C
; 015A-01EE
datablock 015A 0095
; 01EF-028B
datablock 01EF 009D
; 028C-028E
datablock 028C 0003
; 028F-02AB
datablock 028F 001D
; 02AC-02BD
datablock 02AC 0012
; 02BE-0320
datablock 02BE 0063
; 0321 (alignment):
datablock 0321 0001

