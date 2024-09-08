//
//	Intel MCS51 (8051) Disassembler GDC Module
//	Copyright(c)2024 by Donna Whisnant
//
//	Generic Code-Seeking Disassembler
//	Copyright(c)2021-2024 by Donna Whisnant
//

#ifndef _MCS51GDC_H
#define _MCS51GDC_H

// ============================================================================

#include <gdc.h>

// ============================================================================

typedef TDisassemblerTypes<
	uint8_t,		// TOpcodeSymbol
	uint32_t,		// TGroupFlags
	uint32_t		// TControlFlags
	> TMCS51Disassembler;

// ----------------------------------------------------------------------------

class CMCS51Disassembler : public CDisassembler,
						   protected CDisassemblerData<CMCS51Disassembler, TMCS51Disassembler,  COpcodeTableMap<TMCS51Disassembler> >
{
public:
	CMCS51Disassembler();

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
	virtual bool ValidateLabelName(const TLabel & aName) override;

	virtual bool ResolveIndirect(MEMORY_TYPE nMemoryType, TAddress nAddress, TAddress& nResAddress, REFERENCE_TYPE nType) override;

protected:
	virtual TLabel GenLabel(MEMORY_TYPE nMemoryType, TAddress nAddress) override;

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
	static constexpr TMCS51Disassembler::TControlFlags OCTL_MASK = 0xFFFFFF;			// Mask for our control

	static inline TMCS51Disassembler::TControlFlags MAKEOCTL(TMCS51Disassembler::TControlFlags dst, TMCS51Disassembler::TControlFlags src = 0, TMCS51Disassembler::TControlFlags src2 = 0)
	{
		return (((dst & 0xFF) << 16) | ((src & 0xFF) << 8) | (src2 & 0xFF));
	}

	inline TMCS51Disassembler::TControlFlags OCTL_SRC() const
	{
		return ((m_CurrentOpcode.control() >> 8) & 0xFF);
	}

	inline TMCS51Disassembler::TControlFlags OCTL_SRC2() const
	{
		return (m_CurrentOpcode.control() & 0xFF);
	}

	inline TMCS51Disassembler::TControlFlags OCTL_DST() const
	{
		return ((m_CurrentOpcode.control() >> 16) & 0xFF);
	}

	static inline constexpr TMCS51Disassembler::TGroupFlags OGRP_MASK = 0xFFFFFF;		// Mask for our group

	static inline TMCS51Disassembler::TGroupFlags MAKEOGRP(TMCS51Disassembler::TGroupFlags dst, TMCS51Disassembler::TGroupFlags src = 0, TMCS51Disassembler::TGroupFlags src2 = 0)
	{
		return (((dst & 0xFF) << 16) | ((src & 0xFF) << 8) | (src2 & 0xFF));
	}

	inline TMCS51Disassembler::TGroupFlags OGRP_SRC() const
	{
		return ((m_CurrentOpcode.group() >> 8) & 0xFF);
	}

	inline TMCS51Disassembler::TGroupFlags OGRP_SRC2() const
	{
		return (m_CurrentOpcode.group() & 0xFF);
	}

	inline TMCS51Disassembler::TGroupFlags OGRP_DST() const
	{
		return ((m_CurrentOpcode.group() >> 16) & 0xFF);
	}

	// --------------------------------

private:
	bool MoveOpcodeArgs(MEMORY_TYPE nMemoryType, TMCS51Disassembler::TGroupFlags nGroup);
	bool DecodeOpcode(TMCS51Disassembler::TGroupFlags nGroup, TMCS51Disassembler::TControlFlags nControl, bool bAddLabels, std::ostream *msgFile, std::ostream *errFile);
	void CreateOperand(TMCS51Disassembler::TGroupFlags nGroup, std::string& strOpStr);
	std::string FormatOperandRefComments(TMCS51Disassembler::TGroupFlags nGroup);
	std::pair<bool, TAddress> AddressFromOperand(TMCS51Disassembler::TGroupFlags nGroup);	// Returns TAddress and bool that's true if the address is data and false if it's code
	bool CheckBranchOutside(MEMORY_TYPE nMemoryType, TMCS51Disassembler::TGroupFlags nGroup);
	TLabel LabelDeref2(MEMORY_TYPE nMemoryType, TAddress nAddress);
	std::pair<bool, TLabel> LabelDeref4(MEMORY_TYPE nMemoryType, TAddress nAddress);

	decltype(m_OpMemory)::size_type m_nOpPointer;	// Index into m_OpMemory for start of operands and gets incremented during DecodeOpcode()
	TAddress m_nStartPC;		// Address for first instruction byte for m_CurrentOpcode during DecodeOpcode(), CreateOperand(), etc.

	int m_nSectionCount;
};

// ============================================================================

#endif	// _MCS51GDC_H
