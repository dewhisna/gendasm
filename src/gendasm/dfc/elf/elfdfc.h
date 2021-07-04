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

	virtual bool RetrieveFileMapping(std::istream &aFile, TAddress nNewBase, CMemRanges &aRange,
										std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr) const override;

	virtual bool ReadDataFile(std::istream &aFile, TAddress nNewBase, CMemBlocks &aMemory,
													TDescElement nDesc,
													std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr) const override;

	virtual bool WriteDataFile(std::ostream &aFile, const CMemRanges &aRange, TAddress nNewBase,
													const CMemBlocks &aMemory, TDescElement nDesc, bool bUsePhysicalAddr,
													DFC_FILL_MODE_ENUM nFillMode, TMemoryElement nFillValue,
													std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr) const override;
};

#endif   // ELFDFC_H

