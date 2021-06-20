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
//			CTL_DataRegAddr,	//	Disassemble as code, with data address via register (i.e. unknown data address, such as LPM, etc)
//			CTL_IOLabel,		//	Disassemble as code, with I/O addr label
//			CTL_UndetBra,		//	Disassemble as undeterminable branch address (Comment code)
//			CTL_DetBra,			//	Disassemble as determinable branch, add branch addr and label
//			CTL_Skip,			//	Disassemble as Skip Branch.  Next two instructions flagged as code even if next one would be terminal
//			CTL_SkipIOLabel,	//	Disassemble as Skip Branch, but with I/O addr label
//
//			OCTL_ArchExtAddr = 0x10000ul,		// Flag for Architecture Specific extended addressing (such as RAMPD for LDS/STS instructions for >= 64K or EIND for EICALL)
//
//
//	This version setup for compatibility with the avra assembler
//		(https://github.com/Ro5bert/avra)
//

//
//	Format of Function Output File:
//
//		Any line beginning with a ";" is considered to be a commment line and is ignored
//
//		Memory Mapping:
//			#type|addr|size
//			   |    |    |____  Size of Mapped area (hex)
//			   |    |_________  Absolute Address of Mapped area (hex)
//			   |______________  Type of Mapped area (One of following: ROM, RAM, IO)
//
//		Label Definitions:
//			!type|addr|label
//			   |    |    |____  Label(s) for this address(comma separated)
//			   |    |_________  Absolute Address (hex)
//			   |______________  Type of Mapped area (One of following: ROM, RAM, IO)
//
//		Start of New Function:
//			@xxxx|name
//			   |    |____ Name(s) of Function (comma separated list)
//			   |_________ Absolute Address of Function Start relative to overall file (hex)
//
//		Mnemonic Line (inside function):
//			xxxx|xxxx|label|xxxxxxxxxx|xxxxxx|xxxx|DST|SRC|mnemonic|operands
//			  |    |    |        |        |     |   |   |     |        |____  Disassembled operand output (ascii)
//			  |    |    |        |        |     |   |   |     |_____________  Disassembled mnemonic (ascii)
//			  |    |    |        |        |     |   |___|___________________  See below for SRC/DST format
//			  |    |    |        |        |     |___________________________  Addressing/Data bytes from operand portion of instruction (hex)
//			  |    |    |        |        |_________________________________  Opcode bytes from instruction (hex)
//			  |    |    |        |__________________________________________  All bytes from instruction (hex)
//			  |    |    |___________________________________________________  Label(s) for this address (comma separated list)
//			  |    |________________________________________________________  Absolute address of instruction (hex)
//			  |_____________________________________________________________  Relative address of instruction to the function (hex)
//
//		Data Byte Line (inside function):
//			xxxx|xxxx|label|xx
//			  |    |    |    |____  Data Byte (hex)
//			  |    |    |_________  Label(s) for this address (comma separated)
//			  |    |______________  Absolute address of data byte (hex)
//			  |___________________  Relative address of data byte to the function (hex)
//
//		SRC and DST entries:
//			#xxxx			Immediate Data Value (xxxx=hex value)
//			C@xxxx			Absolute Code Address (xxxx=hex address)
//			C^n(xxxx)		Relative Code Address (n=signed offset in hex, xxxx=resolved absolute address in hex)
//			C&xx(r)			Register Code Offset (xx=hex offset always 0, r=register number or name), ex: ijmp -> "C&00(Z)"; eijmp -> "C&00(EIND,Z)"
//			D@xxxx			Absolute Data Address (xxxx=hex address) -- Used for both Data and Memmap I/O (all I/O instructions outputted as Memmap I/O here)
//			D@xxxx,b		Absolute Data Address (xxxx=hex address), (b=bit number, 0-7) -- Used for I/O only
//			D^n(xxxx)		Relative Data Address (n=signed offset in hex, xxxx=resolved absolute address in hex), None of these on AVR
//			D&xx(r)			Register Data Offset (xx=hex offset, r=register number or name), ex: lpm -> "D&00(Z)"; elpm -> "D&00(RAMPZ:Z)"
//			Rn				Register (n=register number, 0-31)
//			Rn,b			Register (n=register number, 0-31), (b=bit number, 0-7)
//			X				X Register
//			X+				X Register with post-increment
//			-X				X Register with pre-decrement
//			Y				Y Register
//			Y+				Y Register with post-increment
//			-Y				Y Register with pre-decrement
//			Y+q				Y Register with 'q' offset (in decimal)
//			Z				Z Register
//			Z+				Z Register with post-increment
//			-Z				Z Register with pre-decrement
//			Z+q				Z Register with 'q' offset (in decimal)
//
//			If any of the above also includes a mask, then the following will be added:
//			,Mxx			Value mask (xx=hex mask value) [Note: Doesn't apply to AVR]
//
//		Note: The address sizes are consistent with the particular process.  That is, 16-bit data or
//			address will use 4 hex digits, a 22-bit address will use 6 hex digits, etc.  Relative
//			addresses (whether 7-bit or 12-bit, etc) in the main memory space will be converted to 16-bit addresses.
//			The size of immediate data entries, offsets, and masks will reflect the actual value.
//			That is, a 16-bit immediate value, offset, or mask, will be outputted as 4 hex digits,
//			but an 8-bit immediate value, offset, or mask, will be outputted as only 2 hex digits.
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
#include <limits>

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

bool CAVRDisassembler::isDUBLSameRegister(const COpcodeEntry<TAVRDisassembler> &anOpcode,
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

	return (opDstReg0_31(arrOpMemory) == opSrcReg0_31(arrOpMemory));
}


CAVRDisassembler::CAVRDisassembler()
	:	m_bCurrentOpcodeIsSkip(false),
		m_bLastOpcodeWasSkip(false),
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

		{ { 0x940E, 0x0000 }, { 0xFE0E, 0x0000 }, S_JMP, CTL_DetBra, "call", },		// call   k        ---- ---k kkkk ---k | kkkk kkkk kkkk kkkk (k: 0-64k, 0-4M), 16/22-bit absolute
		{ { 0x940C, 0x0000 }, { 0xFE0E, 0x0000 }, S_JMP, CTL_DetBra | TAVRDisassembler::OCTL_STOP, "jmp", },		// jmp    k        ---- ---k kkkk ---k | kkkk kkkk kkkk kkkk (k: 0-64k, 0-4M), 16/22-bit absolute

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

		{ { 0x9000, 0x0000 }, { 0xFE0F, 0x0000 }, S_LDS, CTL_DataLabel | OCTL_ArchExtAddr, "lds", },		// lds    Rd, k    ---- ---d dddd ---- | kkkk kkkk kkkk kkkk (d: 0-31),(k: 0-64k)
		{ { 0xA000 }, { 0xF800 }, S_LDSrc, CTL_DataLabel, "lds", },	// lds    Rd, k    ---- -kkk dddd kkkk (d: 16-31), (k: 0-127)
		{ { 0x9200, 0x0000 }, { 0xFE0F, 0x0000 }, S_STS, CTL_DataLabel | OCTL_ArchExtAddr, "sts", },		// sts    k, Rr    ---- ---r rrrr ---- | kkkk kkkk kkkk kkkk (r: 0-31),(k: 0-64k)
		{ { 0xA800 }, { 0xF800 }, S_STSrc, CTL_DataLabel, "sts", },	// sts    k, Rr    ---- -kkk rrrr kkkk (r: 16-31), (k: 0-127)

		{ { 0x95D8 }, { 0xFFFF }, S_INH, CTL_DataRegAddr | OCTL_ArchExtAddr, "elpm", },
																	// elpm   -        ---- ---- ---- ----
		{ { 0x9006 }, { 0xFE0F }, S_SNGL_Z, CTL_DataRegAddr | OCTL_ArchExtAddr, "elpm", },
																	// elpm   Rd, Z    ---- ---d dddd ---- (d: 0-31)
		{ { 0x9007 }, { 0xFE0F }, S_SNGL_Zp, CTL_DataRegAddr | OCTL_ArchExtAddr, "elpm", },
																	// elpm   Rd, Z+   ---- ---d dddd ---- (d: 0-31)
		{ { 0x95C8 }, { 0xFFFF }, S_INH, CTL_DataRegAddr, "lpm", },	// lpm    -        ---- ---- ---- ----
		{ { 0x9004 }, { 0xFE0F }, S_SNGL_Z, CTL_DataRegAddr, "lpm", },
																	// lpm    Rd, Z    ---- ---d dddd ---- (d: 0-31)
		{ { 0x9005 }, { 0xFE0F }, S_SNGL_Zp, CTL_DataRegAddr, "lpm", },
																	// lpm    Rd, Z+   ---- ---d dddd ---- (d: 0-31)

		{ { 0x9519 }, { 0xFFFF }, S_INH, CTL_UndetBra | OCTL_ArchExtAddr, "eicall", },
																	// eicall -        ---- ---- ---- ----
		{ { 0x9419 }, { 0xFFFF }, S_INH, CTL_UndetBra | OCTL_ArchExtAddr | TAVRDisassembler::OCTL_STOP, "eijmp", },
																	// eijmp  -        ---- ---- ---- ----
		{ { 0x9509 }, { 0xFFFF }, S_INH, CTL_UndetBra, "icall", },	// icall  -        ---- ---- ---- ----
		{ { 0x9409 }, { 0xFFFF }, S_INH, CTL_UndetBra | TAVRDisassembler::OCTL_STOP, "ijmp", },
																	// ijmp   -        ---- ---- ---- ----
		{ { 0x9508 }, { 0xFFFF }, S_INH, CTL_None | TAVRDisassembler::OCTL_HARDSTOP, "ret", },
																	// ret    -        ---- ---- ---- ----
		{ { 0x9518 }, { 0xFFFF }, S_INH, CTL_None |  TAVRDisassembler::OCTL_HARDSTOP, "reti", },
																	// reti   -        ---- ---- ---- ----

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
		{ { 0x95E8 }, { 0xFFFF }, S_INH, CTL_DataRegAddr | OCTL_ArchExtAddr, "spm", },
																	// spm    -        ---- ---- ---- ----		AVRe,AVRxm,AVRxt
		{ { 0x95F8 }, { 0xFFFF }, S_INH_Zp, CTL_DataRegAddr | OCTL_ArchExtAddr, "spm", },
																	// spm    Z+       ---- ---- ---- ----		AVRxm,AVRxt
		{ { 0x95A8 }, { 0xFFFF }, S_INH, CTL_None, "wdr", },		// wdr    -        ---- ---- ---- ----

		{ { 0x940B }, { 0xFF0F }, S_DES, CTL_None, "des", },		// des    K        ---- ---- KKKK ----  (K: 0-15)
	};

	for (size_t ndx = 0; ndx < _countof(arrOpcodes); ++ndx) {
		m_Opcodes.AddOpcode(arrOpcodes[ndx]);
	}

	m_bAllowMemRangeOverlap = true;

	// TODO : Allow for various CPU types and associated memory layouts.
	//		For now just use ATmega328PB
	m_Memory[MT_ROM].push_back(CMemBlock{ 0x0ul, 0x0ul, true, 0x8000ul, 0, DMEM_NOTLOADED });	// 32K of code memory available to processor as one block

	// Add Data Labels for memmap of registers:
	for (unsigned int nReg = 0; nReg < 32; ++nReg) {
		char buf[10];
		std::sprintf(buf, "R%u", nReg);
		AddLabel(MT_IO, nReg, false, 0, buf);
	}

	// TODO : Add I/O register names in memmap space (are they per CPU type?)
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

