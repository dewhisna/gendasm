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
	virtual std::string GetLibraryName() const { return "intel"; }
	virtual std::string GetShortDescription() const { return "Intel Hex Data File Converter"; }
	virtual std::string GetDescription() const { return "Intel Hex Data File Converter"; }
	virtual std::vector<std::string> GetLibraryNameAliases() const { return { "hex", "ihex" }; }

	const char *DefaultExtension() const { return "hex"; }

	virtual bool RetrieveFileMapping(std::istream &aFile, TAddress nNewBase, CMemRanges &aRange) const;

	virtual bool ReadDataFile(std::istream &aFile, TAddress nNewBase, CMemBlocks &aMemory,
													TDescElement nDesc) const;

	virtual bool WriteDataFile(std::ostream &aFile, const CMemRanges &aRange, TAddress nNewBase,
													const CMemBlocks &aMemory, TDescElement nDesc, bool bUsePhysicalAddr,
													DFC_FILL_MODE_ENUM nFillMode, TMemoryElement nFillValue) const;

private:
	bool _ReadDataFile(std::istream &aFile, TAddress nNewBase, CMemBlocks *pMemory, CMemRanges *pRange, TDescElement nDesc) const;

};

#endif   // INTELDFC_H

