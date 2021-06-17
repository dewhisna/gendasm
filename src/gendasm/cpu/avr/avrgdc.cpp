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
//	Groups: (Grp) :
//		S_IBYTE,		// 0xF000 : Rd, K    ---- KKKK dddd KKKK (K: 0-255), (d: 16-31)
//		S_IWORD,		// 0xFF00 : Rd, K    ---- ---- KKdd KKKK (K: 0-63), (d: 24, 26, 28, 30)
//		S_SNGL,			// 0xFE0F : Rd       ---- ---d dddd ---- (d: 0-31)
//		S_SAME,			// 0xFC00 : Rd       ---- --dd dddd dddd (d: 0-31)x2 (these are all ambiguous with corresponding S_DUBL, so must come AHEAD of S_DUBL and use a Match Function)
//		S_DUBL,			// 0xFC00 : Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
//		S_MOVW,			// 0xFF00 : Rd, Rr   ---- ---- dddd rrrr (d: 0,2,4,...,30), (r: 0,2,4,...,30)
//		S_MULS,			// 0xFF00 : Rd, Rr   ---- ---- dddd rrrr (d: 16-31), (r: 16-31)
//		S_FMUL,			// 0xFF88 : Rd, Rr   ---- ---- -ddd -rrr (d: 16-23), (r: 16-23)
//		S_SER,			// 0xFF0F : Rd       ---- ---- dddd ---- (d: 16-31)
//		S_TFLG,			// 0xFE08 : Rd, b    ---- ---d dddd -bbb (d: 0-31), (b: 0-7)
//		S_BRA,			// 0xFC07 : k        ---- --kk kkkk k--- (k: -64 - 63)
//		S_JMP,			// 0xFE0E : k        ---- ---k kkkk ---k | kkkk kkkk kkkk kkkk (k: 0-64k, 0-4M), 16/22-bit absolute, PC+2
//		S_RJMP,			// 0xF000 : k        ---- kkkk kkkk kkkk (k: -2k - 2k), 12-bit relative
//		S_IOR,			// 0xFF00 : A, b     ---- ---- AAAA Abbb (A: 0-31), (b: 0-7)
//		S_IN,			// 0xF800 : Rd, A    ---- -AAd dddd AAAA (A: 0-63), (d: 0-31)
//		S_OUT,			// 0xF800 : A, Rr    ---- -AAr rrrr AAAA (A: 0-63), (r: 0-31)
//		S_SNGL_X,		// 0xFE0F : Rd, X    ---- ---d dddd ---- (d: 0-31) (implied X for source)
//		S_SNGL_Xp,		// 0xFE0F : Rd, X+   ---- ---d dddd ---- (d: 0-31) (implied X for source w/Post-Increment)
//		S_SNGL_nX,		// 0xFE0F : Rd, -X   ---- ---d dddd ---- (d: 0-31) (implied X for source w/Pre-Decrement)
//		S_SNGL_Y,		// 0xFE0F : Rd, Y    ---- ---d dddd ---- (d: 0-31) (implied Y for source)
//		S_SNGL_Yp,		// 0xFE0F : Rd, Y+   ---- ---d dddd ---- (d: 0-31) (implied Y for source w/Post-Increment)
//		S_SNGL_nY,		// 0xFE0F : Rd, -Y   ---- ---d dddd ---- (d: 0-31) (implied Y for source w/Pre-Decrement)
//		S_SNGL_Z,		// 0xFE0F : Rd, Z    ---- ---d dddd ---- (d: 0-31) (implied Z for source)
//		S_SNGL_Zp,		// 0xFE0F : Rd, Z+   ---- ---d dddd ---- (d: 0-31) (implied Z for source w/Post-Increment)
//		S_SNGL_nZ,		// 0xFE0F : Rd, -Z   ---- ---d dddd ---- (d: 0-31) (implied Z for source w/Pre-Decrement)
//		S_X_SNGL,		// 0xFE0F : X, Rr    ---- ---r rrrr ---- (r: 0-31) (implied X for destination)
//		S_Xp_SNGL,		// 0xFE0F : X+, Rr   ---- ---r rrrr ---- (r: 0-31) (implied X for destination w/Post-Increment)
//		S_nX_SNGL,		// 0xFE0F : -X, Rr   ---- ---r rrrr ---- (r: 0-31) (implied X for destination w/Pre-Decrement)
//		S_Y_SNGL,		// 0xFE0F : Y, Rr    ---- ---r rrrr ---- (r: 0-31) (implied Y for destination)
//		S_Yp_SNGL,		// 0xFE0F : Y+, Rr   ---- ---r rrrr ---- (r: 0-31) (implied Y for destination w/Post-Increment)
//		S_nY_SNGL,		// 0xFE0F : -Y, Rr   ---- ---r rrrr ---- (r: 0-31) (implied Y for destination w/Pre-Decrement)
//		S_Z_SNGL,		// 0xFE0F : Z, Rr    ---- ---r rrrr ---- (r: 0-31) (implied Z for destination)
//		S_Zp_SNGL,		// 0xFE0F : Z+, Rr   ---- ---r rrrr ---- (r: 0-31) (implied Z for destination w/Post-Increment)
//		S_nZ_SNGL,		// 0xFE0F : -Z, Rr   ---- ---r rrrr ---- (r: 0-31) (implied Z for destination w/Pre-Decrement)
//		S_SNGL_Yq,		// 0xD208 : Rd, Y+q  --q- qq-d dddd -qqq (d: 0-31), (q: 0-63)
//		S_SNGL_Zq,		// 0xD208 : Rd, Z+q  --q- qq-d dddd -qqq (d: 0-31), (q: 0-63)
//		S_Yq_SNGL,		// 0xD208 : Y+q, Rr  --q- qq-r rrrr -qqq (r: 0-31), (q: 0-63)
//		S_Zq_SNGL,		// 0xD208 : Z+q, Rr  --q- qq-r rrrr -qqq (r: 0-31), (q: 0-63)
//		S_LDS,			// 0xFE0F : Rd, k    ---- ---d dddd ---- | kkkk kkkk kkkk kkkk (d: 0-31),(k: 0-64k), PC+2
//		S_LDSrc,		// 0xF800 : Rd, k    ---- -kkk dddd kkkk (d: 16-31), (k: 0-127)
//		S_STS,			// 0xFE0F : k, Rr    ---- ---r rrrr ---- | kkkk kkkk kkkk kkkk (r: 0-31),(k: 0-64k), PC+2
//		S_STSrc,		// 0xF800 : k, Rr    ---- -kkk rrrr kkkk (r: 16-31), (k: 0-127)
//		S_DES,			// 0xFF0F : K        ---- ---- KKKK ----  (K: 0-15)
//		S_INH_Zp,		// 0xFFFF : Z+       ---- ---- ---- ----  (AVRxm,AVRxt)
//		S_INH,			// 0xFFFF : -        ---- ---- ---- ----
//
//			Addressing Types:
//			-----------------
//			- opcode only, nothing follows [S_INH]
//			- Register Direct, Single Register: Destination Register (Rd) [S_SNGL]
//			- Register Direct, Two Registers: Destination Register (Rd), Source Register (Rr) [S_DUBL] [S_SAME]
//			- I/O Direct Addressing, Single Register: Source/Destination Register (Rd/Rr), I/O Address (A) [IO Range 0-63]  [S_IN] [S_OUT]
//			- Direct Addressing, Single Register: Source/Destination Register (Rd/Rr), 16-Bit Data Address (A), Note: LDS instruction uses RAMPD register for >64KB access [S_LDS] [S_STS]
//			- Register Indirect X, Single Register: Source/Destination Register (Rd/Rr) [S_SNGL_X] [S_X_SNGL]
//			- Register Indirect Y, Single Register: Source/Destination Register (Rd/Rr) [S_SNGL_Y] [S_Y_SNGL]
//			- Register Indirect Z, Single Register: Source/Destination Register (Rd/Rr) [S_SNGL_Z] [S_Z_SNGL]
//			- Register Indirect X Pre-Decrement, Single Register: Source/Destination Register (Rd/Rr) [S_SNGL_nX] [S_nX_SNGL]
//			- Register Indirect Y Pre-Decrement, Single Register: Source/Destination Register (Rd/Rr) [S_SNGL_nY] [S_nY_SNGL]
//			- Register Indirect Z Pre-Decrement, Single Register: Source/Destination Register (Rd/Rr) [S_SNGL_nZ] [S_nZ_SNGL]
//			- Register Indirect X Post-Increment, Single Register: Source/Destination Register (Rd/Rr) [S_SNGL_Xp] [S_Xp_SNGL]
//			- Register Indirect Y Post-Increment, Single Register: Source/Destination Register (Rd/Rr) [S_SNGL_Yp] [S_Yp_SNGL]
//			- Register Indirect Z Post-Increment, Single Register: Source/Destination Register (Rd/Rr) [S_SNGL_Zp] [S_Zp_SNGL]
//			- Register Indirect Y w/Displacement, Single Register: Source/Destination Register (Rd/Rr) [S_SNGL_Yq] [S_Yq_SNGL]
//			- Register Indirect Z w/Displacement, Single Register: Source/Destination Register (Rd/Rr) [S_SNGL_Zq] [S_Zq_SNGL]
//			- Program Memory Constant Addressing Z, Single Register: Destination Register (Rd), for LPM, ELPM [S_SNGL_Z]
//					Note: While this is technically for SPM too, SPM always has r0/r1 as Source Registers
//					there is no operand for the "spm" instruction.  Therefore, Group(0) is used for SPM for both Src/Dst
//					Note: LPM/ELPM have an implied form with no arguments that implies r0 which will use Group(0) [S_INH]
//			- Program Memory Constant Addressing Z Post-Increment, Single Register: Destination Register (Rd), for LPM, ELPM, SPM [S_SNGL_Zp]
//					Note: SPM always has r0/r1 as Source Registers
//			- Register Direct with Bit index, Single Register: Destination Register (Rd), w/Bit index (BLD, BST) [S_TFLG]
//			- 16-bit/22-bit absolute address (JMP, CALL) [S_JMP]
//			- 12-bit relative address (RJMP, RCALL) [S_RJMP]
//			- 6-bit relative address (BRBC, BRBS) [S_BRA]
//					Note: Actual BRBC and BRBS opcodes aren't used as they are ambiguous with the
//					individualized ones BRCC, BRCS, BREQ, BRNE, ..., BRxx
//
//
//	Control: (Algorithm control)
//			CTL_None,			//	Disassemble as code
//			CTL_DataLabel,		//	Disassemble as code, with data addr label
//			CTL_IOLabel,		//	Disassemble as code, with I/O addr label
//			CTL_UndetBra,		//	Disassemble as undeterminable branch address (Comment code)
//			CTL_DetBra,			//	Disassemble as determinable branch, add branch addr and label
//			CTL_Skip,			//	Disassemble as Skip Branch.  Next two instructions flagged as code even if next one would be terminal
//			CTL_SkipIOLabel,	//	Disassemble as Skip Branch, but with I/O addr label
//
//			OCTL_TwoWordCode = 0x10000ul,		// Flag for Two-Word Instruction
//			OCTL_ArchExtAddr = 0x20000ul,		// Flag for Architecture Specific extended addressing (such as RAMPD for LDS/STS instructions for >= 64K)
//
//
//	This version setup for compatibility with the avra assembler
//		(https://github.com/Ro5bert/avra)
//