bool CAVRDisassembler::ReadNextObj(bool bTagMemory, std::ostream *msgFile, std::ostream *errFile)
{
	// Here, we'll read the next object code from memory and flag the memory as being
	//	code, unless an illegal opcode is encountered whereby it will be flagged as
	//	illegal code.  m_OpMemory is cleared and loaded with the memory bytes for the
	//	opcode that was processed.  m_CurrentOpcode will be also be set to the opcode
	//	that corresponds to the one found.  m_PC will also be incremented by the number
	//	of words in the complete opcode and in the case of an illegal opcode, it will
	//	increment only by 1 word so we can then test the following word to see if it
	//	is legal to allow us to get back on track.  It sets Skip-Opcode Flags
	//	appropriately.  This function will return True if the opcode was correct and
	//	properly formed and fully existed in memory.  It will return false if the
	//	opcode was illegal or if the opcode crossed over the bounds of loaded source memory.
	// If bTagMemory is false, then memory descriptors aren't changed!  This is useful
	//	if on-the-fly disassembly is desired without modifying the descriptors, such
	//	as in pass 2 of the disassembly process.

	const MEMORY_TYPE nMemType = MT_ROM;

	TAVRDisassembler::TOpcodeSymbol nFirstWord;

	TAddress nSavedPC;

	m_sFunctionalOpcode.clear();

	// Skip Opcode Examples:
	//		cpse r1,r0		<< m_bCurrentOpcodeIsSkip becomes true
	//		nop				<< (random non-skip opcode), m_bLastOpcodeWasSkip becomes true, m_bCurrentOpcodeIsSkip becomes false (Here we disable STOP! due to m_bLastOpcodeWasSkip)
	//		nop				<< (other non-skip opcode), m_bLastOpcodeWasSkip becomes false, m_bCurrentOpcodeIsSkip stays false [Implied inline function jump-to address]
	//
	//		cpse r1,r0		<< m_bCurrentOpcodeIsSkip becomes true
	//		cpse r2,r1		<< Another skip opcode, m_bLastOpcodeWasSkip becomes true, m_bCurrentOpcodeIsSkip stays true (He we disable STOP! due to m_bLastOpcodeWasSkip)
	//		nop				<< (random non-skip opcode), m_bLastOpcodeWasSkip becomes true, m_bCurrentOpcodeIsSkip becomes false (Here we disable STOP! due to m_bLastOpcodeWasSkip)
	//		nop				<< (other non-skip opcode), m_bLastOpcodeWasSkip becomes false, m_bCurrentOpcodeIsSkip stays false
	//
	m_bLastOpcodeWasSkip = m_bCurrentOpcodeIsSkip;
	m_bCurrentOpcodeIsSkip = false;		// Note: This will get set below if this new opcode is a skip, but set now to false in case we exit early

	assert(sizeof(nFirstWord) == 2);
	assert(sizeof(decltype(m_OpMemory)::value_type) == 2);
	assert(opcodeSymbolSize() == 2);
	nFirstWord = m_Memory[nMemType].element(m_PC) | (m_Memory[nMemType].element(m_PC+1) << 8);		// Opcodes are words and are little endian
	m_PC += opcodeSymbolSize();
	m_OpMemory.clear();
	if (!IsAddressLoaded(nMemType, m_PC-opcodeSymbolSize(), opcodeSymbolSize())) return false;

	bool bOpcodeFound = false;
	for (TOpcodeTable_type::const_iterator itrOpcode = m_Opcodes.cbegin(); (!bOpcodeFound && (itrOpcode != m_Opcodes.cend())); ++itrOpcode) {
		if (itrOpcode->opcode().at(0) != (nFirstWord & itrOpcode->opcodeMask().at(0))) continue;

		COpcodeSymbolArray_type arrOpMemory;
		arrOpMemory.push_back(nFirstWord);

		bool bMatch = true;
		for (COpcodeSymbolArray_type::size_type ndx = 1; (bMatch && (ndx < itrOpcode->opcode().size())); ++ndx) {
			TAVRDisassembler::TOpcodeSymbol nNextWord = m_Memory[nMemType].element(m_PC + (ndx-1)*opcodeSymbolSize()) |
														(m_Memory[nMemType].element(m_PC + (ndx-1)*opcodeSymbolSize() + 1) << 8);
			if (!IsAddressLoaded(nMemType, m_PC + (ndx-1)*opcodeSymbolSize(), opcodeSymbolSize())) {
				bMatch = false;
				continue;
			}
			if (itrOpcode->opcode().at(ndx) != (nNextWord & itrOpcode->opcodeMask().at(ndx))) {
				bMatch = false;
				continue;
			}
			arrOpMemory.push_back(nNextWord);
		}
		if (bMatch) {
			if (itrOpcode->matchFunc() && !itrOpcode->matchFunc()(*itrOpcode, arrOpMemory)) bMatch = false;
		}
		if (bMatch) {
			m_OpMemory = arrOpMemory;
			m_CurrentOpcode = *itrOpcode;
			bOpcodeFound = true;
		}
	}

	if (!bOpcodeFound) {
		if (bTagMemory) {
			for (size_t ndx = 0; ndx < opcodeSymbolSize(); ++ndx) {		// Must be a multiple of the opcode symbol size
				m_Memory[nMemType].setDescriptor(m_PC-opcodeSymbolSize()+ndx, DMEM_ILLEGALCODE);
			}
		}
		return false;
	}

	// If we get here, then we've found a valid matching opcode.  m_CurrentOpcode has been set to the opcode value
	//	and m_OpMemory has already been copied, tag the bytes in memory, increment m_PC, and call CompleteObjRead.
	//	If CompleteObjRead returns FALSE, then we'll have to undo everything and return flagging an invalid opcode.
	nSavedPC = m_PC;	// Remember m_PC in case we have to undo things (remember, m_PC has already been incremented by 2)!!
	m_PC += (m_OpMemory.size()-1)*opcodeSymbolSize();		// minus 1 because we already did the first symbol above

	if (!CompleteObjRead(true, msgFile, errFile))  {
		// Undo things here:
		m_OpMemory.clear();
		m_OpMemory.push_back(nFirstWord);		// Keep only the first word in OpMemory for illegal opcode id
		m_PC = nSavedPC;
		if (bTagMemory) {
			for (size_t ndx = 0; ndx < opcodeSymbolSize(); ++ndx) {		// Must be a multiple of the opcode symbol size
				m_Memory[nMemType].setDescriptor(m_PC-opcodeSymbolSize()+ndx, DMEM_ILLEGALCODE);
			}
		}
		return false;
	}


//	assert(CompleteObjRead(true, msgFile, errFile));


	nSavedPC -= opcodeSymbolSize();
	for (decltype(m_OpMemory)::size_type i=0; i<m_OpMemory.size(); ++i) {		// CompleteObjRead may add words to OpMemory, so we simply have to flag memory for that many bytes.  m_PC is already incremented by CompleteObjRead
		if (bTagMemory) {
			for (size_t ndx = 0; ndx < opcodeSymbolSize(); ++ndx) {		// Must be a multiple of the opcode symbol size
				m_Memory[nMemType].setDescriptor(nSavedPC+ndx, DMEM_CODE);
			}
		}
		nSavedPC += opcodeSymbolSize();
	}
	assert(nSavedPC == m_PC);		// If these aren't equal, something is wrong either here or in the CompleteObjRead!  m_PC should be incremented for every byte added to m_OpMemory by the complete routine!
	return true;
}

bool CAVRDisassembler::CompleteObjRead(bool bAddLabels, std::ostream *msgFile, std::ostream *errFile)
{
	// Procedure to finish reading any additional operand data from memory into m_OpMemory.
	//	Plus branch/labels and data/labels are generated (according to function).

	const MEMORY_TYPE nMemType = MT_ROM;

	char strTemp[10];

	m_nStartPC = m_PC - (m_OpMemory.size() * opcodeSymbolSize());		// Get PC of first byte of this instruction for branch references

	// Note: We'll start with the absolute address.  Since we don't know what the relative address is
	//		to the start of the function, then the function outputting function (which should know it),
	//		will have to add it in:

	std::sprintf(strTemp, "%04X|", m_nStartPC);
	m_sFunctionalOpcode = strTemp;

	// Add reference labels to this opcode to the function:
	CLabelTableMap::const_iterator itrLabel = m_LabelTable[nMemType].find(m_nStartPC);
	if (itrLabel != m_LabelTable[nMemType].cend()) {
		for (CLabelArray::size_type i=0; i<itrLabel->second.size(); ++i) {
			if (i != 0) m_sFunctionalOpcode += ",";
			m_sFunctionalOpcode += FormatLabel(nMemType, LC_REF, itrLabel->second.at(i), m_nStartPC);
		}
	}

	m_sFunctionalOpcode += "|";

	// All words of OpMemory:
	for (decltype(m_OpMemory)::size_type i=0; i<m_OpMemory.size(); ++i) {
		std::sprintf(strTemp, "%04X", m_OpMemory.at(i));
		m_sFunctionalOpcode += strTemp;
	}

	m_sFunctionalOpcode += "|";

	// All words of opcode part of OpMemory (will always be full OpMemory on RISC);
	for (decltype(m_OpMemory)::size_type i=0; i<m_OpMemory.size(); ++i) {
		std::sprintf(strTemp, "%04X", m_OpMemory.at(i));
		m_sFunctionalOpcode += strTemp;
	}

	m_sFunctionalOpcode += "|";

	// All bytes of operand part of OpMemory (will always be empty on RISC);

	m_sFunctionalOpcode += "|";

	// Decode Operands:
	if (!DecodeOpcode(bAddLabels, msgFile, errFile)) return false;

	m_sFunctionalOpcode += "|";
	m_sFunctionalOpcode += FormatMnemonic(nMemType, MC_OPCODE, m_nStartPC);
	m_sFunctionalOpcode += "|";
	m_sFunctionalOpcode += FormatOperands(nMemType, MC_OPCODE, m_nStartPC);

//
// This case is now covered in DecodeOpcode??
//
//	// See if this is the end of a function.  Note: These preempt all previously
//	//		decoded function tags -- such as call, etc:
//	if (m_CurrentOpcode.control() & TAVRDisassembler::OCTL_HARDSTOP) {
//		m_FunctionsTable[m_nStartPC] = FUNCF_HARDSTOP;
//	}

	return true;
}

