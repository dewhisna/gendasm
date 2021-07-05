//
//	ELF Data File Converter Class
//
//
//	Generic Code-Seeking Disassembler
//	Copyright(c)2021 by Donna Whisnant
//

#ifndef ELFDFC_H
#define ELFDFC_H

#include <iostream>
#include <dfc.h>
#include <memclass.h>
#include <errmsgs.h>


class CELFDataFileConverter : virtual public CDataFileConverter {
public:
	virtual std::string GetLibraryName() const override { return "elf"; }
	virtual std::string GetShortDescription() const override;
	virtual std::string GetDescription() const override { return "Executable and Linkable Format (ELF) Data File Converter"; }
	virtual std::vector<std::string> GetLibraryNameAliases() const override { return { "axf", "o", "prx", "puff", "ko", "mod", "so" }; }

	const char *DefaultExtension() const override { return "elf"; }

	// ------------------------------------------------------------------------

	virtual bool RetrieveFileMapping(CDisassembler &disassembler,
			const std::string &strFilePathName, TAddress nNewBase,
			std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr) const override;

	virtual bool ReadDataFile(CDisassembler &disassembler,
			const std::string &strFilePathName, TAddress nNewBase, TDescElement nDesc,
			std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr) const override;

	virtual bool WriteDataFile(CDisassembler &disassembler,
			const std::string &strFilePathName, const CMemRanges &aRange, TAddress nNewBase,
			const CMemBlocks &aMemory, TDescElement nDesc, bool bUsePhysicalAddr,
			DFC_FILL_MODE_ENUM nFillMode, TMemoryElement nFillValue,
			std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr) const override;

	// ------------------------------------------------------------------------

	virtual bool RetrieveFileMapping(std::istream &aFile, TAddress nNewBase, CMemRanges &aRange,
										std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr) const override;

	virtual bool ReadDataFile(std::istream &aFile, TAddress nNewBase, CMemBlocks &aMemory,
													TDescElement nDesc,
													std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr) const override;

	virtual bool WriteDataFile(std::ostream &aFile, const CMemRanges &aRange, TAddress nNewBase,
													const CMemBlocks &aMemory, TDescElement nDesc, bool bUsePhysicalAddr,
													DFC_FILL_MODE_ENUM nFillMode, TMemoryElement nFillValue,
													std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr) const override;

private:
	enum ELF_READ_MODE_ENUM {
		ERM_Mapping = 0,		// Read only the mapping (and output messages)
		ERM_Data = 1,			// Read only the data
	};

	bool _ReadDataFile(ELF_READ_MODE_ENUM nReadMode, CDisassembler *pDisassembler, struct Elf *pElf, TAddress nNewBase,
						CMemBlocks *pMemory, CMemRanges *pRange, TDescElement nDesc,
						std::ostream *msgFile, std::ostream *errFile) const;

};

#endif   // ELFDFC_H

