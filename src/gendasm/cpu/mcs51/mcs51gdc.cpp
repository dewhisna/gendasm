//
//	Intel MCS51 (8051) Disassembler GDC Module
//	Copyright(c)2024 by Donna Whisnant
//
//	Generic Code-Seeking Disassembler
//	Copyright(c)2021-2024 by Donna Whisnant
//

//
//	MCS51GDC -- This is the implementation for the MCS51 (8051) Disassembler GDC module
//
//
//	Groups: (Grp) : xy
//	  Where x (msnb = destination = FIRST operand)
//			y (lsnb = source = LAST operand)
//
//			0 = opcode only, nothing follows
//			1 = Register Rn of currently selected register bank (register number encoded with opcode)
//			2 = 8-bit direct internal data address (0-127 internal RAM), (128-255 SFR)
//			3 = R0/R1 indirect 8-bit internal data address (@R0 or @R1, depending on last bit of opcode)
//			4 = 8-bit constant data follows
//			5 = 16-bit constant data follows
//			6 = 16-bit destination address follows (LCALL or LJMP)
//			7 = 11-bit destination address (8-bit value follows, 3-bits in msbits of opcode), 2K-page relative to byte following this instruction
//			8 = 8-bit relative address follows (SJMP and conditional jumps), -128, +127, relative to byte following this instruction
//			9 = Direct Addressed bit in Internal Data RAM or SFR (Special Function Register), This is addresses whose last 3-bits are 0, and last 3-bits are bit index
//			A = Accumulator 'A'
//			B = Accumulator 'AB'
//			C = Carry 'C'
//			D = Data Pointer 'DPTR'
//			E = Accumulator+DPTR Relative '@A+DPTR'
//			F = Accumulator+PC Relative '@A+PC'
//			10 = DPTR Relative '@DPTR'
//			11 = Not Direct Addressed bit in Internal Data RAM or SFR (Special Function Register), This is addresses whose last 3-bits are 0, and last 3-bits are bit index
//

//
//	Control: (Algorithm control) : xy
//	  Where x (msnb = destination = FIRST operand)
//			y (lsnb = source = LAST operand)
//
//			0 = Disassemble as code
//
//			1 = Disassemble as code, with data addr label
//
//			2 = Disassemble as undeterminable branch address (Comment code)
//
//			3 = Disassemble as determinable branch, add branch addr and label
//
//		xxx	4 = Disassemble as conditionally undeterminable branch address (Comment code),
//		xxx			with data addr label, (on opcodes with post-byte, label generation is dependent
//		xxx			upon post-byte value. conditional upon post-byte)
//
//	This version setup for compatibility with the AS8051 assembler
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
//		Indirect Address:
//			=type|addr|name|value
//			   |    |    |    |___  Indirect Address (in proper endian and scaling for MCU)
//			   |    |    |________  Label(s) for this address (comma separated) -- labels for this indirect, NOT the target
//			   |    |_____________  Absolute address of the indirect vector (hex)
//			   |__________________  Indirect Type, C=Code, D=Data
//
//		Start of Data Block:
//			$addr|name
//			   |    |____ Name(s) of Data Block (comma separated list) from labels
//			   |_________ Absolute Adddress of Data Block Start
//
//		Data Byte Line (inside Data Block):
//			xxxx|xxxx|label|xx
//			  |    |    |    |____  Data Bytes (hex) -- multiple of 2-digits per bytes without separation characters (any size)
//			  |    |    |_________  Label(s) for this address (comma separated)
//			  |    |______________  Absolute address of data bytes (hex)
//			  |___________________  Relative address of data bytes to the Data Block (hex)
//
//		Start of New Function:
//			@xxxx|name
//			   |    |____ Name(s) of Function (comma separated list)
//			   |_________ Absolute Address of Function Start relative to overall file (hex)
//
//		Mnemonic Line (inside function):
//			xxxx|xxxx|label|xxxxxxxxxx|xxxxxx|xxxx|DST|SRC|SRC2|mnemonic|operands
//			  |    |    |        |        |     |   |   |    |     |        |____  Disassembled operand output (ascii)
//			  |    |    |        |        |     |   |   |    |     |_____________  Disassembled mnemonic (ascii)
//			  |    |    |        |        |     |   |___|____|______________  See below for SRC/SRC2/DST format
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
//		SRC, SRC2, and DST entries:
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
//		Note: The address sizes are consistent with the particular processor.  For example, an MCS51 will
//			use 16-bit addresses (or 4 hex digits).  The size of immediate data entries, offsets, and masks will
//			reflect the actual value.  A 16-bit immediate value, offset, or mask, will be outputted as 4 hex
//			digits, but an 8-bit immediate value, offset, or mask, will be outputted as only 2 hex digits.
//

#include "mcs51gdc.h"
#include <gdc.h>
#include <stringhelp.h>

#include <sstream>
#include <iomanip>
#include <cstdio>
#include <vector>
#include <iterator>

#include <assert.h>

#ifdef LIBIBERTY_SUPPORT
#include <libiberty/demangle.h>
#endif

constexpr TMCS51Disassembler::TGroupFlags GRP_OPCODE_ONLY = 0;			//			0 = opcode only, nothing follows
constexpr TMCS51Disassembler::TGroupFlags GRP_REGISTER = 1;				//			1 = Register Rn of currently selected register bank (register number encoded with opcode)
constexpr TMCS51Disassembler::TGroupFlags GRP_8BIT_DATA_ADDR = 2;		//			2 = 8-bit direct internal data address (0-127 internal RAM), (128-255 SFR)
constexpr TMCS51Disassembler::TGroupFlags GRP_RIND_DATA_ADDR = 3;		//			3 = R0/R1 indirect 8-bit internal data address (@R0 or @R1, depending on last bit of opcode)
constexpr TMCS51Disassembler::TGroupFlags GRP_8BIT_CONST_DATA = 4;		//			4 = 8-bit constant data follows
constexpr TMCS51Disassembler::TGroupFlags GRP_16BIT_CONST_DATA = 5;		//			5 = 16-bit constant data follows
constexpr TMCS51Disassembler::TGroupFlags GRP_16BIT_ABS_CODE_ADDR = 6;	//			6 = 16-bit destination address follows (LCALL or LJMP)
constexpr TMCS51Disassembler::TGroupFlags GRP_11BIT_PG_CODE_ADDR = 7;	//			7 = 11-bit destination address (8-bit value follows, 3-bits in msbits of opcode), 2K-page relative to byte following this instruction
constexpr TMCS51Disassembler::TGroupFlags GRP_8BIT_REL_CODE_ADDR = 8;	//			8 = 8-bit relative address follows (SJMP and conditional jumps), -128, +127, relative to byte following this instruction
constexpr TMCS51Disassembler::TGroupFlags GRP_BIT_ADDR_DATA_ADDR = 9;	//			9 = Direct Addressed bit in Internal Data RAM or SFR (Special Function Register), This is addresses whose last 3-bits are 0, and last 3-bits are bit index
constexpr TMCS51Disassembler::TGroupFlags GRP_ACCUM = 10;				//			A = Accumulator 'A'
constexpr TMCS51Disassembler::TGroupFlags GRP_AB = 11;					//			B = Accumulator 'AB'
constexpr TMCS51Disassembler::TGroupFlags GRP_CARRY = 12;				//			C = Carry 'C'
constexpr TMCS51Disassembler::TGroupFlags GRP_DPTR = 13;				//			D = Data Pointer 'DPTR'
constexpr TMCS51Disassembler::TGroupFlags GRP_CODE_ACCUM_DPTR = 14;		//			E = Accumulator+DPTR Relative '@A+DPTR'
constexpr TMCS51Disassembler::TGroupFlags GRP_CODE_ACCUM_PC = 15;		//			F = Accumulator+PC Relative '@A+PC'
constexpr TMCS51Disassembler::TGroupFlags GRP_DATA_DPTR = 16;			//			10 = DPTR Relative '@DPTR'
constexpr TMCS51Disassembler::TGroupFlags GRP_NOT_BIT_ADDR_DATA_ADDR = 17;	//			11 = Not Direct Addressed bit in Internal Data RAM or SFR (Special Function Register), This is addresses whose last 3-bits are 0, and last 3-bits are bit index

constexpr TMCS51Disassembler::TControlFlags CTL_CODE_ONLY = 0;			//			0 = Disassemble as code
constexpr TMCS51Disassembler::TControlFlags CTL_CODE_DATA_LABEL = 1;	//			1 = Disassemble as code, with data addr label
constexpr TMCS51Disassembler::TControlFlags CTL_UNDET_BRANCH = 2;		//			2 = Disassemble as undeterminable branch address (Comment code)
constexpr TMCS51Disassembler::TControlFlags CTL_DET_BRANCH = 3;			//			3 = Disassemble as determinable branch, add branch addr and label


// ============================================================================

#define DataDelim	"'"			// Specify ' as delimiter for data literals
#define LblcDelim	":"			// Delimiter between labels and code
#define LbleDelim	""			// Delimiter between labels and equates
#define	ComDelimS	";"			// Comment Start delimiter
#define ComDelimE	""			// Comment End delimiter
#define HexDelim	"0x"		// Hex. delimiter

#define VERSION 0x0100				// MCS51 Disassembler Version number 1.00

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

// ----------------------------------------------------------------------------
//	CMCS51Disassembler
// ----------------------------------------------------------------------------

namespace {
	struct TEntry
	{
		TAddress m_nAddress;
		const std::string m_strLabel;
		CMCS51Disassembler::COMMENT_TYPE_FLAGS m_ctf = CMCS51Disassembler::CTF_ALL;
		const std::string m_strComment = std::string();
	};

	// Special Function Registers:
	static const TEntry g_arrSFRRAM[] = {
		{ 0x80, "P0" },
		{ 0x81, "SP" },
		{ 0x82, "DPL" },
		{ 0x83, "DPH" },
		{ 0x87, "PCON" },
		{ 0x88, "TCON" },
		{ 0x89, "TMOD" },
		{ 0x8A, "TL0" },
		{ 0x8B, "TL1" },
		{ 0x8C, "TH0" },
		{ 0x8D, "TH1" },
		{ 0x90, "P1" },
		{ 0x98, "SCON" },
		{ 0x99, "SBUF" },
		{ 0xA0, "P2" },
		{ 0xA8, "IE" },
		{ 0xB0, "P3" },
		{ 0xB8, "IP" },
		{ 0xC8, "T2CON" },
		{ 0xCA, "RCAP2L" },
		{ 0xCB, "RCAP2H" },
		{ 0xCC, "TL2" },
		{ 0xCD, "TH2" },
		{ 0xD0, "PSW" },
		{ 0xE0, "ACC" },
		{ 0xF0, "B" },
	};

