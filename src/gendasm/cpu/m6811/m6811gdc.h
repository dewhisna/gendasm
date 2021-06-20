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
		uint32_t,		// TGroupFlags
		uint32_t		// TControlFlags
> TM6811Disassembler;

// ----------------------------------------------------------------------------

class CM6811Disassembler : public CDisassembler,
		protected CDisassemblerData<CM6811Disassembler, TM6811Disassembler,  COpcodeTableMap<TM6811Disassembler> >
{
public:
	CM6811Disassembler();

	virtual unsigned int GetVersionNumber() const;
	virtual std::string GetGDCLongName() const;
	virtual std::string GetGDCShortName() const;

protected:
	virtual bool ReadNextObj(bool bTagMemory, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);
	virtual bool CompleteObjRead(bool bAddLabels = true, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);
	virtual bool CurrentOpcodeIsStop() const;

	virtual bool RetrieveIndirect(std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);

	virtual std::string FormatOpBytes(MEMORY_TYPE nMemoryType, MNEMONIC_CODE nMCCode, TAddress nStartAddress);
	virtual std::string FormatMnemonic(MEMORY_TYPE nMemoryType, MNEMONIC_CODE nMCCode, TAddress nStartAddress);
	virtual std::string FormatOperands(MEMORY_TYPE nMemoryType, MNEMONIC_CODE nMCCode, TAddress nStartAddress);
	virtual std::string FormatComments(MEMORY_TYPE nMemoryType, MNEMONIC_CODE nMCCode, TAddress nStartAddress);

	virtual std::string FormatLabel(MEMORY_TYPE nMemoryType, LABEL_CODE nLC, const TLabel & strLabel, TAddress nAddress);

	virtual bool WritePreSection(std::ostream& outFile, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);

	virtual bool ResolveIndirect(TAddress nAddress, TAddress& nResAddress, REFERENCE_TYPE nType);

	using CDisassembler::GenDataLabel;

	virtual std::string GetExcludedPrintChars() const;
	virtual std::string GetHexDelim() const;
	virtual std::string GetCommentStartDelim() const;
	virtual std::string	GetCommentEndDelim() const;

	virtual void clearOpMemory();
	virtual size_t opcodeSymbolSize() const;
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
	std::string FormatOperandRefComments(TM6811Disassembler::TGroupFlags nGroup);
	bool CheckBranchOutside(TM6811Disassembler::TGroupFlags nGroup);
	TLabel LabelDeref2(TAddress nAddress);
	TLabel LabelDeref4(TAddress nAddress);
	void GenDataLabel(TAddress nAddress, TAddress nRefAddress, const TLabel & strLabel, std::ostream *msgFile, std::ostream *errFile);

	decltype(m_OpMemory)::size_type m_nOpPointer;	// Index into m_OpMemory for start of operands and gets incremented during DecodeOpcode()
	TAddress m_nStartPC;		// Address for first instruction byte for m_CurrentOpcode during DecodeOpcode(), CreateOperand(), etc.

	int m_nSectionCount;

	bool m_bCBDef;				// Conditional Branch Defined for use with m6809
};

// ============================================================================

#endif	// _M6811GDC_H
