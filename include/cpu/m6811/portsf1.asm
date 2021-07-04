;
;  68HC11 Port Equivalency:
;
        .module PORTSF1
        .area   REGSCODE (REL,OVR)

pporta  ==      0x00
porta:: .blkb   1                       ; $0000 I/O Port A
pddra   ==      0x01
ddra::  .blkb   1                       ; $0001 Data Direction for A
pportg  ==      0x02
portg:: .blkb   1                       ; $0002 I/O Port G
pddrg   ==      0x03
ddrg::  .blkb   1                       ; $0003 Data Direction for G
pportb  ==      0x04
portb:: .blkb   1                       ; $0004 I/O Port B
pportf  ==      0x05
portf:: .blkb   1                       ; $0005 I/O Port F
pportc  ==      0x06
portc:: .blkb   1                       ; $0006 I/O Port C
pddrc   ==      0x07
ddrc::  .blkb   1                       ; $0007 Data Direction for C
pportd  ==      0x08
portd:: .blkb   1                       ; $0008 I/O Port D
pddrd   ==      0x09
ddrd::  .blkb   1                       ; $0009 Data Direction for D
pporte  ==      0x0A
porte:: .blkb   1                       ; $000A I/O Port E
pcforc  ==      0x0B
cforc:: .blkb   1                       ; $000B Force Output Compare
poc1m   ==      0x0C
oc1m::  .blkb   1                       ; $000C OC1 Control Mask
poc1d   ==      0x0D
oc1d::  .blkb   1                       ; $000D OC1 Control Data
ptcnt   ==      0x0E
ptcnth	==		0x0E
ptcntl	==		0x0F
tcnt::									; $000E-000F Timer Counter (Main)
tcnth::	.blkb	1
tcntl::	.blkb	1
ptic1   ==      0x10
ptic1h	==		0x10
ptic1l	==		0x11
tic1::									; $0010-0011 IC1 Timer Counter
tic1h::	.blkb	1
tic1l::	.blkb	1
ptic2   ==      0x12
ptic2h	==		0x12
ptic2l	==		0x13
tic2::									; $0012-0013 IC2 Timer Counter
tic2h::	.blkb	1
tic2l::	.blkb	1
ptic3   ==      0x14
ptic3h	==		0x14
ptic3l	==		0x15
tic3::									; $0014-0015 IC3 Timer Counter
tic3h::	.blkb	1
tic3l::	.blkb	1
ptoc1   ==      0x16
ptoc1h	==		0x16
ptoc1l	==		0x17
toc1::									; $0016-0017 OC1 Timer Counter
toc1h::	.blkb	1
toc1l::	.blkb	1
ptoc2   ==      0x18
ptoc2h	==		0x18
ptoc2l	==		0x19
toc2::									; $0018-0019 OC2 Timer Counter
toc2h::	.blkb	1
toc2l::	.blkb	1
ptoc3   ==      0x1A
ptoc3h	==		0x1A
ptoc3l	==		0x1B
toc3::									; $001A-001B OC3 Timer Counter
toc3h::	.blkb	1
toc3l::	.blkb	1
ptoc4   ==      0x1C
ptoc4h	==		0x1C
ptoc4l	==		0x1D
toc4::									; $001C-001D OC4 Timer Counter
toc4h::	.blkb	1
toc4l::	.blkb	1
pti4o5  ==      0x1E
pti4o5h	==		0x1E
pti4o5l	==		0x1F
ti4o5::									; $001E-001F IC4 & OC5 Timer Counter
ti4o5h::	.blkb	1
ti4o5l::	.blkb	1
ptctl1  ==      0x20
tctl1:: .blkb   1                       ; $0020 Timer Control 1
ptctl2  ==      0x21
tctl2:: .blkb   1                       ; $0021 Timer Control 2
ptmsk1  ==      0x22
tmsk1:: .blkb   1                       ; $0022 Timer Mask 1
ptflg1  ==      0x23
tflg1:: .blkb   1                       ; $0023 Timer Flag 1
ptmsk2  ==      0x24
tmsk2:: .blkb   1                       ; $0024 Timer Mask 2
ptflg2  ==      0x25
tflg2:: .blkb   1                       ; $0025 Timer Flag 2
ppactl  ==      0x26
pactl:: .blkb   1                       ; $0026 Pulse Accum Control
ppacnt  ==      0x27
pacnt:: .blkb   1                       ; $0027 Pulse Accum Count
pspcr   ==      0x28
spcr::  .blkb   1                       ; $0028 SPI Control Register
pspsr   ==      0x29
spsr::  .blkb   1                       ; $0029 SPI Status Register
pspdr   ==      0x2A
spdr::  .blkb   1                       ; $002A SPI Data Register
pbaud   ==      0x2B
baud::  .blkb   1                       ; $002B Baud Set Register
psccr1  ==      0x2C
sccr1:: .blkb   1                       ; $002C SCI Control Register 1
psccr2  ==      0x2D
sccr2:: .blkb   1                       ; $002D SCI Control Register 2
pscsr   ==      0x2E
scsr::  .blkb   1                       ; $002E SCI Status Register
pscdr   ==      0x2F
scdr::  .blkb   1                       ; $002F SCI Data Register
padctl  ==      0x30
adctl:: .blkb   1                       ; $0030 A/D Conv Control Reg
padr1   ==      0x31
adr1::  .blkb   1                       ; $0031 A/D Result Reg 1
padr2   ==      0x32
adr2::  .blkb   1                       ; $0032 A/D Result Reg 2
padr3   ==      0x33
adr3::  .blkb   1                       ; $0033 A/D Result Reg 3
padr4   ==      0x34
adr4::  .blkb   1                       ; $0034 A/D Result Reg 4
pbprot  ==      0x35
bprot:: .blkb   1                       ; $0035 Protection Control Reg
; Reserved:
        .blkb   2                       ; $0036-0037 Reserved
popt2   ==      0x38
opt2::  .blkb   1                       ; $0038 Option 2 Reg
poption ==      0x39
option:: .blkb  1                       ; $0039 Option Reg
pcoprst ==      0x3A
coprst:: .blkb  1                       ; $003A COP Watch Dog Arm/Reset
ppprog  ==      0x3B
pprog:: .blkb   1                       ; $003B EEProm Prog Ctrl
phprio  ==      0x3C
hprio:: .blkb   1                       ; $003C Hi-Priority Set Reg
pinit   ==      0x3D
init::  .blkb   1                       ; $003D Init Reg
ptest1  ==      0x3E
test1:: .blkb   1                       ; $003E Test Ctrl Reg
pconfig ==      0x3F
config:: .blkb  1                       ; $003F Configuration Reg
; Reserved:       
        .blkb   0x1B                    ; $0040-005B Reserved
pcsstrh ==      0x5C
csstrh:: .blkb  1                       ; $005C Chip-Select Clock Stretch
pcsctl  ==      0x5D
csctl:: .blkb   1                       ; $005D Chip-Select Control
pcsgadr ==      0x5E
csgadr:: .blkb  1                       ; $005E GP Chip-Select Address Reg
pcsgsiz ==      0x5F
csgsiz:: .blkb  1                       ; $005F GP Chip-Select Size Reg
