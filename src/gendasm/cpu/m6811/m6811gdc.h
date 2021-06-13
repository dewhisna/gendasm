//
//	Motorola 6811 Disassembler GDC Module
//	Copyright(c)1996 - 2021 by Donna Whisnant
//
//	Generic Code-Seeking Disassembler
//	Copyright(c)2021 by Donna Whisnant
//

#ifndef _M6811GDC_H
#define _M6811GDC_H

// ============================================================================

#include <gdc.h>

// ============================================================================

typedef TDisassemblerTypes<
		uint8_t,		// TOpcodeSymbol
		uint32_t,		// TControlFlags
		uint32_t		// TGroupFlags
> TM6811Disassembler;

// ----------------------------------------------------------------------------

class CM6811Disassembler : public CDisassembler, protected CDisassemblerData<CM6811Disassembler, TM6811Disassembler>
{
public:
	CM6811Disassembler();

	virtual unsigned int GetVersionNumber();
	virtual std::string GetGDCLongName();
	virtual std::string GetGDCShortName();

	virtual bool ReadNextObj(bool bTagMemory, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);
	virtual bool CompleteObjRead(bool bAddLabels = true, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);
	virtual bool CurrentOpcodeIsStop() const;

	virtual bool RetrieveIndirect(std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);

	virtual std::string FormatMnemonic(MNEMONIC_CODE nMCCode);
	virtual std::string FormatOperands(MNEMONIC_CODE nMCCode);
	virtual std::string FormatComments(MNEMONIC_CODE nMCCode);

	virtual std::string FormatOpBytes();
	virtual std::string FormatLabel(LABEL_CODE nLC, const TLabel & strLabel, TAddress nAddress);

	virtual bool WritePreSection(std::ostream& outFile, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);

	virtual bool ResolveIndirect(TAddress nAddress, TAddress& nResAddress, REFERENCE_TYPE nType);

	virtual std::string GetExcludedPrintChars() const;
	virtual std::string GetHexDelim() const;
	virtual std::string GetCommentStartDelim() const;
	virtual std::string	GetCommentEndDelim() const;

	virtual void clearOpMemory();
	virtual void pushBackOpMemory(TAddress nLogicalAddress, TMemoryElement nValue);
	virtual void pushBackOpMemory(TAddress nLogicalAddress, const CMemoryArray &maValues);

	// --------------------------------

protected:
	static constexpr TM6811Disassembler::TControlFlags OCTL_MASK = 0xFF;			// Mask for our control byte

	static inline TM6811Disassembler::TControlFlags MAKEOCTL(TM6811Disassembler::TControlFlags d, TM6811Disassembler::TControlFlags s)
	{
		return (((d & 0x0F) << 4) | (s & 0x0F));
	}

	inline TM6811Disassembler::TControlFlags OCTL_SRC() const
	{
		return (m_CurrentOpcode.control() & 0x0F);
	}

	inline TM6811Disassembler::TControlFlags OCTL_DST() const
	{
		return ((m_CurrentOpcode.control() >> 4) & 0x0F);
	}

	static inline constexpr TM6811Disassembler::TGroupFlags OGRP_MASK = 0xFF;		// Mask for our group byte

	static inline TM6811Disassembler::TGroupFlags MAKEOGRP(TM6811Disassembler::TGroupFlags d, TM6811Disassembler::TGroupFlags s)
	{
		return (((d & 0x0F) << 4) | (s & 0x0F));
	}

	inline TM6811Disassembler::TGroupFlags OGRP_SRC() const
	{
		return (m_CurrentOpcode.group() & 0x0F);
	}

	inline TM6811Disassembler::TGroupFlags OGRP_DST() const
	{
		return ((m_CurrentOpcode.group() >> 4) & 0x0F);
	}

	// --------------------------------

private:
	bool MoveOpcodeArgs(TM6811Disassembler::TGroupFlags nGroup);
	bool DecodeOpcode(TM6811Disassembler::TGroupFlags nGroup, TM6811Disassembler::TControlFlags nControl, bool bAddLabels, std::ostream *msgFile, std::ostream *errFile);
	void CreateOperand(TM6811Disassembler::TGroupFlags nGroup, std::string& strOpStr);
	bool CheckBranchOutside(TM6811Disassembler::TGroupFlags nGroup);
	TLabel LabelDeref2(TAddress nAddress);
	TLabel LabelDeref4(TAddress nAddress);

	decltype(m_OpMemory)::size_type m_nOpPointer;
	TAddress m_nStartPC;

	int m_nSectionCount;

	bool m_bCBDef;				// Conditional Branch Defined for use with m6809
};

// ============================================================================

#endif	// _M6811GDC_H