// ============================================================================
//
// Opcode List:
//		{	"andi",		S_IBYTE,	0x7000	}, 0xF000  Rd, K    ---- KKKK dddd KKKK (K: 0-255), (d: 16-31)
//		{	"cpi",		S_IBYTE,	0x3000	}, 0xF000  Rd, K    ---- KKKK dddd KKKK (K: 0-255), (d: 16-31)
//		{	"ldi",		S_IBYTE,	0xE000	}, 0xF000  Rd, K    ---- KKKK dddd KKKK (K: 0-255), (d: 16-31)
//		{	"ori",		S_IBYTE,	0x6000	}, 0xF000  Rd, K    ---- KKKK dddd KKKK (K: 0-255), (d: 16-31)
//		{	"sbci",		S_IBYTE,	0x4000	}, 0xF000  Rd, K    ---- KKKK dddd KKKK (K: 0-255), (d: 16-31)
//		{	"sbr",		S_IBYTE,	0x6000	}, 0xF000  Rd, K    ---- KKKK dddd KKKK (K: 0-255), (d: 16-31)
//		{	"subi",		S_IBYTE,	0x5000	}, 0xF000  Rd, K    ---- KKKK dddd KKKK (K: 0-255), (d: 16-31)
//
//		{	"cbr",		S_CBR,		0x7000	}, 0xF000  Rd, k    ---- KKKK dddd KKKK (K: 0-255), (d: 16-31)  xxx dup -> andi Rd,(0xFF-K)
//
//		{	"adiw",		S_IWORD,	0x9600	}, 0xFF00  Rd, K    ---- ---- KKdd KKKK (K: 0-63), (d: 24, 26, 28, 30)
//		{	"sbiw",		S_IWORD,	0x9700	}, 0xFF00  Rd, K    ---- ---- KKdd KKKK (K: 0-63), (d: 24, 26, 28, 30)
//
//		{	"asr",		S_SNGL,		0x9405	}, 0xFE0F  Rd       ---- ---d dddd ---- (d: 0-31)
//		{	"com",		S_SNGL,		0x9400	}, 0xFE0F  Rd       ---- ---d dddd ---- (d: 0-31)
//		{	"dec",		S_SNGL,		0x940A	}, 0xFE0F  Rd       ---- ---d dddd ---- (d: 0-31)
//		{	"inc",		S_SNGL,		0x9403	}, 0xFE0F  Rd       ---- ---d dddd ---- (d: 0-31)
//		{	"lsr",		S_SNGL,		0x9406	}, 0xFE0F  Rd       ---- ---d dddd ---- (d: 0-31)
//		{	"neg",		S_SNGL,		0x9401	}, 0xFE0F  Rd       ---- ---d dddd ---- (d: 0-31)
//		{	"pop",		S_SNGL,		0x900F	}, 0xFE0F  Rd       ---- ---d dddd ---- (d: 0-31)
//		{	"push",		S_SNGL,		0x920F	}, 0xFE0F  Rr       ---- ---r rrrr ---- (r: 0-31)
//		{	"ror",		S_SNGL,		0x9407	}, 0xFE0F  Rd       ---- ---d dddd ---- (d: 0-31)
//		{	"swap",		S_SNGL,		0x9402	}, 0xFE0F  Rd       ---- ---d dddd ---- (d: 0-31)
//		{	"xch",		S_SNGL,		0x9204	}, 0xFE0F  Rd       ---- ---d dddd ---- (d: 0-31)
//
//		{	"lac",		S_Z_SNGL,	0x9206	}, 0xFE0F  Z, Rr    ---- ---r rrrr ---- (r: 0-31) (implied Z for destination)
//		{	"las",		S_Z_SNGL,	0x9205	}, 0xFE0F  Z, Rr    ---- ---r rrrr ---- (r: 0-31) (implied Z for destination)
//		{	"lat",		S_Z_SNGL,	0x9207	}, 0xFE0F  Z, Rr    ---- ---r rrrr ---- (r: 0-31) (implied Z for destination)
//
//		{	"clr",		S_SAME,		0x2400	}, 0xFC00  Rd       ---- --dd dddd dddd (d: 0-31)x2 <<< SPC:  eor Rd,Rd
//		{	"lsl",		S_SAME,		0x0C00	}, 0xFC00  Rd       ---- --dd dddd dddd (d: 0-31)x2 <<< SPC:  add Rd,Rd
//		{	"rol",		S_SAME,		0x1C00	}, 0xFC00  Rd       ---- --dd dddd dddd (d: 0-31)x2 <<< SPC:  adc Rd,Rd
//		{	"tst",		S_SAME,		0x2000	}, 0xFC00  Rd       ---- --dd dddd dddd (d: 0-31)x2 <<< SPC:  and Rd,Rd
//
//		{	"adc",		S_DUBL,		0x1C00	}, 0xFC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
//		{	"add",		S_DUBL,		0x0C00	}, 0xFC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
//		{	"and",		S_DUBL,		0x2000	}, 0xFC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
//		{	"cp",		S_DUBL,		0x1400	}, 0xFC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
//		{	"cpc",		S_DUBL,		0x0400	}, 0xFC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
//		{	"cpse",		S_DUBL,		0x1000	}, 0xFC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31), Difficult control requiring lookahead
//		{	"eor",		S_DUBL,		0x2400	}, 0xFC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
//		{	"mov",		S_DUBL,		0x2C00	}, 0xFC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
//		{	"or",		S_DUBL,		0x2800	}, 0xFC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
//		{	"sbc",		S_DUBL,		0x0800	}, 0xFC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
//		{	"sub",		S_DUBL,		0x1800	}, 0xFC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
//
//		{	"movw",		S_MOVW,		0x0100	}, 0xFF00  Rd, Rr   ---- ---- dddd rrrr (d: 0,2,4,...,30), (r: 0,2,4,...,30)
//
//		{	"mul",		S_MUL,		0x9C00	}, 0xFC00  Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)  ?? S_DUBL
//		{	"muls",		S_MULS,		0x0200	}, 0xFF00  Rd, Rr   ---- ---- dddd rrrr (d: 16-31), (r: 16-31)
//		{	"mulsu",	S_FMUL,		0x0300	}, 0xFF88  Rd, Rr   ---- ---- -ddd -rrr (d: 16-23), (r: 16-23)
//		{	"fmul",		S_FMUL,		0x0308	}, 0xFF88  Rd, Rr   ---- ---- -ddd -rrr (d: 16-23), (r: 16-23)
//		{	"fmuls",	S_FMUL,		0x0380	}, 0xFF88  Rd, Rr   ---- ---- -ddd -rrr (d: 16-23), (r: 16-23)
//		{	"fmulsu",	S_FMUL,		0x0388	}, 0xFF88  Rd, Rr   ---- ---- -ddd -rrr (d: 16-23), (r: 16-23)
//
//		{	"ser",		S_SER,		0xEF0F	}, 0xFF0F  Rd       ---- ---- dddd ---- (d: 16-31)  <<< SPC: ldi Rd,0xFF
//
//		{	"bclr",		S_SREG,		0x9488	}, 0xFF8F  s        ---- ---- -sss ---- (s: 0-7)  xxx dup (see: S_INH)
//		{	"bset",		S_SREG,		0x9408	}, 0xFF8F  s        ---- ---- -sss ---- (s: 0-7)  xxx dup (see: S_INH)
//
//		{	"bld",		S_TFLG,		0xF800	}, 0xFE08  Rd, b    ---- ---d dddd -bbb (d: 0-31), (b: 0-7)
//		{	"bst",		S_TFLG,		0xFA00	}, 0xFE08  Rd, b    ---- ---d dddd -bbb (d: 0-31), (b: 0-7)
//
//		{	"brcc",		S_BRA,		0xF400	}, 0xFC07  k        ---- --kk kkkk k--- (k: -64 - 63)
//		{	"brcs",		S_BRA,		0xF000	}, 0xFC07  k        ---- --kk kkkk k--- (k: -64 - 63)
//		{	"breq",		S_BRA,		0xF001	}, 0xFC07  k        ---- --kk kkkk k--- (k: -64 - 63)
//		{	"brge",		S_BRA,		0xF404	}, 0xFC07  k        ---- --kk kkkk k--- (k: -64 - 63)
//		{	"brhc",		S_BRA,		0xF405	}, 0xFC07  k        ---- --kk kkkk k--- (k: -64 - 63)
//		{	"brhs",		S_BRA,		0xF005	}, 0xFC07  k        ---- --kk kkkk k--- (k: -64 - 63)
//		{	"brid",		S_BRA,		0xF407	}, 0xFC07  k        ---- --kk kkkk k--- (k: -64 - 63)
//		{	"brie",		S_BRA,		0xF007	}, 0xFC07  k        ---- --kk kkkk k--- (k: -64 - 63)
//		{	"brlo",		S_BRA,		0xF000	}, 0xFC07  k        ---- --kk kkkk k--- (k: -64 - 63)  xxx dup brcs
//		{	"brlt",		S_BRA,		0xF004	}, 0xFC07  k        ---- --kk kkkk k--- (k: -64 - 63)
//		{	"brmi",		S_BRA,		0xF002	}, 0xFC07  k        ---- --kk kkkk k--- (k: -64 - 63)
//		{	"brne",		S_BRA,		0xF401	}, 0xFC07  k        ---- --kk kkkk k--- (k: -64 - 63)
//		{	"brpl",		S_BRA,		0xF402	}, 0xFC07  k        ---- --kk kkkk k--- (k: -64 - 63)
//		{	"brsh",		S_BRA,		0xF400  }, 0xFC07  k        ---- --kk kkkk k--- (k: -64 - 63)  xxx dup brcc
//		{	"brtc",		S_BRA,		0xF406	}, 0xFC07  k        ---- --kk kkkk k--- (k: -64 - 63)
//		{	"brts",		S_BRA,		0xF006	}, 0xFC07  k        ---- --kk kkkk k--- (k: -64 - 63)
//		{	"brvc",		S_BRA,		0xF403	}, 0xFC07  k        ---- --kk kkkk k--- (k: -64 - 63)
//		{	"brvs",		S_BRA,		0xF003	}, 0xFC07  k        ---- --kk kkkk k--- (k: -64 - 63)
//
//		{	"brbc",		S_SBRA,		0xF400	}, 0xFC00  s, k     ---- --kk kkkk ksss (k: -64 - 63), (s: 0-7)  xxx dup brxx
//		{	"brbs",		S_SBRA,		0xF000	}, 0xFC00  s, k     ---- --kk kkkk ksss (k: -64 - 63), (s: 0-7)  xxx dup brxx
//
//		{	"sbrc",		S_SKIP,		0xFC00	}, 0xFE08  Rr, b    ---- ---r rrrr -bbb (r: 0-31), (b: 0-7)  ?? S_TFLG, Difficult control requiring lookahead
//		{	"sbrs",		S_SKIP,		0xFE00	}, 0xFE08  Rr, b    ---- ---r rrrr -bbb (r: 0-31), (b: 0-7)  ?? S_TFLG, Difficult control requiring lookahead
//
//		{	"call",		S_JMP,		0x940E	}, 0xFE0E  k        ---- ---k kkkk ---k | kkkk kkkk kkkk kkkk (k: 0-64k, 0-4M), 16/22-bit absolute
//		{	"jmp",		S_JMP,		0x940C	}, 0xFE0E  k        ---- ---k kkkk ---k | kkkk kkkk kkkk kkkk (k: 0-64k, 0-4M), 16/22-bit absolute
//
//		{	"rcall",	S_RJMP,		0xD000	}, 0xF000  k        ---- kkkk kkkk kkkk (k: -2k - 2k), 12-bit relative
//		{	"rjmp",		S_RJMP,		0xC000	}, 0xF000  k        ---- kkkk kkkk kkkk (k: -2k - 2k), 12-bit relative
//
//		{	"cbi",		S_IOR,		0x9800	}, 0xFF00  A, b     ---- ---- AAAA Abbb (A: 0-31), (b: 0-7)
//		{	"sbi",		S_IOR,		0x9A00	}, 0xFF00  A, b     ---- ---- AAAA Abbb (A: 0-31), (b: 0-7)
//		{	"sbic",		S_IOR,		0x9900	}, 0xFF00  A, b     ---- ---- AAAA Abbb (A: 0-31), (b: 0-7), Difficult control requiring lookahead
//		{	"sbis",		S_IOR,		0x9B00	}, 0xFF00  A, b     ---- ---- AAAA Abbb (A: 0-31), (b: 0-7), Difficult control requiring lookahead
//
//		{	"in",		S_IN,		0xB000	}, 0xF800  Rd, A    ---- -AAd dddd AAAA (A: 0-63), (d: 0-31)
//		{	"out",		S_OUT,		0xB800	}, 0xF800  A, Rr    ---- -AAr rrrr AAAA (A: 0-63), (r: 0-31)
//
//		{	"ld",		S_LD,		0x8000	}, ????? (Z only?)
//		{	"ld",		S_LD/X		0x900C	}, 0xFE0F  Rd, X    ---- ---d dddd ---- (d: 0-31)
//		{	"ld",		S_LD/X+		0x900D	}, 0xFE0F  Rd, X+   ---- ---d dddd ---- (d: 0-31)
//		{	"ld",		S_LD/-X		0x900E	}, 0xFE0F  Rd, -X   ---- ---d dddd ---- (d: 0-31)
//		{	"ld",		S_LD/Y		0x8008	}, 0xFE0F  Rd, Y    ---- ---d dddd ---- (d: 0-31)  <<< SPC: ldd Rd,Y+0
//		{	"ld",		S_LD/Y+		0x9009	}, 0xFE0F  Rd, Y+   ---- ---d dddd ---- (d: 0-31)
//		{	"ld",		S_LD/-Y		0x900A	}, 0xFE0F  Rd, -Y   ---- ---d dddd ---- (d: 0-31)
//		{	"ld",		S_LD/Z		0x8000	}, 0xFE0F  Rd, Z    ---- ---d dddd ---- (d: 0-31)  <<< SPC: ldd Rd,Z+0
//		{	"ld",		S_LD/Z+		0x9001	}, 0xFE0F  Rd, Z+   ---- ---d dddd ---- (d: 0-31)
//		{	"ld",		S_LD/-Z		0x9002	}, 0xFE0F  Rd, -Z   ---- ---d dddd ---- (d: 0-31)
//		{	"st",		S_ST,		0x8200	}, ????? (Z only?)
//		{	"st",		S_ST/X		0x920C	}, 0xFE0F  X, Rr    ---- ---r rrrr ---- (r: 0-31)
//		{	"st",		S_ST/X+		0x920D	}, 0xFE0F  X+, Rr   ---- ---r rrrr ---- (r: 0-31)
//		{	"st",		S_ST/-X		0x920E	}, 0xFE0F  -X, Rr   ---- ---r rrrr ---- (r: 0-31)
//		{	"st",		S_ST/Y		0x8208	}, 0xFE0F  Y, Rr    ---- ---r rrrr ---- (r: 0-31)  <<< SPC: std Rd,Y+0
//		{	"st",		S_ST/Y+		0x9209	}, 0xFE0F  Y+, Rr   ---- ---r rrrr ---- (r: 0-31)
//		{	"st",		S_ST/-Y		0x920A	}, 0xFE0F  -Y, Rr   ---- ---r rrrr ---- (r: 0-31)
//		{	"st",		S_ST/Z		0x8200	}, 0xFE0F  Z, Rr    ---- ---r rrrr ---- (r: 0-31)  <<< SPC: std Rd,Z+0
//		{	"st",		S_ST/Z+		0x9201	}, 0xFE0F  Z+, Rr   ---- ---r rrrr ---- (r: 0-31)
//		{	"st",		S_ST/-Z		0x9202	}, 0xFE0F  -Z, Rr   ---- ---r rrrr ---- (r: 0-31)
//
//		{	"ldd",		S_ILD,		0x8000	}, ????? (Z only?)
//		{	"ldd",		S_ILD/Y+q	0x8008	}, 0xD208  Rd, Y+q  --q- qq-d dddd -qqq (d: 0-31), (q: 0-63)
//		{	"ldd",		S_ILD/Z+q	0x8000	}, 0xD208  Rd, Z+q  --q- qq-d dddd -qqq (d: 0-31), (q: 0-63)
//		{	"std",		S_IST,		0x8200	}, ????? (Z only?)
//		{	"std",		S_IST/Y+q	0x8208	}, 0xD208  Y+q, Rr  --q- qq-r rrrr -qqq (r: 0-31), (q: 0-63)
//		{	"std",		S_IST/Z+q	0x8200	}, 0xD208  Z+q, Rr  --q- qq-r rrrr -qqq (r: 0-31), (q: 0-63)
//
//		{	"lds",		S_LDS,		0x9000	}, 0xFE0F  Rd, k    ---- ---d dddd ---- | kkkk kkkk kkkk kkkk (d: 0-31),(k: 0-64k)
//		{	"lds",	S_LDS(AVRrc)	0xA000	}, 0xF800  Rd, k    ---- -kkk dddd kkkk (d: 16-31), (k: 0-127)
//		{	"sts",		S_STS,		0x9200	}, 0xFE0F  k, Rr    ---- ---r rrrr ---- | kkkk kkkk kkkk kkkk (r: 0-31),(k: 0-64k)
//		{	"sts",	S_STS(AVRrc)	0xA800	}, 0xF800  k, Rr    ---- -kkk rrrr kkkk (r: 16-31), (k: 0-127)
//
//		{	"elpm",	S_ELPM|S_INH,	0x95D8	}, 0xFFFF  -        ---- ---- ---- ----
//		{	"elpm",	S_SNGL/Z,		0x9006	}, 0xFE0F  Rd, Z    ---- ---d dddd ---- (d: 0-31)
//		{	"elpm",	S_SNGL/Z+,		0x9007	}, 0xFE0F  Rd, Z+   ---- ---d dddd ---- (d: 0-31)
//		{	"lpm",	S_LPM|S_INH,	0x95C8	}, 0xFFFF  -        ---- ---- ---- ----
//		{	"lpm",	S_SNGL/Z,		0x9004	}, 0xFE0F  Rd, Z    ---- ---d dddd ---- (d: 0-31)
//		{	"lpm",	S_SNGL/Z+,		0x9005	}, 0xFE0F  Rd, Z+   ---- ---d dddd ---- (d: 0-31)
//
//		{	"eicall",	S_INH,		0x9519	}, 0xFFFF  -        ---- ---- ---- ----
//		{	"eijmp",	S_INH,		0x9419	}, 0xFFFF  -        ---- ---- ---- ----
//		{	"icall",	S_INH,		0x9509	}, 0xFFFF  -        ---- ---- ---- ----
//		{	"ijmp",		S_INH,		0x9409	}, 0xFFFF  -        ---- ---- ---- ----
//		{	"ret",		S_INH,		0x9508	}, 0xFFFF  -        ---- ---- ---- ----
//		{	"reti",		S_INH,		0x9518	}, 0xFFFF  -        ---- ---- ---- ----
//
//		{	"sec",		S_INH,		0x9408	}, 0xFFFF  -        ---- ---- ---- ----
//		{	"sez",		S_INH,		0x9418	}, 0xFFFF  -        ---- ---- ---- ----
//		{	"sen",		S_INH,		0x9428	}, 0xFFFF  -        ---- ---- ---- ----
//		{	"sev",		S_INH,		0x9438	}, 0xFFFF  -        ---- ---- ---- ----
//		{	"ses",		S_INH,		0x9448	}, 0xFFFF  -        ---- ---- ---- ----
//		{	"seh",		S_INH,		0x9458	}, 0xFFFF  -        ---- ---- ---- ----
//		{	"set",		S_INH,		0x9468	}, 0xFFFF  -        ---- ---- ---- ----
//		{	"sei",		S_INH,		0x9478	}, 0xFFFF  -        ---- ---- ---- ----
//
//		{	"clc",		S_INH,		0x9488	}, 0xFFFF  -        ---- ---- ---- ----
//		{	"clz",		S_INH,		0x9498	}, 0xFFFF  -        ---- ---- ---- ----
//		{	"cln",		S_INH,		0x94A8	}, 0xFFFF  -        ---- ---- ---- ----
//		{	"clv",		S_INH,		0x94B8	}, 0xFFFF  -        ---- ---- ---- ----
//		{	"cls",		S_INH,		0x94C8	}, 0xFFFF  -        ---- ---- ---- ----
//		{	"clh",		S_INH,		0x94D8	}, 0xFFFF  -        ---- ---- ---- ----
//		{	"clt",		S_INH,		0x94E8	}, 0xFFFF  -        ---- ---- ---- ----
//		{	"cli",		S_INH,		0x94F8	}, 0xFFFF  -        ---- ---- ---- ----
//
//		{	"nop",		S_INH,		0x0000	}, 0xFFFF  -        ---- ---- ---- ----
//		{	"sleep",	S_INH,		0x9588	}, 0xFFFF  -        ---- ---- ---- ----
//		{	"break",	S_INH,		0x9598	}, 0xFFFF  -        ---- ---- ---- ----
//		{	"spm",		S_INH,		0x95E8	}, 0xFFFF  -        ---- ---- ---- ----		AVRe,AVRxm,AVRxt
//		{	"spm",		S_INH/Z+	0x95F8	}, 0xFFFF  Z+       ---- ---- ---- ----		AVRxm,AVRxt
//		{	"wdr",		S_INH,		0x95A8	}, 0xFFFF  -        ---- ---- ---- ----
//
//		{	"des",		S_DES,		0x940B	}, 0xFF0F  K        ---- ---- KKKK ----  (K: 0-15)
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

