//
//	BINARY Data File Converter Class
//
//
//	Generic Code-Seeking Disassembler
//	Copyright(c)2021 by Donna Whisnant
//

#ifndef BINDFC_H
#define BINDFC_H

#include <iostream>
#include <dfc.h>
#include <memclass.h>
#include <errmsgs.h>


class CBinaryDataFileConverter : public CDataFileConverter
{
public:
	virtual std::string GetLibraryName() const override { return "binary"; }
	virtual std::string GetShortDescription() const override { return "Binary Data File Converter"; }
	virtual std::string GetDescription() const override { return "Binary Data File Converter"; }
	virtual std::vector<std::string> GetLibraryNameAliases() const override { return { "bin" }; }

	virtual const char *DefaultExtension() const override { return "bin"; }

	virtual bool RetrieveFileMapping(std::istream &aFile, TAddress nNewBase, CMemRanges &aRange) const override;

	virtual bool ReadDataFile(std::istream &aFile, TAddress nNewBase, CMemBlocks &aMemory,
													TDescElement nDesc) const override;

	virtual bool WriteDataFile(std::ostream &aFile, const CMemRanges &aRange, TAddress nNewBase,
													const CMemBlocks &aMemory, TDescElement nDesc, bool bUsePhysicalAddr,
													DFC_FILL_MODE_ENUM nFillMode, TMemoryElement nFillValue) const override;
};

#endif   // BINDFC_H