	// SFR Bit Map:
	static const TEntry g_arrSFRBITS[] = {
		{ 0x80, "P0.0" },		// P0
		{ 0x81, "P0.1" },
		{ 0x82, "P0.2" },
		{ 0x83, "P0.3" },
		{ 0x84, "P0.4" },
		{ 0x85, "P0.5" },
		{ 0x86, "P0.6" },
		{ 0x87, "P0.7" },
		// ----
		{ 0x88, "IT0" },		// TCON
		{ 0x89, "IE0" },
		{ 0x8A, "ITI" },
		{ 0x8B, "IE1" },
		{ 0x8C, "TR0" },
		{ 0x8D, "TF0" },
		{ 0x8E, "TR1" },
		{ 0x8F, "TF1" },
		// ----
		{ 0x90, "P1.0" },		// P1
		{ 0x91, "P1.1" },
		{ 0x92, "P1.2" },
		{ 0x93, "P1.3" },
		{ 0x94, "P1.4" },
		{ 0x95, "P1.5" },
		{ 0x96, "P1.6" },
		{ 0x97, "P1.7" },
		// ----
		{ 0x98, "RI" },			// SCON
		{ 0x99, "TI" },
		{ 0x9A, "RB8" },
		{ 0x9B, "TB8" },
		{ 0x9C, "REN" },
		{ 0x9D, "SM2" },
		{ 0x9E, "SM1" },
		{ 0x9F, "SM0" },
		// ----
		{ 0xA0, "P2.0" },		// P2
		{ 0xA1, "P2.1" },
		{ 0xA2, "P2.2" },
		{ 0xA3, "P2.3" },
		{ 0xA4, "P2.4" },
		{ 0xA5, "P2.5" },
		{ 0xA6, "P2.6" },
		{ 0xA7, "P2.7" },
		// ----
		{ 0xA8, "EX0" },		// IE
		{ 0xA9, "ET0" },
		{ 0xAA, "EX1" },
		{ 0xAB, "ET1" },
		{ 0xAC, "ES" },
		//{ 0xAD, "" },
		//{ 0xAE, "" },
		{ 0xAF, "EA" },
		// ----
		{ 0xB0, "P3.0" },		// P3
		{ 0xB1, "P3.1" },
		{ 0xB2, "P3.2" },
		{ 0xB3, "P3.3" },
		{ 0xB4, "P3.4" },
		{ 0xB5, "P3.5" },
		{ 0xB6, "P3.6" },
		{ 0xB7, "P3.7" },
		// ----
		{ 0xB8, "PX0" },		// IP
		{ 0xB9, "PT0" },
		{ 0xBA, "PX1" },
		{ 0xBB, "PT1" },
		{ 0xBC, "PS" },
		//{ 0xBD, "" },
		//{ 0xBE, "" },
		//{ 0xBF, "" },
		// ----
		{ 0xC0, "P4.0" },		// P4
		{ 0xC1, "P4.1" },
		{ 0xC2, "P4.2" },
		{ 0xC3, "P4.3" },
		{ 0xC4, "P4.4" },
		{ 0xC5, "P4.5" },
		{ 0xC6, "P4.6" },
		{ 0xC7, "P4.7" },
		// ----
		{ 0xD0, "P" },			// PSW
		//{ 0xD1, "" },
		{ 0xD2, "OV" },
		{ 0xD3, "RS0" },
		{ 0xD4, "RS1" },
		{ 0xD5, "F0" },
		{ 0xD6, "AC" },
		{ 0xD7, "CY" },
		// ----
		{ 0xE0, "A.0" },		// A
		{ 0xE1, "A.1" },
		{ 0xE2, "A.2" },
		{ 0xE3, "A.3" },
		{ 0xE4, "A.4" },
		{ 0xE5, "A.5" },
		{ 0xE6, "A.6" },
		{ 0xE7, "A.7" },
		// ----
		{ 0xF0, "B.0" },		// B
		{ 0xF1, "B.1" },
		{ 0xF2, "B.2" },
		{ 0xF3, "B.3" },
		{ 0xF4, "B.4" },
		{ 0xF5, "B.5" },
		{ 0xF6, "B.6" },
		{ 0xF7, "B.7" },
	};

}