#define VERSION 0x0100			// AVR Disassembler Version number 1.00

#ifndef UNUSED
	#define UNUSED(x) ((void)(x))
#endif

#ifndef _countof
#define _countof(x) (sizeof(x)/sizeof(x[0]))
#endif

// ----------------------------------------------------------------------------
//	CAVRDisassembler
// ----------------------------------------------------------------------------

static bool isDUBLSameRegister(const COpcodeEntry<TAVRDisassembler> &anOpcode,
										 const TAVRDisassembler::COpcodeSymbolArray &arrOpMemory)
{
	assert(arrOpMemory.size() == 1);
	assert(anOpcode.opcode().size() == 1);
	assert(anOpcode.opcodeMask().size() == 1);
	assert(anOpcode.group() == TAVRDisassembler_ENUMS::S_SAME);

	// First make sure this is the same OpCode:
	assert(anOpcode.opcodeMask().at(0) == 0xFC00);
	assert((anOpcode.opcodeMask().at(0) & arrOpMemory.at(0)) == anOpcode.opcode().at(0));
	if ((anOpcode.opcodeMask().at(0) & arrOpMemory.at(0)) != anOpcode.opcode().at(0)) return false;

	// See if Rd == Rr (i.e. that this is a S_DUBL with the same register referenced):
	//	S_SAME,			// 0xFC00 : Rd       ---- --dd dddd dddd (d: 0-31)x2
	//	S_DUBL,			// 0xFC00 : Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)

	TAVRDisassembler::TOpcodeSymbol Rd = ((arrOpMemory.at(0) & 0x01F0) >> 4);
	TAVRDisassembler::TOpcodeSymbol Rr = ((arrOpMemory.at(0) & 0x0200) >> 5) | (arrOpMemory.at(0) & 0x000F);

	return (Rd == Rr);
}