bool CAVRDisassembler::CurrentOpcodeIsStop() const
{
	return (!m_bLastOpcodeWasSkip &&		// Note: Opcode being skipped can't be a stop
			((m_CurrentOpcode.control() & (TAVRDisassembler::OCTL_STOP | TAVRDisassembler::OCTL_HARDSTOP)) != 0));
}

// ----------------------------------------------------------------------------

bool CAVRDisassembler::RetrieveIndirect(std::ostream *msgFile, std::ostream *errFile)
{
	UNUSED(msgFile);
	UNUSED(errFile);

	const MEMORY_TYPE nMemType = MT_ROM;

	MEM_DESC b1d, b2d;

	m_OpMemory.clear();
	m_OpMemory.push_back(m_Memory[nMemType].element(m_PC));
	m_OpMemory.push_back(m_Memory[nMemType].element(m_PC+1));
	// We'll assume all indirects are little endian and are 2-bytes in length,
	//	though someday we may need support for 22-bit addresses.
	b1d = static_cast<MEM_DESC>(m_Memory[nMemType].descriptor(m_PC));
	b2d = static_cast<MEM_DESC>(m_Memory[nMemType].descriptor(m_PC+1));
	if (((b1d != DMEM_CODEINDIRECT) && (b1d != DMEM_DATAINDIRECT)) ||
		((b2d != DMEM_CODEINDIRECT) && (b2d != DMEM_DATAINDIRECT))) {
		m_PC += 2;
		return false;			// Memory should be tagged as indirect!
	}
	if (b1d != b2d) {
		m_PC += 2;
		return false;			// Memory should be tagged as a correct pair!
	}

	if (((b1d == DMEM_CODEINDIRECT) && (!m_CodeIndirectTable.contains(m_PC))) ||
		((b1d == DMEM_DATAINDIRECT) && (!m_DataIndirectTable.contains(m_PC)))) {
		m_PC += 2;
		return false;			// Location must exist in one of the two tables!
	}
	m_PC += 2;
	return true;
}

// ----------------------------------------------------------------------------

std::string CAVRDisassembler::FormatOpBytes(MEMORY_TYPE nMemoryType, MNEMONIC_CODE nMCCode, TAddress nStartAddress)
{
	UNUSED(nMemoryType);
	UNUSED(nStartAddress);

	std::ostringstream sstrTemp;

	for (decltype(m_OpMemory)::size_type i=0; i<m_OpMemory.size(); ++i) {
		if (i) sstrTemp << " ";
		if (nMCCode != MC_OPCODE) {		// MC_OPCODE will be words on AVR, everything else will be bytes here:
			sstrTemp << std::uppercase << std::setfill('0') << std::setw(2) << std::setbase(16) << static_cast<unsigned int>(m_OpMemory.at(i))
						<< std::nouppercase << std::setbase(0);
		} else {
			sstrTemp << std::uppercase << std::setfill('0') << std::setw(2) << std::setbase(16) << static_cast<unsigned int>(m_OpMemory.at(i) & 0xFF)
						<< std::nouppercase << std::setbase(0);

			sstrTemp << " ";

			sstrTemp << std::uppercase << std::setfill('0') << std::setw(2) << std::setbase(16) << static_cast<unsigned int>((m_OpMemory.at(i) >> 8) & 0xFF)
						<< std::nouppercase << std::setbase(0);
		}
	}
	return sstrTemp.str();
}

std::string CAVRDisassembler::FormatMnemonic(MEMORY_TYPE nMemoryType, MNEMONIC_CODE nMCCode, TAddress nStartAddress)
{
	UNUSED(nMemoryType);
	UNUSED(nStartAddress);

	switch (nMCCode) {
		case MC_OPCODE:
			return m_CurrentOpcode.mnemonic();
		case MC_ILLOP:
			return "???";
		case MC_EQUATE:
			return "=";
		case MC_DATABYTE:
			return ".byte";
		case MC_ASCII:
			return ".ascii";
		case MC_INDIRECT:
			return ".word";
		default:
			assert(false);
			break;
	}
	return "???";
}

std::string CAVRDisassembler::FormatOperands(MEMORY_TYPE nMemoryType, MNEMONIC_CODE nMCCode, TAddress nStartAddress)
{
	std::string strOpStr;
	char strTemp[30];
	TAddress nAddress;

	switch (nMCCode) {
		case MC_ILLOP:			// Note: Illop should always be multiples of 2-bytes here
			assert((m_OpMemory.size() % 2) == 0);
			// Unlike real opcode OpMemory symbols, these will always be bytes
			for (decltype(m_OpMemory)::size_type i=0; i<m_OpMemory.size(); i+=2) {
				if (i != 0) strOpStr += ",";
				std::sprintf(strTemp, "%s%02X%02X", GetHexDelim().c_str(), m_OpMemory.at(i+1), m_OpMemory.at(i));	// Little-endian
				strOpStr += strTemp;
			}
			break;
		case MC_DATABYTE:
			// Unlike real opcode OpMemory symbols, these will always be bytes
			for (decltype(m_OpMemory)::size_type i=0; i<m_OpMemory.size(); ++i) {
				if (i != 0) strOpStr += ",";
				std::sprintf(strTemp, "%s%02X", GetHexDelim().c_str(), m_OpMemory.at(i));
				strOpStr += strTemp;
			}
			break;
		case MC_ASCII:
			strOpStr += DataDelim;
			// Unlike real opcode OpMemory symbols, these will always be bytes
			for (decltype(m_OpMemory)::size_type i=0; i<m_OpMemory.size(); ++i) {
				std::sprintf(strTemp, "%c", static_cast<char>(m_OpMemory.at(i)));
				strOpStr += strTemp;
			}
			strOpStr += DataDelim;
			break;
		case MC_EQUATE:
			std::sprintf(strTemp, "%s%04X", GetHexDelim().c_str(), nStartAddress);
			strOpStr = strTemp;
			break;
		case MC_INDIRECT:
		{
			// While AVR doesn't have any inherent indirects, there can
			//	be indirect addresses loaded via commands like LDS and
			//	used with an indirect jump opcode.  So we need this to
			//	process user-found indirects:
			// Will we ever need support for 22-bit indirects?
			if (m_OpMemory.size() != 2) {
				assert(false);			// Check code and make sure RetrieveIndirect got called and called correctly!
				break;
			}
			// Unlike real opcode OpMemory symbols, these will always be bytes
			nAddress = m_OpMemory.at(0)+m_OpMemory.at(1)*256;	// Little Endian, not big
			std::string strLabel;
			CAddressLabelMap::const_iterator itrLabel;
			if ((itrLabel = m_CodeIndirectTable.find(nStartAddress)) != m_CodeIndirectTable.cend()) {
				strLabel = itrLabel->second;
			} else if ((itrLabel = m_DataIndirectTable.find(nStartAddress)) != m_DataIndirectTable.cend()) {
				strLabel = itrLabel->second;
			}
			strOpStr = FormatLabel(nMemoryType, LC_REF, strLabel, nAddress);
			break;
		}
		case MC_OPCODE:
			m_nStartPC = nStartAddress;		// Get PC of first byte of this instruction for branch references
			strOpStr = CreateOperand();
			break;
		default:
			assert(false);
			break;
	}

	return strOpStr;
}

std::string CAVRDisassembler::FormatComments(MEMORY_TYPE nMemoryType, MNEMONIC_CODE nMCCode, TAddress nStartAddress)
{
	std::string strRetVal;
	bool bBranchOutside;
	TAddress nAddress;

	using namespace TAVRDisassembler_ENUMS;

	// Add user comments:
	strRetVal = FormatUserComments(nMemoryType, nMCCode, nStartAddress);
	if (nMCCode == MC_OPCODE) {
		std::string strRefComment = FormatOperandRefComments();
		if (!strRefComment.empty()) {
			if (!strRetVal.empty()) strRetVal += "\n";
			strRetVal += strRefComment;
		}
	}

	// Handle Undetermined Branch:
	switch (nMCCode) {
		case MC_OPCODE:
			if ((m_CurrentOpcode.control() & CTL_MASK) == CTL_UndetBra) {
				if (!strRetVal.empty()) strRetVal += "\n";
				strRetVal = "Undetermined Branch Address";
			}
			break;
		default:
			break;
	}

	// Handle out-of-source branch:
	bBranchOutside = false;
	switch (nMCCode) {
		case MC_OPCODE:
			if (((m_CurrentOpcode.control() & CTL_MASK) == CTL_DetBra)
// Note: We can't do check of skip here because we don't know the size of the next opcode:
//				|| ((m_CurrentOpcode.control() & CTL_MASK) == CTL_Skip)
//				|| ((m_CurrentOpcode.control() & CTL_MASK) == CTL_SkipIOLabel)
				) {
				if (!CheckBranchAddressLoaded(CreateOperandAddress())) bBranchOutside = true;
			}
			break;
		case MC_INDIRECT:
			// While AVR doesn't have any inherent indirects, there can
			//	be indirect addresses loaded via commands like LDS and
			//	used with an indirect jump opcode.  So we need this to
			//	process user-found indirects:
			// Will we ever need support for 22-bit indirects?
			if (m_OpMemory.size() != 2) {
				assert(false);			// Check code and make sure RetrieveIndirect got called and called correctly!
				break;
			}
			// Unlike real opcode OpMemory symbols, these will always be bytes
			nAddress = m_OpMemory.at(0)+m_OpMemory.at(1)*256;	// Little Endian, not big
			if (m_CodeIndirectTable.contains(nStartAddress)) {
				if (!CheckBranchAddressLoaded(nAddress)) bBranchOutside = true;
			}
			break;
		default:
			break;
	}

	if (bBranchOutside) {
		if (!strRetVal.empty()) strRetVal += "\n";
		strRetVal += "Branch Outside Loaded Source(s)";
	}

	// Add general reference stuff:
	if (!strRetVal.empty()) strRetVal += "\n";
	strRetVal += FormatReferences(nMemoryType, nMCCode, nStartAddress);		// Add references

	return strRetVal;
}