CMCS51Disassembler::CMCS51Disassembler()
	:	m_nOpPointer(0),
	m_nStartPC(0),
	m_nSectionCount(0)
{
	//                = ((NBytes: 0; OpCode: (0, 0); Grp: 0; Control: 1; Mnemonic: '???'),
	m_Opcodes.AddOpcode({ { 0x00 }, { 0xFF }, MAKEOGRP(GRP_OPCODE_ONLY), MAKEOCTL(CTL_CODE_ONLY), "nop" });
	m_Opcodes.AddOpcode({ { 0x01 }, { 0x1F }, MAKEOGRP(GRP_11BIT_PG_CODE_ADDR), MAKEOCTL(CTL_DET_BRANCH) | TMCS51Disassembler::OCTL_STOP, "ajmp" });
	m_Opcodes.AddOpcode({ { 0x02 }, { 0xFF }, MAKEOGRP(GRP_16BIT_ABS_CODE_ADDR), MAKEOCTL(CTL_DET_BRANCH) | TMCS51Disassembler::OCTL_STOP, "ljmp" });
	m_Opcodes.AddOpcode({ { 0x03 }, { 0xFF }, MAKEOGRP(GRP_ACCUM), MAKEOCTL(CTL_CODE_ONLY), "rr" });
	m_Opcodes.AddOpcode({ { 0x04 }, { 0xFF }, MAKEOGRP(GRP_ACCUM), MAKEOCTL(CTL_CODE_ONLY), "inc" });
	m_Opcodes.AddOpcode({ { 0x05 }, { 0xFF }, MAKEOGRP(GRP_8BIT_DATA_ADDR), MAKEOCTL(CTL_CODE_DATA_LABEL), "inc" });
	m_Opcodes.AddOpcode({ { 0x06 }, { 0xFE }, MAKEOGRP(GRP_RIND_DATA_ADDR), MAKEOCTL(CTL_CODE_ONLY), "inc" });
	m_Opcodes.AddOpcode({ { 0x08 }, { 0xF8 }, MAKEOGRP(GRP_REGISTER), MAKEOCTL(CTL_CODE_ONLY), "inc" });
	m_Opcodes.AddOpcode({ { 0x10 }, { 0xFF }, MAKEOGRP(GRP_BIT_ADDR_DATA_ADDR, GRP_8BIT_REL_CODE_ADDR), MAKEOCTL(CTL_CODE_DATA_LABEL, CTL_DET_BRANCH), "jbc" });
	m_Opcodes.AddOpcode({ { 0x11 }, { 0x1F }, MAKEOGRP(GRP_11BIT_PG_CODE_ADDR), MAKEOCTL(CTL_DET_BRANCH), "acall" });
	m_Opcodes.AddOpcode({ { 0x12 }, { 0xFF }, MAKEOGRP(GRP_16BIT_ABS_CODE_ADDR), MAKEOCTL(CTL_DET_BRANCH), "lcall" });
	m_Opcodes.AddOpcode({ { 0x13 }, { 0xFF }, MAKEOGRP(GRP_ACCUM), MAKEOCTL(CTL_CODE_ONLY), "rrc" });
	m_Opcodes.AddOpcode({ { 0x14 }, { 0xFF }, MAKEOGRP(GRP_ACCUM), MAKEOCTL(CTL_CODE_ONLY), "dec" });
	m_Opcodes.AddOpcode({ { 0x15 }, { 0xFF }, MAKEOGRP(GRP_8BIT_DATA_ADDR), MAKEOCTL(CTL_CODE_DATA_LABEL), "dec" });
	m_Opcodes.AddOpcode({ { 0x16 }, { 0xFE }, MAKEOGRP(GRP_RIND_DATA_ADDR), MAKEOCTL(CTL_CODE_ONLY), "dec" });
	m_Opcodes.AddOpcode({ { 0x18 }, { 0xF8 }, MAKEOGRP(GRP_REGISTER), MAKEOCTL(CTL_CODE_ONLY), "dec" });
	m_Opcodes.AddOpcode({ { 0x20 }, { 0xFF }, MAKEOGRP(GRP_BIT_ADDR_DATA_ADDR, GRP_8BIT_REL_CODE_ADDR), MAKEOCTL(CTL_CODE_DATA_LABEL, CTL_DET_BRANCH), "jb" });
	m_Opcodes.AddOpcode({ { 0x22 }, { 0xFF }, MAKEOGRP(GRP_OPCODE_ONLY), MAKEOCTL(CTL_CODE_ONLY) | TMCS51Disassembler::OCTL_HARDSTOP, "ret" });
	m_Opcodes.AddOpcode({ { 0x23 }, { 0xFF }, MAKEOGRP(GRP_ACCUM), MAKEOCTL(CTL_CODE_ONLY), "rl" });
	m_Opcodes.AddOpcode({ { 0x24 }, { 0xFF }, MAKEOGRP(GRP_ACCUM, GRP_8BIT_CONST_DATA), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY), "add" });
	m_Opcodes.AddOpcode({ { 0x25 }, { 0xFF }, MAKEOGRP(GRP_ACCUM, GRP_8BIT_DATA_ADDR), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_DATA_LABEL), "add" });
	m_Opcodes.AddOpcode({ { 0x26 }, { 0xFE }, MAKEOGRP(GRP_ACCUM, GRP_RIND_DATA_ADDR), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY), "add" });
	m_Opcodes.AddOpcode({ { 0x28 }, { 0xF8 }, MAKEOGRP(GRP_ACCUM, GRP_REGISTER), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY), "add" });
	m_Opcodes.AddOpcode({ { 0x30 }, { 0xFF }, MAKEOGRP(GRP_BIT_ADDR_DATA_ADDR, GRP_8BIT_REL_CODE_ADDR), MAKEOCTL(CTL_CODE_DATA_LABEL, CTL_DET_BRANCH), "jnb" });
	m_Opcodes.AddOpcode({ { 0x32 }, { 0xFF }, MAKEOGRP(GRP_OPCODE_ONLY), MAKEOCTL(CTL_CODE_ONLY) | TMCS51Disassembler::OCTL_HARDSTOP, "reti" });
	m_Opcodes.AddOpcode({ { 0x33 }, { 0xFF }, MAKEOGRP(GRP_ACCUM), MAKEOCTL(CTL_CODE_ONLY), "rlc" });
	m_Opcodes.AddOpcode({ { 0x34 }, { 0xFF }, MAKEOGRP(GRP_ACCUM, GRP_8BIT_CONST_DATA), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY), "addc" });
	m_Opcodes.AddOpcode({ { 0x35 }, { 0xFF }, MAKEOGRP(GRP_ACCUM, GRP_8BIT_DATA_ADDR), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_DATA_LABEL), "addc" });
	m_Opcodes.AddOpcode({ { 0x36 }, { 0xFE }, MAKEOGRP(GRP_ACCUM, GRP_RIND_DATA_ADDR), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY), "addc" });
	m_Opcodes.AddOpcode({ { 0x38 }, { 0xF8 }, MAKEOGRP(GRP_ACCUM, GRP_REGISTER), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY), "addc" });
	m_Opcodes.AddOpcode({ { 0x40 }, { 0xFF }, MAKEOGRP(GRP_8BIT_REL_CODE_ADDR), MAKEOCTL(CTL_DET_BRANCH), "jc" });
	m_Opcodes.AddOpcode({ { 0x42 }, { 0xFF }, MAKEOGRP(GRP_8BIT_DATA_ADDR, GRP_ACCUM), MAKEOCTL(CTL_CODE_DATA_LABEL, CTL_CODE_ONLY), "orl" });
	m_Opcodes.AddOpcode({ { 0x43 }, { 0xFF }, MAKEOGRP(GRP_8BIT_DATA_ADDR, GRP_8BIT_CONST_DATA), MAKEOCTL(CTL_CODE_DATA_LABEL, CTL_CODE_ONLY), "orl" });
	m_Opcodes.AddOpcode({ { 0x44 }, { 0xFF }, MAKEOGRP(GRP_ACCUM, GRP_8BIT_CONST_DATA), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY), "orl" });
	m_Opcodes.AddOpcode({ { 0x45 }, { 0xFF }, MAKEOGRP(GRP_ACCUM, GRP_8BIT_DATA_ADDR), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_DATA_LABEL), "orl" });
	m_Opcodes.AddOpcode({ { 0x46 }, { 0xFE }, MAKEOGRP(GRP_ACCUM, GRP_RIND_DATA_ADDR), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY), "orl" });
	m_Opcodes.AddOpcode({ { 0x48 }, { 0xF8 }, MAKEOGRP(GRP_ACCUM, GRP_REGISTER), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY), "orl" });
	m_Opcodes.AddOpcode({ { 0x50 }, { 0xFF }, MAKEOGRP(GRP_8BIT_REL_CODE_ADDR), MAKEOCTL(CTL_DET_BRANCH), "jnc" });
	m_Opcodes.AddOpcode({ { 0x52 }, { 0xFF }, MAKEOGRP(GRP_8BIT_DATA_ADDR, GRP_ACCUM), MAKEOCTL(CTL_CODE_DATA_LABEL, CTL_CODE_ONLY), "anl" });
	m_Opcodes.AddOpcode({ { 0x53 }, { 0xFF }, MAKEOGRP(GRP_8BIT_DATA_ADDR, GRP_8BIT_CONST_DATA), MAKEOCTL(CTL_CODE_DATA_LABEL, CTL_CODE_ONLY), "anl" });
	m_Opcodes.AddOpcode({ { 0x54 }, { 0xFF }, MAKEOGRP(GRP_ACCUM, GRP_8BIT_CONST_DATA), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY), "anl" });
	m_Opcodes.AddOpcode({ { 0x55 }, { 0xFF }, MAKEOGRP(GRP_ACCUM, GRP_8BIT_DATA_ADDR), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_DATA_LABEL), "anl" });
	m_Opcodes.AddOpcode({ { 0x56 }, { 0xFE }, MAKEOGRP(GRP_ACCUM, GRP_RIND_DATA_ADDR), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY), "anl" });
	m_Opcodes.AddOpcode({ { 0x58 }, { 0xF8 }, MAKEOGRP(GRP_ACCUM, GRP_REGISTER), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY), "anl" });
	m_Opcodes.AddOpcode({ { 0x60 }, { 0xFF }, MAKEOGRP(GRP_8BIT_REL_CODE_ADDR), MAKEOCTL(CTL_DET_BRANCH), "jz" });
	m_Opcodes.AddOpcode({ { 0x62 }, { 0xFF }, MAKEOGRP(GRP_8BIT_DATA_ADDR, GRP_ACCUM), MAKEOCTL(CTL_CODE_DATA_LABEL, CTL_CODE_ONLY), "xrl" });
	m_Opcodes.AddOpcode({ { 0x63 }, { 0xFF }, MAKEOGRP(GRP_8BIT_DATA_ADDR, GRP_8BIT_CONST_DATA), MAKEOCTL(CTL_CODE_DATA_LABEL, CTL_CODE_ONLY), "xrl" });
	m_Opcodes.AddOpcode({ { 0x64 }, { 0xFF }, MAKEOGRP(GRP_ACCUM, GRP_8BIT_CONST_DATA), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY), "xrl" });
	m_Opcodes.AddOpcode({ { 0x65 }, { 0xFF }, MAKEOGRP(GRP_ACCUM, GRP_8BIT_DATA_ADDR), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_DATA_LABEL), "xrl" });
	m_Opcodes.AddOpcode({ { 0x66 }, { 0xFE }, MAKEOGRP(GRP_ACCUM, GRP_RIND_DATA_ADDR), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY), "xrl" });
	m_Opcodes.AddOpcode({ { 0x68 }, { 0xF8 }, MAKEOGRP(GRP_ACCUM, GRP_REGISTER), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY), "xrl" });
	m_Opcodes.AddOpcode({ { 0x70 }, { 0xFF }, MAKEOGRP(GRP_8BIT_REL_CODE_ADDR), MAKEOCTL(CTL_DET_BRANCH), "jnz" });
	m_Opcodes.AddOpcode({ { 0x72 }, { 0xFF }, MAKEOGRP(GRP_CARRY, GRP_BIT_ADDR_DATA_ADDR), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_DATA_LABEL), "orl" });
	m_Opcodes.AddOpcode({ { 0x73 }, { 0xFF }, MAKEOGRP(GRP_CODE_ACCUM_DPTR), MAKEOCTL(CTL_UNDET_BRANCH) | TMCS51Disassembler::OCTL_STOP, "jmp" });
	m_Opcodes.AddOpcode({ { 0x74 }, { 0xFF }, MAKEOGRP(GRP_ACCUM, GRP_8BIT_CONST_DATA), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY), "mov" });
	m_Opcodes.AddOpcode({ { 0x75 }, { 0xFF }, MAKEOGRP(GRP_8BIT_DATA_ADDR, GRP_8BIT_CONST_DATA), MAKEOCTL(CTL_CODE_DATA_LABEL, CTL_CODE_ONLY), "mov" });
	m_Opcodes.AddOpcode({ { 0x76 }, { 0xFE }, MAKEOGRP(GRP_RIND_DATA_ADDR, GRP_8BIT_CONST_DATA), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY), "mov" });
	m_Opcodes.AddOpcode({ { 0x78 }, { 0xF8 }, MAKEOGRP(GRP_REGISTER, GRP_8BIT_CONST_DATA), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY), "mov" });
	m_Opcodes.AddOpcode({ { 0x80 }, { 0xFF }, MAKEOGRP(GRP_8BIT_REL_CODE_ADDR), MAKEOCTL(CTL_DET_BRANCH) | TMCS51Disassembler::OCTL_STOP, "sjmp" });
	m_Opcodes.AddOpcode({ { 0x82 }, { 0xFF }, MAKEOGRP(GRP_CARRY, GRP_BIT_ADDR_DATA_ADDR), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_DATA_LABEL), "anl" });
	m_Opcodes.AddOpcode({ { 0x83 }, { 0xFF }, MAKEOGRP(GRP_ACCUM, GRP_CODE_ACCUM_PC), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY), "movc" });
	m_Opcodes.AddOpcode({ { 0x84 }, { 0xFF }, MAKEOGRP(GRP_AB), MAKEOCTL(CTL_CODE_ONLY), "div" });
	m_Opcodes.AddOpcode({ { 0x85 }, { 0xFF }, MAKEOGRP(GRP_8BIT_DATA_ADDR, GRP_8BIT_DATA_ADDR), MAKEOCTL(CTL_CODE_DATA_LABEL, CTL_CODE_DATA_LABEL), "mov" });
	m_Opcodes.AddOpcode({ { 0x86 }, { 0xFE }, MAKEOGRP(GRP_8BIT_DATA_ADDR, GRP_RIND_DATA_ADDR), MAKEOCTL(CTL_CODE_DATA_LABEL, CTL_CODE_ONLY), "mov" });
	m_Opcodes.AddOpcode({ { 0x88 }, { 0xF8 }, MAKEOGRP(GRP_8BIT_DATA_ADDR, GRP_REGISTER), MAKEOCTL(CTL_CODE_DATA_LABEL, CTL_CODE_ONLY), "mov" });
	m_Opcodes.AddOpcode({ { 0x90 }, { 0xFF }, MAKEOGRP(GRP_DPTR, GRP_16BIT_CONST_DATA), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY), "mov" });
	m_Opcodes.AddOpcode({ { 0x92 }, { 0xFF }, MAKEOGRP(GRP_BIT_ADDR_DATA_ADDR, GRP_CARRY), MAKEOCTL(CTL_CODE_DATA_LABEL, CTL_CODE_ONLY), "mov" });
	m_Opcodes.AddOpcode({ { 0x93 }, { 0xFF }, MAKEOGRP(GRP_ACCUM, GRP_CODE_ACCUM_DPTR), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY), "movc" });
	m_Opcodes.AddOpcode({ { 0x94 }, { 0xFF }, MAKEOGRP(GRP_ACCUM, GRP_8BIT_CONST_DATA), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY), "subb" });
	m_Opcodes.AddOpcode({ { 0x95 }, { 0xFF }, MAKEOGRP(GRP_ACCUM, GRP_8BIT_DATA_ADDR), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_DATA_LABEL), "subb" });
	m_Opcodes.AddOpcode({ { 0x96 }, { 0xFE }, MAKEOGRP(GRP_ACCUM, GRP_RIND_DATA_ADDR), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY), "subb" });
	m_Opcodes.AddOpcode({ { 0x98 }, { 0xF8 }, MAKEOGRP(GRP_ACCUM, GRP_REGISTER), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY), "subb" });
	m_Opcodes.AddOpcode({ { 0xA0 }, { 0xFF }, MAKEOGRP(GRP_CARRY, GRP_NOT_BIT_ADDR_DATA_ADDR), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_DATA_LABEL), "orl" });
	m_Opcodes.AddOpcode({ { 0xA2 }, { 0xFF }, MAKEOGRP(GRP_CARRY, GRP_BIT_ADDR_DATA_ADDR), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_DATA_LABEL), "mov" });
	m_Opcodes.AddOpcode({ { 0xA3 }, { 0xFF }, MAKEOGRP(GRP_DPTR), MAKEOCTL(CTL_CODE_ONLY), "inc" });
	m_Opcodes.AddOpcode({ { 0xA4 }, { 0xFF }, MAKEOGRP(GRP_AB), MAKEOCTL(CTL_CODE_ONLY), "mul" });
	m_Opcodes.AddOpcode({ { 0xA5 }, { 0xFF }, MAKEOGRP(GRP_OPCODE_ONLY), MAKEOCTL(CTL_CODE_ONLY), ".byte 0xA5" });
	m_Opcodes.AddOpcode({ { 0xA6 }, { 0xFE }, MAKEOGRP(GRP_RIND_DATA_ADDR, GRP_8BIT_DATA_ADDR), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_DATA_LABEL), "mov" });
	m_Opcodes.AddOpcode({ { 0xA8 }, { 0xF8 }, MAKEOGRP(GRP_REGISTER, GRP_8BIT_DATA_ADDR), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_DATA_LABEL), "mov" });
	m_Opcodes.AddOpcode({ { 0xB0 }, { 0xFF }, MAKEOGRP(GRP_CARRY, GRP_NOT_BIT_ADDR_DATA_ADDR), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_DATA_LABEL), "anl" });
	m_Opcodes.AddOpcode({ { 0xB2 }, { 0xFF }, MAKEOGRP(GRP_BIT_ADDR_DATA_ADDR), MAKEOCTL(CTL_CODE_DATA_LABEL), "cpl" });
	m_Opcodes.AddOpcode({ { 0xB3 }, { 0xFF }, MAKEOGRP(GRP_CARRY), MAKEOCTL(CTL_CODE_ONLY), "cpl" });
	m_Opcodes.AddOpcode({ { 0xB4 }, { 0xFF }, MAKEOGRP(GRP_ACCUM, GRP_8BIT_CONST_DATA, GRP_8BIT_REL_CODE_ADDR), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY, CTL_DET_BRANCH), "cjne" });
	m_Opcodes.AddOpcode({ { 0xB5 }, { 0xFF }, MAKEOGRP(GRP_ACCUM, GRP_8BIT_DATA_ADDR, GRP_8BIT_REL_CODE_ADDR), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_DATA_LABEL, CTL_DET_BRANCH), "cjne" });
	m_Opcodes.AddOpcode({ { 0xB6 }, { 0xFE }, MAKEOGRP(GRP_RIND_DATA_ADDR, GRP_8BIT_CONST_DATA, GRP_8BIT_REL_CODE_ADDR), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY, CTL_DET_BRANCH), "cjne" });
	m_Opcodes.AddOpcode({ { 0xB8 }, { 0xF8 }, MAKEOGRP(GRP_REGISTER, GRP_8BIT_CONST_DATA, GRP_8BIT_REL_CODE_ADDR), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY, CTL_DET_BRANCH), "cjne" });
	m_Opcodes.AddOpcode({ { 0xC0 }, { 0xFF }, MAKEOGRP(GRP_8BIT_DATA_ADDR), MAKEOCTL(CTL_CODE_DATA_LABEL), "push" });
	m_Opcodes.AddOpcode({ { 0xC2 }, { 0xFF }, MAKEOGRP(GRP_BIT_ADDR_DATA_ADDR), MAKEOCTL(CTL_CODE_DATA_LABEL), "clr" });
	m_Opcodes.AddOpcode({ { 0xC3 }, { 0xFF }, MAKEOGRP(GRP_CARRY), MAKEOCTL(CTL_CODE_ONLY), "clr" });
	m_Opcodes.AddOpcode({ { 0xC4 }, { 0xFF }, MAKEOGRP(GRP_ACCUM), MAKEOCTL(CTL_CODE_ONLY), "swap" });
	m_Opcodes.AddOpcode({ { 0xC5 }, { 0xFF }, MAKEOGRP(GRP_ACCUM, GRP_8BIT_DATA_ADDR), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_DATA_LABEL), "xch" });
	m_Opcodes.AddOpcode({ { 0xC6 }, { 0xFE }, MAKEOGRP(GRP_ACCUM, GRP_RIND_DATA_ADDR), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY), "xch" });
	m_Opcodes.AddOpcode({ { 0xC8 }, { 0xF8 }, MAKEOGRP(GRP_ACCUM, GRP_REGISTER), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY), "xch" });
	m_Opcodes.AddOpcode({ { 0xD0 }, { 0xFF }, MAKEOGRP(GRP_8BIT_DATA_ADDR), MAKEOCTL(CTL_CODE_DATA_LABEL), "pop" });
	m_Opcodes.AddOpcode({ { 0xD2 }, { 0xFF }, MAKEOGRP(GRP_BIT_ADDR_DATA_ADDR), MAKEOCTL(CTL_CODE_DATA_LABEL), "setb" });
	m_Opcodes.AddOpcode({ { 0xD3 }, { 0xFF }, MAKEOGRP(GRP_CARRY), MAKEOCTL(CTL_CODE_ONLY), "setb" });
	m_Opcodes.AddOpcode({ { 0xD4 }, { 0xFF }, MAKEOGRP(GRP_ACCUM), MAKEOCTL(CTL_CODE_ONLY), "da" });
	m_Opcodes.AddOpcode({ { 0xD5 }, { 0xFF }, MAKEOGRP(GRP_8BIT_DATA_ADDR, GRP_8BIT_REL_CODE_ADDR), MAKEOCTL(CTL_CODE_DATA_LABEL, CTL_DET_BRANCH), "djnz" });
	m_Opcodes.AddOpcode({ { 0xD6 }, { 0xFE }, MAKEOGRP(GRP_ACCUM, GRP_RIND_DATA_ADDR), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY), "xchd" });
	m_Opcodes.AddOpcode({ { 0xD8 }, { 0xF8 }, MAKEOGRP(GRP_REGISTER, GRP_8BIT_REL_CODE_ADDR), MAKEOCTL(CTL_CODE_ONLY, CTL_DET_BRANCH), "djnz" });
	m_Opcodes.AddOpcode({ { 0xE0 }, { 0xFF }, MAKEOGRP(GRP_ACCUM, GRP_DATA_DPTR), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY), "movx" });
	m_Opcodes.AddOpcode({ { 0xE2 }, { 0xFE }, MAKEOGRP(GRP_ACCUM, GRP_RIND_DATA_ADDR), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY), "movx" });
	m_Opcodes.AddOpcode({ { 0xE4 }, { 0xFF }, MAKEOGRP(GRP_ACCUM), MAKEOCTL(CTL_CODE_ONLY), "clr" });
	m_Opcodes.AddOpcode({ { 0xE5 }, { 0xFF }, MAKEOGRP(GRP_ACCUM, GRP_8BIT_DATA_ADDR), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_DATA_LABEL), "mov" });
	m_Opcodes.AddOpcode({ { 0xE6 }, { 0xFE }, MAKEOGRP(GRP_ACCUM, GRP_RIND_DATA_ADDR), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY), "mov" });
	m_Opcodes.AddOpcode({ { 0xE8 }, { 0xF8 }, MAKEOGRP(GRP_ACCUM, GRP_REGISTER), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY), "mov" });
	m_Opcodes.AddOpcode({ { 0xF0 }, { 0xFF }, MAKEOGRP(GRP_DATA_DPTR, GRP_ACCUM), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY), "movx" });
	m_Opcodes.AddOpcode({ { 0xF2 }, { 0xFE }, MAKEOGRP(GRP_RIND_DATA_ADDR, GRP_ACCUM), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY), "movx" });
	m_Opcodes.AddOpcode({ { 0xF4 }, { 0xFF }, MAKEOGRP(GRP_ACCUM), MAKEOCTL(CTL_CODE_ONLY), "cpl" });
	m_Opcodes.AddOpcode({ { 0xF5 }, { 0xFF }, MAKEOGRP(GRP_8BIT_DATA_ADDR, GRP_ACCUM), MAKEOCTL(CTL_CODE_DATA_LABEL, CTL_CODE_ONLY), "mov" });
	m_Opcodes.AddOpcode({ { 0xF6 }, { 0xFE }, MAKEOGRP(GRP_RIND_DATA_ADDR, GRP_ACCUM), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY), "mov" });
	m_Opcodes.AddOpcode({ { 0xF8 }, { 0xF8 }, MAKEOGRP(GRP_REGISTER, GRP_ACCUM), MAKEOCTL(CTL_CODE_ONLY, CTL_CODE_ONLY), "mov" });

	m_bAllowMemRangeOverlap = true;

	// Internal RAM and SFR (Note: Don't set this as loaded memory or
	//	else the assembler will output it as data in the .s19 file and
	//	cause overlap issues with doing mot2bin conversion:
	m_MemoryRanges[MT_RAM].push_back(CMemRange(0x0000ul, 0x100ul));

	// Add bit addressable RAM as I/O memory, since we don't have an explicit MT_BIT or anything
	m_MemoryRanges[MT_IO].push_back(CMemRange(0x0000ul, 0x100ul));

	// SFR RAM:
	for (auto const & entry : g_arrSFRRAM) {
		assert(ValidateLabelName(entry.m_strLabel));
		AddLabel(MT_RAM, entry.m_nAddress, false, 0, entry.m_strLabel);
		if (!entry.m_strComment.empty()) {
			AddComment(MT_RAM, entry.m_nAddress, CComment(entry.m_ctf, entry.m_strComment));
		}
	}

	// SFR Bits:
	for (auto const & entry : g_arrSFRBITS) {
		assert(ValidateLabelName(entry.m_strLabel));
		AddLabel(MT_IO, entry.m_nAddress, false, 0, entry.m_strLabel);
		if (!entry.m_strComment.empty()) {
			AddComment(MT_IO, entry.m_nAddress, CComment(entry.m_ctf, entry.m_strComment));
		}
	}

	m_bVBreakEquateLabels = false;
	m_bVBreakCodeLabels = true;
	m_bVBreakDataLabels = true;
}

