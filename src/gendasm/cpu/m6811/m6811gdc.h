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

	virtual unsigned int GetVersionNumber() const override;
	virtual std::string GetGDCLongName() const override;
	virtual std::string GetGDCShortName() const override;

protected:
	virtual bool ReadNextObj(MEMORY_TYPE nMemoryType, bool bTagMemory, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr) override;
	virtual bool CompleteObjRead(MEMORY_TYPE nMemoryType, bool bAddLabels = true, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr) override;
	virtual bool CurrentOpcodeIsStop() const override;

	virtual bool RetrieveIndirect(MEMORY_TYPE nMemoryType, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr) override;

	virtual std::string FormatOpBytes(MEMORY_TYPE nMemoryType, MNEMONIC_CODE nMCCode, TAddress nStartAddress) override;
	virtual std::string FormatMnemonic(MEMORY_TYPE nMemoryType, MNEMONIC_CODE nMCCode, TAddress nStartAddress) override;
	virtual std::string FormatOperands(MEMORY_TYPE nMemoryType, MNEMONIC_CODE nMCCode, TAddress nStartAddress) override;
	virtual std::string FormatComments(MEMORY_TYPE nMemoryType, MNEMONIC_CODE nMCCode, TAddress nStartAddress) override;

	virtual std::string FormatLabel(MEMORY_TYPE nMemoryType, LABEL_CODE nLC, const TLabel & strLabel, TAddress nAddress) override;

	virtual bool WritePreSection(MEMORY_TYPE nMemoryType, const CMemBlock& memBlock, std::ostream& outFile, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr) override;

public:
	virtual bool ResolveIndirect(MEMORY_TYPE nMemoryType, TAddress nAddress, TAddress& nResAddress, REFERENCE_TYPE nType) override;

protected:
	using CDisassembler::GenDataLabel;

	virtual std::string GetExcludedPrintChars() const override;
	virtual std::string GetHexDelim() const override;
	virtual std::string GetCommentStartDelim() const override;
	virtual std::string	GetCommentEndDelim() const override;

	virtual void clearOpMemory() override;
	virtual size_t opcodeSymbolSize() const override;
	virtual size_t getOpMemorySize() const override;
	virtual void pushBackOpMemory(TAddress nLogicalAddress, TMemoryElement nValue) override;
	virtual void pushBackOpMemory(TAddress nLogicalAddress, const CMemoryArray &maValues) override;

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
	bool MoveOpcodeArgs(MEMORY_TYPE nMemoryType, TM6811Disassembler::TGroupFlags nGroup);
	bool DecodeOpcode(TM6811Disassembler::TGroupFlags nGroup, TM6811Disassembler::TControlFlags nControl, bool bAddLabels, std::ostream *msgFile, std::ostream *errFile);
	void CreateOperand(TM6811Disassembler::TGroupFlags nGroup, std::string& strOpStr);
	std::string FormatOperandRefComments(TM6811Disassembler::TGroupFlags nGroup);
	bool CheckBranchOutside(MEMORY_TYPE nMemoryType, TM6811Disassembler::TGroupFlags nGroup);
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
