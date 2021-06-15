//
//	Motorola 6811 Disassembler GDC Module
//	Copyright(c)1996 - 2021 by Donna Whisnant
//
//	Generic Code-Seeking Disassembler
//	Copyright(c)2021 by Donna Whisnant
//

//
//	M6811GDC -- This is the implementation for the M6811 Disassembler GDC module
//
//
//	Groups: (Grp) : xy
//	  Where x (msnb = destination = FIRST operand)
//			y (lsnb = source = LAST operand)
//
//			0 = opcode only, nothing follows
//			1 = 8-bit absolute address follows only
//			2 = 16-bit absolute address follows only
//			3 = 8-bit relative address follows only
//			4 = 16-bit relative address follows only		(Not used on HC11)
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
//	This version setup for compatibility with the AS6811 assembler
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
//			!addr|label
//			   |    |____ Label(s) for this address(comma separated)
//			   |_________ Absolute Address (hex)
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
//			C&xx(r)			Register Code Offset (xx=hex offset, r=register number or name), ex: jmp 2,x -> "C$02(x)"
//			D@xxxx			Absolute Data Address (xxxx=hex address)
//			D^n(xxxx)		Relative Data Address (n=signed offset in hex, xxxx=resolved absolute address in hex)
//			D&xx(r)			Register Data Offset (xx=hex offset, r=register number or name), ex: ldaa 1,y -> "D$01(y)"
//
//			If any of the above also includes a mask, then the following will be added:
//			,Mxx			Value mask (xx=hex mask value)
//
//		Note: The address sizes are consistent with the particular process.  For example, an HC11 will
//			use 16-bit addresses (or 4 hex digits).  The size of immediate data entries, offsets, and masks will
//			reflect the actual value.  A 16-bit immediate value, offset, or mask, will be outputted as 4 hex
//			digits, but an 8-bit immediate value, offset, or mask, will be outputted as only 2 hex digits.
//