// ----------------------------------------------------------------------------

std::string CAVRDisassembler::FormatLabel(MEMORY_TYPE nMemoryType, LABEL_CODE nLC, const TLabel & strLabel, TAddress nAddress)
{
	std::string strTemp = CDisassembler::FormatLabel(nMemoryType, nLC, strLabel, nAddress);	// Call parent

	switch (nLC) {
		case LC_EQUATE:
			strTemp += LbleDelim;
			break;
		case LC_DATA:
		case LC_CODE:
			strTemp += LblcDelim;
			break;
		default:
			break;
	}
	return strTemp;
}

// ----------------------------------------------------------------------------

bool CAVRDisassembler::WritePreSection(std::ostream& outFile, std::ostream *msgFile, std::ostream *errFile)
{
	UNUSED(msgFile);
	UNUSED(errFile);

	CStringArray saOutLine;
	char strTemp[30];

	ClearOutputLine(saOutLine);
	saOutLine[FC_ADDRESS] = FormatAddress(m_PC);
	saOutLine[FC_MNEMONIC] = ".area";
	std::sprintf(strTemp, "CODE%d\t(ABS)", ++m_nSectionCount);
	saOutLine[FC_OPERANDS] = strTemp;
	outFile << MakeOutputLine(saOutLine) << "\n";

	saOutLine[FC_MNEMONIC] = ".org";
	std::sprintf(strTemp, "%s%04X", GetHexDelim().c_str(), m_PC);
	saOutLine[FC_OPERANDS] = strTemp;
	outFile << MakeOutputLine(saOutLine) << "\n";

	saOutLine[FC_MNEMONIC].clear();
	saOutLine[FC_OPERANDS].clear();
	outFile << MakeOutputLine(saOutLine) << "\n";
	return true;
}

// ----------------------------------------------------------------------------

bool CAVRDisassembler::ResolveIndirect(TAddress nAddress, TAddress& nResAddress, REFERENCE_TYPE nType)
{
	const MEMORY_TYPE nMemType = MT_ROM;

	// We will assume that all indirect addresses are 2-bytes in length
	//	and stored in little endian format.
	if (!IsAddressLoaded(nMemType, nAddress, 2) ||				// Not only must it be loaded, but we must have never examined it before!
		(m_Memory[nMemType].descriptor(nAddress) != DMEM_LOADED) ||
		(m_Memory[nMemType].descriptor(nAddress+1) != DMEM_LOADED)) {
		nResAddress = 0;
		return false;
	}
	TAddress nVector = m_Memory[nMemType].element(nAddress) + m_Memory[nMemType].element(nAddress + 1) * 256ul;
	nResAddress = nVector;
	m_Memory[nMemType].setDescriptor(nAddress, static_cast<TDescElement>(DMEM_CODEINDIRECT) + nType);		// Flag the addresses as being the proper vector type
	m_Memory[nMemType].setDescriptor(nAddress+1, static_cast<TDescElement>(DMEM_CODEINDIRECT) + nType);
	return true;
}

// ----------------------------------------------------------------------------

std::string CAVRDisassembler::GetExcludedPrintChars() const
{
	return "\\" DataDelim;
}

std::string CAVRDisassembler::GetHexDelim() const
{
	return HexDelim;
}

std::string CAVRDisassembler::GetCommentStartDelim() const
{
	return ComDelimS;
}

std::string CAVRDisassembler::GetCommentEndDelim() const
{
	return ComDelimE;
}

// ----------------------------------------------------------------------------

void CAVRDisassembler::clearOpMemory()
{
	m_OpMemory.clear();
}

size_t CAVRDisassembler::opcodeSymbolSize() const
{
	return sizeof(TAVRDisassembler::TOpcodeSymbol);
}

void CAVRDisassembler::pushBackOpMemory(TAddress nLogicalAddress, TMemoryElement nValue)
{
	UNUSED(nLogicalAddress);
	m_OpMemory.push_back(nValue);
}

void CAVRDisassembler::pushBackOpMemory(TAddress nLogicalAddress, const CMemoryArray &maValues)
{
	UNUSED(nLogicalAddress);
	m_OpMemory.insert(m_OpMemory.end(), maValues.cbegin(), maValues.cend());
}

// ----------------------------------------------------------------------------

unsigned int CAVRDisassembler::opDstReg0_31(const TAVRDisassembler::COpcodeSymbolArray &arrOpMemory)
{
	// Rd       ---- ---d dddd ---- (d: 0-31)

	assert(arrOpMemory.size() >= 1);
	return ((arrOpMemory.at(0) >> 4) & 0x1F);
}

unsigned int CAVRDisassembler::opDstReg16_31(const TAVRDisassembler::COpcodeSymbolArray &arrOpMemory)
{
	// Rd       ---- ---- dddd ---- (d: 16-31)

	assert(arrOpMemory.size() >= 1);
	return (((arrOpMemory.at(0) >> 4) | 0x10) & 0x1F);
}

unsigned int CAVRDisassembler::opDstReg16_23(const TAVRDisassembler::COpcodeSymbolArray &arrOpMemory)
{
	// Rd       ---- ---- -ddd ---- (d: 16-23)

	assert(arrOpMemory.size() >= 1);
	return (((arrOpMemory.at(0) >> 4) | 0x10) & 0x17);
}

unsigned int CAVRDisassembler::opDstRegEven0_30(const TAVRDisassembler::COpcodeSymbolArray &arrOpMemory)
{
	// Rd       ---- ---- dddd ---- (d: 0,2,4,...,30)

	assert(arrOpMemory.size() >= 1);
	return ((arrOpMemory.at(0) >> 3) & 0x1E);
}

unsigned int CAVRDisassembler::opDstRegEven24_30(const TAVRDisassembler::COpcodeSymbolArray &arrOpMemory)
{
	// Rd       ---- ---- --dd ---- (d: 24, 26, 28, 30)

	assert(arrOpMemory.size() >= 1);
	return (((arrOpMemory.at(0) >> 3) | 0x18) & 0x1E);
}

unsigned int CAVRDisassembler::opSrcReg0_31(const TAVRDisassembler::COpcodeSymbolArray &arrOpMemory)
{
	// Rr       ---- --r- ---- rrrr (r: 0-31)

	assert(arrOpMemory.size() >= 1);
	return ((arrOpMemory.at(0) & 0x0200) >> 5) | (arrOpMemory.at(0) & 0x000F);
}

unsigned int CAVRDisassembler::opSrcReg16_31(const TAVRDisassembler::COpcodeSymbolArray &arrOpMemory)
{
	// Rr       ---- ---- ---- rrrr (r: 16-31)

	assert(arrOpMemory.size() >= 1);
	return ((arrOpMemory.at(0) | 0x10) & 0x1F);
}

unsigned int CAVRDisassembler::opSrcReg16_23(const TAVRDisassembler::COpcodeSymbolArray &arrOpMemory)
{
	// Rr       ---- ---- ---- -rrr (r: 16-23)

	assert(arrOpMemory.size() >= 1);
	return ((arrOpMemory.at(0) | 0x10) & 0x17);
}

unsigned int CAVRDisassembler::opSrcRegEven0_30(const TAVRDisassembler::COpcodeSymbolArray &arrOpMemory)
{
	// Rr       ---- ---- ---- rrrr (r: 0,2,4,...,30)

	assert(arrOpMemory.size() >= 1);
	return ((arrOpMemory.at(0) << 1) & 0x1E);
}

uint8_t CAVRDisassembler::opIByte(const TAVRDisassembler::COpcodeSymbolArray &arrOpMemory)
{
	// K        ---- KKKK ---- KKKK (K: 0-255)

	assert(arrOpMemory.size() >= 1);
	return ((arrOpMemory.at(0) & 0x0F00) >> 4) | (arrOpMemory.at(0) & 0x000F);
}

uint8_t CAVRDisassembler::opIWord(const TAVRDisassembler::COpcodeSymbolArray &arrOpMemory)
{
	// K        ---- ---- KK-- KKKK (K: 0-63)

	assert(arrOpMemory.size() >= 1);
	return ((arrOpMemory.at(0) & 0x00C0) >> 2) | (arrOpMemory.at(0) & 0x000F);
}

unsigned int CAVRDisassembler::opBit(const TAVRDisassembler::COpcodeSymbolArray &arrOpMemory)
{
	// b        ---- ---- ---- -bbb (b: 0-7)

	assert(arrOpMemory.size() >= 1);
	return (arrOpMemory.at(0) & 0x07);
}

TAddress CAVRDisassembler::opIO0_31(const TAVRDisassembler::COpcodeSymbolArray &arrOpMemory)
{
	// A        ---- ---- AAAA A--- (A: 0-31)

	assert(arrOpMemory.size() >= 1);
	return ((arrOpMemory.at(0) >> 3) & 0x1F);
}

TAddress CAVRDisassembler::opIO0_63(const TAVRDisassembler::COpcodeSymbolArray &arrOpMemory)
{
	// A        ---- -AA- ---- AAAA (A: 0-63)

	assert(arrOpMemory.size() >= 1);
	return ((arrOpMemory.at(0) & 0x0600) >> 5) | (arrOpMemory.at(0) & 0x000F);
}

