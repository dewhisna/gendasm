//
//	Atmel(Microchip) AVR Disassembler GDC Module
//	Copyright(c)2021 by Donna Whisnant
//
//	Generic Code-Seeking Disassembler
//	Copyright(c)2021 by Donna Whisnant
//

#ifndef _AVRGDC_H
#define _AVRGDC_H

// ============================================================================

#include <gdc.h>

// ============================================================================

namespace TAVRDisassembler_ENUMS {
	enum OpcodeGroups {
		S_IBYTE,		// 0xF000 : Rd, K    ---- KKKK dddd KKKK (K: 0-255), (d: 16-31)
		S_IWORD,		// 0xFF00 : Rd, K    ---- ---- KKdd KKKK (K: 0-63), (d: 24, 26, 28, 30)
		S_SNGL,			// 0xFE0F : Rd       ---- ---d dddd ---- (d: 0-31)
		S_SAME,			// 0xFC00 : Rd       ---- --dd dddd dddd (d: 0-31)x2 (these are all ambiguous with corresponding S_DUBL, so must come AHEAD of S_DUBL and use a Match Function)
		S_DUBL,			// 0xFC00 : Rd, Rr   ---- --rd dddd rrrr (d: 0-31), (r: 0-31)
		S_MOVW,			// 0xFF00 : Rd, Rr   ---- ---- dddd rrrr (d: 0,2,4,...,30), (r: 0,2,4,...,30)
		S_MULS,			// 0xFF00 : Rd, Rr   ---- ---- dddd rrrr (d: 16-31), (r: 16-31)
		S_FMUL,			// 0xFF88 : Rd, Rr   ---- ---- -ddd -rrr (d: 16-23), (r: 16-23)
		S_SER,			// 0xFF0F : Rd       ---- ---- dddd ---- (d: 16-31)
		S_TFLG,			// 0xFE08 : Rd, b    ---- ---d dddd -bbb (d: 0-31), (b: 0-7)
		S_BRA,			// 0xFC07 : k        ---- --kk kkkk k--- (k: -64 - 63)
		S_JMP,			// 0xFE0E : k        ---- ---k kkkk ---k | kkkk kkkk kkkk kkkk (k: 0-64k, 0-4M), 16/22-bit absolute, PC+2
		S_RJMP,			// 0xF000 : k        ---- kkkk kkkk kkkk (k: -2k - 2k), 12-bit relative
		S_IOR,			// 0xFF00 : A, b     ---- ---- AAAA Abbb (A: 0-31), (b: 0-7)
		S_IN,			// 0xF800 : Rd, A    ---- -AAd dddd AAAA (A: 0-63), (d: 0-31)
		S_OUT,			// 0xF800 : A, Rr    ---- -AAr rrrr AAAA (A: 0-63), (r: 0-31)
		S_SNGL_X,		// 0xFE0F : Rd, X    ---- ---d dddd ---- (d: 0-31) (implied X for source)
		S_SNGL_Xp,		// 0xFE0F : Rd, X+   ---- ---d dddd ---- (d: 0-31) (implied X for source w/Post-Increment)
		S_SNGL_nX,		// 0xFE0F : Rd, -X   ---- ---d dddd ---- (d: 0-31) (implied X for source w/Pre-Decrement)
		S_SNGL_Y,		// 0xFE0F : Rd, Y    ---- ---d dddd ---- (d: 0-31) (implied Y for source)
		S_SNGL_Yp,		// 0xFE0F : Rd, Y+   ---- ---d dddd ---- (d: 0-31) (implied Y for source w/Post-Increment)
		S_SNGL_nY,		// 0xFE0F : Rd, -Y   ---- ---d dddd ---- (d: 0-31) (implied Y for source w/Pre-Decrement)
		S_SNGL_Z,		// 0xFE0F : Rd, Z    ---- ---d dddd ---- (d: 0-31) (implied Z for source)
		S_SNGL_Zp,		// 0xFE0F : Rd, Z+   ---- ---d dddd ---- (d: 0-31) (implied Z for source w/Post-Increment)
		S_SNGL_nZ,		// 0xFE0F : Rd, -Z   ---- ---d dddd ---- (d: 0-31) (implied Z for source w/Pre-Decrement)
		S_X_SNGL,		// 0xFE0F : X, Rr    ---- ---r rrrr ---- (r: 0-31) (implied X for destination)
		S_Xp_SNGL,		// 0xFE0F : X+, Rr   ---- ---r rrrr ---- (r: 0-31) (implied X for destination w/Post-Increment)
		S_nX_SNGL,		// 0xFE0F : -X, Rr   ---- ---r rrrr ---- (r: 0-31) (implied X for destination w/Pre-Decrement)
		S_Y_SNGL,		// 0xFE0F : Y, Rr    ---- ---r rrrr ---- (r: 0-31) (implied Y for destination)
		S_Yp_SNGL,		// 0xFE0F : Y+, Rr   ---- ---r rrrr ---- (r: 0-31) (implied Y for destination w/Post-Increment)
		S_nY_SNGL,		// 0xFE0F : -Y, Rr   ---- ---r rrrr ---- (r: 0-31) (implied Y for destination w/Pre-Decrement)
		S_Z_SNGL,		// 0xFE0F : Z, Rr    ---- ---r rrrr ---- (r: 0-31) (implied Z for destination)
		S_Zp_SNGL,		// 0xFE0F : Z+, Rr   ---- ---r rrrr ---- (r: 0-31) (implied Z for destination w/Post-Increment)
		S_nZ_SNGL,		// 0xFE0F : -Z, Rr   ---- ---r rrrr ---- (r: 0-31) (implied Z for destination w/Pre-Decrement)
		S_SNGL_Yq,		// 0xD208 : Rd, Y+q  --q- qq-d dddd -qqq (d: 0-31), (q: 0-63)
		S_SNGL_Zq,		// 0xD208 : Rd, Z+q  --q- qq-d dddd -qqq (d: 0-31), (q: 0-63)
		S_Yq_SNGL,		// 0xD208 : Y+q, Rr  --q- qq-r rrrr -qqq (r: 0-31), (q: 0-63)
		S_Zq_SNGL,		// 0xD208 : Z+q, Rr  --q- qq-r rrrr -qqq (r: 0-31), (q: 0-63)
		S_LDS,			// 0xFE0F : Rd, k    ---- ---d dddd ---- | kkkk kkkk kkkk kkkk (d: 0-31),(k: 0-64k), PC+2
		S_LDSrc,		// 0xF800 : Rd, k    ---- -kkk dddd kkkk (d: 16-31), (k: 0-127)
		S_STS,			// 0xFE0F : k, Rr    ---- ---r rrrr ---- | kkkk kkkk kkkk kkkk (r: 0-31),(k: 0-64k), PC+2
		S_STSrc,		// 0xF800 : k, Rr    ---- -kkk rrrr kkkk (r: 16-31), (k: 0-127)
		S_DES,			// 0xFF0F : K        ---- ---- KKKK ----  (K: 0-15)
		S_INH_Zp,		// 0xFFFF : Z+       ---- ---- ---- ----  (AVRxm,AVRxt)
		S_INH,			// 0xFFFF : -        ---- ---- ---- ----
	};


