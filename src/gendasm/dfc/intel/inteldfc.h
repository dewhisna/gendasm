//
//	Intel (Hex) Data File Converter Class
//
//
//	Generic Code-Seeking Disassembler
//	Copyright(c)2021 by Donna Whisnant
//

#ifndef INTELDFC_H
#define INTELDFC_H

#include <iostream>
#include <dfc.h>
#include <memclass.h>
#include <errmsgs.h>


class CIntelDataFileConverter : virtual public CDataFileConverter {
public:
	virtual std::string GetLibraryName() const override { return "intel"; }
	virtual std::string GetShortDescription() const override { return "Intel Hex Data File Converter"; }
	virtual std::string GetDescription() const override { return "Intel Hex Data File Converter"; }
	virtual std::vector<std::string> GetLibraryNameAliases() const override { return { "hex", "ihx", "ihex" }; }

	const char *DefaultExtension() const override { return "hex"; }

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

#endif   // INTELDFC_H