TAddressOffset CAVRDisassembler::opCRel7bit(const TAVRDisassembler::COpcodeSymbolArray &arrOpMemory)
{
	// k        ---- --kk kkkk k--- (k: -64 - 63)	NOTE: 'k' is opcode WORDS not bytes of memory!

	assert(arrOpMemory.size() >= 1);
	TAVRDisassembler::TOpcodeSymbol nValue = ((arrOpMemory.at(0) >> 2) & 0x00FE);
	if (nValue & 0x80) {
		TAddressOffset nOffset = -nValue;
		nValue = nOffset &  0xFF;
		nOffset = -nValue;
		return nOffset;		// Sign extend if negative
	}
	return nValue;
}

TAddressOffset CAVRDisassembler::opCRel12bit(const TAVRDisassembler::COpcodeSymbolArray &arrOpMemory)
{
	// k        ---- kkkk kkkk kkkk (k: -2k - 2k), 12-bit relative, NOTE: 'k' is opcode WORDS not bytes of memory!

	assert(arrOpMemory.size() >= 1);
	TAVRDisassembler::TOpcodeSymbol nValue = (arrOpMemory.at(0) & 0x0FFF) << 1;
	if (nValue & 0x1000) {
		TAddressOffset nOffset = -nValue;
		nValue = nOffset &  0x1FFF;
		nOffset = -nValue;
		return nOffset;		// Sign extend if negative
	}
	return nValue;
}

TAddress CAVRDisassembler::opCAbs22bit(const TAVRDisassembler::COpcodeSymbolArray &arrOpMemory)
{
	// k        ---- ---k kkkk ---k | kkkk kkkk kkkk kkkk (k: 0-64k, 0-4M), 16/22-bit absolute, NOTE: 'k' is opcode WORDS not bytes of memory!

	assert(arrOpMemory.size() >= 2);
	TAddress nAddress = ((arrOpMemory.at(0) & 0x01F0) >> 3) | (arrOpMemory.at(0) & 0x0001);	// Upper 6-bits
	nAddress = nAddress << 16;
	nAddress |= arrOpMemory.at(1);	// Lower 16-bits
	return (nAddress << 1);
}

TAddress CAVRDisassembler::opDAbs16bit(const TAVRDisassembler::COpcodeSymbolArray &arrOpMemory)
{
	// k        ---- ---- ---- ---- | kkkk kkkk kkkk kkkk (k: 0-64k), NOTE: 'k' is DATA BYTES of memory!

	assert(arrOpMemory.size() >= 2);
	return arrOpMemory.at(1);
}

TAddress CAVRDisassembler::opLDSrc7bit(const TAVRDisassembler::COpcodeSymbolArray &arrOpMemory)
{
	// k        ---- -kkk ---- kkkk (k: 0-127), NOTE: 'k' is DATA BYTES of memory!

	assert(arrOpMemory.size() >= 1);
	return (((arrOpMemory.at(0) & 0x0700) >> 4) | (arrOpMemory.at(0) & 0x000F));
}

unsigned int CAVRDisassembler::opRegYZqval(const TAVRDisassembler::COpcodeSymbolArray &arrOpMemory)
{
	// Y+q      --q- qq-- ---- -qqq (q: 0-63)

	assert(arrOpMemory.size() >= 1);
	return (((arrOpMemory.at(0) & 0x2000) >> 8) |
			((arrOpMemory.at(0) & 0x0C00) >> 7) |
			(arrOpMemory.at(0) & 0x0007));
}

unsigned int CAVRDisassembler::opDESK(const TAVRDisassembler::COpcodeSymbolArray &arrOpMemory)
{
	// K        ---- ---- KKKK ----  (K: 0-15)

	assert(arrOpMemory.size() >= 1);
	return ((arrOpMemory.at(0) & 0x00F0) >> 4);
}

// ----------------------------------------------------------------------------