	enum OpcodeControl {
		CTL_None,				//	Disassemble as code
		CTL_DataLabel,			//	Disassemble as code, with data addr label
		CTL_IOLabel,			//	Disassemble as code, with I/O addr label
		CTL_UndetBra,			//	Disassemble as undeterminable branch address (Comment code)
		CTL_DetBra,				//	Disassemble as determinable branch, add branch addr and label
		CTL_Skip,				//	Disassemble as Skip Branch.  Next two instructions flagged as code even if next one would be terminal
		CTL_SkipIOLabel,		//	Disassemble as Skip Branch, but with I/O addr label

		CTL_MASK = 0xFFFFul,	//	Mask for the CTL portion above

		OCTL_TwoWordCode = 0x10000ul,		// Flag for Two-Word Instruction
		OCTL_ArchExtAddr = 0x20000ul,		// Flag for Architecture Specific extended addressing (such as RAMPD for LDS/STS instructions for >= 64K)
	};
	DEFINE_ENUM_FLAG_OPERATORS(OpcodeControl)
}

typedef TDisassemblerTypes<
		uint16_t,								// TOpcodeSymbol (AVR Uses Words instead of bytes)
		TAVRDisassembler_ENUMS::OpcodeGroups,	// TGroupFlags
		TAVRDisassembler_ENUMS::OpcodeControl	// TControlFlags
> TAVRDisassembler;

// ----------------------------------------------------------------------------

class CAVRDisassembler : public CDisassembler, protected CDisassemblerData<CAVRDisassembler, TAVRDisassembler>
{
public:
	CAVRDisassembler();

	virtual unsigned int GetVersionNumber() const;
	virtual std::string GetGDCLongName() const;
	virtual std::string GetGDCShortName() const;

	// --------------------------------

protected:

	// --------------------------------

private:
	decltype(m_OpMemory)::size_type m_nOpPointer;
	TAddress m_nStartPC;

	int m_nSectionCount;
};

// ============================================================================

#endif	// _AVRGDC_H