CAVRDisassembler::CAVRDisassembler()
	:	m_nOpPointer(0),
		m_nStartPC(0),
		m_nSectionCount(0)
{
	using namespace TAVRDisassembler_ENUMS;

	const COpcodeEntry<TAVRDisassembler> arrOpcodes[] = {
		{ { 0xEF0F }, { 0xFF0F }, S_SER, CTL_None, "ser", },		// ser    Rd       ---- ---- dddd ---- (d: 16-31)  <<< SPC: ldi Rd,0xFF  (must be listed ahead of it)

		{ { 0x7000 }, { 0xF000 }, S_IBYTE, CTL_None, "andi"  },		// andi   Rd, K    ---- KKKK dddd KKKK (K: 0-255), (d: 16-31)
		{ { 0x3000 }, { 0xF000 }, S_IBYTE, CTL_None, "cpi",  },		// cpi    Rd, K    ---- KKKK dddd KKKK (K: 0-255), (d: 16-31)
		{ { 0xE000 }, { 0xF000 }, S_IBYTE, CTL_None, "ldi", },		// ldi    Rd, K    ---- KKKK dddd KKKK (K: 0-255), (d: 16-31)
		{ { 0x6000 }, { 0xF000 }, S_IBYTE, CTL_None, "ori", },		// ori    Rd, K    ---- KKKK dddd KKKK (K: 0-255), (d: 16-31)
		{ { 0x4000 }, { 0xF000 }, S_IBYTE, CTL_None, "sbci", },		// sbci   Rd, K    ---- KKKK dddd KKKK (K: 0-255), (d: 16-31)
		{ { 0x6000 }, { 0xF000 }, S_IBYTE, CTL_None, "sbr", },		// sbr    Rd, K    ---- KKKK dddd KKKK (K: 0-255), (d: 16-31)
		{ { 0x5000 }, { 0xF000 }, S_IBYTE, CTL_None, "subi", },		// subi   Rd, K    ---- KKKK dddd KKKK (K: 0-255), (d: 16-31)

		{ { 0x9600 }, { 0xFF00 }, S_IWORD, CTL_None, "adiw", },		// adiw   Rd, K    ---- ---- KKdd KKKK (K: 0-63), (d: 24, 26, 28, 30)
		{ { 0x9700 }, { 0xFF00 }, S_IWORD, CTL_None, "sbiw", },		// sbiw   Rd, K    ---- ---- KKdd KKKK (K: 0-63), (d: 24, 26, 28, 30)

		{ { 0x9405 }, { 0xFE0F }, S_SNGL, CTL_None, "asr", },		// asr    Rd       ---- ---d dddd ---- (d: 0-31)
		{ { 0x9400 }, { 0xFE0F }, S_SNGL, CTL_None, "com", },		// com    Rd       ---- ---d dddd ---- (d: 0-31)
		{ { 0x940A }, { 0xFE0F }, S_SNGL, CTL_None, "dec", },		// dec    Rd       ---- ---d dddd ---- (d: 0-31)
		{ { 0x9403 }, { 0xFE0F }, S_SNGL, CTL_None, "inc", },		// inc    Rd       ---- ---d dddd ---- (d: 0-31)
		{ { 0x9406 }, { 0xFE0F }, S_SNGL, CTL_None, "lsr", },		// lsr    Rd       ---- ---d dddd ---- (d: 0-31)
		{ { 0x9401 }, { 0xFE0F }, S_SNGL, CTL_None, "neg", },		// neg    Rd       ---- ---d dddd ---- (d: 0-31)
		{ { 0x900F }, { 0xFE0F }, S_SNGL, CTL_None, "pop", },		// pop    Rd       ---- ---d dddd ---- (d: 0-31)
		{ { 0x920F }, { 0xFE0F }, S_SNGL, CTL_None, "push", },		// push   Rr       ---- ---r rrrr ---- (r: 0-31)
		{ { 0x9407 }, { 0xFE0F }, S_SNGL, CTL_None, "ror", },		// ror    Rd       ---- ---d dddd ---- (d: 0-31)
		{ { 0x9402 }, { 0xFE0F }, S_SNGL, CTL_None, "swap", },		// swap   Rd       ---- ---d dddd ---- (d: 0-31)
		{ { 0x9204 }, { 0xFE0F }, S_SNGL, CTL_None, "xch", },		// xch    Rd       ---- ---d dddd ---- (d: 0-31)

		{ { 0x9206 }, { 0xFE0F }, S_Z_SNGL, CTL_None, "lac", },		// lac    Z, Rr    ---- ---r rrrr ---- (r: 0-31) (implied Z for destination)
		{ { 0x9205 }, { 0xFE0F }, S_Z_SNGL, CTL_None, "las", },		// las    Z, Rr    ---- ---r rrrr ---- (r: 0-31) (implied Z for destination)
		{ { 0x9207 }, { 0xFE0F }, S_Z_SNGL, CTL_None, "lat", },		// lat    Z, Rr    ---- ---r rrrr ---- (r: 0-31) (implied Z for destination)

		{ { 0x2400 }, { 0xFC00 }, S_SAME, CTL_None, "clr", isDUBLSameRegister, },		// clr    Rd       ---- --dd dddd dddd (d: 0-31)x2 <<< SPC:  eor Rd,Rd  (must be listed ahead of it)
		{ { 0x0C00 }, { 0xFC00 }, S_SAME, CTL_None, "lsl", isDUBLSameRegister, },		// lsl    Rd       ---- --dd dddd dddd (d: 0-31)x2 <<< SPC:  add Rd,Rd  (must be listed ahead of it)
		{ { 0x1C00 }, { 0xFC00 }, S_SAME, CTL_None, "rol", isDUBLSameRegister, },		// rol    Rd       ---- --dd dddd dddd (d: 0-31)x2 <<< SPC:  adc Rd,Rd  (must be listed ahead of it)
		{ { 0x2000 }, { 0xFC00 }, S_SAME, CTL_None, "tst", isDUBLSameRegister, },		// tst    Rd       ---- --dd dddd dddd (d: 0-31)x2 <<< SPC:  and Rd,Rd  (must be listed ahead of it)

		{ { 0x1C00 }, { 0xFC00 }, S_DUBL, CTL_None, "adc", },		// adc    Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
		{ { 0x0C00 }, { 0xFC00 }, S_DUBL, CTL_None, "add", },		// add    Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
		{ { 0x2000 }, { 0xFC00 }, S_DUBL, CTL_None, "and", },		// and    Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
		{ { 0x1400 }, { 0xFC00 }, S_DUBL, CTL_None, "cp", },		// cp     Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
		{ { 0x0400 }, { 0xFC00 }, S_DUBL, CTL_None, "cpc", },		// cpc    Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
		{ { 0x1000 }, { 0xFC00 }, S_DUBL, CTL_Skip, "cpse", },		// cpse   Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31), Difficult control requiring lookahead
		{ { 0x2400 }, { 0xFC00 }, S_DUBL, CTL_None, "eor", },		// eor    Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
		{ { 0x2C00 }, { 0xFC00 }, S_DUBL, CTL_None, "mov", },		// mov    Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
		{ { 0x2800 }, { 0xFC00 }, S_DUBL, CTL_None, "or", },		// or     Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
		{ { 0x0800 }, { 0xFC00 }, S_DUBL, CTL_None, "sbc", },		// sbc    Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
		{ { 0x1800 }, { 0xFC00 }, S_DUBL, CTL_None, "sub", },		// sub    Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)

		{ { 0x0100 }, { 0xFF00 }, S_MOVW, CTL_None, "movw", },		// movw   Rd, Rr   ---- ---- dddd rrrr (d: 0,2,4,...,30), (r: 0,2,4,...,30)

		{ { 0x9C00 }, { 0xFC00 }, S_DUBL, CTL_None, "mul", },		// mul    Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
		{ { 0x0200 }, { 0xFF00 }, S_MULS, CTL_None, "muls", },		// muls   Rd, Rr   ---- ---- dddd rrrr (d: 16-31), (r: 16-31)
		{ { 0x0300 }, { 0xFF88 }, S_FMUL, CTL_None, "mulsu", },		// mulsu  Rd, Rr   ---- ---- -ddd -rrr (d: 16-23), (r: 16-23)
		{ { 0x0308 }, { 0xFF88 }, S_FMUL, CTL_None, "fmul", },		// fmul   Rd, Rr   ---- ---- -ddd -rrr (d: 16-23), (r: 16-23)
		{ { 0x0380 }, { 0xFF88 }, S_FMUL, CTL_None, "fmuls", },		// fmuls  Rd, Rr   ---- ---- -ddd -rrr (d: 16-23), (r: 16-23)
		{ { 0x0388 }, { 0xFF88 }, S_FMUL, CTL_None, "fmulsu", },	// fmulsu Rd, Rr   ---- ---- -ddd -rrr (d: 16-23), (r: 16-23)

		{ { 0xF800 }, { 0xFE08 }, S_TFLG, CTL_None, "bld", },		// bld    Rd, b    ---- ---d dddd -bbb (d: 0-31), (b: 0-7)
		{ { 0xFA00 }, { 0xFE08 }, S_TFLG, CTL_None, "bst", },		// bst    Rd, b    ---- ---d dddd -bbb (d: 0-31), (b: 0-7)

		{ { 0xF400 }, { 0xFC07 }, S_BRA, CTL_DetBra, "brcc", },		// brcc   k        ---- --kk kkkk k--- (k: -64 - 63)
		{ { 0xF000 }, { 0xFC07 }, S_BRA, CTL_DetBra, "brcs", },		// brcs   k        ---- --kk kkkk k--- (k: -64 - 63)
		{ { 0xF001 }, { 0xFC07 }, S_BRA, CTL_DetBra, "breq", },		// breq   k        ---- --kk kkkk k--- (k: -64 - 63)
		{ { 0xF404 }, { 0xFC07 }, S_BRA, CTL_DetBra, "brge", },		// brge   k        ---- --kk kkkk k--- (k: -64 - 63)
		{ { 0xF405 }, { 0xFC07 }, S_BRA, CTL_DetBra, "brhc", },		// brhc   k        ---- --kk kkkk k--- (k: -64 - 63)
		{ { 0xF005 }, { 0xFC07 }, S_BRA, CTL_DetBra, "brhs", },		// brhs   k        ---- --kk kkkk k--- (k: -64 - 63)
		{ { 0xF407 }, { 0xFC07 }, S_BRA, CTL_DetBra, "brid", },		// brid   k        ---- --kk kkkk k--- (k: -64 - 63)
		{ { 0xF007 }, { 0xFC07 }, S_BRA, CTL_DetBra, "brie", },		// brie   k        ---- --kk kkkk k--- (k: -64 - 63)
		{ { 0xF004 }, { 0xFC07 }, S_BRA, CTL_DetBra, "brlt", },		// brlt   k        ---- --kk kkkk k--- (k: -64 - 63)
		{ { 0xF002 }, { 0xFC07 }, S_BRA, CTL_DetBra, "brmi", },		// brmi   k        ---- --kk kkkk k--- (k: -64 - 63)
		{ { 0xF401 }, { 0xFC07 }, S_BRA, CTL_DetBra, "brne", },		// brne   k        ---- --kk kkkk k--- (k: -64 - 63)
		{ { 0xF402 }, { 0xFC07 }, S_BRA, CTL_DetBra, "brpl", },		// brpl   k        ---- --kk kkkk k--- (k: -64 - 63)
		{ { 0xF406 }, { 0xFC07 }, S_BRA, CTL_DetBra, "brtc", },		// brtc   k        ---- --kk kkkk k--- (k: -64 - 63)
		{ { 0xF006 }, { 0xFC07 }, S_BRA, CTL_DetBra, "brts", },		// brts   k        ---- --kk kkkk k--- (k: -64 - 63)
		{ { 0xF403 }, { 0xFC07 }, S_BRA, CTL_DetBra, "brvc", },		// brvc   k        ---- --kk kkkk k--- (k: -64 - 63)
		{ { 0xF003 }, { 0xFC07 }, S_BRA, CTL_DetBra, "brvs", },		// brvs   k        ---- --kk kkkk k--- (k: -64 - 63)

		{ { 0xFC00 }, { 0xFE08 }, S_TFLG, CTL_Skip, "sbrc", },		// sbrc   Rr, b    ---- ---r rrrr -bbb (r: 0-31), (b: 0-7), Difficult control requiring lookahead
		{ { 0xFE00 }, { 0xFE08 }, S_TFLG, CTL_Skip, "sbrs", },		// sbrs   Rr, b    ---- ---r rrrr -bbb (r: 0-31), (b: 0-7), Difficult control requiring lookahead

		{ { 0x940E }, { 0xFE0E }, S_JMP, CTL_DetBra | OCTL_TwoWordCode, "call", },		// call   k        ---- ---k kkkk ---k | kkkk kkkk kkkk kkkk (k: 0-64k, 0-4M), 16/22-bit absolute
		{ { 0x940C }, { 0xFE0E }, S_JMP, CTL_DetBra | OCTL_TwoWordCode | TAVRDisassembler::OCTL_STOP, "jmp", },		// jmp    k        ---- ---k kkkk ---k | kkkk kkkk kkkk kkkk (k: 0-64k, 0-4M), 16/22-bit absolute

		{ { 0xD000 }, { 0xF000 }, S_RJMP, CTL_DetBra, "rcall", },	// rcall  k        ---- kkkk kkkk kkkk (k: -2k - 2k), 12-bit relative
		{ { 0xC000 }, { 0xF000 }, S_RJMP, CTL_DetBra | TAVRDisassembler::OCTL_STOP, "rjmp", },		// rjmp   k        ---- kkkk kkkk kkkk (k: -2k - 2k), 12-bit relative

		{ { 0x9800 }, { 0xFF00 }, S_IOR, CTL_IOLabel, "cbi", },		// cbi    A, b     ---- ---- AAAA Abbb (A: 0-31), (b: 0-7)
		{ { 0x9A00 }, { 0xFF00 }, S_IOR, CTL_IOLabel, "sbi", },		// sbi    A, b     ---- ---- AAAA Abbb (A: 0-31), (b: 0-7)
		{ { 0x9900 }, { 0xFF00 }, S_IOR, CTL_SkipIOLabel, "sbic", },	// sbic   A, b     ---- ---- AAAA Abbb (A: 0-31), (b: 0-7), Difficult control requiring lookahead
		{ { 0x9B00 }, { 0xFF00 }, S_IOR, CTL_SkipIOLabel, "sbis", },	// sbis   A, b     ---- ---- AAAA Abbb (A: 0-31), (b: 0-7), Difficult control requiring lookahead

		{ { 0xB000 }, { 0xF800 }, S_IN, CTL_IOLabel, "in", },		// in     Rd, A    ---- -AAd dddd AAAA (A: 0-63), (d: 0-31)
		{ { 0xB800 }, { 0xF800 }, S_OUT, CTL_IOLabel, "out", },		// out    A, Rr    ---- -AAr rrrr AAAA (A: 0-63), (r: 0-31)

		{ { 0x900C }, { 0xFE0F }, S_SNGL_X, CTL_None, "ld", },		// ld     Rd, X    ---- ---d dddd ---- (d: 0-31)
		{ { 0x900D }, { 0xFE0F }, S_SNGL_Xp, CTL_None, "ld", },		// ld     Rd, X+   ---- ---d dddd ---- (d: 0-31)
		{ { 0x900E }, { 0xFE0F }, S_SNGL_nX, CTL_None, "ld", },		// ld     Rd, -X   ---- ---d dddd ---- (d: 0-31)
		{ { 0x8008 }, { 0xFE0F }, S_SNGL_Y, CTL_None, "ld", },		// ld     Rd, Y    ---- ---d dddd ---- (d: 0-31)  <<< SPC: ldd Rd,Y+0  (must be listed ahead of it)
		{ { 0x9009 }, { 0xFE0F }, S_SNGL_Yp, CTL_None, "ld", },		// ld     Rd, Y+   ---- ---d dddd ---- (d: 0-31)
		{ { 0x900A }, { 0xFE0F }, S_SNGL_nY, CTL_None, "ld", },		// ld     Rd, -Y   ---- ---d dddd ---- (d: 0-31)
		{ { 0x8000 }, { 0xFE0F }, S_SNGL_Z, CTL_None, "ld", },		// ld     Rd, Z    ---- ---d dddd ---- (d: 0-31)  <<< SPC: ldd Rd,Z+0  (must be listed ahead of it)
		{ { 0x9001 }, { 0xFE0F }, S_SNGL_Zp, CTL_None, "ld", },		// ld     Rd, Z+   ---- ---d dddd ---- (d: 0-31)
		{ { 0x9002 }, { 0xFE0F }, S_SNGL_nZ, CTL_None, "ld", },		// ld     Rd, -Z   ---- ---d dddd ---- (d: 0-31)
		{ { 0x920C }, { 0xFE0F }, S_X_SNGL, CTL_None, "st", },		// st     X, Rr    ---- ---r rrrr ---- (r: 0-31)
		{ { 0x920D }, { 0xFE0F }, S_Xp_SNGL, CTL_None, "st", },		// st     X+, Rr   ---- ---r rrrr ---- (r: 0-31)
		{ { 0x920E }, { 0xFE0F }, S_nX_SNGL, CTL_None, "st", },		// st     -X, Rr   ---- ---r rrrr ---- (r: 0-31)
		{ { 0x8208 }, { 0xFE0F }, S_Y_SNGL, CTL_None, "st", },		// st     Y, Rr    ---- ---r rrrr ---- (r: 0-31)  <<< SPC: std Rd,Y+0  (must be listed ahead of it)
		{ { 0x9209 }, { 0xFE0F }, S_Yp_SNGL, CTL_None, "st", },		// st     Y+, Rr   ---- ---r rrrr ---- (r: 0-31)
		{ { 0x920A }, { 0xFE0F }, S_nY_SNGL, CTL_None, "st", },		// st     -Y, Rr   ---- ---r rrrr ---- (r: 0-31)
		{ { 0x8200 }, { 0xFE0F }, S_Z_SNGL, CTL_None, "st", },		// st     Z, Rr    ---- ---r rrrr ---- (r: 0-31)  <<< SPC: std Rd,Z+0  (must be listed ahead of it)
		{ { 0x9201 }, { 0xFE0F }, S_Zp_SNGL, CTL_None, "st", },		// st     Z+, Rr   ---- ---r rrrr ---- (r: 0-31)
		{ { 0x9202 }, { 0xFE0F }, S_nZ_SNGL, CTL_None, "st", },		// st     -Z, Rr   ---- ---r rrrr ---- (r: 0-31)

		{ { 0x8008 }, { 0xD208 }, S_SNGL_Yq, CTL_None, "ldd", },	// ldd    Rd, Y+q  --q- qq-d dddd -qqq (d: 0-31), (q: 0-63)
		{ { 0x8000 }, { 0xD208 }, S_SNGL_Zq, CTL_None, "ldd", },	// ldd    Rd, Z+q  --q- qq-d dddd -qqq (d: 0-31), (q: 0-63)
		{ { 0x8208 }, { 0xD208 }, S_Yq_SNGL, CTL_None, "std", },	// std    Y+q, Rr  --q- qq-r rrrr -qqq (r: 0-31), (q: 0-63)
		{ { 0x8200 }, { 0xD208 }, S_Zq_SNGL, CTL_None, "std", },	// std    Z+q, Rr  --q- qq-r rrrr -qqq (r: 0-31), (q: 0-63)

		{ { 0x9000 }, { 0xFE0F }, S_LDS, CTL_DataLabel | OCTL_TwoWordCode | OCTL_ArchExtAddr, "lds", },		// lds    Rd, k    ---- ---d dddd ---- | kkkk kkkk kkkk kkkk (d: 0-31),(k: 0-64k)
		{ { 0xA000 }, { 0xF800 }, S_LDSrc, CTL_DataLabel, "lds", },	// lds    Rd, k    ---- -kkk dddd kkkk (d: 16-31), (k: 0-127)
		{ { 0x9200 }, { 0xFE0F }, S_STS, CTL_DataLabel | OCTL_TwoWordCode | OCTL_ArchExtAddr, "sts", },		// sts    k, Rr    ---- ---r rrrr ---- | kkkk kkkk kkkk kkkk (r: 0-31),(k: 0-64k)
		{ { 0xA800 }, { 0xF800 }, S_STSrc, CTL_DataLabel, "sts", },	// sts    k, Rr    ---- -kkk rrrr kkkk (r: 16-31), (k: 0-127)

		{ { 0x95D8 }, { 0xFFFF }, S_INH, CTL_None, "elpm", },		// elpm   -        ---- ---- ---- ----
		{ { 0x9006 }, { 0xFE0F }, S_SNGL_Z, CTL_None, "elpm", },	// elpm   Rd, Z    ---- ---d dddd ---- (d: 0-31)
		{ { 0x9007 }, { 0xFE0F }, S_SNGL_Zp, CTL_None, "elpm", },	// elpm   Rd, Z+   ---- ---d dddd ---- (d: 0-31)
		{ { 0x95C8 }, { 0xFFFF }, S_INH, CTL_None, "lpm", },		// lpm    -        ---- ---- ---- ----
		{ { 0x9004 }, { 0xFE0F }, S_SNGL_Z, CTL_None, "lpm", },		// lpm    Rd, Z    ---- ---d dddd ---- (d: 0-31)
		{ { 0x9005 }, { 0xFE0F }, S_SNGL_Zp, CTL_None, "lpm", },	// lpm    Rd, Z+   ---- ---d dddd ---- (d: 0-31)

		{ { 0x9519 }, { 0xFFFF }, S_INH, CTL_UndetBra, "eicall", },	// eicall -        ---- ---- ---- ----
		{ { 0x9419 }, { 0xFFFF }, S_INH, CTL_UndetBra | TAVRDisassembler::OCTL_STOP, "eijmp", },	// eijmp  -        ---- ---- ---- ----
		{ { 0x9509 }, { 0xFFFF }, S_INH, CTL_UndetBra, "icall", },	// icall  -        ---- ---- ---- ----
		{ { 0x9409 }, { 0xFFFF }, S_INH, CTL_UndetBra | TAVRDisassembler::OCTL_STOP, "ijmp", },		// ijmp   -        ---- ---- ---- ----
		{ { 0x9508 }, { 0xFFFF }, S_INH, CTL_None | TAVRDisassembler::OCTL_STOP, "ret", },		// ret    -        ---- ---- ---- ----
		{ { 0x9518 }, { 0xFFFF }, S_INH, CTL_None | TAVRDisassembler::OCTL_STOP, "reti", },		// reti   -        ---- ---- ---- ----

		{ { 0x9408 }, { 0xFFFF }, S_INH, CTL_None, "sec", },		// sec    -        ---- ---- ---- ----
		{ { 0x9418 }, { 0xFFFF }, S_INH, CTL_None, "sez", },		// sez    -        ---- ---- ---- ----
		{ { 0x9428 }, { 0xFFFF }, S_INH, CTL_None, "sen", },		// sen    -        ---- ---- ---- ----
		{ { 0x9438 }, { 0xFFFF }, S_INH, CTL_None, "sev", },		// sev    -        ---- ---- ---- ----
		{ { 0x9448 }, { 0xFFFF }, S_INH, CTL_None, "ses", },		// ses    -        ---- ---- ---- ----
		{ { 0x9458 }, { 0xFFFF }, S_INH, CTL_None, "seh", },		// seh    -        ---- ---- ---- ----
		{ { 0x9468 }, { 0xFFFF }, S_INH, CTL_None, "set", },		// set    -        ---- ---- ---- ----
		{ { 0x9478 }, { 0xFFFF }, S_INH, CTL_None, "sei", },		// sei    -        ---- ---- ---- ----

		{ { 0x9488 }, { 0xFFFF }, S_INH, CTL_None, "clc", },		// clc    -        ---- ---- ---- ----
		{ { 0x9498 }, { 0xFFFF }, S_INH, CTL_None, "clz", },		// clz    -        ---- ---- ---- ----
		{ { 0x94A8 }, { 0xFFFF }, S_INH, CTL_None, "cln", },		// cln    -        ---- ---- ---- ----
		{ { 0x94B8 }, { 0xFFFF }, S_INH, CTL_None, "clv", },		// clv    -        ---- ---- ---- ----
		{ { 0x94C8 }, { 0xFFFF }, S_INH, CTL_None, "cls", },		// cls    -        ---- ---- ---- ----
		{ { 0x94D8 }, { 0xFFFF }, S_INH, CTL_None, "clh", },		// clh    -        ---- ---- ---- ----
		{ { 0x94E8 }, { 0xFFFF }, S_INH, CTL_None, "clt", },		// clt    -        ---- ---- ---- ----
		{ { 0x94F8 }, { 0xFFFF }, S_INH, CTL_None, "cli", },		// cli    -        ---- ---- ---- ----

		{ { 0x0000 }, { 0xFFFF }, S_INH, CTL_None, "nop", },		// nop    -        ---- ---- ---- ----
		{ { 0x9588 }, { 0xFFFF }, S_INH, CTL_None, "sleep", },		// sleep  -        ---- ---- ---- ----
		{ { 0x9598 }, { 0xFFFF }, S_INH, CTL_None, "break", },		// break  -        ---- ---- ---- ----
		{ { 0x95E8 }, { 0xFFFF }, S_INH, CTL_None, "spm", },		// spm    -        ---- ---- ---- ----		AVRe,AVRxm,AVRxt
		{ { 0x95F8 }, { 0xFFFF }, S_INH_Zp, CTL_None, "spm", },		// spm    Z+       ---- ---- ---- ----		AVRxm,AVRxt
		{ { 0x95A8 }, { 0xFFFF }, S_INH, CTL_None, "wdr", },		// wdr    -        ---- ---- ---- ----

		{ { 0x940B }, { 0xFF0F }, S_DES, CTL_None, "des", },		// des    K        ---- ---- KKKK ----  (K: 0-15)
	};

	for (size_t ndx = 0; ndx < _countof(arrOpcodes); ++ndx) {
		m_Opcodes.AddOpcode(arrOpcodes[ndx]);
	}

	m_bAllowMemRangeOverlap = true;

	// TODO : Allow for various CPU types and associated memory layouts.
	//		For now just use ATmega328PB
	m_Memory.push_back(CMemBlock{ 0x0ul, 0x0ul, true, 0x8000ul, 0, DMEM_NOTLOADED });	// 32K of code memory available to processor as one block
}

// ----------------------------------------------------------------------------

unsigned int CAVRDisassembler::GetVersionNumber() const
{
	return (CDisassembler::GetVersionNumber() | VERSION);			// Get GDC version and append our version to it
}

std::string CAVRDisassembler::GetGDCLongName() const
{
	return "AVR Disassembler";
}

std::string CAVRDisassembler::GetGDCShortName() const
{
	return "AVR";
}

// ----------------------------------------------------------------------------