// ----------------------------------------------------------------------------

unsigned int CMCS51Disassembler::GetVersionNumber() const
{
	return (CDisassembler::GetVersionNumber() | VERSION);			// Get GDC version and append our version to it
}

std::string CMCS51Disassembler::GetGDCLongName() const
{
	return "MCS51 Disassembler";
}

std::string CMCS51Disassembler::GetGDCShortName() const
{
	return "MCS51";
}

// ----------------------------------------------------------------------------

bool CMCS51Disassembler::ReadNextObj(MEMORY_TYPE nMemoryType, bool bTagMemory, std::ostream *msgFile, std::ostream *errFile)
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

	TMCS51Disassembler::TOpcodeSymbol nFirstByte;
	TAddress nSavedPC;

	m_sFunctionalOpcode.clear();

	nFirstByte = m_Memory[nMemoryType].element(m_PC++);
	m_OpMemory.clear();
	m_OpMemory.push_back(nFirstByte);
	if (IsAddressLoaded(nMemoryType, m_PC-1, 1) == false) return false;

	bool bOpcodeFound = false;
	for (TOpcodeTable_type::const_iterator itrOpcode = m_Opcodes.cbegin(); (!bOpcodeFound && (itrOpcode != m_Opcodes.cend())); ++itrOpcode) {
		assert(itrOpcode->second.at(0).opcode().size() == 1);			// 8051 opcodes are only 1 byte
		assert(itrOpcode->second.at(0).opcodeMask().size() == 1);
		if (itrOpcode->second.at(0).opcode().at(0) != (nFirstByte & itrOpcode->second.at(0).opcodeMask().at(0))) continue;
		bOpcodeFound = true;
		m_CurrentOpcode = itrOpcode->second.at(0);
	}

	if (!bOpcodeFound) {
		if (bTagMemory) m_Memory[nMemoryType].setDescriptor(m_PC-1, DMEM_ILLEGALCODE);
		return false;
	}

	// If we get here, then we've found a valid matching opcode.  m_CurrentOpcode has been set to the opcode value
	//	so we must now finish copying to m_OpMemory, tag the bytes in memory, increment m_PC, and call CompleteObjRead.
	//	If CompleteObjRead returns FALSE, then we'll have to undo everything and return flagging an invalid opcode.
	nSavedPC = m_PC;	// Remember m_PC in case we have to undo things (remember, m_PC has already been incremented by 1)!!

	if (!CompleteObjRead(nMemoryType, true, msgFile, errFile))  {
		// Undo things here:
		m_OpMemory.clear();
		m_OpMemory.push_back(nFirstByte);		// Keep only the first byte in OpMemory for illegal opcode id
		m_PC = nSavedPC;
		if (bTagMemory) m_Memory[nMemoryType].setDescriptor(m_PC-1, DMEM_ILLEGALCODE);
		return false;
	}


	//	assert(CompleteObjRead(true, msgFile, errFile));


	--nSavedPC;
	for (decltype(m_OpMemory)::size_type i=0; i<m_OpMemory.size(); ++i) {		// CompleteObjRead will add bytes to OpMemory, so we simply have to flag memory for that many bytes.  m_PC is already incremented by CompleteObjRead
		if (bTagMemory) m_Memory[nMemoryType].setDescriptor(nSavedPC, DMEM_CODE);
		++nSavedPC;
	}
	assert(nSavedPC == m_PC);		// If these aren't equal, something is wrong in the CompleteObjRead!  m_PC should be incremented for every byte added to m_OpMemory by the complete routine!
	return true;
}

