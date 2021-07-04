//
//	Motorola (Srec) Data File Converter Class
//
//
//	Generic Code-Seeking Disassembler
//	Copyright(c)2021 by Donna Whisnant
//

#ifndef SFILEDFC_H
#define SFILEDFC_H

#include <iostream>
#include <dfc.h>
#include <memclass.h>
#include <errmsgs.h>

class CSrecDataFileConverter : virtual public CDataFileConverter {
public:
	virtual std::string GetLibraryName() const override { return "motorola"; }
	virtual std::string GetShortDescription() const override { return "Motorola Srec Hex Data File Converter"; }
	virtual std::string GetDescription() const override { return "Motorola Srec Hex Data File Converter"; }
	virtual std::vector<std::string> GetLibraryNameAliases() const override { return { "sfile", "srec", "mot", "s19", "s28", "s37" }; }

	const char *DefaultExtension() const override { return "mot"; }

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
	bool _ReadDataFile(std::istream &aFile, TAddress nNewBase, CMemBlocks *pMemory, CMemRanges *pRange, TDescElement nDesc) const;
};

#endif   // SFILEDFC_H