bool CAVRDisassembler::DecodeOpcode(bool bAddLabels, std::ostream *msgFile, std::ostream *errFile)
{
	using namespace TAVRDisassembler_ENUMS;

	bool bRetVal = true;

	char strDstTemp[40];
	char strSrcTemp[30];
	TAddress nTargetAddr = -1;
	bool bAddFunctionBranchRef = false;

	strDstTemp[0] = 0;
	strSrcTemp[0] = 0;

	switch (m_CurrentOpcode.group()) {
		case S_IBYTE:		// Immediate 8-bit data -- no label
			std::sprintf(strDstTemp, "R%u", opDstReg16_31(m_OpMemory));
			std::sprintf(strSrcTemp, "#%02X", static_cast<unsigned int>(opIByte(m_OpMemory)));
			break;

		case S_IWORD:		// Immediate 6-bit data -- no label
			std::sprintf(strDstTemp, "R%u", opDstRegEven24_30(m_OpMemory));
			std::sprintf(strSrcTemp, "#%02X", static_cast<unsigned int>(opIWord(m_OpMemory)));
			break;

		case S_SNGL:		// Single Register -- no label
			std::sprintf(strDstTemp, "R%u", opDstReg0_31(m_OpMemory));
			break;

		case S_SAME:		// Single Register (Same Double Register) -- no label
			std::sprintf(strDstTemp, "R%u", opDstReg0_31(m_OpMemory));
			assert(opDstReg0_31(m_OpMemory) == opSrcReg0_31(m_OpMemory));
			break;

		case S_DUBL:		// Double Register -- no label
			std::sprintf(strDstTemp, "R%u", opDstReg0_31(m_OpMemory));
			std::sprintf(strSrcTemp, "R%u", opSrcReg0_31(m_OpMemory));
			break;

		case S_MOVW:		// Double Word Registers -- no label
			std::sprintf(strDstTemp, "R%u", opDstRegEven0_30(m_OpMemory));
			std::sprintf(strSrcTemp, "R%u", opSrcRegEven0_30(m_OpMemory));
			break;

		case S_MULS:		// Double High Registers -- no label
			std::sprintf(strDstTemp, "R%u", opDstReg16_31(m_OpMemory));
			std::sprintf(strSrcTemp, "R%u", opSrcReg16_31(m_OpMemory));
			break;

		case S_FMUL:		// Double Middle-High Registers -- no label
			std::sprintf(strDstTemp, "R%u", opDstReg16_23(m_OpMemory));
			std::sprintf(strSrcTemp, "R%u", opSrcReg16_23(m_OpMemory));
			break;

		case S_SER:			// Single High Register -- no label
			std::sprintf(strDstTemp, "R%u", opDstReg16_31(m_OpMemory));
			break;

		case S_TFLG:		// Single Register and bit -- no label
			std::sprintf(strDstTemp, "R%u,%u", opDstReg0_31(m_OpMemory), opBit(m_OpMemory));
			break;

		case S_BRA:			// 7-bit Relative Code Address
			assert((m_CurrentOpcode.control() & CTL_MASK) == CTL_DetBra);
			if (bAddLabels) GenAddrLabel(m_PC + opCRel7bit(m_OpMemory), m_nStartPC, TLabel(), msgFile, errFile);
			std::sprintf(strDstTemp, "C^%c%02lX(%06lX)",
									((opCRel7bit(m_OpMemory) != labs(opCRel7bit(m_OpMemory))) ? '-' : '+'),
									labs(opCRel7bit(m_OpMemory)),
									static_cast<unsigned long>(m_PC+opCRel7bit(m_OpMemory)));
			nTargetAddr = m_PC + opCRel7bit(m_OpMemory);
			bAddFunctionBranchRef = true;
			break;

		case S_JMP:			// 22-bit Absolute Code Address
			assert((m_CurrentOpcode.control() & CTL_MASK) == CTL_DetBra);
			if (bAddLabels) GenAddrLabel(opCAbs22bit(m_OpMemory), m_nStartPC, TLabel(), msgFile, errFile);
			std::sprintf(strDstTemp, "C@%06lX", static_cast<unsigned long>(opCAbs22bit(m_OpMemory)));
			nTargetAddr = opCAbs22bit(m_OpMemory);
			bAddFunctionBranchRef = true;
			break;

		case S_RJMP:		// 12-bit Relative Code Address
			assert((m_CurrentOpcode.control() & CTL_MASK) == CTL_DetBra);
			if (bAddLabels) GenAddrLabel(m_PC + opCRel12bit(m_OpMemory), m_nStartPC, TLabel(), msgFile, errFile);
			std::sprintf(strDstTemp, "C^%c%04lX(%06lX)",
									((opCRel12bit(m_OpMemory) != labs(opCRel12bit(m_OpMemory))) ? '-' : '+'),
									labs(opCRel12bit(m_OpMemory)),
									static_cast<unsigned long>(m_PC+opCRel12bit(m_OpMemory)));
			nTargetAddr = m_PC + opCRel12bit(m_OpMemory);
			bAddFunctionBranchRef = true;
			break;

		case S_IOR:			// 5-bit Absolute I/O Address and bit
			// Note: Memmap Offset for I/O addresses is +0x20 to skip past registers:
			if (bAddLabels) GenDataLabel(MemTypeIORAM(opIO0_31(m_OpMemory) + 0x20), opIO0_31(m_OpMemory) + 0x20, m_nStartPC, TLabel(), msgFile, errFile);
			std::sprintf(strDstTemp, "D@%04X,%u", opIO0_31(m_OpMemory) + 0x20, opBit(m_OpMemory));
			break;

		case S_IN:			// 6-bit Absolute I/O Address and Register
			// Note: Memmap Offset for I/O addresses is +0x20 to skip past registers:
			if (bAddLabels) GenDataLabel(MemTypeIORAM(opIO0_63(m_OpMemory) + 0x20), opIO0_63(m_OpMemory) + 0x20, m_nStartPC, TLabel(), msgFile, errFile);
			std::sprintf(strDstTemp, "R%u", opDstReg0_31(m_OpMemory));
			std::sprintf(strSrcTemp, "D@%04X", opIO0_63(m_OpMemory) + 0x20);
			break;

		case S_OUT:			// 6-bit Absolute I/O Address and Register
			// Note: Memmap Offset for I/O addresses is +0x20 to skip past registers:
			if (bAddLabels) GenDataLabel(MemTypeIORAM(opIO0_63(m_OpMemory) + 0x20), opIO0_63(m_OpMemory) + 0x20, m_nStartPC, TLabel(), msgFile, errFile);
			std::sprintf(strDstTemp, "D@%04X", opIO0_63(m_OpMemory) + 0x20);
			std::sprintf(strSrcTemp, "R%u", opDstReg0_31(m_OpMemory));	// SrcReg uses DstReg slot here
			break;

		case S_SNGL_X:		// Single Register and X Register -- no label
			std::sprintf(strDstTemp, "R%u", opDstReg0_31(m_OpMemory));
			std::sprintf(strSrcTemp, "X");
			break;

		case S_SNGL_Xp:		// Single Register and X Register -- no label
			std::sprintf(strDstTemp, "R%u", opDstReg0_31(m_OpMemory));
			std::sprintf(strSrcTemp, "X+");
			break;

		case S_SNGL_nX:		// Single Register and X Register -- no label
			std::sprintf(strDstTemp, "R%u", opDstReg0_31(m_OpMemory));
			std::sprintf(strSrcTemp, "-X");
			break;

		case S_SNGL_Y:		// Single Register and Y Register -- no label
			std::sprintf(strDstTemp, "R%u", opDstReg0_31(m_OpMemory));
			std::sprintf(strSrcTemp, "Y");
			break;

		case S_SNGL_Yp:		// Single Register and Y Register -- no label
			std::sprintf(strDstTemp, "R%u", opDstReg0_31(m_OpMemory));
			std::sprintf(strSrcTemp, "Y+");
			break;

		case S_SNGL_nY:		// Single Register and Y Register -- no label
			std::sprintf(strDstTemp, "R%u", opDstReg0_31(m_OpMemory));
			std::sprintf(strSrcTemp, "-Y");
			break;

		case S_SNGL_Z:		// Single Register and Z Register -- no label
			std::sprintf(strDstTemp, "R%u", opDstReg0_31(m_OpMemory));
			if ((m_CurrentOpcode.control() & CTL_MASK) == CTL_DataRegAddr) {
				if (m_CurrentOpcode.control() & OCTL_ArchExtAddr) {
					std::sprintf(strSrcTemp, "D&00(RAMPZ,Z)");		// Special Case for elpm
				} else {
					std::sprintf(strSrcTemp, "D&00(Z)");			// Special Case for elpm
				}
			} else {
				std::sprintf(strSrcTemp, "Z");
			}
			break;

		case S_SNGL_Zp:		// Single Register and Z Register -- no label
			std::sprintf(strDstTemp, "R%u", opDstReg0_31(m_OpMemory));
			if ((m_CurrentOpcode.control() & CTL_MASK) == CTL_DataRegAddr) {
				if (m_CurrentOpcode.control() & OCTL_ArchExtAddr) {
					std::sprintf(strSrcTemp, "D&00(RAMPZ,Z+)");		// Special Case for elpm
				} else {
					std::sprintf(strSrcTemp, "D&00(Z+)");			// Special Case for elpm
				}
			} else {
				std::sprintf(strSrcTemp, "Z+");
			}
			break;

		case S_SNGL_nZ:		// Single Register and Z Register -- no label
			std::sprintf(strDstTemp, "R%u", opDstReg0_31(m_OpMemory));
			std::sprintf(strSrcTemp, "-Z");
			break;

		case S_X_SNGL:		// Single Register and X Register -- no label
			std::sprintf(strDstTemp, "X");
			std::sprintf(strSrcTemp, "R%u", opDstReg0_31(m_OpMemory));	// SrcReg uses DstReg slot here
			break;

		case S_Xp_SNGL:		// Single Register and X Register -- no label
			std::sprintf(strDstTemp, "X+");
			std::sprintf(strSrcTemp, "R%u", opDstReg0_31(m_OpMemory));	// SrcReg uses DstReg slot here
			break;

		case S_nX_SNGL:		// Single Register and X Register -- no label
			std::sprintf(strDstTemp, "-X");
			std::sprintf(strSrcTemp, "R%u", opDstReg0_31(m_OpMemory));	// SrcReg uses DstReg slot here
			break;

		case S_Y_SNGL:		// Single Register and Y Register -- no label
			std::sprintf(strDstTemp, "Y");
			std::sprintf(strSrcTemp, "R%u", opDstReg0_31(m_OpMemory));	// SrcReg uses DstReg slot here
			break;

		case S_Yp_SNGL:		// Single Register and Y Register -- no label
			std::sprintf(strDstTemp, "Y+");
			std::sprintf(strSrcTemp, "R%u", opDstReg0_31(m_OpMemory));	// SrcReg uses DstReg slot here
			break;

		case S_nY_SNGL:		// Single Register and Y Register -- no label
			std::sprintf(strDstTemp, "-Y");
			std::sprintf(strSrcTemp, "R%u", opDstReg0_31(m_OpMemory));	// SrcReg uses DstReg slot here
			break;

		case S_Z_SNGL:		// Single Register and Z Register -- no label
			std::sprintf(strDstTemp, "Z");
			std::sprintf(strSrcTemp, "R%u", opDstReg0_31(m_OpMemory));	// SrcReg uses DstReg slot here
			break;

		case S_Zp_SNGL:		// Single Register and Z Register -- no label
			std::sprintf(strDstTemp, "Z+");
			std::sprintf(strSrcTemp, "R%u", opDstReg0_31(m_OpMemory));	// SrcReg uses DstReg slot here
			break;

		case S_nZ_SNGL:		// Single Register and Z Register -- no label
			std::sprintf(strDstTemp, "-Z");
			std::sprintf(strSrcTemp, "R%u", opDstReg0_31(m_OpMemory));	// SrcReg uses DstReg slot here
			break;

		case S_SNGL_Yq:		// Single Register and Y Register with Offset -- no label
			std::sprintf(strDstTemp, "R%u", opDstReg0_31(m_OpMemory));
			std::sprintf(strSrcTemp, "Y+%u", opRegYZqval(m_OpMemory));
			break;

		case S_SNGL_Zq:		// Single Register and Z Register with Offset -- no label
			std::sprintf(strDstTemp, "R%u", opDstReg0_31(m_OpMemory));
			std::sprintf(strSrcTemp, "Z+%u", opRegYZqval(m_OpMemory));
			break;

		case S_Yq_SNGL:		// Single Register and Y Register with Offset -- no label
			std::sprintf(strDstTemp, "Y+%u", opRegYZqval(m_OpMemory));
			std::sprintf(strSrcTemp, "R%u", opDstReg0_31(m_OpMemory));	// SrcReg uses DstReg slot here
			break;

		case S_Zq_SNGL:		// Single Register and Z Register with Offset -- no label
			std::sprintf(strDstTemp, "Z+%u", opRegYZqval(m_OpMemory));
			std::sprintf(strSrcTemp, "R%u", opDstReg0_31(m_OpMemory));	// SrcReg uses DstReg slot here
			break;

		case S_LDS:			// 16-bit Absolute Data Address
			assert((m_CurrentOpcode.control() & CTL_MASK) == CTL_DataLabel);
			// TODO : Determine how to detect architectures where RAMPD is used (OCTL_ArchExtAddr) for non-deterministic data labels
			if (bAddLabels) GenDataLabel(MemTypeIORAM(opDAbs16bit(m_OpMemory)), opDAbs16bit(m_OpMemory), m_nStartPC, TLabel(), msgFile, errFile);
			std::sprintf(strDstTemp, "R%u", opDstReg0_31(m_OpMemory));
			std::sprintf(strSrcTemp, "D@%04lX", static_cast<unsigned long>(opDAbs16bit(m_OpMemory)));
			break;

		case S_LDSrc:		// 7-bit Absolute Data Address
			assert((m_CurrentOpcode.control() & CTL_MASK) == CTL_DataLabel);
			if (bAddLabels) GenDataLabel(MemTypeIORAM(opLDSrc7bit(m_OpMemory)), opLDSrc7bit(m_OpMemory), m_nStartPC, TLabel(), msgFile, errFile);
			std::sprintf(strDstTemp, "R%u", opDstReg16_31(m_OpMemory));
			std::sprintf(strSrcTemp, "D@%02lX", static_cast<unsigned long>(opLDSrc7bit(m_OpMemory)));
			break;

		case S_STS:			// 16-bit Absolute Data Address
			assert((m_CurrentOpcode.control() & CTL_MASK) == CTL_DataLabel);
			// TODO : Determine how to detect architectures where RAMPD is used (OCTL_ArchExtAddr) for non-deterministic data labels
			if (bAddLabels) GenDataLabel(MemTypeIORAM(opDAbs16bit(m_OpMemory)), opDAbs16bit(m_OpMemory), m_nStartPC, TLabel(), msgFile, errFile);
			std::sprintf(strDstTemp, "D@%04lX", static_cast<unsigned long>(opDAbs16bit(m_OpMemory)));
			std::sprintf(strSrcTemp, "R%u", opDstReg0_31(m_OpMemory));	// SrcReg uses DstReg slot here
			break;

		case S_STSrc:		// 7-bit Absolute Data Address
			assert((m_CurrentOpcode.control() & CTL_MASK) == CTL_DataLabel);
			if (bAddLabels) GenDataLabel(MemTypeIORAM(opLDSrc7bit(m_OpMemory)), opLDSrc7bit(m_OpMemory), m_nStartPC, TLabel(), msgFile, errFile);
			std::sprintf(strDstTemp, "D@%02lX", static_cast<unsigned long>(opLDSrc7bit(m_OpMemory)));
			std::sprintf(strSrcTemp, "R%u", opDstReg16_31(m_OpMemory));	// SrcReg uses DstReg slot here
			break;

		case S_DES:			// Immediate "K" value -- no label
			std::sprintf(strDstTemp, "#%01X", opDESK(m_OpMemory));
			break;

		case S_INH_Zp:		// Inherent with Z+ Register -- no label
			if ((m_CurrentOpcode.control() & CTL_MASK) == CTL_DataRegAddr) {
				if (m_CurrentOpcode.control() & OCTL_ArchExtAddr) {
					std::sprintf(strDstTemp, "D&00(RAMPD,Z+)");
				} else {
					std::sprintf(strDstTemp, "D&00(Z+)");
				}
			} else {
				std::sprintf(strDstTemp, "Z+");
			}
			break;

		case S_INH:			// Inherent -- no label
			if ((m_CurrentOpcode.control() & CTL_MASK) == CTL_UndetBra) {
				if (m_CurrentOpcode.control() & OCTL_ArchExtAddr) {
					std::sprintf(strDstTemp, "C&00(EIND,Z)");
				} else {
					std::sprintf(strDstTemp, "C&00(Z)");
				}
			} else if ((m_CurrentOpcode.control() & CTL_MASK) == CTL_DataRegAddr) {
				if (compareNoCase(m_CurrentOpcode.mnemonic(), "spm") == 0) {	// Distinguish between lpm and spm
					if (m_CurrentOpcode.control() & OCTL_ArchExtAddr) {
						std::sprintf(strDstTemp, "D&00(RAMPZ,Z)");
					} else {
						std::sprintf(strDstTemp, "D&00(Z)");
					}
					std::sprintf(strSrcTemp, "R0");
				} else {
					std::sprintf(strDstTemp, "R0");
					if (m_CurrentOpcode.control() & OCTL_ArchExtAddr) {
						std::sprintf(strSrcTemp, "D&00(RAMPZ,Z)");				// elpm
					} else {
						std::sprintf(strSrcTemp, "D&00(Z)");					// lpm
					}
				}
			}
			break;

		default:
			bRetVal = false;
			break;
	}

	m_sFunctionalOpcode += strDstTemp;

	m_sFunctionalOpcode += "|";

	m_sFunctionalOpcode += strSrcTemp;

	// See if this is a function branch reference that needs to be added:
	if (bAddFunctionBranchRef) {
		if (((m_CurrentOpcode.group() == S_JMP) || (m_CurrentOpcode.group() == S_RJMP)) &&
			((m_CurrentOpcode.control() & TAVRDisassembler::OCTL_STOP) == 0)) {		// Check for 'call' or 'rcall'
			// Add these only if it is replacing a lower priority value:
			CFuncMap::const_iterator itrFunction = m_FunctionsTable.find(nTargetAddr);
			if (itrFunction == m_FunctionsTable.cend()) {
				m_FunctionsTable[nTargetAddr] = FUNCF_CALL;
			} else {
				if (itrFunction->second > FUNCF_CALL) m_FunctionsTable[nTargetAddr] = FUNCF_CALL;
			}

			// See if this is the end of a function (explicitly tagged):
			if (m_FuncExitAddresses.contains(nTargetAddr)) {
				m_FunctionsTable[m_nStartPC] = FUNCF_EXITBRANCH;
			}
		} else {
			// Here on 'brxx' branch statements and 'jmp' and 'rjmp'
			// Add these only if there isn't a function tag here:
			if (!m_FunctionsTable.contains(nTargetAddr)) m_FunctionsTable[nTargetAddr] = FUNCF_BRANCHIN;
		}
	}

	if (m_bLastOpcodeWasSkip) {
		// If the previous opcode was a skip, add a branchin for the next
		//	opcode since it's what we "skipped" to:
		if (!m_FunctionsTable.contains(m_PC)) m_FunctionsTable[m_PC] = FUNCF_BRANCHIN;
	}

	// See if this is the end of a function:
	if (m_CurrentOpcode.control() & (TAVRDisassembler::OCTL_HARDSTOP | TAVRDisassembler::OCTL_STOP)) {		// jmp, rjmp, ret, reti, etc.
		if (bAddFunctionBranchRef) {
			if (m_FuncExitAddresses.contains(nTargetAddr)) {
				m_FunctionsTable[m_nStartPC] = FUNCF_EXITBRANCH;
			} else {
				m_FunctionsTable[m_nStartPC] = FUNCF_BRANCHOUT;
			}
		} else {
			if (CurrentOpcodeIsStop()) {		// Distinguish between just a branchout and hardstop on CTL_Skip cases
				// Non-Deterministic branches get tagged as a hardstop (usually it exits the function):
				m_FunctionsTable[m_nStartPC] = FUNCF_HARDSTOP;
			} else {
				// Non-Deterministic branches in a skip get tagged as a branchout:
				m_FunctionsTable[m_nStartPC] = FUNCF_BRANCHOUT;
			}
		}
	}

	return bRetVal;
}

