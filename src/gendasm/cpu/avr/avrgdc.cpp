//
//	Atmel(Microchip) AVR Disassembler GDC Module
//	Copyright(c)2021 by Donna Whisnant
//
//	Generic Code-Seeking Disassembler
//	Copyright(c)2021 by Donna Whisnant
//

//
//	AVRGDC -- This is the implementation for the AVR Disassembler GDC module
//
//
//	Groups: (Grp) : xy
//	  Where x (msnb = destination = FIRST operand)
//			y (lsnb = source = LAST operand)
//
//			0 = opcode only, nothing follows
//			1 = Register Direct, Single Register: Destination Register (Rd)
//			2 = Register Direct, Two Registers: Destination Register (Rd), Source Register (Rr)
//			3 = I/O Direct Addressing, Single Register: Source/Destination Register (Rd/Rr), I/O Address (A) [IO Range 0-63]
//			4 = Direct Addressing, Single Register: Source/Destination Register (Rd/Rr), 16-Bit Data Address (A), Note: LDS instruction uses RAMPD register for >64KB access
//			5 = Register Indirect X, Single Register: Source/Destination Register (Rd/Rr)
//			6 = Register Indirect Y, Single Register: Source/Destination Register (Rd/Rr)
//			7 = Register Indirect Z, Single Register: Source/Destination Register (Rd/Rr)
//			8 = Register Indirect X Pre-Decrement, Single Register: Source/Destination Register (Rd/Rr)
//			9 = Register Indirect Y Pre-Decrement, Single Register: Source/Destination Register (Rd/Rr)
//			A = Register Indirect Z Pre-Decrement, Single Register: Source/Destination Register (Rd/Rr)
//			B = Register Indirect X Post-Increment, Single Register: Source/Destination Register (Rd/Rr)
//			C = Register Indirect Y Post-Increment, Single Register: Source/Destination Register (Rd/Rr)
//			D = Register Indirect Z Post-Increment, Single Register: Source/Destination Register (Rd/Rr)
//			E = Register Indirect Y w/Displacement, Single Register: Source/Destination Register (Rd/Rr)
//			F = Register Indirect Z w/Displacement, Single Register: Source/Destination Register (Rd/Rr)
//			10 = Program Memory Constant Addressing Z, Single Register: Destination Register (Rd), for LPM, ELPM
//					Note: While this is technically for SPM too, SPM always has r0/r1 as Source Registers
//					there is no operand for the "spm" instruction.  Therefore, Group(0) is used for SPM for both Src/Dst
//					Note: LPM/ELPM have an implied form with no arguments that implies r0 which will use Group(0)
//			11 = Program Memory Constant Addressing Z Post-Increment, Single Register: Destination Register (Rd), for LPM, ELPM, SPM
//					Note: SPM always has r0/r1 as Source Registers
//			12 = Register Direct with Bit index, Single Register: Destination Register (Rd), w/Bit index (BLD, BST)
//			13 = 16-bit/22-bit absolute address (JMP, CALL)
//			14 = 12-bit relative address (RJMP, RCALL)
//			15 = 6-bit relative address (BRBC, BRBS)
//					Note: Actual BRBC and BRBS opcodes aren't used as they are ambiguous with the
//					individualized ones BRCC, BRCS, BREQ, BRNE, etc.
//


//			1 = 8-bit absolute address follows only
//			2 = 16-bit absolute address follows only
//			3 = 8-bit relative address follows only
//			4 = 16-bit relative address follows only
//			5 = 8-bit data follows only
//			6 = 16-bit data follows only
//			7 = 8-bit absolute address followed by 8-bit mask
//			8 = 8-bit X offset address followed by 8-bit mask
//			9 = 8-bit Y offset address followed by 8-bit mask
//			A = 8-bit X offset address
//			B = 8-bit Y offset address
//
//	Control: (Algorithm control) : xy
//	  Where x (msnb = destination = FIRST operand)
//			y (lsnb = source = LAST operand)
//
//			0 = Disassemble as code
//
//			1 = Disassemble as code, with data addr label
//					(on opcodes with post-byte, label generation is dependent upon post-byte value.)
//
//			2 = Disassemble as undeterminable branch address (Comment code)
//
//			3 = Disassemble as determinable branch, add branch addr and label
//
//			4 = Disassemble as conditionally undeterminable branch address (Comment code),
//					with data addr label, (on opcodes with post-byte, label generation is dependent
//					upon post-byte value. conditional upon post-byte)
//
//	This version setup for compatibility with the asavr assembler
//

