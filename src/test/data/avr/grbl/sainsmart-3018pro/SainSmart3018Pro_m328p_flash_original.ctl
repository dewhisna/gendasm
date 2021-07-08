;
; AVR GDC Disassembler Control File for:
;
;  grbl

mcu m328p

load 0000 SainSmart3018Pro_m328p_flash_original.hex intel
output disassembly SainSmart3018Pro_m328p_flash_original.dis
output functions SainSmart3018Pro_m328p_flash_original.fnc

tabs off
addresses on
ascii on
dataopbytes on
opbytes on

; Jump Table:
indirect code 7494
indirect code 7496
indirect code 7498
indirect code 749a
indirect code 749c
indirect code 749e
indirect code 74a0
indirect code 74a2
indirect code 74a4
indirect code 74a6
indirect code 74a8
indirect code 74aa
indirect code 74ac
indirect code 74ae
indirect code 74b0
indirect code 74b2
indirect code 74b4
indirect code 74b6
indirect code 74b8
indirect code 74ba
indirect code 74bc
indirect code 74be
indirect code 74c0
indirect code 74c2
indirect code 74c4
indirect code 74c6
indirect code 74c8
indirect code 74ca
indirect code 74cc
indirect code 74ce
indirect code 74d0
indirect code 74d2
indirect code 74d4
indirect code 74d6
indirect code 74d8
indirect code 74da
indirect code 74dc
indirect code 74de
indirect code 74e0
indirect code 74e2
indirect code 74e4
indirect code 74e6
indirect code 74e8
indirect code 74ea
indirect code 74ec
indirect code 74ee
indirect code 74f0
indirect code 74f2
indirect code 74f4
indirect code 74f6
indirect code 74f8
indirect code 74fa
indirect code 74fc
indirect code 74fe
indirect code 7500
indirect code 7502
indirect code 7504
indirect code 7506
indirect code 7508
indirect code 750a
indirect code 750c
indirect code 750e
indirect code 7510
indirect code 7512
indirect code 7514
indirect code 7516
indirect code 7518
indirect code 751a
indirect code 751c
indirect code 751e
indirect code 7520
indirect code 7522
indirect code 7524
indirect code 7526
indirect code 7528
indirect code 752a
indirect code 752c
indirect code 752e
indirect code 7530
indirect code 7532
indirect code 7534
indirect code 7536
indirect code 7538

; This image has a bootloader at 0x7800
; But it will only be in conflict with the
;	original grbl code which doesn't
;	have a bootloader in its image, so
;	we will ignore entry points for it