#include "m6811gdc.h"
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
//	CM6811Disassembler
// ----------------------------------------------------------------------------
CM6811Disassembler::CM6811Disassembler()
	:	m_nOpPointer(0),
		m_nStartPC(0),
		m_nSectionCount(0),
		m_bCBDef(true)
{
//                = ((NBytes: 0; OpCode: (0, 0); Grp: 0; Control: 1; Mnemonic: '???'),
	m_Opcodes.AddOpcode({ { 0x00 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "test" });
	m_Opcodes.AddOpcode({ { 0x01 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "nop" });
	m_Opcodes.AddOpcode({ { 0x02 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "idiv" });
	m_Opcodes.AddOpcode({ { 0x03 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "fdiv" });
	m_Opcodes.AddOpcode({ { 0x04 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "lsrd" });
	m_Opcodes.AddOpcode({ { 0x05 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "lsld" });
	m_Opcodes.AddOpcode({ { 0x06 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "tap" });
	m_Opcodes.AddOpcode({ { 0x07 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "tpa" });
	m_Opcodes.AddOpcode({ { 0x08 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "inx" });
	m_Opcodes.AddOpcode({ { 0x09 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "dex" });
	m_Opcodes.AddOpcode({ { 0x0A }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "clv" });
	m_Opcodes.AddOpcode({ { 0x0B }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "sev" });
	m_Opcodes.AddOpcode({ { 0x0C }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "clc" });
	m_Opcodes.AddOpcode({ { 0x0D }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "sec" });
	m_Opcodes.AddOpcode({ { 0x0E }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "cli" });
	m_Opcodes.AddOpcode({ { 0x0F }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "sei" });
	m_Opcodes.AddOpcode({ { 0x10 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "sba" });
	m_Opcodes.AddOpcode({ { 0x11 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "cba" });
	m_Opcodes.AddOpcode({ { 0x12 }, { 0xFF }, MAKEOGRP(0x7, 0x3), MAKEOCTL(0x1, 0x3), "brset" });
	m_Opcodes.AddOpcode({ { 0x13 }, { 0xFF }, MAKEOGRP(0x7, 0x3), MAKEOCTL(0x1, 0x3), "brclr" });
	m_Opcodes.AddOpcode({ { 0x14 }, { 0xFF }, MAKEOGRP(0x0, 0x7), MAKEOCTL(0x0, 0x1), "bset" });
	m_Opcodes.AddOpcode({ { 0x15 }, { 0xFF }, MAKEOGRP(0x0, 0x7), MAKEOCTL(0x0, 0x1), "bclr" });
	m_Opcodes.AddOpcode({ { 0x16 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "tab" });
	m_Opcodes.AddOpcode({ { 0x17 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "tba" });
	m_Opcodes.AddOpcode({ { 0x18, 0x08 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "iny" });
	m_Opcodes.AddOpcode({ { 0x18, 0x09 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "dey" });
	m_Opcodes.AddOpcode({ { 0x18, 0x1C }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0x9), MAKEOCTL(0x0, 0x0), "bset" });
	m_Opcodes.AddOpcode({ { 0x18, 0x1D }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0x9), MAKEOCTL(0x0, 0x0), "bclr" });
	m_Opcodes.AddOpcode({ { 0x18, 0x1E }, { 0xFF, 0xFF }, MAKEOGRP(0x9, 0x3), MAKEOCTL(0x0, 0x3), "brset" });
	m_Opcodes.AddOpcode({ { 0x18, 0x1F }, { 0xFF, 0xFF }, MAKEOGRP(0x9, 0x3), MAKEOCTL(0x0, 0x3), "brclr" });
	m_Opcodes.AddOpcode({ { 0x18, 0x30 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "tsy" });
	m_Opcodes.AddOpcode({ { 0x18, 0x35 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "tys" });
	m_Opcodes.AddOpcode({ { 0x18, 0x38 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "puly" });
	m_Opcodes.AddOpcode({ { 0x18, 0x3A }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "aby" });
	m_Opcodes.AddOpcode({ { 0x18, 0x3C }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "pshy" });
	m_Opcodes.AddOpcode({ { 0x18, 0x60 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "neg" });
	m_Opcodes.AddOpcode({ { 0x18, 0x63 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "com" });
	m_Opcodes.AddOpcode({ { 0x18, 0x64 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "lsr" });
	m_Opcodes.AddOpcode({ { 0x18, 0x66 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "ror" });
	m_Opcodes.AddOpcode({ { 0x18, 0x67 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "asr" });
	m_Opcodes.AddOpcode({ { 0x18, 0x68 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "lsl" });
	m_Opcodes.AddOpcode({ { 0x18, 0x69 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "rol" });
	m_Opcodes.AddOpcode({ { 0x18, 0x6A }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "dec" });
	m_Opcodes.AddOpcode({ { 0x18, 0x6C }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "inc" });
	m_Opcodes.AddOpcode({ { 0x18, 0x6D }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "tst" });
	m_Opcodes.AddOpcode({ { 0x18, 0x6E }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x2) | TM6811Disassembler::OCTL_STOP, "jmp" });
	m_Opcodes.AddOpcode({ { 0x18, 0x6F }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "clr" });
	m_Opcodes.AddOpcode({ { 0x18, 0x8C }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0x6), MAKEOCTL(0x0, 0x0), "cpy" });
	m_Opcodes.AddOpcode({ { 0x18, 0x8F }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "xgdy" });
	m_Opcodes.AddOpcode({ { 0x18, 0x9C }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x1), "cpy" });
	m_Opcodes.AddOpcode({ { 0x18, 0xA0 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "suba" });
	m_Opcodes.AddOpcode({ { 0x18, 0xA1 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "cmpa" });
	m_Opcodes.AddOpcode({ { 0x18, 0xA2 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "sbca" });
	m_Opcodes.AddOpcode({ { 0x18, 0xA3 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "subd" });
	m_Opcodes.AddOpcode({ { 0x18, 0xA4 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "anda" });
	m_Opcodes.AddOpcode({ { 0x18, 0xA5 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "bita" });
	m_Opcodes.AddOpcode({ { 0x18, 0xA6 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "ldaa" });
	m_Opcodes.AddOpcode({ { 0x18, 0xA7 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "staa" });
	m_Opcodes.AddOpcode({ { 0x18, 0xA8 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "eora" });
	m_Opcodes.AddOpcode({ { 0x18, 0xA9 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "adca" });
	m_Opcodes.AddOpcode({ { 0x18, 0xAA }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "oraa" });
	m_Opcodes.AddOpcode({ { 0x18, 0xAB }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "adda" });
	m_Opcodes.AddOpcode({ { 0x18, 0xAC }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "cpy" });
	m_Opcodes.AddOpcode({ { 0x18, 0xAD }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x2), "jsr" });
	m_Opcodes.AddOpcode({ { 0x18, 0xAE }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "lds" });
	m_Opcodes.AddOpcode({ { 0x18, 0xAF }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "sts" });
	m_Opcodes.AddOpcode({ { 0x18, 0xBC }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "cpy" });
	m_Opcodes.AddOpcode({ { 0x18, 0xCE }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0x6), MAKEOCTL(0x0, 0x0), "ldy" });
	m_Opcodes.AddOpcode({ { 0x18, 0xDE }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x1), "ldy" });
	m_Opcodes.AddOpcode({ { 0x18, 0xDF }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x1), "sty" });
	m_Opcodes.AddOpcode({ { 0x18, 0xE0 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "subb" });
	m_Opcodes.AddOpcode({ { 0x18, 0xE1 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "cmpb" });
	m_Opcodes.AddOpcode({ { 0x18, 0xE2 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "sbcb" });
	m_Opcodes.AddOpcode({ { 0x18, 0xE3 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "addd" });
	m_Opcodes.AddOpcode({ { 0x18, 0xE4 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "andb" });
	m_Opcodes.AddOpcode({ { 0x18, 0xE5 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "bitb" });
	m_Opcodes.AddOpcode({ { 0x18, 0xE6 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "ldab" });
	m_Opcodes.AddOpcode({ { 0x18, 0xE7 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "stab" });
	m_Opcodes.AddOpcode({ { 0x18, 0xE8 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "eorb" });
	m_Opcodes.AddOpcode({ { 0x18, 0xE9 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "adcb" });
	m_Opcodes.AddOpcode({ { 0x18, 0xEA }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "orab" });
	m_Opcodes.AddOpcode({ { 0x18, 0xEB }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "addb" });
	m_Opcodes.AddOpcode({ { 0x18, 0xEC }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "ldd" });
	m_Opcodes.AddOpcode({ { 0x18, 0xED }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "std" });
	m_Opcodes.AddOpcode({ { 0x18, 0xEE }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "ldy" });
	m_Opcodes.AddOpcode({ { 0x18, 0xEF }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "sty" });
	m_Opcodes.AddOpcode({ { 0x18, 0xFE }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "ldy" });
	m_Opcodes.AddOpcode({ { 0x18, 0xFF }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "sty" });
	m_Opcodes.AddOpcode({ { 0x19 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "daa" });
	m_Opcodes.AddOpcode({ { 0x1A, 0x83 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0x6), MAKEOCTL(0x0, 0x0), "cpd" });
	m_Opcodes.AddOpcode({ { 0x1A, 0x93 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x1), "cpd" });
	m_Opcodes.AddOpcode({ { 0x1A, 0xA3 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "cpd" });
	m_Opcodes.AddOpcode({ { 0x1A, 0xAC }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "cpy" });
	m_Opcodes.AddOpcode({ { 0x1A, 0xB3 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "cpd" });
	m_Opcodes.AddOpcode({ { 0x1A, 0xEE }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "ldy" });
	m_Opcodes.AddOpcode({ { 0x1A, 0xEF }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "sty" });
	m_Opcodes.AddOpcode({ { 0x1B }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "aba" });
	m_Opcodes.AddOpcode({ { 0x1C }, { 0xFF }, MAKEOGRP(0x0, 0x8), MAKEOCTL(0x0, 0x0), "bset" });
	m_Opcodes.AddOpcode({ { 0x1D }, { 0xFF }, MAKEOGRP(0x0, 0x8), MAKEOCTL(0x0, 0x0), "bclr" });
	m_Opcodes.AddOpcode({ { 0x1E }, { 0xFF }, MAKEOGRP(0x8, 0x3), MAKEOCTL(0x0, 0x3), "brset" });
	m_Opcodes.AddOpcode({ { 0x1F }, { 0xFF }, MAKEOGRP(0x8, 0x3), MAKEOCTL(0x0, 0x3), "brclr" });
	m_Opcodes.AddOpcode({ { 0x20 }, { 0xFF }, MAKEOGRP(0x0, 0x3), MAKEOCTL(0x0, 0x3) | TM6811Disassembler::OCTL_STOP, "bra" });
	m_Opcodes.AddOpcode({ { 0x21 }, { 0xFF }, MAKEOGRP(0x0, 0x3), MAKEOCTL(0x0, 0x3), "brn" });
	m_Opcodes.AddOpcode({ { 0x22 }, { 0xFF }, MAKEOGRP(0x0, 0x3), MAKEOCTL(0x0, 0x3), "bhi" });
	m_Opcodes.AddOpcode({ { 0x23 }, { 0xFF }, MAKEOGRP(0x0, 0x3), MAKEOCTL(0x0, 0x3), "bls" });
	m_Opcodes.AddOpcode({ { 0x24 }, { 0xFF }, MAKEOGRP(0x0, 0x3), MAKEOCTL(0x0, 0x3), "bcc" });
	m_Opcodes.AddOpcode({ { 0x25 }, { 0xFF }, MAKEOGRP(0x0, 0x3), MAKEOCTL(0x0, 0x3), "bcs" });
	m_Opcodes.AddOpcode({ { 0x26 }, { 0xFF }, MAKEOGRP(0x0, 0x3), MAKEOCTL(0x0, 0x3), "bne" });
	m_Opcodes.AddOpcode({ { 0x27 }, { 0xFF }, MAKEOGRP(0x0, 0x3), MAKEOCTL(0x0, 0x3), "beq" });
	m_Opcodes.AddOpcode({ { 0x28 }, { 0xFF }, MAKEOGRP(0x0, 0x3), MAKEOCTL(0x0, 0x3), "bvc" });
	m_Opcodes.AddOpcode({ { 0x29 }, { 0xFF }, MAKEOGRP(0x0, 0x3), MAKEOCTL(0x0, 0x3), "bvs" });
	m_Opcodes.AddOpcode({ { 0x2A }, { 0xFF }, MAKEOGRP(0x0, 0x3), MAKEOCTL(0x0, 0x3), "bpl" });
	m_Opcodes.AddOpcode({ { 0x2B }, { 0xFF }, MAKEOGRP(0x0, 0x3), MAKEOCTL(0x0, 0x3), "bmi" });
	m_Opcodes.AddOpcode({ { 0x2C }, { 0xFF }, MAKEOGRP(0x0, 0x3), MAKEOCTL(0x0, 0x3), "bge" });
	m_Opcodes.AddOpcode({ { 0x2D }, { 0xFF }, MAKEOGRP(0x0, 0x3), MAKEOCTL(0x0, 0x3), "blt" });
	m_Opcodes.AddOpcode({ { 0x2E }, { 0xFF }, MAKEOGRP(0x0, 0x3), MAKEOCTL(0x0, 0x3), "bgt" });
	m_Opcodes.AddOpcode({ { 0x2F }, { 0xFF }, MAKEOGRP(0x0, 0x3), MAKEOCTL(0x0, 0x3), "ble" });
	m_Opcodes.AddOpcode({ { 0x30 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "tsx" });
	m_Opcodes.AddOpcode({ { 0x31 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "ins" });
	m_Opcodes.AddOpcode({ { 0x32 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "pula" });
	m_Opcodes.AddOpcode({ { 0x33 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "pulb" });
	m_Opcodes.AddOpcode({ { 0x34 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "des" });
	m_Opcodes.AddOpcode({ { 0x35 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "txs" });
	m_Opcodes.AddOpcode({ { 0x36 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "psha" });
	m_Opcodes.AddOpcode({ { 0x37 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "pshb" });
	m_Opcodes.AddOpcode({ { 0x38 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "pulx" });
	m_Opcodes.AddOpcode({ { 0x39 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0) | TM6811Disassembler::OCTL_STOP, "rts" });
	m_Opcodes.AddOpcode({ { 0x3A }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "abx" });
	m_Opcodes.AddOpcode({ { 0x3B }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0) | TM6811Disassembler::OCTL_STOP, "rti" });
	m_Opcodes.AddOpcode({ { 0x3C }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "pshx" });
	m_Opcodes.AddOpcode({ { 0x3D }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "mul" });
	m_Opcodes.AddOpcode({ { 0x3E }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "wai" });
	m_Opcodes.AddOpcode({ { 0x3F }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "swi" });
	m_Opcodes.AddOpcode({ { 0x40 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "nega" });
	m_Opcodes.AddOpcode({ { 0x43 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "coma" });
	m_Opcodes.AddOpcode({ { 0x44 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "lsra" });
	m_Opcodes.AddOpcode({ { 0x46 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "rora" });
	m_Opcodes.AddOpcode({ { 0x47 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "asra" });
	m_Opcodes.AddOpcode({ { 0x48 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "lsla" });
	m_Opcodes.AddOpcode({ { 0x49 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "rola" });
	m_Opcodes.AddOpcode({ { 0x4A }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "deca" });
	m_Opcodes.AddOpcode({ { 0x4C }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "inca" });
	m_Opcodes.AddOpcode({ { 0x4D }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "tsta" });
	m_Opcodes.AddOpcode({ { 0x4F }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "clra" });
	m_Opcodes.AddOpcode({ { 0x50 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "negb" });
	m_Opcodes.AddOpcode({ { 0x53 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "comb" });
	m_Opcodes.AddOpcode({ { 0x54 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "lsrb" });
	m_Opcodes.AddOpcode({ { 0x56 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "rorb" });
	m_Opcodes.AddOpcode({ { 0x57 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "asrb" });
	m_Opcodes.AddOpcode({ { 0x58 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "lslb" });
	m_Opcodes.AddOpcode({ { 0x59 }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "rolb" });
	m_Opcodes.AddOpcode({ { 0x5A }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "decb" });
	m_Opcodes.AddOpcode({ { 0x5C }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "incb" });
	m_Opcodes.AddOpcode({ { 0x5D }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "tstb" });
	m_Opcodes.AddOpcode({ { 0x5F }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "clrb" });
	m_Opcodes.AddOpcode({ { 0x60 }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "neg" });
	m_Opcodes.AddOpcode({ { 0x63 }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "com" });
	m_Opcodes.AddOpcode({ { 0x64 }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "lsr" });
	m_Opcodes.AddOpcode({ { 0x66 }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "ror" });
	m_Opcodes.AddOpcode({ { 0x67 }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "asr" });
	m_Opcodes.AddOpcode({ { 0x68 }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "lsl" });
	m_Opcodes.AddOpcode({ { 0x69 }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "rol" });
	m_Opcodes.AddOpcode({ { 0x6A }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "dec" });
	m_Opcodes.AddOpcode({ { 0x6C }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "inc" });
	m_Opcodes.AddOpcode({ { 0x6D }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "tst" });
	m_Opcodes.AddOpcode({ { 0x6E }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x2) | TM6811Disassembler::OCTL_STOP, "jmp" });
	m_Opcodes.AddOpcode({ { 0x6F }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "clr" });
	m_Opcodes.AddOpcode({ { 0x70 }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "neg" });
	m_Opcodes.AddOpcode({ { 0x73 }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "com" });
	m_Opcodes.AddOpcode({ { 0x74 }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "lsr" });
	m_Opcodes.AddOpcode({ { 0x76 }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "ror" });
	m_Opcodes.AddOpcode({ { 0x77 }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "asr" });
	m_Opcodes.AddOpcode({ { 0x78 }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "lsl" });
	m_Opcodes.AddOpcode({ { 0x79 }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "rol" });
	m_Opcodes.AddOpcode({ { 0x7A }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "dec" });
	m_Opcodes.AddOpcode({ { 0x7C }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "inc" });
	m_Opcodes.AddOpcode({ { 0x7D }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "tst" });
	m_Opcodes.AddOpcode({ { 0x7E }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x3) | TM6811Disassembler::OCTL_STOP, "jmp" });
	m_Opcodes.AddOpcode({ { 0x7F }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "clr" });
	m_Opcodes.AddOpcode({ { 0x80 }, { 0xFF }, MAKEOGRP(0x0, 0x5), MAKEOCTL(0x0, 0x0), "suba" });
	m_Opcodes.AddOpcode({ { 0x81 }, { 0xFF }, MAKEOGRP(0x0, 0x5), MAKEOCTL(0x0, 0x0), "cmpa" });
	m_Opcodes.AddOpcode({ { 0x82 }, { 0xFF }, MAKEOGRP(0x0, 0x5), MAKEOCTL(0x0, 0x0), "sbca" });
	m_Opcodes.AddOpcode({ { 0x83 }, { 0xFF }, MAKEOGRP(0x0, 0x6), MAKEOCTL(0x0, 0x0), "subd" });
	m_Opcodes.AddOpcode({ { 0x84 }, { 0xFF }, MAKEOGRP(0x0, 0x5), MAKEOCTL(0x0, 0x0), "anda" });
	m_Opcodes.AddOpcode({ { 0x85 }, { 0xFF }, MAKEOGRP(0x0, 0x5), MAKEOCTL(0x0, 0x0), "bita" });
	m_Opcodes.AddOpcode({ { 0x86 }, { 0xFF }, MAKEOGRP(0x0, 0x5), MAKEOCTL(0x0, 0x0), "ldaa" });
	m_Opcodes.AddOpcode({ { 0x88 }, { 0xFF }, MAKEOGRP(0x0, 0x5), MAKEOCTL(0x0, 0x0), "eora" });
	m_Opcodes.AddOpcode({ { 0x89 }, { 0xFF }, MAKEOGRP(0x0, 0x5), MAKEOCTL(0x0, 0x0), "adca" });
	m_Opcodes.AddOpcode({ { 0x8A }, { 0xFF }, MAKEOGRP(0x0, 0x5), MAKEOCTL(0x0, 0x0), "oraa" });
	m_Opcodes.AddOpcode({ { 0x8B }, { 0xFF }, MAKEOGRP(0x0, 0x5), MAKEOCTL(0x0, 0x0), "adda" });
	m_Opcodes.AddOpcode({ { 0x8C }, { 0xFF }, MAKEOGRP(0x0, 0x6), MAKEOCTL(0x0, 0x0), "cpx" });
	m_Opcodes.AddOpcode({ { 0x8D }, { 0xFF }, MAKEOGRP(0x0, 0x3), MAKEOCTL(0x0, 0x3), "bsr" });
	m_Opcodes.AddOpcode({ { 0x8E }, { 0xFF }, MAKEOGRP(0x0, 0x6), MAKEOCTL(0x0, 0x0), "lds" });
	m_Opcodes.AddOpcode({ { 0x8F }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "xgdx" });
	m_Opcodes.AddOpcode({ { 0x90 }, { 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x1), "suba" });
	m_Opcodes.AddOpcode({ { 0x91 }, { 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x1), "cmpa" });
	m_Opcodes.AddOpcode({ { 0x92 }, { 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x1), "sbca" });
	m_Opcodes.AddOpcode({ { 0x93 }, { 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x1), "subd" });
	m_Opcodes.AddOpcode({ { 0x94 }, { 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x1), "anda" });
	m_Opcodes.AddOpcode({ { 0x95 }, { 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x1), "bita" });
	m_Opcodes.AddOpcode({ { 0x96 }, { 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x1), "ldaa" });
	m_Opcodes.AddOpcode({ { 0x97 }, { 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x1), "staa" });
	m_Opcodes.AddOpcode({ { 0x98 }, { 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x1), "eora" });
	m_Opcodes.AddOpcode({ { 0x99 }, { 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x1), "adca" });
	m_Opcodes.AddOpcode({ { 0x9A }, { 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x1), "oraa" });
	m_Opcodes.AddOpcode({ { 0x9B }, { 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x1), "adda" });
	m_Opcodes.AddOpcode({ { 0x9C }, { 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x1), "cpx" });
	m_Opcodes.AddOpcode({ { 0x9D }, { 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x3), "jsr" });
	m_Opcodes.AddOpcode({ { 0x9E }, { 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x1), "lds" });
	m_Opcodes.AddOpcode({ { 0x9F }, { 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x1), "sts" });
	m_Opcodes.AddOpcode({ { 0xA0 }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "suba" });
	m_Opcodes.AddOpcode({ { 0xA1 }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "cmpa" });
	m_Opcodes.AddOpcode({ { 0xA2 }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "sbca" });
	m_Opcodes.AddOpcode({ { 0xA3 }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "subd" });
	m_Opcodes.AddOpcode({ { 0xA4 }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "anda" });
	m_Opcodes.AddOpcode({ { 0xA5 }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "bita" });
	m_Opcodes.AddOpcode({ { 0xA6 }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "ldaa" });
	m_Opcodes.AddOpcode({ { 0xA7 }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "staa" });
	m_Opcodes.AddOpcode({ { 0xA8 }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "eora" });
	m_Opcodes.AddOpcode({ { 0xA9 }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "adca" });
	m_Opcodes.AddOpcode({ { 0xAA }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "oraa" });
	m_Opcodes.AddOpcode({ { 0xAB }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "adda" });
	m_Opcodes.AddOpcode({ { 0xAC }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "cpx" });
	m_Opcodes.AddOpcode({ { 0xAD }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x2), "jsr" });
	m_Opcodes.AddOpcode({ { 0xAE }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "lds" });
	m_Opcodes.AddOpcode({ { 0xAF }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "sts" });
	m_Opcodes.AddOpcode({ { 0xB0 }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "suba" });
	m_Opcodes.AddOpcode({ { 0xB1 }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "cmpa" });
	m_Opcodes.AddOpcode({ { 0xB2 }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "sbca" });
	m_Opcodes.AddOpcode({ { 0xB3 }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "subd" });
	m_Opcodes.AddOpcode({ { 0xB4 }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "anda" });
	m_Opcodes.AddOpcode({ { 0xB5 }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "bita" });
	m_Opcodes.AddOpcode({ { 0xB6 }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "ldaa" });
	m_Opcodes.AddOpcode({ { 0xB7 }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "staa" });
	m_Opcodes.AddOpcode({ { 0xB8 }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "eora" });
	m_Opcodes.AddOpcode({ { 0xB9 }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "adca" });
	m_Opcodes.AddOpcode({ { 0xBA }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "oraa" });
	m_Opcodes.AddOpcode({ { 0xBB }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "adda" });
	m_Opcodes.AddOpcode({ { 0xBC }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "cpx" });
	m_Opcodes.AddOpcode({ { 0xBD }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x3), "jsr" });
	m_Opcodes.AddOpcode({ { 0xBE }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "lds" });
	m_Opcodes.AddOpcode({ { 0xBF }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "sts" });
	m_Opcodes.AddOpcode({ { 0xC0 }, { 0xFF }, MAKEOGRP(0x0, 0x5), MAKEOCTL(0x0, 0x0), "subb" });
	m_Opcodes.AddOpcode({ { 0xC1 }, { 0xFF }, MAKEOGRP(0x0, 0x5), MAKEOCTL(0x0, 0x0), "cmpb" });
	m_Opcodes.AddOpcode({ { 0xC2 }, { 0xFF }, MAKEOGRP(0x0, 0x5), MAKEOCTL(0x0, 0x0), "sbcb" });
	m_Opcodes.AddOpcode({ { 0xC3 }, { 0xFF }, MAKEOGRP(0x0, 0x6), MAKEOCTL(0x0, 0x0), "addd" });
	m_Opcodes.AddOpcode({ { 0xC4 }, { 0xFF }, MAKEOGRP(0x0, 0x5), MAKEOCTL(0x0, 0x0), "andb" });
	m_Opcodes.AddOpcode({ { 0xC5 }, { 0xFF }, MAKEOGRP(0x0, 0x5), MAKEOCTL(0x0, 0x0), "bitb" });
	m_Opcodes.AddOpcode({ { 0xC6 }, { 0xFF }, MAKEOGRP(0x0, 0x5), MAKEOCTL(0x0, 0x0), "ldab" });
	m_Opcodes.AddOpcode({ { 0xC8 }, { 0xFF }, MAKEOGRP(0x0, 0x5), MAKEOCTL(0x0, 0x0), "eorb" });
	m_Opcodes.AddOpcode({ { 0xC9 }, { 0xFF }, MAKEOGRP(0x0, 0x5), MAKEOCTL(0x0, 0x0), "adcb" });
	m_Opcodes.AddOpcode({ { 0xCA }, { 0xFF }, MAKEOGRP(0x0, 0x5), MAKEOCTL(0x0, 0x0), "orab" });
	m_Opcodes.AddOpcode({ { 0xCB }, { 0xFF }, MAKEOGRP(0x0, 0x5), MAKEOCTL(0x0, 0x0), "addb" });
	m_Opcodes.AddOpcode({ { 0xCC }, { 0xFF }, MAKEOGRP(0x0, 0x6), MAKEOCTL(0x0, 0x0), "ldd" });
	m_Opcodes.AddOpcode({ { 0xCD, 0xA3 }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "cpd" });
	m_Opcodes.AddOpcode({ { 0xCD, 0xAC }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "cpx" });
	m_Opcodes.AddOpcode({ { 0xCD, 0xEE }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "ldx" });
	m_Opcodes.AddOpcode({ { 0xCD, 0xEF }, { 0xFF, 0xFF }, MAKEOGRP(0x0, 0xB), MAKEOCTL(0x0, 0x0), "stx" });
	m_Opcodes.AddOpcode({ { 0xCE }, { 0xFF }, MAKEOGRP(0x0, 0x6), MAKEOCTL(0x0, 0x0), "ldx" });
	m_Opcodes.AddOpcode({ { 0xCF }, { 0xFF }, MAKEOGRP(0x0, 0x0), MAKEOCTL(0x0, 0x0), "stop" });
	m_Opcodes.AddOpcode({ { 0xD0 }, { 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x1), "subb" });
	m_Opcodes.AddOpcode({ { 0xD1 }, { 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x1), "cmpb" });
	m_Opcodes.AddOpcode({ { 0xD2 }, { 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x1), "sbcb" });
	m_Opcodes.AddOpcode({ { 0xD3 }, { 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x1), "addd" });
	m_Opcodes.AddOpcode({ { 0xD4 }, { 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x1), "andb" });
	m_Opcodes.AddOpcode({ { 0xD5 }, { 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x1), "bitb" });
	m_Opcodes.AddOpcode({ { 0xD6 }, { 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x1), "ldab" });
	m_Opcodes.AddOpcode({ { 0xD7 }, { 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x1), "stab" });
	m_Opcodes.AddOpcode({ { 0xD8 }, { 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x1), "eorb" });
	m_Opcodes.AddOpcode({ { 0xD9 }, { 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x1), "adcb" });
	m_Opcodes.AddOpcode({ { 0xDA }, { 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x1), "orab" });
	m_Opcodes.AddOpcode({ { 0xDB }, { 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x1), "addb" });
	m_Opcodes.AddOpcode({ { 0xDC }, { 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x1), "ldd" });
	m_Opcodes.AddOpcode({ { 0xDD }, { 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x1), "std" });
	m_Opcodes.AddOpcode({ { 0xDE }, { 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x1), "ldx" });
	m_Opcodes.AddOpcode({ { 0xDF }, { 0xFF }, MAKEOGRP(0x0, 0x1), MAKEOCTL(0x0, 0x1), "stx" });
	m_Opcodes.AddOpcode({ { 0xE0 }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "subb" });
	m_Opcodes.AddOpcode({ { 0xE1 }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "cmpb" });
	m_Opcodes.AddOpcode({ { 0xE2 }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "sbcb" });
	m_Opcodes.AddOpcode({ { 0xE3 }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "addd" });
	m_Opcodes.AddOpcode({ { 0xE4 }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "andb" });
	m_Opcodes.AddOpcode({ { 0xE5 }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "bitb" });
	m_Opcodes.AddOpcode({ { 0xE6 }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "ldab" });
	m_Opcodes.AddOpcode({ { 0xE7 }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "stab" });
	m_Opcodes.AddOpcode({ { 0xE8 }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "eorb" });
	m_Opcodes.AddOpcode({ { 0xE9 }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "adcb" });
	m_Opcodes.AddOpcode({ { 0xEA }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "orab" });
	m_Opcodes.AddOpcode({ { 0xEB }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "addb" });
	m_Opcodes.AddOpcode({ { 0xEC }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "ldd" });
	m_Opcodes.AddOpcode({ { 0xED }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "std" });
	m_Opcodes.AddOpcode({ { 0xEE }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "ldx" });
	m_Opcodes.AddOpcode({ { 0xEF }, { 0xFF }, MAKEOGRP(0x0, 0xA), MAKEOCTL(0x0, 0x0), "stx" });
	m_Opcodes.AddOpcode({ { 0xF0 }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "subb" });
	m_Opcodes.AddOpcode({ { 0xF1 }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "cmpb" });
	m_Opcodes.AddOpcode({ { 0xF2 }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "sbcb" });
	m_Opcodes.AddOpcode({ { 0xF3 }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "addd" });
	m_Opcodes.AddOpcode({ { 0xF4 }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "andb" });
	m_Opcodes.AddOpcode({ { 0xF5 }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "bitb" });
	m_Opcodes.AddOpcode({ { 0xF6 }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "ldab" });
	m_Opcodes.AddOpcode({ { 0xF7 }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "stab" });
	m_Opcodes.AddOpcode({ { 0xF8 }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "eorb" });
	m_Opcodes.AddOpcode({ { 0xF9 }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "adcb" });
	m_Opcodes.AddOpcode({ { 0xFA }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "orab" });
	m_Opcodes.AddOpcode({ { 0xFB }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "addb" });
	m_Opcodes.AddOpcode({ { 0xFC }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "ldd" });
	m_Opcodes.AddOpcode({ { 0xFD }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "std" });
	m_Opcodes.AddOpcode({ { 0xFE }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "ldx" });
	m_Opcodes.AddOpcode({ { 0xFF }, { 0xFF }, MAKEOGRP(0x0, 0x2), MAKEOCTL(0x0, 0x1), "stx" });

	m_Memory.push_back(CMemBlock{ 0x0ul, 0x0ul, true, 0x10000ul, 0x00 });	// 64K of memory available to processor as one block
}

// ----------------------------------------------------------------------------

unsigned int CM6811Disassembler::GetVersionNumber() const
{
	return (CDisassembler::GetVersionNumber() | VERSION);			// Get GDC version and append our version to it
}

std::string CM6811Disassembler::GetGDCLongName() const
{
	return "M6811 Disassembler";
}

std::string CM6811Disassembler::GetGDCShortName() const
{
	return "M6811";
}

// ----------------------------------------------------------------------------

bool CM6811Disassembler::ReadNextObj(bool bTagMemory, std::ostream *msgFile, std::ostream *errFile)
{
	// Here, we'll read the next object code from memory and flag the memory as being
	//	code, unless an illegal opcode is encountered whereby it will be flagged as
	//	illegal code.  m_OpMemory is cleared and loaded with the memory bytes for the
	//	opcode that was processed.  m_CurrentOpcode will be also be set to the opcode
	//	that corresponds to the one found.  m_PC will also be incremented by the number
	//	of bytes in the complete opcode and in the case of an illegal opcode, it will
	//	increment only by 1 byte so we can then test the following byte to see if it
	//	is legal to allow us to get back on track.  This function will return True
	//	if the opcode was correct and properly formed and fully existed in memory.  It
	//	will return false if the opcode was illegal or if the opcode crossed over the
	//	bounds of loaded source memory.
	// If bTagMemory is false, then memory descriptors aren't changed!  This is useful
	//	if on-the-fly disassembly is desired without modifying the descriptors, such
	//	as in pass 2 of the disassembly process.

	TM6811Disassembler::TOpcodeSymbol nFirstByte;
	TAddress nSavedPC;
	bool bFlag;

	m_sFunctionalOpcode.clear();

	nFirstByte = m_Memory.element(m_PC++);
	m_OpMemory.clear();
	m_OpMemory.push_back(nFirstByte);
	if (IsAddressLoaded(m_PC-1, 1) == false) return false;

	COpcodeTable<TM6811Disassembler>::const_iterator itrOpcode = m_Opcodes.find(nFirstByte);
	bFlag = false;
	if (itrOpcode != m_Opcodes.cend()) {
		for (COpcodeEntryArray<TM6811Disassembler>::size_type ndx = 0; ((ndx < itrOpcode->second.size()) && !bFlag); ++ndx) {
			m_CurrentOpcode = itrOpcode->second.at(ndx);
			bFlag = true;
			for (TM6811Disassembler::COpcodeSymbolArray::size_type i=1; ((i<m_CurrentOpcode.opcode().size()) && (bFlag)); ++i) {
				if (m_CurrentOpcode.opcode().at(i) != m_Memory.element(m_PC+i-1)) bFlag = false;
			}
		}
	}
	if (!bFlag || !IsAddressLoaded(m_PC, m_CurrentOpcode.opcode().size()-1)) {
		if (bTagMemory) m_Memory.setDescriptor(m_PC-1, DMEM_ILLEGALCODE);
		return false;
	}

	// If we get here, then we've found a valid matching opcode.  m_CurrentOpcode has been set to the opcode value
	//	so we must now finish copying to m_OpMemory, tag the bytes in memory, increment m_PC, and call CompleteObjRead.
	//	If CompleteObjRead returns FALSE, then we'll have to undo everything and return flagging an invalid opcode.
	nSavedPC = m_PC;	// Remember m_PC in case we have to undo things (remember, m_PC has already been incremented by 1)!!
	for (TM6811Disassembler::COpcodeSymbolArray::size_type i=1; i<m_CurrentOpcode.opcode().size(); ++i) {			// Finish copying opcode, but don't tag memory until dependent code is called successfully
		m_OpMemory.push_back(m_Memory.element(m_PC++));
	}

	if (!CompleteObjRead(true, msgFile, errFile) || !IsAddressLoaded(nSavedPC-1, m_OpMemory.size()))  {
		// Undo things here:
		m_OpMemory.clear();
		m_OpMemory.push_back(nFirstByte);		// Keep only the first byte in OpMemory for illegal opcode id
		m_PC = nSavedPC;
		if (bTagMemory) m_Memory.setDescriptor(m_PC-1, DMEM_ILLEGALCODE);
		return false;
	}


//	assert(CompleteObjRead(true, msgFile, errFile));


	--nSavedPC;
	for (decltype(m_OpMemory)::size_type i=0; i<m_OpMemory.size(); ++i) {		// CompleteObjRead will add bytes to OpMemory, so we simply have to flag memory for that many bytes.  m_PC is already incremented by CompleteObjRead
		if (bTagMemory) m_Memory.setDescriptor(nSavedPC, DMEM_CODE);
		++nSavedPC;
	}
	assert(nSavedPC == m_PC);		// If these aren't equal, something is wrong in the CompleteObjRead!  m_PC should be incremented for every byte added to m_OpMemory by the complete routine!
	return true;
}

bool CM6811Disassembler::CompleteObjRead(bool bAddLabels, std::ostream *msgFile, std::ostream *errFile)
{
	// Procedure to finish reading opcode from memory.  This procedure reads operand bytes,
	//	placing them in m_OpMemory.  Plus branch/labels and data/labels are generated (according to function).

	bool bA;
	bool bB;
	char strTemp[10];

	m_bCBDef = true;							// On HC11 all conditionally defined are defined, unlike 6809

	m_nOpPointer = m_OpMemory.size();			// Get position of start of operands following opcode
	m_nStartPC = m_PC - m_OpMemory.size();		// Get PC of first byte of this instruction for branch references

	// Note: We'll start with the absolute address.  Since we don't know what the relative address is
	//		to the start of the function, then the function outputting function (which should know it),
	//		will have to add it in:

	std::sprintf(strTemp, "%04X|", m_nStartPC);
	m_sFunctionalOpcode = strTemp;

	// Move Bytes to m_OpMemory:
	bA = MoveOpcodeArgs(OGRP_DST());
	bB = MoveOpcodeArgs(OGRP_SRC());
	if (!bA || !bB) return false;

	CLabelTableMap::const_iterator itrLabel = m_LabelTable.find(m_nStartPC);
	if (itrLabel != m_LabelTable.cend()) {
		for (CLabelArray::size_type i=0; i<itrLabel->second.size(); ++i) {
			if (i != 0) m_sFunctionalOpcode += ",";
			m_sFunctionalOpcode += FormatLabel(LC_REF, itrLabel->second.at(i), m_nStartPC);
		}
	}

	m_sFunctionalOpcode += "|";

	for (decltype(m_OpMemory)::size_type i=0; i<m_OpMemory.size(); ++i) {
		std::sprintf(strTemp, "%02X", m_OpMemory.at(i));
		m_sFunctionalOpcode += strTemp;
	}

	m_sFunctionalOpcode += "|";

	for (decltype(m_OpMemory)::size_type i=0; i<m_nOpPointer; ++i) {
		std::sprintf(strTemp, "%02X", m_OpMemory.at(i));
		m_sFunctionalOpcode += strTemp;
	}

	m_sFunctionalOpcode += "|";

	for (decltype(m_OpMemory)::size_type i=m_nOpPointer; i<m_OpMemory.size(); ++i) {
		std::sprintf(strTemp, "%02X", m_OpMemory.at(i));
		m_sFunctionalOpcode += strTemp;
	}

	m_sFunctionalOpcode += "|";

	// Decode Operands:
	bA = DecodeOpcode(OGRP_DST(), OCTL_DST(), bAddLabels, msgFile, errFile);

	m_sFunctionalOpcode += "|";

	bB = DecodeOpcode(OGRP_SRC(), OCTL_SRC(), bAddLabels, msgFile, errFile);
	if (!bA || !bB) return false;

	m_sFunctionalOpcode += "|";
	m_sFunctionalOpcode += FormatMnemonic(MC_OPCODE);
	m_sFunctionalOpcode += "|";
	m_sFunctionalOpcode += FormatOperands(MC_OPCODE);

	// See if this is the end of a function.  Note: These preempt all previously
	//		decoded function tags -- such as call, etc:
	if ((compareNoCase(m_CurrentOpcode.mnemonic(), "rts") == 0) ||
		(compareNoCase(m_CurrentOpcode.mnemonic(), "rti") == 0)) {
		m_FunctionsTable[m_nStartPC] = FUNCF_HARDSTOP;
	}

	return true;
}

bool CM6811Disassembler::CurrentOpcodeIsStop() const
{
	return ((m_CurrentOpcode.control() & TM6811Disassembler::OCTL_STOP) != 0);
}

// ----------------------------------------------------------------------------

bool CM6811Disassembler::RetrieveIndirect(std::ostream *msgFile, std::ostream *errFile)
{
	UNUSED(msgFile);
	UNUSED(errFile);

	MEM_DESC b1d, b2d;

	m_OpMemory.clear();
	m_OpMemory.push_back(m_Memory.element(m_PC));
	m_OpMemory.push_back(m_Memory.element(m_PC+1));
	// All HC11 indirects are Motorola format and are 2-bytes in length, regardless:
	b1d = static_cast<MEM_DESC>(m_Memory.descriptor(m_PC));
	b2d = static_cast<MEM_DESC>(m_Memory.descriptor(m_PC+1));
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

std::string CM6811Disassembler::FormatMnemonic(MNEMONIC_CODE nMCCode)
{
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

std::string CM6811Disassembler::FormatOperands(MNEMONIC_CODE nMCCode)
{
	std::string strOpStr;
	char strTemp[30];
	TAddress nAddress;

	switch (nMCCode) {
		case MC_ILLOP:
		case MC_DATABYTE:
			for (decltype(m_OpMemory)::size_type i=0; i<m_OpMemory.size(); ++i) {
				std::sprintf(strTemp, "%s%02X", GetHexDelim().c_str(), m_OpMemory.at(i));
				strOpStr += strTemp;
				if (i < (m_OpMemory.size()-1)) strOpStr += ",";
			}
			break;
		case MC_ASCII:
			strOpStr += DataDelim;
			for (decltype(m_OpMemory)::size_type i=0; i<m_OpMemory.size(); ++i) {
				std::sprintf(strTemp, "%c", m_OpMemory.at(i));
				strOpStr += strTemp;
			}
			strOpStr += DataDelim;
			break;
		case MC_EQUATE:
			std::sprintf(strTemp, "%s%04X", GetHexDelim().c_str(), m_PC);
			strOpStr = strTemp;
			break;
		case MC_INDIRECT:
		{
			if (m_OpMemory.size() != 2) {
				assert(false);			// Check code and make sure RetrieveIndirect got called and called correctly!
				break;
			}
			nAddress = m_OpMemory.at(0)*256+m_OpMemory.at(1);
			std::string strLabel;
			CAddressLabelMap::const_iterator itrLabel;
			if ((itrLabel = m_CodeIndirectTable.find(m_PC-2)) != m_CodeIndirectTable.cend()) {
				strLabel = itrLabel->second;
			} else if ((itrLabel = m_DataIndirectTable.find(m_PC-2)) != m_DataIndirectTable.cend()) {
				strLabel = itrLabel->second;
			}
			strOpStr = FormatLabel(LC_REF, strLabel, nAddress);
			break;
		}
		case MC_OPCODE:
			m_nOpPointer = m_CurrentOpcode.opcode().size();	// Get position of start of operands following opcode
			m_nStartPC = m_PC - m_OpMemory.size();			// Get PC of first byte of this instruction for branch references

			CreateOperand(OGRP_DST(), strOpStr);			// Handle Destination operand first
			if (OGRP_DST() != 0x0) strOpStr += ",";
			CreateOperand(OGRP_SRC(), strOpStr);			// Handle Source operand last
			break;
		default:
			assert(false);
			break;
	}

	return strOpStr;
}

std::string CM6811Disassembler::FormatComments(MNEMONIC_CODE nMCCode)
{
	std::string strRetVal;
	bool bBranchOutside;
	TAddress nAddress;

	// Add user comments:
	strRetVal = FormatUserComments(nMCCode, m_PC - m_OpMemory.size());
	if (nMCCode == MC_OPCODE) {
		m_nOpPointer = m_CurrentOpcode.opcode().size();	// Get position of start of operands following opcode

		std::string strRefComment = FormatOperandRefComments(OGRP_DST());
		if (!strRefComment.empty()) {
			if (!strRetVal.empty()) strRetVal += "\n";
			strRetVal += strRefComment;
		}
		strRefComment = FormatOperandRefComments(OGRP_SRC());
		if (!strRefComment.empty()) {
			if (!strRetVal.empty()) strRetVal += "\n";
			strRetVal += strRefComment;
		}
	}

	// Handle Undetermined Branch:
	switch (nMCCode) {
		case MC_OPCODE:
			if ((OCTL_SRC() == 0x2) || (OCTL_DST() == 0x2) ||
				((!m_bCBDef) && ((OCTL_SRC() == 0x4) || (OCTL_DST() == 0x4)))) {
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
			m_nOpPointer = m_CurrentOpcode.opcode().size();	// Get position of start of operands following opcode
			m_nStartPC = m_PC - m_OpMemory.size();		// Get PC of first byte of this instruction for branch references
			switch (OCTL_DST()) {
				case 0x2:
				case 0x3:
				case 0x4:
					if (!CheckBranchOutside(OGRP_DST())) bBranchOutside = true;
					break;
			}
			switch (OCTL_SRC()) {
				case 0x2:
				case 0x3:
				case 0x4:
					if (!CheckBranchOutside(OGRP_SRC())) bBranchOutside = true;
					break;
			}
			break;
		case MC_INDIRECT:
			nAddress = m_OpMemory.at(0)*256+m_OpMemory.at(1);
			if (m_CodeIndirectTable.contains(m_PC-2)) {
				if (!IsAddressLoaded(nAddress, 1)) bBranchOutside = true;
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
	strRetVal += FormatReferences(nMCCode, m_PC - m_OpMemory.size());		// Add references

	return strRetVal;
}

// ----------------------------------------------------------------------------

std::string CM6811Disassembler::FormatOpBytes()
{
	std::ostringstream sstrTemp;

	for (decltype(m_OpMemory)::size_type i=0; i<m_OpMemory.size(); ++i) {
		if (i) sstrTemp << " ";
		sstrTemp << std::uppercase << std::setfill('0') << std::setw(2) << std::setbase(16) << static_cast<unsigned int>(m_OpMemory.at(i))
					<< std::nouppercase << std::setbase(0);
	}
	return sstrTemp.str();
}

std::string CM6811Disassembler::FormatLabel(LABEL_CODE nLC, const TLabel & strLabel, TAddress nAddress)
{
	std::string strTemp = CDisassembler::FormatLabel(nLC, strLabel, nAddress);	// Call parent

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

bool CM6811Disassembler::WritePreSection(std::ostream& outFile, std::ostream *msgFile, std::ostream *errFile)
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

bool CM6811Disassembler::ResolveIndirect(TAddress nAddress, TAddress& nResAddress, REFERENCE_TYPE nType)
{
	// In the HC11, we can assume that all indirect addresses are 2-bytes in length
	//	and stored in little endian format.
	if (!IsAddressLoaded(nAddress, 2) ||				// Not only must it be loaded, but we must have never examined it before!
		(m_Memory.descriptor(nAddress) != DMEM_LOADED) ||
		(m_Memory.descriptor(nAddress+1) != DMEM_LOADED)) {
		nResAddress = 0;
		return false;
	}
	TAddress nVector = m_Memory.element(nAddress) * 256ul + m_Memory.element(nAddress + 1);
	nResAddress = nVector;
	m_Memory.setDescriptor(nAddress, static_cast<TDescElement>(DMEM_CODEINDIRECT) + nType);		// Flag the addresses as being the proper vector type
	m_Memory.setDescriptor(nAddress+1, static_cast<TDescElement>(DMEM_CODEINDIRECT) + nType);
	return true;
}

// ----------------------------------------------------------------------------

std::string CM6811Disassembler::GetExcludedPrintChars() const
{
	return "\\" DataDelim;
}

std::string CM6811Disassembler::GetHexDelim() const
{
	return HexDelim;
}

std::string CM6811Disassembler::GetCommentStartDelim() const
{
	return ComDelimS;
}

std::string CM6811Disassembler::GetCommentEndDelim() const
{
	return ComDelimE;
}

// ----------------------------------------------------------------------------

void CM6811Disassembler::clearOpMemory()
{
	m_OpMemory.clear();
}

void CM6811Disassembler::pushBackOpMemory(TAddress nLogicalAddress, TMemoryElement nValue)
{
	UNUSED(nLogicalAddress);
	m_OpMemory.push_back(nValue);
}

void CM6811Disassembler::pushBackOpMemory(TAddress nLogicalAddress, const CMemoryArray &maValues)
{
	UNUSED(nLogicalAddress);
	m_OpMemory.insert(m_OpMemory.end(), maValues.cbegin(), maValues.cend());
}

// ----------------------------------------------------------------------------

bool CM6811Disassembler::MoveOpcodeArgs(TM6811Disassembler::TGroupFlags nGroup)
{
	switch (nGroup) {
		case 0x2:
		case 0x4:
		case 0x6:
		case 0x7:
		case 0x8:
		case 0x9:
			m_OpMemory.push_back(m_Memory.element(m_PC++));
			// Fall through and do it again:
		case 0x1:
		case 0x3:
		case 0x5:
		case 0xA:
		case 0xB:
			m_OpMemory.push_back(m_Memory.element(m_PC++));
			break;
		case 0x0:
			break;
		default:
			return false;
	}
	return true;
}

bool CM6811Disassembler::DecodeOpcode(TM6811Disassembler::TGroupFlags nGroup, TM6811Disassembler::TControlFlags nControl, bool bAddLabels, std::ostream *msgFile, std::ostream *errFile)
{
	bool bRetVal = true;

	char strTemp[40];
	char strTemp2[30];
	TAddress nTargetAddr;

	strTemp[0] = 0;
	nTargetAddr = 0xFFFFFFFFul;

	switch (nGroup) {
		case 0x0:								// No operand
			break;
		case 0x1:								// absolute 8-bit, assume msb=$00
			switch (nControl) {
				case 1:
					if (bAddLabels) GenDataLabel(m_OpMemory.at(m_nOpPointer), m_nStartPC, TLabel(), msgFile, errFile);
					std::sprintf(strTemp, "D@%02lX", static_cast<unsigned long>(m_OpMemory.at(m_nOpPointer)));
					break;
				case 3:
					if (bAddLabels) GenAddrLabel(m_OpMemory.at(m_nOpPointer), m_nStartPC, TLabel(), msgFile, errFile);
					std::sprintf(strTemp, "C@%02lX", static_cast<unsigned long>(m_OpMemory.at(m_nOpPointer)));
					nTargetAddr = m_OpMemory.at(m_nOpPointer);
					break;
				default:
					bRetVal = false;
			}
			m_nOpPointer++;
			break;
		case 0x2:								// 16-bit Absolute
			switch (nControl) {
				case 1:
					if (bAddLabels) GenDataLabel(m_OpMemory.at(m_nOpPointer)*256+m_OpMemory.at(m_nOpPointer+1), m_nStartPC, TLabel(), msgFile, errFile);
					std::sprintf(strTemp, "D@%04lX", static_cast<unsigned long>(m_OpMemory.at(m_nOpPointer)*256+m_OpMemory.at(m_nOpPointer+1)));
					break;
				case 3:
					if (bAddLabels) GenAddrLabel(m_OpMemory.at(m_nOpPointer)*256+m_OpMemory.at(m_nOpPointer+1), m_nStartPC, TLabel(), msgFile, errFile);
					std::sprintf(strTemp, "C@%04lX", static_cast<unsigned long>(m_OpMemory.at(m_nOpPointer)*256+m_OpMemory.at(m_nOpPointer+1)));
					nTargetAddr = m_OpMemory.at(m_nOpPointer)*256+m_OpMemory.at(m_nOpPointer+1);
					break;
				default:
					bRetVal = false;
			}
			m_nOpPointer += 2;
			break;
		case 0x3:								// 8-bit Relative
			switch (nControl) {
				case 1:
					if (bAddLabels) GenDataLabel(m_PC+(signed long)(signed char)(m_OpMemory.at(m_nOpPointer)), m_nStartPC, TLabel(), msgFile, errFile);
					std::sprintf(strTemp, "D^%c%02lX(%04lX)",
											((((signed long)(signed char)(m_OpMemory.at(m_nOpPointer))) !=
											labs((signed long)(signed char)(m_OpMemory.at(m_nOpPointer)))) ? '-' : '+'),
											labs((signed long)(signed char)(m_OpMemory.at(m_nOpPointer))),
											m_PC+(signed long)(signed char)(m_OpMemory.at(m_nOpPointer)));
					break;
				case 3:
					if (bAddLabels) GenAddrLabel(m_PC+(signed long)(signed char)(m_OpMemory.at(m_nOpPointer)), m_nStartPC, TLabel(), msgFile, errFile);
					std::sprintf(strTemp, "C^%c%02lX(%04lX)",
											((((signed long)(signed char)(m_OpMemory.at(m_nOpPointer))) !=
											labs((signed long)(signed char)(m_OpMemory.at(m_nOpPointer)))) ? '-' : '+'),
											labs((signed long)(signed char)(m_OpMemory.at(m_nOpPointer))),
											m_PC+(signed long)(signed char)(m_OpMemory.at(m_nOpPointer)));
					nTargetAddr = m_PC+(signed long)(signed char)(m_OpMemory.at(m_nOpPointer));
					break;
				default:
					bRetVal = false;
			}
			m_nOpPointer++;
			break;
		case 0x4:								// 16-bit Relative
			switch (nControl) {
				case 1:
					if (bAddLabels) GenDataLabel(m_PC+(signed long)(signed short)(m_OpMemory.at(m_nOpPointer)*256+m_OpMemory.at(m_nOpPointer+1)), m_nStartPC, TLabel(), msgFile, errFile);
					std::sprintf(strTemp, "D^%c%04lX(%04lX)",
											((((signed long)(signed short)(m_OpMemory.at(m_nOpPointer)*256+m_OpMemory.at(m_nOpPointer+1))) !=
											labs((signed long)(signed short)(m_OpMemory.at(m_nOpPointer)*256+m_OpMemory.at(m_nOpPointer+1)))) ? '-' : '+'),
											labs((signed long)(signed short)(m_OpMemory.at(m_nOpPointer)*256+m_OpMemory.at(m_nOpPointer+1))),
											m_PC+(signed long)(signed short)(m_OpMemory.at(m_nOpPointer)*256+m_OpMemory.at(m_nOpPointer+1)));
					break;
				case 3:
					if (bAddLabels) GenAddrLabel(m_PC+(signed long)(signed short)(m_OpMemory.at(m_nOpPointer)*256+m_OpMemory.at(m_nOpPointer+1)), m_nStartPC, TLabel(), msgFile, errFile);
					std::sprintf(strTemp, "C^%c%04lX(%04lX)",
											((((signed long)(signed short)(m_OpMemory.at(m_nOpPointer)*256+m_OpMemory.at(m_nOpPointer+1))) !=
											labs((signed long)(signed short)(m_OpMemory.at(m_nOpPointer)*256+m_OpMemory.at(m_nOpPointer+1)))) ? '-' : '+'),
											labs((signed long)(signed short)(m_OpMemory.at(m_nOpPointer)*256+m_OpMemory.at(m_nOpPointer+1))),
											m_PC+(signed long)(signed short)(m_OpMemory.at(m_nOpPointer)*256+m_OpMemory.at(m_nOpPointer+1)));
					nTargetAddr = m_PC+(signed long)(signed short)(m_OpMemory.at(m_nOpPointer)*256+m_OpMemory.at(m_nOpPointer+1));
					break;
				default:
					bRetVal = false;
			}
			m_nOpPointer += 2;
			break;
		case 0x5:								// Immediate 8-bit data -- no label
			std::sprintf(strTemp, "#%02lX", static_cast<unsigned long>(m_OpMemory.at(m_nOpPointer)));
			m_nOpPointer++;
			break;
		case 0x6:								// Immediate 16-bit data -- no label
			std::sprintf(strTemp, "#%04lX", static_cast<unsigned long>(m_OpMemory.at(m_nOpPointer)*256+m_OpMemory.at(m_nOpPointer+1)));
			m_nOpPointer += 2;
			break;
		case 0x7:								// 8-bit Absolute address, and mask
			strTemp2[0] = 0;
			switch (nControl) {
				case 1:
					if (bAddLabels) GenDataLabel(m_OpMemory.at(m_nOpPointer), m_nStartPC, TLabel(), msgFile, errFile);
					std::sprintf(strTemp2, "D@%02lX", static_cast<unsigned long>(m_OpMemory.at(m_nOpPointer)));
					break;
				case 3:
					if (bAddLabels) GenAddrLabel(m_OpMemory.at(m_nOpPointer), m_nStartPC, TLabel(), msgFile, errFile);
					std::sprintf(strTemp2, "C@%02lX", static_cast<unsigned long>(m_OpMemory.at(m_nOpPointer)));
					nTargetAddr = m_OpMemory.at(m_nOpPointer);
					break;
				default:
					bRetVal = false;
			}
			std::sprintf(strTemp, "%s,M%02lX", strTemp2, static_cast<unsigned long>(m_OpMemory.at(m_nOpPointer+1)));
			m_nOpPointer += 2;
			break;
		case 0x8:								// 8-bit offset and 8-bit mask (x)
			strTemp2[0] = 0;
			switch (nControl) {
				case 0:
				case 1:
					std::sprintf(strTemp2, "D&%02lX(x)", static_cast<unsigned long>(m_OpMemory.at(m_nOpPointer)));
					break;

				case 2:
				case 3:
				case 4:
					std::sprintf(strTemp2, "C&%02lX(x)", static_cast<unsigned long>(m_OpMemory.at(m_nOpPointer)));
					break;
			}
			std::sprintf(strTemp, "%s,M%02lX", strTemp2, static_cast<unsigned long>(m_OpMemory.at(m_nOpPointer+1)));
			m_nOpPointer += 2;
			break;
		case 0x9:								// 8-bit offset and 8-bit mask (y)
			strTemp2[0] = 0;
			switch (nControl) {
				case 0:
				case 1:
					std::sprintf(strTemp2, "D&%02lX(y)", static_cast<unsigned long>(m_OpMemory.at(m_nOpPointer)));
					break;

				case 2:
				case 3:
				case 4:
					std::sprintf(strTemp2, "C&%02lX(y)", static_cast<unsigned long>(m_OpMemory.at(m_nOpPointer)));
					break;
			}
			std::sprintf(strTemp, "%s,M%02lX", strTemp2, static_cast<unsigned long>(m_OpMemory.at(m_nOpPointer+1)));
			m_nOpPointer += 2;
			break;
		case 0xA:								// 8-bit offset (x)
			switch (nControl) {
				case 0:
				case 1:
					std::sprintf(strTemp, "D&%02lX(x)", static_cast<unsigned long>(m_OpMemory.at(m_nOpPointer)));
					break;

				case 2:
				case 3:
				case 4:
					std::sprintf(strTemp, "C&%02lX(x)", static_cast<unsigned long>(m_OpMemory.at(m_nOpPointer)));
					break;
			}
			m_nOpPointer++;
			break;
		case 0xB:								// 8-bit offset (y)
			switch (nControl) {
				case 0:
				case 1:
					std::sprintf(strTemp, "D&%02lX(y)", static_cast<unsigned long>(m_OpMemory.at(m_nOpPointer)));
					break;

				case 2:
				case 3:
				case 4:
					std::sprintf(strTemp, "C&%02lX(y)", static_cast<unsigned long>(m_OpMemory.at(m_nOpPointer)));
					break;
			}
			m_nOpPointer++;
			break;
		default:
			bRetVal = false;
	}

	m_sFunctionalOpcode += strTemp;

	// See if this is a function branch reference that needs to be added:
	if (nTargetAddr != 0xFFFFFFFFul) {
		if ((compareNoCase(m_CurrentOpcode.mnemonic(), "jsr") == 0) ||
			(compareNoCase(m_CurrentOpcode.mnemonic(), "bsr") == 0)) {
			// Add these only if it is replacing a lower priority value:
			CFuncMap::const_iterator itrFunction = m_FunctionsTable.find(nTargetAddr);
			if (itrFunction == m_FunctionsTable.cend()) {
				m_FunctionsTable[nTargetAddr] = FUNCF_CALL;
			} else {
				if (itrFunction->second > FUNCF_CALL) m_FunctionsTable[nTargetAddr] = FUNCF_CALL;
			}

			// See if this is the end of a function:
			if (m_FuncExitAddresses.contains(nTargetAddr)) {
				m_FunctionsTable[m_nStartPC] = FUNCF_EXITBRANCH;
			}
		}

		if ((compareNoCase(m_CurrentOpcode.mnemonic(), "brset") == 0) ||
			(compareNoCase(m_CurrentOpcode.mnemonic(), "brclr") == 0) ||
			(compareNoCase(m_CurrentOpcode.mnemonic(), "brn") == 0) ||
			(compareNoCase(m_CurrentOpcode.mnemonic(), "bhi") == 0) ||
			(compareNoCase(m_CurrentOpcode.mnemonic(), "bls") == 0) ||
			(compareNoCase(m_CurrentOpcode.mnemonic(), "bcc") == 0) ||
			(compareNoCase(m_CurrentOpcode.mnemonic(), "bcs") == 0) ||
			(compareNoCase(m_CurrentOpcode.mnemonic(), "bne") == 0) ||
			(compareNoCase(m_CurrentOpcode.mnemonic(), "beq") == 0) ||
			(compareNoCase(m_CurrentOpcode.mnemonic(), "bvc") == 0) ||
			(compareNoCase(m_CurrentOpcode.mnemonic(), "bvs") == 0) ||
			(compareNoCase(m_CurrentOpcode.mnemonic(), "bpl") == 0) ||
			(compareNoCase(m_CurrentOpcode.mnemonic(), "bmi") == 0) ||
			(compareNoCase(m_CurrentOpcode.mnemonic(), "bge") == 0) ||
			(compareNoCase(m_CurrentOpcode.mnemonic(), "blt") == 0) ||
			(compareNoCase(m_CurrentOpcode.mnemonic(), "bgt") == 0) ||
			(compareNoCase(m_CurrentOpcode.mnemonic(), "ble") == 0) ||
			(compareNoCase(m_CurrentOpcode.mnemonic(), "bra") == 0) ||
			(compareNoCase(m_CurrentOpcode.mnemonic(), "jmp") == 0)) {
			// Add these only if there isn't a function tag here:
			if (!m_FunctionsTable.contains(nTargetAddr)) m_FunctionsTable[nTargetAddr] = FUNCF_BRANCHIN;
		}
	}

	// See if this is the end of a function:
	if ((compareNoCase(m_CurrentOpcode.mnemonic(), "jmp") == 0) ||
		(compareNoCase(m_CurrentOpcode.mnemonic(), "bra") == 0)) {
		if (nTargetAddr != 0xFFFFFFFFul) {
			if (m_FuncExitAddresses.contains(nTargetAddr)) {
				m_FunctionsTable[m_nStartPC] = FUNCF_EXITBRANCH;
			} else {
				m_FunctionsTable[m_nStartPC] = FUNCF_BRANCHOUT;
			}
		} else {
			// Non-Deterministic branches get tagged as a branchout:
			//m_FunctionsTable[m_nStartPC] = FUNCF_BRANCHOUT;

			// Non-Deterministic branches get tagged as a hardstop (usually it exits the function):
			m_FunctionsTable[m_nStartPC] = FUNCF_HARDSTOP;
		}
	}

	return bRetVal;
}

void CM6811Disassembler::CreateOperand(TM6811Disassembler::TGroupFlags nGroup, std::string& strOpStr)
{
	char strTemp[30];

	switch (nGroup) {
		case 0x1:								// absolute 8-bit, assume msb=$00
			strOpStr += "*";
			strOpStr += LabelDeref2(m_OpMemory.at(m_nOpPointer));
			m_nOpPointer++;
			break;
		case 0x2:								// 16-bit Absolute
			strOpStr += LabelDeref4(m_OpMemory.at(m_nOpPointer)*256+m_OpMemory.at(m_nOpPointer+1));
			m_nOpPointer += 2;
			break;
		case 0x3:								// 8-bit Relative
			strOpStr += LabelDeref4(m_PC+(signed long)(signed char)(m_OpMemory.at(m_nOpPointer)));
			m_nOpPointer++;
			break;
		case 0x4:								// 16-bit Relative
			strOpStr += LabelDeref4(m_PC+(signed long)(signed short)(m_OpMemory.at(m_nOpPointer)*256+m_OpMemory.at(m_nOpPointer+1)));
			m_nOpPointer += 2;
			break;
		case 0x5:								// Immediate 8-bit data
			std::sprintf(strTemp, "#%s%02X", GetHexDelim().c_str(), m_OpMemory.at(m_nOpPointer));
			strOpStr += strTemp;
			m_nOpPointer++;
			break;
		case 0x6:								// Immediate 16-bit data
			std::sprintf(strTemp, "#%s%04X", GetHexDelim().c_str(), m_OpMemory.at(m_nOpPointer)*256+m_OpMemory.at(m_nOpPointer+1));
			strOpStr += strTemp;
			m_nOpPointer += 2;
			break;
		case 0x7:								// 8-bit Absolute address, and mask
			strOpStr += "*";
			strOpStr += LabelDeref2(m_OpMemory.at(m_nOpPointer));
			strOpStr += ",";
			std::sprintf(strTemp, "#%s%02X", GetHexDelim().c_str(), m_OpMemory.at(m_nOpPointer+1));
			strOpStr += strTemp;
			m_nOpPointer += 2;
			break;
		case 0x8:								// 8-bit offset and 8-bit mask (x)
			std::sprintf(strTemp, "%s%02X,x,#%s%02X", GetHexDelim().c_str(), m_OpMemory.at(m_nOpPointer),
													  GetHexDelim().c_str(), m_OpMemory.at(m_nOpPointer+1));
			strOpStr += strTemp;
			m_nOpPointer += 2;
			break;
		case 0x9:								// 8-bit offset and 8-bit mask (y)
			std::sprintf(strTemp, "%s%02X,y,#%s%02X", GetHexDelim().c_str(), m_OpMemory.at(m_nOpPointer),
													  GetHexDelim().c_str(), m_OpMemory.at(m_nOpPointer+1));
			strOpStr += strTemp;
			m_nOpPointer += 2;
			break;
		case 0xA:								// 8-bit offset (x)
			std::sprintf(strTemp, "%s%02X,x", GetHexDelim().c_str(), m_OpMemory.at(m_nOpPointer));
			strOpStr += strTemp;
			m_nOpPointer++;
			break;
		case 0xB:								// 8-bit offset (y)
			std::sprintf(strTemp, "%s%02X,y", GetHexDelim().c_str(), m_OpMemory.at(m_nOpPointer));
			strOpStr += strTemp;
			m_nOpPointer++;
			break;
	}
}

std::string CM6811Disassembler::FormatOperandRefComments(TM6811Disassembler::TGroupFlags nGroup)
{
	TAddress nAddress = 0;

	switch (nGroup) {
		case 0x1:								// absolute 8-bit, assume msb=$00
			nAddress = m_OpMemory.at(m_nOpPointer);
			m_nOpPointer++;
			break;
		case 0x2:								// 16-bit Absolute
			nAddress = m_OpMemory.at(m_nOpPointer)*256+m_OpMemory.at(m_nOpPointer+1);
			m_nOpPointer += 2;
			break;
		case 0x3:								// 8-bit Relative
			nAddress = m_PC+(signed long)(signed char)(m_OpMemory.at(m_nOpPointer));
			m_nOpPointer++;
			break;
		case 0x4:								// 16-bit Relative
			nAddress = m_PC+(signed long)(signed short)(m_OpMemory.at(m_nOpPointer)*256+m_OpMemory.at(m_nOpPointer+1));
			m_nOpPointer += 2;
			break;
		case 0x5:								// Immediate 8-bit data
			m_nOpPointer++;
			return std::string();
		case 0x6:								// Immediate 16-bit data
			m_nOpPointer += 2;
			return std::string();
		case 0x7:								// 8-bit Absolute address, and mask
			nAddress = m_OpMemory.at(m_nOpPointer);
			m_nOpPointer += 2;
			break;
		case 0x8:								// 8-bit offset and 8-bit mask (x)
			m_nOpPointer += 2;
			return std::string();
		case 0x9:								// 8-bit offset and 8-bit mask (y)
			m_nOpPointer += 2;
			return std::string();
		case 0xA:								// 8-bit offset (x)
			m_nOpPointer++;
			return std::string();
		case 0xB:								// 8-bit offset (y)
			m_nOpPointer++;
			return std::string();
}

	return FormatUserComments(MC_INDIRECT, nAddress);
}

bool CM6811Disassembler::CheckBranchOutside(TM6811Disassembler::TGroupFlags nGroup)
{
	bool bRetVal = true;
	TAddress nAddress;

	switch (nGroup) {
		case 0x1:								// absolute 8-bit, assume msb=$00
			nAddress = m_OpMemory.at(m_nOpPointer);
			bRetVal = IsAddressLoaded(nAddress, 1);
			m_nOpPointer++;
			break;
		case 0x2:								// 16-bit Absolute
			nAddress = m_OpMemory.at(m_nOpPointer)*256+m_OpMemory.at(m_nOpPointer+1);
			bRetVal = IsAddressLoaded(nAddress, 1);
			m_nOpPointer += 2;
			break;
		case 0x3:								// 8-bit Relative
			nAddress = m_PC+(signed long)(signed char)(m_OpMemory.at(m_nOpPointer));
			bRetVal = IsAddressLoaded(nAddress, 1);
			m_nOpPointer++;
			break;
		case 0x4:								// 16-bit Relative
			nAddress = m_PC+(signed long)(signed short)(m_OpMemory.at(m_nOpPointer)*256+m_OpMemory.at(m_nOpPointer+1));
			bRetVal = IsAddressLoaded(nAddress, 1);
			m_nOpPointer += 2;
			break;
	}
	return bRetVal;
}

TLabel CM6811Disassembler::LabelDeref2(TAddress nAddress)
{
	std::string strTemp;
	char strCharTemp[30];

	CLabelTableMap::const_iterator itrLabel = m_LabelTable.find(nAddress);
	if (itrLabel != m_LabelTable.cend()) {
		if (itrLabel->second.size()) {
			strTemp = FormatLabel(LC_REF, itrLabel->second.at(0), nAddress);
		} else {
			strTemp = FormatLabel(LC_REF, TLabel(), nAddress);
		}
	} else {
		std::sprintf(strCharTemp, "%s%02X", GetHexDelim().c_str(), nAddress);
		strTemp = strCharTemp;
	}
	return strTemp;
}

TLabel CM6811Disassembler::LabelDeref4(TAddress nAddress)
{
	std::string strTemp;
	char strCharTemp[30];

	CLabelTableMap::const_iterator itrLabel = m_LabelTable.find(nAddress);
	if (itrLabel != m_LabelTable.cend()) {
		if (itrLabel->second.size()) {
			strTemp = FormatLabel(LC_REF, itrLabel->second.at(0), nAddress);
		} else {
			strTemp = FormatLabel(LC_REF, TLabel(), nAddress);
		}
	} else {
		std::sprintf(strCharTemp, "%s%04X", GetHexDelim().c_str(), nAddress);
		strTemp = strCharTemp;
	}
	return strTemp;
}

// ============================================================================