// ============================================================================
//
// Opcode List:
//		{	"andi",		S_IBYTE,	0x7000	}, F000  Rd, K    ---- KKKK dddd KKKK (K: 0-255), (d: 16-31)
//		{	"cpi",		S_IBYTE,	0x3000	}, F000  Rd, K    ---- KKKK dddd KKKK (K: 0-255), (d: 16-31)
//		{	"ldi",		S_IBYTE,	0xE000	}, F000  Rd, K    ---- KKKK dddd KKKK (K: 0-255), (d: 16-31)
//		{	"ori",		S_IBYTE,	0x6000	}, F000  Rd, K    ---- KKKK dddd KKKK (K: 0-255), (d: 16-31)
//		{	"sbci",		S_IBYTE,	0x4000	}, F000  Rd, K    ---- KKKK dddd KKKK (K: 0-255), (d: 16-31)
//		{	"sbr",		S_IBYTE,	0x6000	}, F000  Rd, K    ---- KKKK dddd KKKK (K: 0-255), (d: 16-31)
//		{	"subi",		S_IBYTE,	0x5000	}, F000  Rd, K    ---- KKKK dddd KKKK (K: 0-255), (d: 16-31)
//
//		{	"cbr",		S_CBR,		0x7000	}, F000  Rd, k    ---- KKKK dddd KKKK (K: 0-255), (d: 16-31)  xxx dup -> andi Rd,(0xFF-K)
//
//		{	"adiw",		S_IWORD,	0x9600	}, FF00  Rd, K    ---- ---- KKdd KKKK (K: 0-63), (d: 24, 26, 28, 30)
//		{	"sbiw",		S_IWORD,	0x9700	}, FF00  Rd, K    ---- ---- KKdd KKKK (K: 0-63), (d: 24, 26, 28, 30)
//
//		{	"asr",		S_SNGL,		0x9405	}, FE0F  Rd       ---- ---d dddd ---- (d: 0-31)
//		{	"com",		S_SNGL,		0x9400	}, FE0F  Rd       ---- ---d dddd ---- (d: 0-31)
//		{	"dec",		S_SNGL,		0x940A	}, FE0F  Rd       ---- ---d dddd ---- (d: 0-31)
//		{	"inc",		S_SNGL,		0x9403	}, FE0F  Rd       ---- ---d dddd ---- (d: 0-31)
//		{	"lsr",		S_SNGL,		0x9406	}, FE0F  Rd       ---- ---d dddd ---- (d: 0-31)
//		{	"neg",		S_SNGL,		0x9401	}, FE0F  Rd       ---- ---d dddd ---- (d: 0-31)
//		{	"pop",		S_SNGL,		0x900F	}, FE0F  Rd       ---- ---d dddd ---- (d: 0-31)
//		{	"push",		S_SNGL,		0x920F	}, FE0F  Rr       ---- ---r rrrr ---- (r: 0-31)
//		{	"ror",		S_SNGL,		0x9407	}, FE0F  Rd       ---- ---d dddd ---- (d: 0-31)
//		{	"swap",		S_SNGL,		0x9402	}, FE0F  Rd       ---- ---d dddd ---- (d: 0-31)
//		{	"xch",		S_SNGL,		0x9204	}, FE0F  Rd       ---- ---d dddd ---- (d: 0-31)
//
//		{	"lac",		S_LA??,		0x9206	}, FE0F  Z, Rr    ---- ---r rrrr ---- (r: 0-31) (implied Z for destination)
//		{	"las",		S_LA??,		0x9205	}, FE0F  Z, Rr    ---- ---r rrrr ---- (r: 0-31) (implied Z for destination)
//		{	"lat",		S_LA??,		0x9207	}, FE0F  Z, Rr    ---- ---r rrrr ---- (r: 0-31) (implied Z for destination)
//
//		{	"clr",		S_SAME,		0x2400	}, FC00  Rd       ---- --dd dddd dddd (d: 0-31)x2 xxx dup eor
//		{	"lsl",		S_SAME,		0x0C00	}, FC00  Rd       ---- --dd dddd dddd (d: 0-31)x2 xxx dup add
//		{	"rol",		S_SAME,		0x1C00	}, FC00  Rd       ---- --dd dddd dddd (d: 0-31)x2 xxx dup adc
//		{	"tst",		S_SAME,		0x2000	}, FC00  Rd       ---- --dd dddd dddd (d: 0-31)x2 xxx dup and
//
//		{	"adc",		S_DUBL,		0x1C00	}, FC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
//		{	"add",		S_DUBL,		0x0C00	}, FC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
//		{	"and",		S_DUBL,		0x2000	}, FC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
//		{	"cp",		S_DUBL,		0x1400	}, FC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
//		{	"cpc",		S_DUBL,		0x0400	}, FC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
//		{	"cpse",		S_DUBL,		0x1000	}, FC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31), Difficult control requiring lookahead
//		{	"eor",		S_DUBL,		0x2400	}, FC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
//		{	"mov",		S_DUBL,		0x2C00	}, FC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
//		{	"or",		S_DUBL,		0x2800	}, FC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
//		{	"sbc",		S_DUBL,		0x0800	}, FC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
//		{	"sub",		S_DUBL,		0x1800	}, FC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
//
//		{	"movw",		S_MOVW,		0x0100	}, FF00  Rd, Rr   ---- ---- dddd rrrr (d: 0,2,4,...,30), (r: 0,2,4,...,30)
//
//		{	"mul",		S_MUL,		0x9C00	}, FC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)  ?? S_DUBL
//		{	"muls",		S_MULS,		0x0200	}, FF00  Rd, Rr   ---- ---- dddd rrrr (d: 16-31), (r: 16-31)
//		{	"mulsu",	S_FMUL,		0x0300	}, FF88  Rd, Rr   ---- ---- -ddd -rrr (d: 16-23), (r: 16-23)
//		{	"fmul",		S_FMUL,		0x0308	}, FF88  Rd, Rr   ---- ---- -ddd -rrr (d: 16-23), (r: 16-23)
//		{	"fmuls",	S_FMUL,		0x0380	}, FF88  Rd, Rr   ---- ---- -ddd -rrr (d: 16-23), (r: 16-23)
//		{	"fmulsu",	S_FMUL,		0x0388	}, FF88  Rd, Rr   ---- ---- -ddd -rrr (d: 16-23), (r: 16-23)
//
//		{	"ser",		S_SER,		0xEF0F	}, FF0F  Rd       ---- ---- dddd ---- (d: 16-31)  <<< SPC: ldi Rd,0xFF
//
//		{	"bclr",		S_SREG,		0x9488	}, FF8F  s        ---- ---- -sss ---- (s: 0-7)  xxx dup (see: S_INH)
//		{	"bset",		S_SREG,		0x9408	}, FF8F  s        ---- ---- -sss ---- (s: 0-7)  xxx dup (see: S_INH)
//
//		{	"bld",		S_TFLG,		0xF800	}, FE08  Rd, b    ---- ---d dddd -bbb (d: 0-31), (b: 0-7)
//		{	"bst",		S_TFLG,		0xFA00	}, FE08  Rd, b    ---- ---d dddd -bbb (d: 0-31), (b: 0-7)
//
//		{	"brcc",		S_BRA,		0xF400	}, FC07  k        ---- --kk kkkk k000 (k: -64 - 63)
//		{	"brcs",		S_BRA,		0xF000	}, FC07  k        ---- --kk kkkk k000 (k: -64 - 63)
//		{	"breq",		S_BRA,		0xF001	}, FC07  k        ---- --kk kkkk k001 (k: -64 - 63)
//		{	"brge",		S_BRA,		0xF404	}, FC07  k        ---- --kk kkkk k100 (k: -64 - 63)
//		{	"brhc",		S_BRA,		0xF405	}, FC07  k        ---- --kk kkkk k101 (k: -64 - 63)
//		{	"brhs",		S_BRA,		0xF005	}, FC07  k        ---- --kk kkkk k101 (k: -64 - 63)
//		{	"brid",		S_BRA,		0xF407	}, FC07  k        ---- --kk kkkk k111 (k: -64 - 63)
//		{	"brie",		S_BRA,		0xF007	}, FC07  k        ---- --kk kkkk k111 (k: -64 - 63)
//		{	"brlo",		S_BRA,		0xF000	}, FC07  k        ---- --kk kkkk k000 (k: -64 - 63)  xxx dup brcs
//		{	"brlt",		S_BRA,		0xF004	}, FC07  k        ---- --kk kkkk k100 (k: -64 - 63)
//		{	"brmi",		S_BRA,		0xF002	}, FC07  k        ---- --kk kkkk k010 (k: -64 - 63)
//		{	"brne",		S_BRA,		0xF401	}, FC07  k        ---- --kk kkkk k001 (k: -64 - 63)
//		{	"brpl",		S_BRA,		0xF402	}, FC07  k        ---- --kk kkkk k010 (k: -64 - 63)
//		{	"brsh",		S_BRA,		0xF400  }, FC07  k        ---- --kk kkkk k000 (k: -64 - 63)  xxx dup brcc
//		{	"brtc",		S_BRA,		0xF406	}, FC07  k        ---- --kk kkkk k110 (k: -64 - 63)
//		{	"brts",		S_BRA,		0xF006	}, FC07  k        ---- --kk kkkk k110 (k: -64 - 63)
//		{	"brvc",		S_BRA,		0xF403	}, FC07  k        ---- --kk kkkk k011 (k: -64 - 63)
//		{	"brvs",		S_BRA,		0xF003	}, FC07  k        ---- --kk kkkk k011 (k: -64 - 63)
//
//		{	"brbc",		S_SBRA,		0xF400	}, FC00  s, k     ---- --kk kkkk ksss (k: -64 - 63), (s: 0-7)
//		{	"brbs",		S_SBRA,		0xF000	}, FC00  s, k     ---- --kk kkkk ksss (k: -64 - 63), (s: 0-7)
//
//		{	"sbrc",		S_SKIP,		0xFC00	}, FE08  Rr, b    ---- ---r rrrr -bbb (r: 0-31), (b: 0-7)  ?? S_TFLG, Difficult control requiring lookahead
//		{	"sbrs",		S_SKIP,		0xFE00	}, FE08  Rr, b    ---- ---r rrrr -bbb (r: 0-31), (b: 0-7)  ?? S_TFLG, Difficult control requiring lookahead
//
//		{	"call",		S_JMP,		0x940E	}, FE0E  k        ---- ---k kkkk ---k | kkkk kkkk kkkk kkkk (k: 0-64k, 0-4M), 16/22-bit absolute
//		{	"jmp",		S_JMP,		0x940C	}, FE0E  k        ---- ---k kkkk ---k | kkkk kkkk kkkk kkkk (k: 0-64k, 0-4M), 16/22-bit absolute
//
//		{	"rcall",	S_RJMP,		0xD000	}, F000  k        ---- kkkk kkkk kkkk (k: -2k - 2k), 12-bit relative
//		{	"rjmp",		S_RJMP,		0xC000	}, F000  k        ---- kkkk kkkk kkkk (k: -2k - 2k), 12-bit relative
//
//		{	"cbi",		S_IOR,		0x9800	}, FF00  A, b     ---- ---- AAAA Abbb (A: 0-31), (b: 0-7)
//		{	"sbi",		S_IOR,		0x9A00	}, FF00  A, b     ---- ---- AAAA Abbb (A: 0-31), (b: 0-7)
//		{	"sbic",		S_IOR,		0x9900	}, FF00  A, b     ---- ---- AAAA Abbb (A: 0-31), (b: 0-7), Difficult control requiring lookahead
//		{	"sbis",		S_IOR,		0x9B00	}, FF00  A, b     ---- ---- AAAA Abbb (A: 0-31), (b: 0-7), Difficult control requiring lookahead
//
//		{	"in",		S_IN,		0xB000	}, F800  Rd, A    ---- -AAd dddd AAAA (A: 0-63), (d: 0-31)
//		{	"out",		S_OUT,		0xB800	}, F800  A, Rr    ---- -AAr rrrr AAAA (A: 0-63), (r: 0-31)
//
//		{	"ld",		S_LD,		0x8000	}, ????? (Z only?)
//		{	"ld",		S_LD/X		0x900C	}, FE0F  Rd, X    ---- ---d dddd ---- (d: 0-31)
//		{	"ld",		S_LD/X+		0x900D	}, FE0F  Rd, X+   ---- ---d dddd ---- (d: 0-31)
//		{	"ld",		S_LD/-X		0x900E	}, FE0F  Rd, -X   ---- ---d dddd ---- (d: 0-31)
//		{	"ld",		S_LD/Y		0x8008	}, FE0F  Rd, Y    ---- ---d dddd ---- (d: 0-31)  <<< SPC: ldd Rd,Y+0
//		{	"ld",		S_LD/Y+		0x9009	}, FE0F  Rd, Y+   ---- ---d dddd ---- (d: 0-31)
//		{	"ld",		S_LD/-Y		0x900A	}, FE0F  Rd, -Y   ---- ---d dddd ---- (d: 0-31)
//		{	"ld",		S_LD/Z		0x8000	}, FE0F  Rd, Z    ---- ---d dddd ---- (d: 0-31)  <<< SPC: ldd Rd,Z+0
//		{	"ld",		S_LD/Z+		0x9001	}, FE0F  Rd, Z+   ---- ---d dddd ---- (d: 0-31)
//		{	"ld",		S_LD/-Z		0x9002	}, FE0F  Rd, -Z   ---- ---d dddd ---- (d: 0-31)
//		{	"st",		S_ST,		0x8200	}, ????? (Z only?)
//		{	"st",		S_ST/X		0x920C	}, FE0F  X, Rr    ---- ---r rrrr ---- (r: 0-31)
//		{	"st",		S_ST/X+		0x920D	}, FE0F  X+, Rr   ---- ---r rrrr ---- (r: 0-31)
//		{	"st",		S_ST/-X		0x920E	}, FE0F  -X, Rr   ---- ---r rrrr ---- (r: 0-31)
//		{	"st",		S_ST/Y		0x8208	}, FE0F  Y, Rr    ---- ---r rrrr ---- (r: 0-31)  <<< SPC: std Rd,Y+0
//		{	"st",		S_ST/Y+		0x9209	}, FE0F  Y+, Rr   ---- ---r rrrr ---- (r: 0-31)
//		{	"st",		S_ST/-Y		0x920A	}, FE0F  -Y, Rr   ---- ---r rrrr ---- (r: 0-31)
//		{	"st",		S_ST/Z		0x8200	}, FE0F  Z, Rr    ---- ---r rrrr ---- (r: 0-31)  <<< SPC: std Rd,Z+0
//		{	"st",		S_ST/Z+		0x9201	}, FE0F  Z+, Rr   ---- ---r rrrr ---- (r: 0-31)
//		{	"st",		S_ST/-Z		0x9202	}, FE0F  -Z, Rr   ---- ---r rrrr ---- (r: 0-31)
//
//		{	"ldd",		S_ILD,		0x8000	}, ????? (Z only?)
//		{	"ldd",		S_ILD/Y+q	0x8008	}, D208  Rd, Y+q  --q- qq-d dddd -qqq (d: 0-31), (q: 0-63)
//		{	"ldd",		S_ILD/Z+q	0x8000	}, D208  Rd, Z+q  --q- qq-d dddd -qqq (d: 0-31), (q: 0-63)
//		{	"std",		S_IST,		0x8200	}, ????? (Z only?)
//		{	"std",		S_IST/Y+q	0x8208	}, D208  Y+q, Rr  --q- qq-r rrrr -qqq (r: 0-31), (q: 0-63)
//		{	"std",		S_IST/Z+q	0x8200	}, D208  Z+q, Rr  --q- qq-r rrrr -qqq (r: 0-31), (q: 0-63)
//
//		{	"lds",		S_LDS,		0x9000	}, FE0F  Rd, k    ---- ---d dddd ---- | kkkk kkkk kkkk kkkk (d: 0-31),(k: 0-64k)
//		{	"lds",	S_LDS(AVRrc)	0xA000	}, F800  Rd, k    ---- -kkk dddd kkkk (d: 16-31), (k: 0-127)
//		{	"sts",		S_STS,		0x9200	}, FE0F  k, Rr    ---- ---r rrrr ---- | kkkk kkkk kkkk kkkk (r: 0-31),(k: 0-64k)
//		{	"sts",	S_STS(AVRrc)	0xA800	}, F800  k, Rr    ---- -kkk rrrr kkkk (r: 16-31), (k: 0-127)
//
//		{	"elpm",	S_ELPM|S_INH,	0x95D8	}, FFFF  -        ---- ---- ---- ----
//		{	"elpm",	S_SNGL/Z,		0x9006	}, FE0F  Rd, Z    ---- ---d dddd ---- (d: 0-31)
//		{	"elpm",	S_SNGL/Z+,		0x9007	}, FE0F  Rd, Z+   ---- ---d dddd ---- (d: 0-31)
//		{	"lpm",	S_LPM|S_INH,	0x95C8	}, FFFF  -        ---- ---- ---- ----
//		{	"lpm",	S_SNGL/Z,		0x9004	}, FE0F  Rd, Z    ---- ---d dddd ---- (d: 0-31)
//		{	"lpm",	S_SNGL/Z+,		0x9005	}, FE0F  Rd, Z+   ---- ---d dddd ---- (d: 0-31)
//
//		{	"eicall",	S_INH,		0x9519	}, FFFF  -        ---- ---- ---- ----
//		{	"eijmp",	S_INH,		0x9419	}, FFFF  -        ---- ---- ---- ----
//		{	"ijmp",		S_INH,		0x9409	}, FFFF  -        ---- ---- ---- ----
//		{	"icall",	S_INH,		0x9509	}, FFFF  -        ---- ---- ---- ----
//		{	"ret",		S_INH,		0x9508	}, FFFF  -        ---- ---- ---- ----
//		{	"reti",		S_INH,		0x9518	}, FFFF  -        ---- ---- ---- ----
//
//		{	"sec",		S_INH,		0x9408	}, FFFF  -        ---- ---- ---- ----
//		{	"sez",		S_INH,		0x9418	}, FFFF  -        ---- ---- ---- ----
//		{	"sen",		S_INH,		0x9428	}, FFFF  -        ---- ---- ---- ----
//		{	"sev",		S_INH,		0x9438	}, FFFF  -        ---- ---- ---- ----
//		{	"ses",		S_INH,		0x9448	}, FFFF  -        ---- ---- ---- ----
//		{	"seh",		S_INH,		0x9458	}, FFFF  -        ---- ---- ---- ----
//		{	"set",		S_INH,		0x9468	}, FFFF  -        ---- ---- ---- ----
//		{	"sei",		S_INH,		0x9478	}, FFFF  -        ---- ---- ---- ----
//
//		{	"clc",		S_INH,		0x9488	}, FFFF  -        ---- ---- ---- ----
//		{	"clz",		S_INH,		0x9498	}, FFFF  -        ---- ---- ---- ----
//		{	"cln",		S_INH,		0x94A8	}, FFFF  -        ---- ---- ---- ----
//		{	"clv",		S_INH,		0x94B8	}, FFFF  -        ---- ---- ---- ----
//		{	"cls",		S_INH,		0x94C8	}, FFFF  -        ---- ---- ---- ----
//		{	"clh",		S_INH,		0x94D8	}, FFFF  -        ---- ---- ---- ----
//		{	"clt",		S_INH,		0x94E8	}, FFFF  -        ---- ---- ---- ----
//		{	"cli",		S_INH,		0x94F8	}, FFFF  -        ---- ---- ---- ----
//
//		{	"nop",		S_INH,		0x0000	}, FFFF  -        ---- ---- ---- ----
//		{	"sleep",	S_INH,		0x9588	}, FFFF  -        ---- ---- ---- ----
//		{	"break",	S_INH,		0x9598	}, FFFF  -        ---- ---- ---- ----
//		{	"spm",		S_INH,		0x95E8	}, FFFF  -        ---- ---- ---- ----		AVRe,AVRxm,AVRxt
//		{	"spm",		S_INH/Z+	0x95F8	}, FFFF  Z+       ---- ---- ---- ----		AVRxm,AVRxt
//		{	"wdr",		S_INH,		0x95A8	}, FFFF  -        ---- ---- ---- ----
//
//		{	"des",		S_DES,		0x940B	}, FF0F  K        ---- ---- KKKK ----  (K: 0-15)
//
//
// ============================================================================