std::string CAVRDisassembler::CreateOperand()
{
	using namespace TAVRDisassembler_ENUMS;

	std::string strOpStr;
	char strTemp[30];
	strTemp[0] = 0;

	switch (m_CurrentOpcode.group()) {
		case S_IBYTE:		// Immediate 8-bit data -- no label
			std::sprintf(strTemp, "R%u,%s%02X", opDstReg16_31(m_OpMemory), GetHexDelim().c_str(), static_cast<unsigned int>(opIByte(m_OpMemory)));
			break;

		case S_IWORD:		// Immediate 6-bit data -- no label
			std::sprintf(strTemp, "R%u,%s%02X", opDstRegEven24_30(m_OpMemory), GetHexDelim().c_str(), static_cast<unsigned int>(opIWord(m_OpMemory)));
			break;

		case S_SNGL:		// Single Register -- no label
			std::sprintf(strTemp, "R%u", opDstReg0_31(m_OpMemory));
			break;

		case S_SAME:		// Single Register (Same Double Register) -- no label
			std::sprintf(strTemp, "R%u", opDstReg0_31(m_OpMemory));
			assert(opDstReg0_31(m_OpMemory) == opSrcReg0_31(m_OpMemory));
			break;

		case S_DUBL:		// Double Register -- no label
			std::sprintf(strTemp, "R%u,R%u", opDstReg0_31(m_OpMemory), opSrcReg0_31(m_OpMemory));
			break;

		case S_MOVW:		// Double Word Registers -- no label
			std::sprintf(strTemp, "R%u,R%u", opDstRegEven0_30(m_OpMemory), opSrcRegEven0_30(m_OpMemory));
			break;

		case S_MULS:		// Double High Registers -- no label
			std::sprintf(strTemp, "R%u,R%u", opDstReg16_31(m_OpMemory), opSrcReg16_31(m_OpMemory));
			break;

		case S_FMUL:		// Double Middle-High Registers -- no label
			std::sprintf(strTemp, "R%u,R%u", opDstReg16_23(m_OpMemory), opSrcReg16_23(m_OpMemory));
			break;

		case S_SER:			// Single High Register -- no label
			std::sprintf(strTemp, "R%u", opDstReg16_31(m_OpMemory));
			break;

		case S_TFLG:		// Single Register and bit -- no label
			std::sprintf(strTemp, "R%u,%u", opDstReg0_31(m_OpMemory), opBit(m_OpMemory));
			break;

		case S_BRA:			// 7-bit Relative Code Address
			assert((m_CurrentOpcode.control() & CTL_MASK) == CTL_DetBra);
			strOpStr += CodeLabelDeref(m_PC + opCRel7bit(m_OpMemory));
			break;

		case S_JMP:			// 22-bit Absolute Code Address
			assert((m_CurrentOpcode.control() & CTL_MASK) == CTL_DetBra);
			strOpStr += CodeLabelDeref(opCAbs22bit(m_OpMemory));
			break;

		case S_RJMP:		// 12-bit Relative Code Address
			assert((m_CurrentOpcode.control() & CTL_MASK) == CTL_DetBra);
			strOpStr += CodeLabelDeref(m_PC + opCRel12bit(m_OpMemory));
			break;

		case S_IOR:			// 5-bit Absolute I/O Address and bit
			std::sprintf(strTemp, "%s,%u", IOLabelDeref(opIO0_31(m_OpMemory) + 0x20).c_str(), opBit(m_OpMemory));
			break;

		case S_IN:			// 6-bit Absolute I/O Address and Register
			std::sprintf(strTemp, "R%u,%s", opDstReg0_31(m_OpMemory), IOLabelDeref(opIO0_63(m_OpMemory) + 0x20).c_str());
			break;

		case S_OUT:			// 6-bit Absolute I/O Address and Register
			std::sprintf(strTemp, "%s,R%u", IOLabelDeref(opIO0_63(m_OpMemory) + 0x20).c_str(), opDstReg0_31(m_OpMemory));	// SrcReg uses DstReg slot here
			break;

		case S_SNGL_X:		// Single Register and X Register -- no label
			std::sprintf(strTemp, "R%u,X", opDstReg0_31(m_OpMemory));
			break;

		case S_SNGL_Xp:		// Single Register and X Register -- no label
			std::sprintf(strTemp, "R%u,X+", opDstReg0_31(m_OpMemory));
			break;

		case S_SNGL_nX:		// Single Register and X Register -- no label
			std::sprintf(strTemp, "R%u,-X", opDstReg0_31(m_OpMemory));
			break;

		case S_SNGL_Y:		// Single Register and Y Register -- no label
			std::sprintf(strTemp, "R%u,Y", opDstReg0_31(m_OpMemory));
			break;

		case S_SNGL_Yp:		// Single Register and Y Register -- no label
			std::sprintf(strTemp, "R%u,Y+", opDstReg0_31(m_OpMemory));
			break;

		case S_SNGL_nY:		// Single Register and Y Register -- no label
			std::sprintf(strTemp, "R%u,-Y", opDstReg0_31(m_OpMemory));
			break;

		case S_SNGL_Z:		// Single Register and Z Register -- no label
			std::sprintf(strTemp, "R%u,Z", opDstReg0_31(m_OpMemory));
			break;

		case S_SNGL_Zp:		// Single Register and Z Register -- no label
			std::sprintf(strTemp, "R%u,Z+", opDstReg0_31(m_OpMemory));
			break;

		case S_SNGL_nZ:		// Single Register and Z Register -- no label
			std::sprintf(strTemp, "R%u,-Z", opDstReg0_31(m_OpMemory));
			break;

		case S_X_SNGL:		// Single Register and X Register -- no label
			std::sprintf(strTemp, "X,R%u", opDstReg0_31(m_OpMemory));	// SrcReg uses DstReg slot here
			break;

		case S_Xp_SNGL:		// Single Register and X Register -- no label
			std::sprintf(strTemp, "X+,R%u", opDstReg0_31(m_OpMemory));	// SrcReg uses DstReg slot here
			break;

		case S_nX_SNGL:		// Single Register and X Register -- no label
			std::sprintf(strTemp, "-X,R%u", opDstReg0_31(m_OpMemory));	// SrcReg uses DstReg slot here
			break;

		case S_Y_SNGL:		// Single Register and Y Register -- no label
			std::sprintf(strTemp, "Y,R%u", opDstReg0_31(m_OpMemory));	// SrcReg uses DstReg slot here
			break;

		case S_Yp_SNGL:		// Single Register and Y Register -- no label
			std::sprintf(strTemp, "Y+,R%u", opDstReg0_31(m_OpMemory));	// SrcReg uses DstReg slot here
			break;

		case S_nY_SNGL:		// Single Register and Y Register -- no label
			std::sprintf(strTemp, "-Y,R%u", opDstReg0_31(m_OpMemory));	// SrcReg uses DstReg slot here
			break;

		case S_Z_SNGL:		// Single Register and Z Register -- no label
			std::sprintf(strTemp, "Z,R%u", opDstReg0_31(m_OpMemory));	// SrcReg uses DstReg slot here
			break;

		case S_Zp_SNGL:		// Single Register and Z Register -- no label
			std::sprintf(strTemp, "Z+,R%u", opDstReg0_31(m_OpMemory));	// SrcReg uses DstReg slot here
			break;

		case S_nZ_SNGL:		// Single Register and Z Register -- no label
			std::sprintf(strTemp, "-Z,R%u", opDstReg0_31(m_OpMemory));	// SrcReg uses DstReg slot here
			break;

		case S_SNGL_Yq:		// Single Register and Y Register with Offset -- no label
			std::sprintf(strTemp, "R%u,Y+%u", opDstReg0_31(m_OpMemory), opRegYZqval(m_OpMemory));
			break;

		case S_SNGL_Zq:		// Single Register and Z Register with Offset -- no label
			std::sprintf(strTemp, "R%u,Z+%u", opDstReg0_31(m_OpMemory), opRegYZqval(m_OpMemory));
			break;

		case S_Yq_SNGL:		// Single Register and Y Register with Offset -- no label
			std::sprintf(strTemp, "Y+%u,R%u", opRegYZqval(m_OpMemory), opDstReg0_31(m_OpMemory));	// SrcReg uses DstReg slot here
			break;

		case S_Zq_SNGL:		// Single Register and Z Register with Offset -- no label
			std::sprintf(strTemp, "Z+%u,R%u", opRegYZqval(m_OpMemory), opDstReg0_31(m_OpMemory));	// SrcReg uses DstReg slot here
			break;

		case S_LDS:			// 16-bit Absolute Data Address
			assert((m_CurrentOpcode.control() & CTL_MASK) == CTL_DataLabel);
			// TODO : Determine how to detect architectures where RAMPD is used (OCTL_ArchExtAddr) for non-deterministic data labels
			std::sprintf(strTemp, "R%u,%s", opDstReg0_31(m_OpMemory), DataLabelDeref(opDAbs16bit(m_OpMemory)).c_str());
			break;

		case S_LDSrc:		// 7-bit Absolute Data Address
			assert((m_CurrentOpcode.control() & CTL_MASK) == CTL_DataLabel);
			std::sprintf(strTemp, "R%u,%s", opDstReg16_31(m_OpMemory), DataLabelDeref(opLDSrc7bit(m_OpMemory)).c_str());
			break;

		case S_STS:			// 16-bit Absolute Data Address
			assert((m_CurrentOpcode.control() & CTL_MASK) == CTL_DataLabel);
			// TODO : Determine how to detect architectures where RAMPD is used (OCTL_ArchExtAddr) for non-deterministic data labels
			std::sprintf(strTemp, "%s,R%u", DataLabelDeref(opDAbs16bit(m_OpMemory)).c_str(), opDstReg0_31(m_OpMemory));	// SrcReg uses DstReg slot here
			break;

		case S_STSrc:		// 7-bit Absolute Data Address
			assert((m_CurrentOpcode.control() & CTL_MASK) == CTL_DataLabel);
			std::sprintf(strTemp, "%s,R%u", DataLabelDeref(opLDSrc7bit(m_OpMemory)).c_str(), opDstReg16_31(m_OpMemory));	// SrcReg uses DstReg slot here
			break;

		case S_DES:			// Immediate "K" value -- no label
			std::sprintf(strTemp, "%u", opDESK(m_OpMemory));
			break;

		case S_INH_Zp:		// Inherent with Z+ Register -- no label
			strOpStr += "Z+";
			break;

		case S_INH:			// Inherent -- no label
			break;
	}

	strOpStr += strTemp;

	return strOpStr;
}