bool CMCS51Disassembler::CompleteObjRead(MEMORY_TYPE nMemoryType, bool bAddLabels, std::ostream *msgFile, std::ostream *errFile)
{
	// Procedure to finish reading opcode from memory.  This procedure reads operand bytes,
	//	placing them in m_OpMemory.  Plus branch/labels and data/labels are generated (according to function).

	bool bA;
	bool bB;
	bool bC;
	char strTemp[10];

	m_nOpPointer = m_OpMemory.size();			// Get position of start of operands following opcode
	m_nStartPC = m_PC - m_OpMemory.size();		// Get PC of first byte of this instruction for branch references

	// Note: We'll start with the absolute address.  Since we don't know what the relative address is
	//		to the start of the function, then the function outputting function (which should know it),
	//		will have to add it in:

	std::sprintf(strTemp, "%04X|", m_nStartPC);
	m_sFunctionalOpcode = strTemp;

	// Move Bytes to m_OpMemory:
	bA = MoveOpcodeArgs(nMemoryType, OGRP_DST());
	bB = MoveOpcodeArgs(nMemoryType, OGRP_SRC());
	bC = MoveOpcodeArgs(nMemoryType, OGRP_SRC2());
	if (!bA || !bB || !bC) return false;

	if (m_OpMemory.at(0) == 0x85) {		// Note: mov direct to direct opcode 0x85 is encoded backwards on the 8051 with the src and dest swapped
		assert(m_OpMemory.size() == 3);
		TMCS51Disassembler::TOpcodeSymbol nTemp = m_OpMemory.at(1);
		m_OpMemory.at(1) = m_OpMemory.at(2);
		m_OpMemory.at(2) = nTemp;
	}

	// If the opcode bytes moved weren't in our memory range, bail out so we don't create invalid labels and refs:
	if (!IsAddressLoaded(nMemoryType, m_nStartPC, m_OpMemory.size())) return false;

	// Add reference labels to this opcode to the function:
	CLabelTableMap::const_iterator itrLabel = m_LabelTable[nMemoryType].find(m_nStartPC);
	if (itrLabel != m_LabelTable[nMemoryType].cend()) {
		for (CLabelArray::size_type i=0; i<itrLabel->second.size(); ++i) {
			if (i != 0) m_sFunctionalOpcode += ",";
			m_sFunctionalOpcode += FormatLabel(nMemoryType, LC_REF, itrLabel->second.at(i), m_nStartPC);
		}
	}

	m_sFunctionalOpcode += "|";

	// All bytes of OpMemory:
	for (decltype(m_OpMemory)::size_type i=0; i<m_OpMemory.size(); ++i) {
		std::sprintf(strTemp, "%02X", m_OpMemory.at(i));
		m_sFunctionalOpcode += strTemp;
	}

	m_sFunctionalOpcode += "|";

	// All bytes of opcode part of OpMemory;
	for (decltype(m_OpMemory)::size_type i=0; i<m_nOpPointer; ++i) {
		std::sprintf(strTemp, "%02X", m_OpMemory.at(i));
		m_sFunctionalOpcode += strTemp;
	}

	m_sFunctionalOpcode += "|";

	// All bytes of operand part of OpMemory;
	for (decltype(m_OpMemory)::size_type i=m_nOpPointer; i<m_OpMemory.size(); ++i) {
		std::sprintf(strTemp, "%02X", m_OpMemory.at(i));
		m_sFunctionalOpcode += strTemp;
	}

	m_sFunctionalOpcode += "|";

	// Decode Operands:
	bA = DecodeOpcode(OGRP_DST(), OCTL_DST(), bAddLabels, msgFile, errFile);

	m_sFunctionalOpcode += "|";

	bB = DecodeOpcode(OGRP_SRC(), OCTL_SRC(), bAddLabels, msgFile, errFile);

	m_sFunctionalOpcode += "|";

	bC = DecodeOpcode(OGRP_SRC2(), OCTL_SRC2(), bAddLabels, msgFile, errFile);
	if (!bA || !bB || !bC) return false;

	m_sFunctionalOpcode += "|";
	m_sFunctionalOpcode += FormatMnemonic(nMemoryType, MC_OPCODE, m_nStartPC);
	m_sFunctionalOpcode += "|";
	m_sFunctionalOpcode += FormatOperands(nMemoryType, MC_OPCODE, m_nStartPC);

//	// See if this is the end of a function.  Note: These preempt all previously
//	//		decoded function tags -- such as call, etc:
//	if ((compareNoCase(m_CurrentOpcode.mnemonic(), "rts") == 0) ||
//		(compareNoCase(m_CurrentOpcode.mnemonic(), "rti") == 0)) {
//		m_FunctionExitTable[m_nStartPC] = FUNCF_HARDSTOP;
//	}

	return true;
}

bool CMCS51Disassembler::CurrentOpcodeIsStop() const
{
	return ((m_CurrentOpcode.control() & (TMCS51Disassembler::OCTL_STOP | TMCS51Disassembler::OCTL_HARDSTOP)) != 0);
}

// ----------------------------------------------------------------------------

bool CMCS51Disassembler::RetrieveIndirect(MEMORY_TYPE nMemoryType, std::ostream *msgFile, std::ostream *errFile)
{
	UNUSED(msgFile);
	UNUSED(errFile);

	MEM_DESC b1d, b2d;

	TAddress nVector = m_Memory[nMemoryType].element(m_PC) * 256ul + m_Memory[nMemoryType].element(m_PC + 1);
	std::ostringstream sstrTemp;
	sstrTemp << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << nVector
			 << std::nouppercase << std::setbase(0);
	m_sFunctionalOpcode += sstrTemp.str();

	m_OpMemory.clear();
	m_OpMemory.push_back(m_Memory[nMemoryType].element(m_PC));
	m_OpMemory.push_back(m_Memory[nMemoryType].element(m_PC+1));

	// TODO : Do we need to investigate 1-byte data indirects??  How
	//	about indirects pointing to data in code vs. indirects in the
	//	data memory??

	// All MCS51 indirects are Intel format and are 2-bytes in length, regardless:
	b1d = static_cast<MEM_DESC>(m_Memory[nMemoryType].descriptor(m_PC));
	b2d = static_cast<MEM_DESC>(m_Memory[nMemoryType].descriptor(m_PC+1));
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

std::string CMCS51Disassembler::FormatOpBytes(MEMORY_TYPE nMemoryType, MNEMONIC_CODE nMCCode, TAddress nStartAddress)
{
	UNUSED(nMemoryType);
	UNUSED(nMCCode);			// Note: On MCS51, opcode elements are bytes and so are data elements. No need to decode nMCCode
	UNUSED(nStartAddress);

	std::ostringstream sstrTemp;

	for (decltype(m_OpMemory)::size_type i=0; i<m_OpMemory.size(); ++i) {
		if (i) sstrTemp << " ";
		sstrTemp << std::uppercase << std::setfill('0') << std::setw(2) << std::setbase(16) << static_cast<unsigned int>(m_OpMemory.at(i))
				 << std::nouppercase << std::setbase(0);
	}
	return sstrTemp.str();
}

std::string CMCS51Disassembler::FormatMnemonic(MEMORY_TYPE nMemoryType, MNEMONIC_CODE nMCCode, TAddress nStartAddress)
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

std::string CMCS51Disassembler::FormatOperands(MEMORY_TYPE nMemoryType, MNEMONIC_CODE nMCCode, TAddress nStartAddress)
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
			std::sprintf(strTemp, "%s%04X", GetHexDelim().c_str(), nStartAddress);
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
			if ((itrLabel = m_CodeIndirectTable.find(nStartAddress)) != m_CodeIndirectTable.cend()) {
				strLabel = itrLabel->second;
			} else if ((itrLabel = m_DataIndirectTable.find(nStartAddress)) != m_DataIndirectTable.cend()) {
				strLabel = itrLabel->second;
			}
			if (strLabel.empty()) {
				CLabelTableMap::const_iterator itrTable = m_LabelTable[nMemoryType].find(nAddress);
				if ((itrTable != m_LabelTable[nMemoryType].cend()) && !itrTable->second.empty()) {
					strLabel = itrTable->second.at(0);
				}
			}
			strOpStr = FormatLabel(nMemoryType, LC_REF, strLabel, nAddress);
			break;
		}
		case MC_OPCODE:
			m_nOpPointer = m_CurrentOpcode.opcode().size();	// Get position of start of operands following opcode
			m_nStartPC = nStartAddress;			// Get PC of first byte of this instruction for branch references

			CreateOperand(OGRP_DST(), strOpStr);			// Handle Destination operand first
			if (OGRP_SRC() != GRP_OPCODE_ONLY) strOpStr += ",";
			CreateOperand(OGRP_SRC(), strOpStr);
			if (OGRP_SRC2() != GRP_OPCODE_ONLY) strOpStr += ",";
			CreateOperand(OGRP_SRC2(), strOpStr);			// Handle Source2 operand last
			break;
		default:
			assert(false);
			break;
	}

	return strOpStr;
}

// ----------------------------------------------------------------------------

#ifdef LIBIBERTY_SUPPORT
extern "C" {
	static void demangle_callback(const char *pString, size_t nSize, void *pData)
	{
		std::string *pRetVal = (std::string *)(pData);
		if (pRetVal) {
			(*pRetVal) += std::string(pString, nSize);
		}
	}
}
#endif

// ----------------------------------------------------------------------------