#include "avrgdc.h"
#include <gdc.h>
#include <stringhelp.h>

#include <sstream>
#include <iomanip>
#include <cstdio>
#include <vector>
#include <iterator>

#include <assert.h>

// ============================================================================

#define DataDelim	"'"			// Specify ' as delimiter for data literals
#define LblcDelim	":"			// Delimiter between labels and code
#define LbleDelim	""			// Delimiter between labels and equates
#define	ComDelimS	";"			// Comment Start delimiter
#define ComDelimE	""			// Comment End delimiter
#define HexDelim	"0x"		// Hex. delimiter

#define VERSION 0x0200				// M6811 Disassembler Version number 2.00

#ifndef UNUSED
	#define UNUSED(x) ((void)(x))
#endif

// ----------------------------------------------------------------------------
//	CAVRDisassembler
// ----------------------------------------------------------------------------
CAVRDisassembler::CAVRDisassembler()
{
	//                = ((NBytes: 0; OpCode: (0, 0); Grp: 0; Control: 1; Mnemonic: '???'),


	{	"andi",		S_IBYTE,	0x7000	}, F000  Rd, K    ---- KKKK dddd KKKK (K: 0-255), (d: 16-31)
	{	"cpi",		S_IBYTE,	0x3000	}, F000  Rd, K    ---- KKKK dddd KKKK (K: 0-255), (d: 16-31)
	{	"ldi",		S_IBYTE,	0xE000	}, F000  Rd, K    ---- KKKK dddd KKKK (K: 0-255), (d: 16-31)
	{	"ori",		S_IBYTE,	0x6000	}, F000  Rd, K    ---- KKKK dddd KKKK (K: 0-255), (d: 16-31)
	{	"sbci",		S_IBYTE,	0x4000	}, F000  Rd, K    ---- KKKK dddd KKKK (K: 0-255), (d: 16-31)
	{	"sbr",		S_IBYTE,	0x6000	}, F000  Rd, K    ---- KKKK dddd KKKK (K: 0-255), (d: 16-31)
	{	"subi",		S_IBYTE,	0x5000	}, F000  Rd, K    ---- KKKK dddd KKKK (K: 0-255), (d: 16-31)

	{	"cbr",		S_CBR,		0x7000	}, F000  Rd, k    ---- KKKK dddd KKKK (K: 0-255), (d: 16-31)  xxx dup -> andi Rd,(0xFF-K)

	{	"adiw",		S_IWORD,	0x9600	}, FF00  Rd, K    ---- ---- KKdd KKKK (K: 0-63), (d: 24, 26, 28, 30)
	{	"sbiw",		S_IWORD,	0x9700	}, FF00  Rd, K    ---- ---- KKdd KKKK (K: 0-63), (d: 24, 26, 28, 30)

	{	"asr",		S_SNGL,		0x9405	}, FE0F  Rd       ---- ---d dddd ---- (d: 0-31)
	{	"com",		S_SNGL,		0x9400	}, FE0F  Rd       ---- ---d dddd ---- (d: 0-31)
	{	"dec",		S_SNGL,		0x940A	}, FE0F  Rd       ---- ---d dddd ---- (d: 0-31)
	{	"inc",		S_SNGL,		0x9403	}, FE0F  Rd       ---- ---d dddd ---- (d: 0-31)
	{	"lsr",		S_SNGL,		0x9406	}, FE0F  Rd       ---- ---d dddd ---- (d: 0-31)
	{	"neg",		S_SNGL,		0x9401	}, FE0F  Rd       ---- ---d dddd ---- (d: 0-31)
	{	"pop",		S_SNGL,		0x900F	}, FE0F  Rd       ---- ---d dddd ---- (d: 0-31)
	{	"push",		S_SNGL,		0x920F	}, FE0F  Rr       ---- ---r rrrr ---- (r: 0-31)
	{	"ror",		S_SNGL,		0x9407	}, FE0F  Rd       ---- ---d dddd ---- (d: 0-31)
	{	"swap",		S_SNGL,		0x9402	}, FE0F  Rd       ---- ---d dddd ---- (d: 0-31)
	{	"xch",		S_SNGL,		0x9204	}, FE0F  Rd       ---- ---d dddd ---- (d: 0-31)

	{	"lac",		S_LA??,		0x9206	}, FE0F  Z, Rr    ---- ---r rrrr ---- (r: 0-31) (implied Z for destination)
	{	"las",		S_LA??,		0x9205	}, FE0F  Z, Rr    ---- ---r rrrr ---- (r: 0-31) (implied Z for destination)
	{	"lat",		S_LA??,		0x9207	}, FE0F  Z, Rr    ---- ---r rrrr ---- (r: 0-31) (implied Z for destination)

	{	"clr",		S_SAME,		0x2400	}, FC00  Rd       ---- --dd dddd dddd (d: 0-31)x2 xxx dup eor
	{	"lsl",		S_SAME,		0x0C00	}, FC00  Rd       ---- --dd dddd dddd (d: 0-31)x2 xxx dup add
	{	"rol",		S_SAME,		0x1C00	}, FC00  Rd       ---- --dd dddd dddd (d: 0-31)x2 xxx dup adc
	{	"tst",		S_SAME,		0x2000	}, FC00  Rd       ---- --dd dddd dddd (d: 0-31)x2 xxx dup and

	{	"adc",		S_DUBL,		0x1C00	}, FC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
	{	"add",		S_DUBL,		0x0C00	}, FC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
	{	"and",		S_DUBL,		0x2000	}, FC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
	{	"cp",		S_DUBL,		0x1400	}, FC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
	{	"cpc",		S_DUBL,		0x0400	}, FC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
	{	"cpse",		S_DUBL,		0x1000	}, FC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31), Difficult control requiring lookahead
	{	"eor",		S_DUBL,		0x2400	}, FC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
	{	"mov",		S_DUBL,		0x2C00	}, FC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
	{	"or",		S_DUBL,		0x2800	}, FC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
	{	"sbc",		S_DUBL,		0x0800	}, FC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
	{	"sub",		S_DUBL,		0x1800	}, FC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)

	{	"movw",		S_MOVW,		0x0100	}, FF00  Rd, Rr   ---- ---- dddd rrrr (d: 0,2,4,...,30), (r: 0,2,4,...,30)

	{	"mul",		S_MUL,		0x9C00	}, FC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)  ?? S_DUBL
	{	"muls",		S_MULS,		0x0200	}, FF00  Rd, Rr   ---- ---- dddd rrrr (d: 16-31), (r: 16-31)
	{	"mulsu",	S_FMUL,		0x0300	}, FF88  Rd, Rr   ---- ---- -ddd -rrr (d: 16-23), (r: 16-23)
	{	"fmul",		S_FMUL,		0x0308	}, FF88  Rd, Rr   ---- ---- -ddd -rrr (d: 16-23), (r: 16-23)
	{	"fmuls",	S_FMUL,		0x0380	}, FF88  Rd, Rr   ---- ---- -ddd -rrr (d: 16-23), (r: 16-23)
	{	"fmulsu",	S_FMUL,		0x0388	}, FF88  Rd, Rr   ---- ---- -ddd -rrr (d: 16-23), (r: 16-23)

	{	"ser",		S_SER,		0xEF0F	}, FF0F  Rd       ---- ---- dddd ---- (d: 16-31)  <<< SPC: ldi Rd,0xFF

	{	"bclr",		S_SREG,		0x9488	}, FF8F  s        ---- ---- -sss ---- (s: 0-7)  xxx dup (see: S_INH)
	{	"bset",		S_SREG,		0x9408	}, FF8F  s        ---- ---- -sss ---- (s: 0-7)  xxx dup (see: S_INH)

	{	"bld",		S_TFLG,		0xF800	}, FE08  Rd, b    ---- ---d dddd -bbb (d: 0-31), (b: 0-7)
	{	"bst",		S_TFLG,		0xFA00	}, FE08  Rd, b    ---- ---d dddd -bbb (d: 0-31), (b: 0-7)

	{	"brcc",		S_BRA,		0xF400	}, FC07  k        ---- --kk kkkk k000 (k: -64 - 63)
	{	"brcs",		S_BRA,		0xF000	}, FC07  k        ---- --kk kkkk k000 (k: -64 - 63)
	{	"breq",		S_BRA,		0xF001	}, FC07  k        ---- --kk kkkk k001 (k: -64 - 63)
	{	"brge",		S_BRA,		0xF404	}, FC07  k        ---- --kk kkkk k100 (k: -64 - 63)
	{	"brhc",		S_BRA,		0xF405	}, FC07  k        ---- --kk kkkk k101 (k: -64 - 63)
	{	"brhs",		S_BRA,		0xF005	}, FC07  k        ---- --kk kkkk k101 (k: -64 - 63)
	{	"brid",		S_BRA,		0xF407	}, FC07  k        ---- --kk kkkk k111 (k: -64 - 63)
	{	"brie",		S_BRA,		0xF007	}, FC07  k        ---- --kk kkkk k111 (k: -64 - 63)
	{	"brlo",		S_BRA,		0xF000	}, FC07  k        ---- --kk kkkk k000 (k: -64 - 63)  xxx dup brcs
	{	"brlt",		S_BRA,		0xF004	}, FC07  k        ---- --kk kkkk k100 (k: -64 - 63)
	{	"brmi",		S_BRA,		0xF002	}, FC07  k        ---- --kk kkkk k010 (k: -64 - 63)
	{	"brne",		S_BRA,		0xF401	}, FC07  k        ---- --kk kkkk k001 (k: -64 - 63)
	{	"brpl",		S_BRA,		0xF402	}, FC07  k        ---- --kk kkkk k010 (k: -64 - 63)
	{	"brsh",		S_BRA,		0xF400  }, FC07  k        ---- --kk kkkk k000 (k: -64 - 63)  xxx dup brcc
	{	"brtc",		S_BRA,		0xF406	}, FC07  k        ---- --kk kkkk k110 (k: -64 - 63)
	{	"brts",		S_BRA,		0xF006	}, FC07  k        ---- --kk kkkk k110 (k: -64 - 63)
	{	"brvc",		S_BRA,		0xF403	}, FC07  k        ---- --kk kkkk k011 (k: -64 - 63)
	{	"brvs",		S_BRA,		0xF003	}, FC07  k        ---- --kk kkkk k011 (k: -64 - 63)

	{	"brbc",		S_SBRA,		0xF400	}, FC00  s, k     ---- --kk kkkk ksss (k: -64 - 63), (s: 0-7)
	{	"brbs",		S_SBRA,		0xF000	}, FC00  s, k     ---- --kk kkkk ksss (k: -64 - 63), (s: 0-7)

	{	"sbrc",		S_SKIP,		0xFC00	}, FE08  Rr, b    ---- ---r rrrr -bbb (r: 0-31), (b: 0-7)  ?? S_TFLG, Difficult control requiring lookahead
	{	"sbrs",		S_SKIP,		0xFE00	}, FE08  Rr, b    ---- ---r rrrr -bbb (r: 0-31), (b: 0-7)  ?? S_TFLG, Difficult control requiring lookahead

	{	"call",		S_JMP,		0x940E	}, FE0E  k        ---- ---k kkkk ---k | kkkk kkkk kkkk kkkk (k: 0-64k, 0-4M), 16/22-bit absolute
	{	"jmp",		S_JMP,		0x940C	}, FE0E  k        ---- ---k kkkk ---k | kkkk kkkk kkkk kkkk (k: 0-64k, 0-4M), 16/22-bit absolute

	{	"rcall",	S_RJMP,		0xD000	}, F000  k        ---- kkkk kkkk kkkk (k: -2k - 2k), 12-bit relative
	{	"rjmp",		S_RJMP,		0xC000	}, F000  k        ---- kkkk kkkk kkkk (k: -2k - 2k), 12-bit relative

	{	"cbi",		S_IOR,		0x9800	}, FF00  A, b     ---- ---- AAAA Abbb (A: 0-31), (b: 0-7)
	{	"sbi",		S_IOR,		0x9A00	}, FF00  A, b     ---- ---- AAAA Abbb (A: 0-31), (b: 0-7)
	{	"sbic",		S_IOR,		0x9900	}, FF00  A, b     ---- ---- AAAA Abbb (A: 0-31), (b: 0-7), Difficult control requiring lookahead
	{	"sbis",		S_IOR,		0x9B00	}, FF00  A, b     ---- ---- AAAA Abbb (A: 0-31), (b: 0-7), Difficult control requiring lookahead

	{	"in",		S_IN,		0xB000	}, F800  Rd, A    ---- -AAd dddd AAAA (A: 0-63), (d: 0-31)
	{	"out",		S_OUT,		0xB800	}, F800  A, Rr    ---- -AAr rrrr AAAA (A: 0-63), (r: 0-31)

	{	"ld",		S_LD,		0x8000	}, ????? (Z only?)
	{	"ld",		S_LD/X		0x900C	}, FE0F  Rd, X    ---- ---d dddd ---- (d: 0-31)
	{	"ld",		S_LD/X+		0x900D	}, FE0F  Rd, X+   ---- ---d dddd ---- (d: 0-31)
	{	"ld",		S_LD/-X		0x900E	}, FE0F  Rd, -X   ---- ---d dddd ---- (d: 0-31)
	{	"ld",		S_LD/Y		0x8008	}, FE0F  Rd, Y    ---- ---d dddd ---- (d: 0-31)  <<< SPC: ldd Rd,Y+0
	{	"ld",		S_LD/Y+		0x9009	}, FE0F  Rd, Y+   ---- ---d dddd ---- (d: 0-31)
	{	"ld",		S_LD/-Y		0x900A	}, FE0F  Rd, -Y   ---- ---d dddd ---- (d: 0-31)
	{	"ld",		S_LD/Z		0x8000	}, FE0F  Rd, Z    ---- ---d dddd ---- (d: 0-31)  <<< SPC: ldd Rd,Z+0
	{	"ld",		S_LD/Z+		0x9001	}, FE0F  Rd, Z+   ---- ---d dddd ---- (d: 0-31)
	{	"ld",		S_LD/-Z		0x9002	}, FE0F  Rd, -Z   ---- ---d dddd ---- (d: 0-31)
	{	"st",		S_ST,		0x8200	}, ????? (Z only?)
	{	"st",		S_ST/X		0x920C	}, FE0F  X, Rr    ---- ---r rrrr ---- (r: 0-31)
	{	"st",		S_ST/X+		0x920D	}, FE0F  X+, Rr   ---- ---r rrrr ---- (r: 0-31)
	{	"st",		S_ST/-X		0x920E	}, FE0F  -X, Rr   ---- ---r rrrr ---- (r: 0-31)
	{	"st",		S_ST/Y		0x8208	}, FE0F  Y, Rr    ---- ---r rrrr ---- (r: 0-31)  <<< SPC: std Rd,Y+0
	{	"st",		S_ST/Y+		0x9209	}, FE0F  Y+, Rr   ---- ---r rrrr ---- (r: 0-31)
	{	"st",		S_ST/-Y		0x920A	}, FE0F  -Y, Rr   ---- ---r rrrr ---- (r: 0-31)
	{	"st",		S_ST/Z		0x8200	}, FE0F  Z, Rr    ---- ---r rrrr ---- (r: 0-31)  <<< SPC: std Rd,Z+0
	{	"st",		S_ST/Z+		0x9201	}, FE0F  Z+, Rr   ---- ---r rrrr ---- (r: 0-31)
	{	"st",		S_ST/-Z		0x9202	}, FE0F  -Z, Rr   ---- ---r rrrr ---- (r: 0-31)

	{	"ldd",		S_ILD,		0x8000	}, ????? (Z only?)
	{	"ldd",		S_ILD/Y+q	0x8008	}, D208  Rd, Y+q  --q- qq-d dddd -qqq (d: 0-31), (q: 0-63)
	{	"ldd",		S_ILD/Z+q	0x8000	}, D208  Rd, Z+q  --q- qq-d dddd -qqq (d: 0-31), (q: 0-63)
	{	"std",		S_IST,		0x8200	}, ????? (Z only?)
	{	"std",		S_IST/Y+q	0x8208	}, D208  Y+q, Rr  --q- qq-r rrrr -qqq (r: 0-31), (q: 0-63)
	{	"std",		S_IST/Z+q	0x8200	}, D208  Z+q, Rr  --q- qq-r rrrr -qqq (r: 0-31), (q: 0-63)

	{	"lds",		S_LDS,		0x9000	}, FE0F  Rd, k    ---- ---d dddd ---- | kkkk kkkk kkkk kkkk (d: 0-31),(k: 0-64k)
	{	"lds",	S_LDS(AVRrc)	0xA000	}, F800  Rd, k    ---- -kkk dddd kkkk (d: 16-31), (k: 0-127)
	{	"sts",		S_STS,		0x9200	}, FE0F  k, Rr    ---- ---r rrrr ---- | kkkk kkkk kkkk kkkk (r: 0-31),(k: 0-64k)
	{	"sts",	S_STS(AVRrc)	0xA800	}, F800  k, Rr    ---- -kkk rrrr kkkk (r: 16-31), (k: 0-127)

	{	"elpm",	S_ELPM|S_INH,	0x95D8	}, FFFF  -        ---- ---- ---- ----
	{	"elpm",	S_SNGL/Z,		0x9006	}, FE0F  Rd, Z    ---- ---d dddd ---- (d: 0-31)
	{	"elpm",	S_SNGL/Z+,		0x9007	}, FE0F  Rd, Z+   ---- ---d dddd ---- (d: 0-31)
	{	"lpm",	S_LPM|S_INH,	0x95C8	}, FFFF  -        ---- ---- ---- ----
	{	"lpm",	S_SNGL/Z,		0x9004	}, FE0F  Rd, Z    ---- ---d dddd ---- (d: 0-31)
	{	"lpm",	S_SNGL/Z+,		0x9005	}, FE0F  Rd, Z+   ---- ---d dddd ---- (d: 0-31)

	{	"eicall",	S_INH,		0x9519	}, FFFF  -        ---- ---- ---- ----
	{	"eijmp",	S_INH,		0x9419	}, FFFF  -        ---- ---- ---- ----
	{	"ijmp",		S_INH,		0x9409	}, FFFF  -        ---- ---- ---- ----
	{	"icall",	S_INH,		0x9509	}, FFFF  -        ---- ---- ---- ----
	{	"ret",		S_INH,		0x9508	}, FFFF  -        ---- ---- ---- ----
	{	"reti",		S_INH,		0x9518	}, FFFF  -        ---- ---- ---- ----

	{	"sec",		S_INH,		0x9408	}, FFFF  -        ---- ---- ---- ----
	{	"sez",		S_INH,		0x9418	}, FFFF  -        ---- ---- ---- ----
	{	"sen",		S_INH,		0x9428	}, FFFF  -        ---- ---- ---- ----
	{	"sev",		S_INH,		0x9438	}, FFFF  -        ---- ---- ---- ----
	{	"ses",		S_INH,		0x9448	}, FFFF  -        ---- ---- ---- ----
	{	"seh",		S_INH,		0x9458	}, FFFF  -        ---- ---- ---- ----
	{	"set",		S_INH,		0x9468	}, FFFF  -        ---- ---- ---- ----
	{	"sei",		S_INH,		0x9478	}, FFFF  -        ---- ---- ---- ----

	{	"clc",		S_INH,		0x9488	}, FFFF  -        ---- ---- ---- ----
	{	"clz",		S_INH,		0x9498	}, FFFF  -        ---- ---- ---- ----
	{	"cln",		S_INH,		0x94A8	}, FFFF  -        ---- ---- ---- ----
	{	"clv",		S_INH,		0x94B8	}, FFFF  -        ---- ---- ---- ----
	{	"cls",		S_INH,		0x94C8	}, FFFF  -        ---- ---- ---- ----
	{	"clh",		S_INH,		0x94D8	}, FFFF  -        ---- ---- ---- ----
	{	"clt",		S_INH,		0x94E8	}, FFFF  -        ---- ---- ---- ----
	{	"cli",		S_INH,		0x94F8	}, FFFF  -        ---- ---- ---- ----

	{	"nop",		S_INH,		0x0000	}, FFFF  -        ---- ---- ---- ----
	{	"sleep",	S_INH,		0x9588	}, FFFF  -        ---- ---- ---- ----
	{	"break",	S_INH,		0x9598	}, FFFF  -        ---- ---- ---- ----
	{	"spm",		S_INH,		0x95E8	}, FFFF  -        ---- ---- ---- ----		AVRe,AVRxm,AVRxt
	{	"spm",		S_INH/Z+	0x95F8	}, FFFF  Z+       ---- ---- ---- ----		AVRxm,AVRxt
	{	"wdr",		S_INH,		0x95A8	}, FFFF  -        ---- ---- ---- ----

	{	"des",		S_DES,		0x940B	}, FF0F  K        ---- ---- KKKK ----  (K: 0-15)

}