TAddress CAVRDisassembler::CreateOperandAddress()
{
	using namespace TAVRDisassembler_ENUMS;

	// Return address for things like deterministic branches.
	//	Will also return operand address for data, such as LDS, etc.
	//	Note: It would be ideal to also return deterministic
	//	branches like CTL_Skip and CTL_SkipIOLabel. However,
	//	we can't return those because we don't know the size of
	//	the next opcode (the one being skipped).

	TAddress nAddress = 0;
	switch (m_CurrentOpcode.group()) {
		case S_BRA:			// 7-bit Relative Code Address
			nAddress = (m_PC + opCRel7bit(m_OpMemory));
			break;

		case S_JMP:			// 22-bit Absolute Code Address
			nAddress = opCAbs22bit(m_OpMemory);
			break;

		case S_RJMP:		// 12-bit Relative Code Address
			nAddress = (m_PC + opCRel12bit(m_OpMemory));
			break;

		case S_IOR:			// 5-bit Absolute I/O Address and bit
			nAddress = opIO0_31(m_OpMemory) + 0x20;
			break;

		case S_IN:			// 6-bit Absolute I/O Address and Register
		case S_OUT:			// 6-bit Absolute I/O Address and Register
			nAddress = opIO0_63(m_OpMemory) + 0x20;
			break;

		case S_LDS:			// 16-bit Absolute Data Address
		case S_STS:			// 16-bit Absolute Data Address
			nAddress = opDAbs16bit(m_OpMemory);
			break;

		case S_LDSrc:		// 7-bit Absolute Data Address
		case S_STSrc:		// 7-bit Absolute Data Address
			nAddress = opLDSrc7bit(m_OpMemory);
			break;

		default:
			break;
	}

	return nAddress;
}

std::string CAVRDisassembler::FormatOperandRefComments()
{
	using namespace TAVRDisassembler_ENUMS;

	if ((m_CurrentOpcode.group() == S_BRA) ||
		(m_CurrentOpcode.group() == S_JMP) ||
		(m_CurrentOpcode.group() == S_RJMP) ||
//		(m_CurrentOpcode.group() == S_IOR) ||			// TODO : Built-in Comments for I/O ??
//		(m_CurrentOpcode.group() == S_IN) ||
//		(m_CurrentOpcode.group() == S_OUT) ||
		(m_CurrentOpcode.group() == S_LDS) ||
		(m_CurrentOpcode.group() == S_LDSrc) ||
		(m_CurrentOpcode.group() == S_STS) ||
		(m_CurrentOpcode.group() == S_STSrc)) {
		return FormatUserComments(MT_ROM, MC_INDIRECT, CreateOperandAddress());		// TODO : Figure out how to distinguish Code vs Data indirect(ref) comments
	}

	return std::string();
}

bool CAVRDisassembler::CheckBranchAddressLoaded(TAddress nAddress)
{
	return IsAddressLoaded(MT_ROM, nAddress, opcodeSymbolSize());
}

TLabel CAVRDisassembler::CodeLabelDeref(TAddress nAddress)
{
	const MEMORY_TYPE nMemType = MT_ROM;

	std::string strTemp;
	char strCharTemp[30];

	CLabelTableMap::const_iterator itrLabel = m_LabelTable[nMemType].find(nAddress);
	if (itrLabel != m_LabelTable[nMemType].cend()) {
		if (itrLabel->second.size()) {
			strTemp = FormatLabel(nMemType, LC_REF, itrLabel->second.at(0), nAddress);
		} else {
			strTemp = FormatLabel(nMemType, LC_REF, TLabel(), nAddress);
		}
	} else {
		std::sprintf(strCharTemp, "%s%04X", GetHexDelim().c_str(), nAddress);
		strTemp = strCharTemp;
	}
	return strTemp;
}

TLabel CAVRDisassembler::DataLabelDeref(TAddress nAddress)
{
	const MEMORY_TYPE nMemType = MemTypeIORAM(nAddress);

	std::string strTemp;
	char strCharTemp[30];

	CLabelTableMap::const_iterator itrLabel = m_LabelTable[nMemType].find(nAddress);
	if (itrLabel != m_LabelTable[nMemType].cend()) {
		if (itrLabel->second.size()) {
			strTemp = FormatLabel(nMemType, LC_REF, itrLabel->second.at(0), nAddress);
		} else {
			strTemp = FormatLabel(nMemType, LC_REF, TLabel(), nAddress);
		}
	} else {
		std::sprintf(strCharTemp, "%s%04X", GetHexDelim().c_str(), nAddress);
		strTemp = strCharTemp;
	}
	return strTemp;
}

TLabel CAVRDisassembler::IOLabelDeref(TAddress nAddress)
{
	const MEMORY_TYPE nMemType = MemTypeIORAM(nAddress);

	std::string strTemp;
	char strCharTemp[30];

	CLabelTableMap::const_iterator itrLabel = m_LabelTable[nMemType].find(nAddress);
	if (itrLabel != m_LabelTable[nMemType].cend()) {
		if (itrLabel->second.size()) {
			strTemp = FormatLabel(nMemType, LC_REF, itrLabel->second.at(0), nAddress);
		} else {
			strTemp = FormatLabel(nMemType, LC_REF, TLabel(), nAddress);
		}
	} else {
		std::sprintf(strCharTemp, "%s%02X", GetHexDelim().c_str(), nAddress - 0x20);	// I/O Addresses are offset by 0x20
		strTemp = strCharTemp;
	}
	return strTemp;
}

CAVRDisassembler::MEMORY_TYPE CAVRDisassembler::MemTypeIORAM(TAddress nAddress)
{
	if (m_MemoryRanges[MT_IO].addressInRange(nAddress)) return MT_IO;
	return MT_RAM;
}