std::string CMCS51Disassembler::FormatComments(MEMORY_TYPE nMemoryType, MNEMONIC_CODE nMCCode, TAddress nStartAddress)
{
	std::string strRetVal;
	char strTemp[30];

	// Output demangled label in comments if it's available:
	TLabel strLabel;
#ifdef LIBIBERTY_SUPPORT
	CAddressLabelMap::const_iterator itrSymbol;
	itrSymbol = m_SymbolTable[nMemoryType].find(nStartAddress);
	if (itrSymbol != m_SymbolTable[nMemoryType].cend()) {
		if (!itrSymbol->second.empty()) {
			if (!cplus_demangle_v3_callback(itrSymbol->second.c_str(), DMGL_PARAMS | DMGL_ANSI | DMGL_VERBOSE | DMGL_TYPES, demangle_callback, &strLabel))
				cplus_demangle_v3_callback(itrSymbol->second.c_str(), DMGL_NO_OPTS, demangle_callback, &strLabel);
		}
		if (!strLabel.empty()) {
			if (!strRetVal.empty()) strRetVal += "\n";
			strRetVal += strLabel;
		}
	}
#endif

	// If it's a branch into the middle of a function (and we know that function's offset),
	//	show the function name and offset into the function the comments:
	if (nMCCode == MC_OPCODE) {
		bool bHaveBranch = false;
		m_nOpPointer = m_CurrentOpcode.opcode().size();	// Get position of start of operands following opcode

		TAddress nBranchAddress = 0;
		std::pair<bool, TAddress> nOperandAddress;

		nOperandAddress = AddressFromOperand(OGRP_DST());
		switch (OGRP_DST()) {
			case GRP_11BIT_PG_CODE_ADDR:
			case GRP_8BIT_REL_CODE_ADDR:
			case GRP_16BIT_ABS_CODE_ADDR:
				nBranchAddress = nOperandAddress.second;
				bHaveBranch = true;
				break;
		}

		nOperandAddress = AddressFromOperand(OGRP_SRC());
		switch (OCTL_SRC()) {
			case GRP_11BIT_PG_CODE_ADDR:
			case GRP_8BIT_REL_CODE_ADDR:
			case GRP_16BIT_ABS_CODE_ADDR:
				nBranchAddress = nOperandAddress.second;
				bHaveBranch = true;
				break;
		}

		nOperandAddress = AddressFromOperand(OGRP_SRC2());
		switch (OCTL_SRC2()) {
			case GRP_11BIT_PG_CODE_ADDR:
			case GRP_8BIT_REL_CODE_ADDR:
			case GRP_16BIT_ABS_CODE_ADDR:
				nBranchAddress = nOperandAddress.second;
				bHaveBranch = true;
				break;
		}

		if (bHaveBranch) {
			CAddressMap::const_iterator itrObjectMap = m_ObjectMap[nMemoryType].find(nBranchAddress);
			if (itrObjectMap != m_ObjectMap[nMemoryType].cend()) {
				TAddress nBaseAddress = itrObjectMap->second;
				TAddressOffset nOffset = itrObjectMap->first - itrObjectMap->second;

				bool bIsDemangled = false;
				CLabelTableMap::const_iterator itrLabel = m_LabelTable[nMemoryType].find(nBaseAddress);
				strLabel.clear();
#ifdef LIBIBERTY_SUPPORT
				itrSymbol = m_SymbolTable[nMemoryType].find(nBaseAddress);
				if (itrSymbol != m_SymbolTable[nMemoryType].cend() && !itrSymbol->second.empty()) {
					cplus_demangle_v3_callback(itrSymbol->second.c_str(), DMGL_NO_OPTS, demangle_callback, &strLabel);
					if (!strLabel.empty()) bIsDemangled = true;
				}
#endif
				if (strLabel.empty() && (itrLabel != m_LabelTable[nMemoryType].cend())) {
					if (itrLabel->second.size()) {
						strLabel = FormatLabel(nMemoryType, LC_REF, itrLabel->second.at(0), nBaseAddress);
					} else {
						strLabel = FormatLabel(nMemoryType, LC_REF, TLabel(), nBaseAddress);
					}
				}

				if ((!strLabel.empty() && nOffset) || bIsDemangled) {
					if (!strRetVal.empty()) strRetVal += "\n";
					if (nOffset) {
						std::sprintf(strTemp, " +%s%X", GetHexDelim().c_str(), nOffset);
						strLabel += strTemp;
					}
					strRetVal += "<" + strLabel + ">";
				}
			}
		}
	}

	// Add Function Flag Debug Comments (if enabled):
	std::string strFuncFlagComments = FormatFunctionFlagComments(nMemoryType, nMCCode, nStartAddress);
	if (!strFuncFlagComments.empty()) {
		if (!strRetVal.empty()) strRetVal += "\n";
		strRetVal += strFuncFlagComments;
	}

	// Add user comments:
	std::string strUserComments = FormatUserComments(nMemoryType, nMCCode, nStartAddress);
	if (!strUserComments.empty()) {
		if (!strRetVal.empty()) strRetVal += "\n";
		strRetVal += strUserComments;
	}
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
		strRefComment = FormatOperandRefComments(OGRP_SRC2());
		if (!strRefComment.empty()) {
			if (!strRetVal.empty()) strRetVal += "\n";
			strRetVal += strRefComment;
		}
	}

	// Handle Undetermined Branch:
	switch (nMCCode) {
		case MC_OPCODE:
			if ((OCTL_DST() == CTL_UNDET_BRANCH) ||
				(OCTL_SRC() == CTL_UNDET_BRANCH) ||
				(OCTL_SRC2() == CTL_UNDET_BRANCH)) {
				if (!strRetVal.empty()) strRetVal += "\n";
				strRetVal = "Undetermined Branch Address";
			}
			break;
		default:
			break;
	}

	// Handle out-of-source branch:
	bool bBranchOutside = false;
	TAddress nAddress;
	switch (nMCCode) {
		case MC_OPCODE:
			m_nOpPointer = m_CurrentOpcode.opcode().size();	// Get position of start of operands following opcode
			m_nStartPC = nStartAddress;		// Get PC of first byte of this instruction for branch references
			if (CheckBranchOutside(nMemoryType, OGRP_DST())) {
				switch (OGRP_DST()) {
					case GRP_11BIT_PG_CODE_ADDR:
					case GRP_8BIT_REL_CODE_ADDR:
					case GRP_16BIT_ABS_CODE_ADDR:
						bBranchOutside = true;
						break;
				}
			}
			if (CheckBranchOutside(nMemoryType, OGRP_SRC())) {
				switch (OGRP_SRC()) {
					case GRP_11BIT_PG_CODE_ADDR:
					case GRP_8BIT_REL_CODE_ADDR:
					case GRP_16BIT_ABS_CODE_ADDR:
						bBranchOutside = true;
						break;
				}
			}
			if (CheckBranchOutside(nMemoryType, OGRP_SRC2())) {
				switch (OGRP_SRC2()) {
					case GRP_11BIT_PG_CODE_ADDR:
					case GRP_8BIT_REL_CODE_ADDR:
					case GRP_16BIT_ABS_CODE_ADDR:
						bBranchOutside = true;
						break;
				}
			}
			break;
		case MC_INDIRECT:
			nAddress = m_OpMemory.at(0)*256+m_OpMemory.at(1);
			if (m_CodeIndirectTable.contains(nStartAddress)) {
				if (!IsAddressLoaded(nMemoryType, nAddress, 1)) bBranchOutside = true;
			}
#ifdef LIBIBERTY_SUPPORT
			itrSymbol = m_SymbolTable[nMemoryType].find(nAddress);
			if (itrSymbol != m_SymbolTable[nMemoryType].cend()) {
				strLabel.clear();
				if (!itrSymbol->second.empty()) {
					if (!cplus_demangle_v3_callback(itrSymbol->second.c_str(), DMGL_PARAMS | DMGL_ANSI | DMGL_VERBOSE | DMGL_TYPES, demangle_callback, &strLabel))
						cplus_demangle_v3_callback(itrSymbol->second.c_str(), DMGL_NO_OPTS, demangle_callback, &strLabel);
				}
				if (!strLabel.empty()) {
					if (!strRetVal.empty()) strRetVal += "\n";
					strRetVal += "Indirect: " + strLabel;
				}
			}
#endif
			if (!strRetVal.empty()) strRetVal += "\n";
			if (m_CodeIndirectTable.contains(nStartAddress)) {
				strRetVal += "Code";
			} else {
				strRetVal += "Data";
			}
			strRetVal += " Indirect->";
			sprintf(strTemp, "%s%04X", GetHexDelim().c_str(), nAddress);
			strRetVal += strTemp;
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

std::string CMCS51Disassembler::FormatLabel(MEMORY_TYPE nMemoryType, LABEL_CODE nLC, const TLabel & strLabel, TAddress nAddress)
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

bool CMCS51Disassembler::WritePreSection(MEMORY_TYPE nMemoryType, const CMemBlock &memBlock, std::ostream& outFile, std::ostream *msgFile, std::ostream *errFile)
{
	if (!CDisassembler::WritePreSection(nMemoryType, memBlock, outFile, msgFile, errFile)) return false;

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

bool CMCS51Disassembler::ValidateLabelName(const TLabel & aName)
{
	if (aName.empty()) return false;					// Must have a length
	if ((!isalpha(aName.at(0))) && (aName.at(0) != '_')) return false;	// Must start with alpha or '_'
	for (TLabel::size_type i=1; i<aName.size(); ++i) {
		if ((!isalnum(aName.at(i))) && (aName.at(i) != '_') && (aName.at(i) != '.')) return false;	// Each character must be alphanumeric or '_' or '.'
	}
	return true;
}

// ----------------------------------------------------------------------------

bool CMCS51Disassembler::ResolveIndirect(MEMORY_TYPE nMemoryType, TAddress nAddress, TAddress& nResAddress, REFERENCE_TYPE nType)
{
	// In the MCS51, we can assume that all indirect addresses are 2-bytes in length
	//	and stored in big endian format.
	if (!IsAddressLoaded(nMemoryType, nAddress, 2) ||				// Not only must it be loaded, but we must have never examined it before!
		(m_Memory[nMemoryType].descriptor(nAddress) != DMEM_LOADED) ||
		(m_Memory[nMemoryType].descriptor(nAddress+1) != DMEM_LOADED)) {
		nResAddress = 0;
		return false;
	}
	TAddress nVector = m_Memory[nMemoryType].element(nAddress) * 256ul + m_Memory[nMemoryType].element(nAddress + 1);
	nResAddress = nVector;
	m_Memory[nMemoryType].setDescriptor(nAddress, static_cast<TDescElement>(DMEM_CODEINDIRECT) + nType);		// Flag the addresses as being the proper vector type
	m_Memory[nMemoryType].setDescriptor(nAddress+1, static_cast<TDescElement>(DMEM_CODEINDIRECT) + nType);
	return true;
}

// ----------------------------------------------------------------------------

TLabel CMCS51Disassembler::GenLabel(MEMORY_TYPE nMemoryType, TAddress nAddress)
{
	std::ostringstream sstrTemp;

	std::string strPrefix;
	switch (nMemoryType) {
		case MT_ROM:
			strPrefix = "CL";		// Label Code
			break;
		case MT_RAM:
			strPrefix = "DL";		// Label Data
			sstrTemp << strPrefix << std::uppercase << std::setfill('0') << std::setw(2) << std::setbase(16) << nAddress;
			break;
		case MT_IO:
		{
			// Add bit addressable RAM as I/O memory, since we don't have an explicit MT_BIT or anything
			TAddress nBitAddr = (nAddress >= 0x80) ? nAddress&0xF8 :
									((nAddress >> 3) + 0x20);
			strPrefix = "BIT";		// Label Bits
			sstrTemp << strPrefix << std::uppercase << std::setfill('0') << std::setw(2) << std::setbase(16) << nBitAddr;
			sstrTemp << "." << (nAddress & 0x07);
			assert(ValidateLabelName(sstrTemp.str()));
			break;
		}
		case MT_EE:
			strPrefix = "EL";		// Label EE
			break;
		default:
			strPrefix = "L";
			break;
	}

	if ((nMemoryType != MT_RAM) && (nMemoryType != MT_IO)) {
		sstrTemp << strPrefix << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << nAddress;
	}
	return sstrTemp.str();
}

// ----------------------------------------------------------------------------

std::string CMCS51Disassembler::GetExcludedPrintChars() const
{
	return "\\" DataDelim;
}

std::string CMCS51Disassembler::GetHexDelim() const
{
	return HexDelim;
}

std::string CMCS51Disassembler::GetCommentStartDelim() const
{
	return ComDelimS;
}

std::string CMCS51Disassembler::GetCommentEndDelim() const
{
	return ComDelimE;
}

// ----------------------------------------------------------------------------

void CMCS51Disassembler::clearOpMemory()
{
	m_OpMemory.clear();
}

size_t CMCS51Disassembler::opcodeSymbolSize() const
{
	return sizeof(TMCS51Disassembler::TOpcodeSymbol);
}

size_t CMCS51Disassembler::getOpMemorySize() const
{
	return m_OpMemory.size();
}

void CMCS51Disassembler::pushBackOpMemory(TAddress nLogicalAddress, TMemoryElement nValue)
{
	UNUSED(nLogicalAddress);
	m_OpMemory.push_back(nValue);
}

void CMCS51Disassembler::pushBackOpMemory(TAddress nLogicalAddress, const CMemoryArray &maValues)
{
	UNUSED(nLogicalAddress);
	m_OpMemory.insert(m_OpMemory.end(), maValues.cbegin(), maValues.cend());
}

// ----------------------------------------------------------------------------

bool CMCS51Disassembler::MoveOpcodeArgs(MEMORY_TYPE nMemoryType, TMCS51Disassembler::TGroupFlags nGroup)
{
	switch (nGroup) {
		case GRP_OPCODE_ONLY:					// Opcodes with no additional bytes
		case GRP_REGISTER:
		case GRP_RIND_DATA_ADDR:
		case GRP_ACCUM:
		case GRP_AB:
		case GRP_CARRY:
		case GRP_DPTR:
		case GRP_CODE_ACCUM_DPTR:
		case GRP_CODE_ACCUM_PC:
		case GRP_DATA_DPTR:
			break;
		case GRP_8BIT_DATA_ADDR:				// Opcodes with one additional bytes
		case GRP_8BIT_CONST_DATA:
		case GRP_11BIT_PG_CODE_ADDR:
		case GRP_8BIT_REL_CODE_ADDR:
		case GRP_BIT_ADDR_DATA_ADDR:
		case GRP_NOT_BIT_ADDR_DATA_ADDR:
			m_OpMemory.push_back(m_Memory[nMemoryType].element(m_PC++));
			break;
		case GRP_16BIT_CONST_DATA:				// Opcodes with two additional bytes
		case GRP_16BIT_ABS_CODE_ADDR:
			m_OpMemory.push_back(m_Memory[nMemoryType].element(m_PC++));
			m_OpMemory.push_back(m_Memory[nMemoryType].element(m_PC++));
			break;

		default:
			return false;
	}
	return true;
}

bool CMCS51Disassembler::DecodeOpcode(TMCS51Disassembler::TGroupFlags nGroup, TMCS51Disassembler::TControlFlags nControl, bool bAddLabels, std::ostream *msgFile, std::ostream *errFile)
{
	bool bRetVal = true;

	char strTemp[40];
	TAddress nTargetAddr;

	strTemp[0] = 0;
	nTargetAddr = 0xFFFFFFFFul;				// For functional code branch only!

	switch (nGroup) {
		case GRP_OPCODE_ONLY:					// No operand
			assert(nControl == CTL_CODE_ONLY);
			break;

		case GRP_REGISTER:
			assert(nControl == CTL_CODE_ONLY);
			std::sprintf(strTemp, "R%d", static_cast<int>(m_OpMemory.at(0) & 0x07));
			break;

		case GRP_RIND_DATA_ADDR:
			assert(nControl == CTL_CODE_ONLY);
			std::sprintf(strTemp, "D@R%d", static_cast<int>(m_OpMemory.at(0) & 0x01));
			break;

		case GRP_ACCUM:
			assert(nControl == CTL_CODE_ONLY);
			std::sprintf(strTemp, "A");
			break;

		case GRP_AB:
			assert(nControl == CTL_CODE_ONLY);
			std::sprintf(strTemp, "AB");
			break;

		case GRP_CARRY:
			assert(nControl == CTL_CODE_ONLY);
			std::sprintf(strTemp, "C");
			break;

		case GRP_DPTR:
			assert(nControl == CTL_CODE_ONLY);
			std::sprintf(strTemp, "DPTR");
			break;

		case GRP_CODE_ACCUM_DPTR:
			assert((nControl == CTL_CODE_ONLY) || (nControl == CTL_UNDET_BRANCH));
			std::sprintf(strTemp, "C@A+DPTR");
			break;

		case GRP_CODE_ACCUM_PC:
			assert(nControl == CTL_CODE_ONLY);
			std::sprintf(strTemp, "C@A+PC");
			break;

		case GRP_DATA_DPTR:
			assert(nControl == CTL_CODE_ONLY);
			std::sprintf(strTemp, "D@DPTR");
			break;

		case GRP_8BIT_DATA_ADDR:
			assert(nControl == CTL_CODE_DATA_LABEL);
			if (bAddLabels) GenDataLabel(MT_RAM, m_OpMemory.at(m_nOpPointer), m_nStartPC, TLabel(), msgFile, errFile);
			std::sprintf(strTemp, "D@%02lX", static_cast<unsigned long>(m_OpMemory.at(m_nOpPointer)));
			m_nOpPointer++;
			break;

		case GRP_8BIT_CONST_DATA:				// Immediate 8-bit data -- no label
			assert(nControl == CTL_CODE_ONLY);
			std::sprintf(strTemp, "#%02lX", static_cast<unsigned long>(m_OpMemory.at(m_nOpPointer)));
			m_nOpPointer++;
			break;

		case GRP_11BIT_PG_CODE_ADDR:			// 11-bit Code Page Address
			assert(nControl == CTL_DET_BRANCH);
			nTargetAddr = (m_PC & 0xF800) | ((m_OpMemory.at(0) & 0xE0) << 3) | m_OpMemory.at(m_nOpPointer);
			if (bAddLabels) GenAddrLabel(nTargetAddr, m_nStartPC, TLabel(), msgFile, errFile);
			std::sprintf(strTemp, "C@%04lX", static_cast<unsigned long>(nTargetAddr));
			m_nOpPointer++;
			break;

		case GRP_8BIT_REL_CODE_ADDR:			// 8-bit Relative Code Address
			assert(nControl == CTL_DET_BRANCH);
			nTargetAddr = m_PC+(signed long)(signed char)(m_OpMemory.at(m_nOpPointer));
			if (bAddLabels) GenAddrLabel(nTargetAddr, m_nStartPC, TLabel(), msgFile, errFile);
			std::sprintf(strTemp, "C^%c%02lX(%04lX)",
						 ((((signed long)(signed char)(m_OpMemory.at(m_nOpPointer))) !=
						   labs((signed long)(signed char)(m_OpMemory.at(m_nOpPointer)))) ? '-' : '+'),
						 labs((signed long)(signed char)(m_OpMemory.at(m_nOpPointer))),
						 static_cast<unsigned long>(nTargetAddr));
			break;

		case GRP_BIT_ADDR_DATA_ADDR:			// Bit Address
		{
			TAddress nBitAddr = (m_OpMemory.at(m_nOpPointer) >= 0x80) ? m_OpMemory.at(m_nOpPointer)&0xF8 :
									((m_OpMemory.at(m_nOpPointer) >> 3) + 0x20);
			assert(nControl == CTL_CODE_DATA_LABEL);
			if (bAddLabels) GenDataLabel(MT_IO, m_OpMemory.at(m_nOpPointer), m_nStartPC, TLabel(), msgFile, errFile);
			std::sprintf(strTemp, "D@%02lX.%d", static_cast<unsigned long>(nBitAddr), static_cast<int>(m_OpMemory.at(m_nOpPointer)&0x07));
			m_nOpPointer++;
			break;
		}

		case GRP_NOT_BIT_ADDR_DATA_ADDR:		// Bit Address
		{
			TAddress nBitAddr = (m_OpMemory.at(m_nOpPointer) >= 0x80) ? m_OpMemory.at(m_nOpPointer)&0xF8 :
									((m_OpMemory.at(m_nOpPointer) >> 3) + 0x20);
			assert(nControl == CTL_CODE_DATA_LABEL);
			if (bAddLabels) GenDataLabel(MT_IO, m_OpMemory.at(m_nOpPointer), m_nStartPC, TLabel(), msgFile, errFile);
			std::sprintf(strTemp, "/D@%02lX.%d", static_cast<unsigned long>(nBitAddr), static_cast<int>(m_OpMemory.at(m_nOpPointer)&0x07));
			m_nOpPointer++;
			break;
		}

		case GRP_16BIT_CONST_DATA:				// Immediate 16-bit data -- no label
			assert(nControl == CTL_CODE_ONLY);
			std::sprintf(strTemp, "#%04lX", static_cast<unsigned long>(m_OpMemory.at(m_nOpPointer)*256+m_OpMemory.at(m_nOpPointer+1)));
			m_nOpPointer += 2;
			break;

		case GRP_16BIT_ABS_CODE_ADDR:			// 16-bit Absolute Code Addr
			assert(nControl == CTL_DET_BRANCH);
			nTargetAddr = m_OpMemory.at(m_nOpPointer)*256+m_OpMemory.at(m_nOpPointer+1);
			if (bAddLabels) GenAddrLabel(nTargetAddr, m_nStartPC, TLabel(), msgFile, errFile);
			std::sprintf(strTemp, "C@%04lX", static_cast<unsigned long>(nTargetAddr));
			m_nOpPointer += 2;
			break;

		default:
			bRetVal = false;
	}

	m_sFunctionalOpcode += strTemp;

	// See if this is a function branch reference that needs to be added:
	if (nTargetAddr != 0xFFFFFFFFul) {
		if ((compareNoCase(m_CurrentOpcode.mnemonic(), "acall") == 0) ||
			(compareNoCase(m_CurrentOpcode.mnemonic(), "lcall") == 0)) {
			// Add these only if it is replacing a lower priority value:
			CFuncEntryMap::const_iterator itrFunction = m_FunctionEntryTable.find(nTargetAddr);
			if (itrFunction == m_FunctionEntryTable.cend()) {
				m_FunctionEntryTable[nTargetAddr] = FUNCF_CALL;
			} else {
				if (itrFunction->second > FUNCF_CALL) m_FunctionEntryTable[nTargetAddr] = FUNCF_CALL;
			}

			// See if this is the end of a function:
			if (m_FuncExitAddresses.contains(nTargetAddr)) {
				m_FunctionExitTable[m_nStartPC] = FUNCF_EXITBRANCH;
			}
		}

		if ((compareNoCase(m_CurrentOpcode.mnemonic(), "ajmp") == 0) ||
			(compareNoCase(m_CurrentOpcode.mnemonic(), "ljmp") == 0) ||
			(compareNoCase(m_CurrentOpcode.mnemonic(), "sjmp") == 0) ||
			(compareNoCase(m_CurrentOpcode.mnemonic(), "jbc") == 0) ||
			(compareNoCase(m_CurrentOpcode.mnemonic(), "jb") == 0) ||
			(compareNoCase(m_CurrentOpcode.mnemonic(), "jc") == 0) ||
			(compareNoCase(m_CurrentOpcode.mnemonic(), "jnc") == 0) ||
			(compareNoCase(m_CurrentOpcode.mnemonic(), "jz") == 0) ||
			(compareNoCase(m_CurrentOpcode.mnemonic(), "jnz") == 0) ||
			(compareNoCase(m_CurrentOpcode.mnemonic(), "cjne") == 0) ||		// cjne is debatable
			(compareNoCase(m_CurrentOpcode.mnemonic(), "djnz") == 0)) {		// djnz is debatable
			// Add these only if there isn't a function tag here:
			if (!m_FunctionEntryTable.contains(nTargetAddr)) m_FunctionEntryTable[nTargetAddr] = FUNCF_BRANCHIN;
		}
	}

	// See if this is the end of a function:
	if ((compareNoCase(m_CurrentOpcode.mnemonic(), "ajmp") == 0) ||
		(compareNoCase(m_CurrentOpcode.mnemonic(), "ljmp") == 0) ||
		(compareNoCase(m_CurrentOpcode.mnemonic(), "sjmp") == 0)) {
		if (nTargetAddr != 0xFFFFFFFFul) {
			if (m_FuncExitAddresses.contains(nTargetAddr)) {
				m_FunctionExitTable[m_nStartPC] = FUNCF_EXITBRANCH;
			} else {
				m_FunctionExitTable[m_nStartPC] = FUNCF_BRANCHOUT;
			}
		} else {
			// Non-Deterministic branches get tagged as a branchout:
			//m_FunctionExitTable[m_nStartPC] = FUNCF_BRANCHOUT;

			// Non-Deterministic branches get tagged as a hardstop (usually it exits the function):
			m_FunctionExitTable[m_nStartPC] = FUNCF_HARDSTOP;
		}
	}

	return bRetVal;
}

void CMCS51Disassembler::CreateOperand(TMCS51Disassembler::TGroupFlags nGroup, std::string& strOpStr)
{
	char strTemp[30];

	switch (nGroup) {
		case GRP_OPCODE_ONLY:
			break;

		case GRP_REGISTER:
			std::sprintf(strTemp, "R%d", static_cast<int>(m_OpMemory.at(0) & 0x07));
			strOpStr += strTemp;
			break;

		case GRP_RIND_DATA_ADDR:
			std::sprintf(strTemp, "@R%d", static_cast<int>(m_OpMemory.at(0) & 0x01));
			strOpStr += strTemp;
			break;

		case GRP_ACCUM:
			strOpStr += "A";
			break;

		case GRP_AB:
			strOpStr += "AB";
			break;

		case GRP_CARRY:
			strOpStr += "C";
			break;

		case GRP_DPTR:
			strOpStr += "DPTR";
			break;

		case GRP_CODE_ACCUM_DPTR:
			strOpStr += "@A+DPTR";
			break;

		case GRP_CODE_ACCUM_PC:
			strOpStr += "@A+PC";
			break;

		case GRP_DATA_DPTR:
			strOpStr += "@DPTR";
			break;

		case GRP_8BIT_DATA_ADDR:
			strOpStr += LabelDeref2(MT_RAM, m_OpMemory.at(m_nOpPointer));
			m_nOpPointer++;
			break;

		case GRP_8BIT_CONST_DATA:
			std::sprintf(strTemp, "#%s%02X", GetHexDelim().c_str(), m_OpMemory.at(m_nOpPointer));
			strOpStr += strTemp;
			m_nOpPointer++;
			break;

		case GRP_11BIT_PG_CODE_ADDR:
			strOpStr += LabelDeref4(MT_ROM, (m_PC & 0xF800) | ((m_OpMemory.at(0) & 0xE0) << 3) | m_OpMemory.at(m_nOpPointer));
			m_nOpPointer++;
			break;

		case GRP_8BIT_REL_CODE_ADDR:
			strOpStr += LabelDeref4(MT_ROM, m_PC+(signed long)(signed char)(m_OpMemory.at(m_nOpPointer)));
			m_nOpPointer++;
			break;

		case GRP_BIT_ADDR_DATA_ADDR:
			strOpStr += LabelDeref2(MT_IO, m_OpMemory.at(m_nOpPointer));
			m_nOpPointer++;
			break;

		case GRP_NOT_BIT_ADDR_DATA_ADDR:
			strOpStr += "/";
			strOpStr += LabelDeref2(MT_IO, m_OpMemory.at(m_nOpPointer));
			m_nOpPointer++;
			break;

		case GRP_16BIT_CONST_DATA:
			std::sprintf(strTemp, "#%s%04X", GetHexDelim().c_str(), m_OpMemory.at(m_nOpPointer)*256+m_OpMemory.at(m_nOpPointer+1));
			strOpStr += strTemp;
			m_nOpPointer += 2;
			break;

		case GRP_16BIT_ABS_CODE_ADDR:
			strOpStr += LabelDeref4(MT_ROM, m_OpMemory.at(m_nOpPointer)*256+m_OpMemory.at(m_nOpPointer+1));
			m_nOpPointer += 2;
			break;
	}
}

std::string CMCS51Disassembler::FormatOperandRefComments(TMCS51Disassembler::TGroupFlags nGroup)
{
	std::pair<bool, TAddress> pairAddr = AddressFromOperand(nGroup);
	MEMORY_TYPE nMemType = pairAddr.first ? MT_RAM : MT_ROM;

	switch (nGroup) {
		case GRP_OPCODE_ONLY:
		case GRP_REGISTER:
		case GRP_RIND_DATA_ADDR:
		case GRP_ACCUM:
		case GRP_AB:
		case GRP_CARRY:
		case GRP_DPTR:
		case GRP_CODE_ACCUM_DPTR:
		case GRP_CODE_ACCUM_PC:
		case GRP_DATA_DPTR:
		case GRP_8BIT_CONST_DATA:
		case GRP_16BIT_CONST_DATA:
			return std::string();

		case GRP_8BIT_DATA_ADDR:
		case GRP_11BIT_PG_CODE_ADDR:
		case GRP_8BIT_REL_CODE_ADDR:
		case GRP_16BIT_ABS_CODE_ADDR:
			break;

		case GRP_BIT_ADDR_DATA_ADDR:
		case GRP_NOT_BIT_ADDR_DATA_ADDR:
			nMemType = MT_IO;
			break;
	}

	return FormatUserComments(nMemType, MC_INDIRECT, pairAddr.second);
}

std::pair<bool, TAddress> CMCS51Disassembler::AddressFromOperand(TMCS51Disassembler::TGroupFlags nGroup)
{
	bool bData = false;
	TAddress nAddress = 0;

	switch (nGroup) {
		case GRP_OPCODE_ONLY:
		case GRP_REGISTER:
		case GRP_RIND_DATA_ADDR:
		case GRP_ACCUM:
		case GRP_AB:
		case GRP_CARRY:
		case GRP_DPTR:
		case GRP_CODE_ACCUM_DPTR:
		case GRP_CODE_ACCUM_PC:
		case GRP_DATA_DPTR:
			break;

		case GRP_8BIT_DATA_ADDR:
			bData = true;
			nAddress = m_OpMemory.at(m_nOpPointer);
			m_nOpPointer++;
			break;

		case GRP_8BIT_CONST_DATA:
			m_nOpPointer++;
			break;

		case GRP_11BIT_PG_CODE_ADDR:
			nAddress = (m_PC & 0xF800) | ((m_OpMemory.at(0) & 0xE0) << 3) | m_OpMemory.at(m_nOpPointer);
			m_nOpPointer++;
			break;

		case GRP_8BIT_REL_CODE_ADDR:
			nAddress = m_PC+(signed long)(signed char)(m_OpMemory.at(m_nOpPointer));
			m_nOpPointer++;
			break;

		case GRP_BIT_ADDR_DATA_ADDR:
		case GRP_NOT_BIT_ADDR_DATA_ADDR:
			bData = true;
			nAddress = m_OpMemory.at(m_nOpPointer);
			m_nOpPointer++;
			break;

		case GRP_16BIT_CONST_DATA:
			m_nOpPointer += 2;
			break;

		case GRP_16BIT_ABS_CODE_ADDR:
			nAddress = m_OpMemory.at(m_nOpPointer)*256+m_OpMemory.at(m_nOpPointer+1);
			m_nOpPointer += 2;
			break;
	}

	return std::pair<bool, TAddress>(bData, nAddress);
}

bool CMCS51Disassembler::CheckBranchOutside(MEMORY_TYPE nMemoryType, TMCS51Disassembler::TGroupFlags nGroup)
{
	bool bRetVal = false;
	std::pair<bool, TAddress> pairAddr = AddressFromOperand(nGroup);

	if (!pairAddr.first) bRetVal = !IsAddressLoaded(nMemoryType, pairAddr.second, 1);

	return bRetVal;
}

TLabel CMCS51Disassembler::LabelDeref2(MEMORY_TYPE nMemoryType, TAddress nAddress)
{
	std::string strTemp;
	char strCharTemp[30];

	CLabelTableMap::const_iterator itrLabel = m_LabelTable[nMemoryType].find(nAddress);
	if (itrLabel != m_LabelTable[nMemoryType].cend()) {
		if (itrLabel->second.size()) {
			strTemp = FormatLabel(nMemoryType, LC_REF, itrLabel->second.at(0), nAddress);
		} else {
			strTemp = FormatLabel(nMemoryType, LC_REF, TLabel(), nAddress);
		}
	} else {
		std::sprintf(strCharTemp, "%s%02X", GetHexDelim().c_str(), nAddress);
		strTemp = strCharTemp;
	}
	return strTemp;
}

TLabel CMCS51Disassembler::LabelDeref4(MEMORY_TYPE nMemoryType, TAddress nAddress)
{
	std::string strTemp;
	char strCharTemp[30];

	TAddress nBaseAddress = nAddress;
	TAddressOffset nOffset = 0;
	CAddressMap::const_iterator itrObjectMap = m_ObjectMap[nMemoryType].find(nAddress);
	if (itrObjectMap != m_ObjectMap[nMemoryType].cend()) {
		nBaseAddress = itrObjectMap->second;
		nOffset = nAddress - itrObjectMap->second;
	}

	CLabelTableMap::const_iterator itrLabel = m_LabelTable[nMemoryType].find(nBaseAddress);
	if (itrLabel != m_LabelTable[nMemoryType].cend()) {
		if (itrLabel->second.size()) {
			strTemp = FormatLabel(nMemoryType, LC_REF, itrLabel->second.at(0), nBaseAddress);
		} else {
			strTemp = FormatLabel(nMemoryType, LC_REF, TLabel(), nBaseAddress);
		}
	} else {
		std::sprintf(strCharTemp, "%s%04X", GetHexDelim().c_str(), nBaseAddress);
		strTemp = strCharTemp;
	}

	if (nOffset) {
		std::sprintf(strCharTemp, "+%s%X", GetHexDelim().c_str(), nOffset);
		strTemp += strCharTemp;
	}

	return strTemp;
}

// ============================================================================
