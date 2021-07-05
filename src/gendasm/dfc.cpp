//
//	Data File Converter Class
//
//
//	Generic Code-Seeking Disassembler
//	Copyright(c)2021 by Donna Whisnant
//

#include <iostream>
#include "dfc.h"
#include "stringhelp.h"
#include "memclass.h"
#include "errmsgs.h"

#include "gdc.h"

#ifndef UNUSED
	#define UNUSED(x) ((void)(x))
#endif

/////////////////////////////////////////////////////////////////////////////
// CDataFileConverter Implementation

bool CDataFileConverter::RetrieveFileMapping(CDisassembler &disassembler,
		const std::string &strFilePathName, TAddress nNewBase,
		std::ostream *msgFile, std::ostream *errFile) const
{
	std::fstream theFile;

	theFile.open(strFilePathName, std::ios_base::in | std::ios_base::binary);
	if (theFile.is_open() == 0) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_OPENREAD);

	CMemRanges ranges;
	bool bRetVal = RetrieveFileMapping(theFile, nNewBase, ranges, msgFile, errFile);

	disassembler.memory(CDisassembler::MT_ROM).initFromRanges(ranges, 0, true, 0, CDisassembler::DMEM_NOTLOADED);

	theFile.close();

	return bRetVal;
}

bool CDataFileConverter::ReadDataFile(CDisassembler &disassembler,
		const std::string &strFilePathName, TAddress nNewBase, TDescElement nDesc,
		std::ostream *msgFile, std::ostream *errFile) const
{
	std::fstream theFile;

	theFile.open(strFilePathName, std::ios_base::in | std::ios_base::binary);
	if (theFile.is_open() == 0) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_OPENREAD);

	bool bRetVal = ReadDataFile(theFile, nNewBase, disassembler.memory(CDisassembler::MT_ROM), nDesc, msgFile, errFile);

	theFile.close();

	return bRetVal;
}

bool CDataFileConverter::WriteDataFile(CDisassembler &disassembler,
		const std::string &strFilePathName, const CMemRanges &aRange, TAddress nNewBase,
		const CMemBlocks &aMemory, TDescElement nDesc, bool bUsePhysicalAddr,
		DFC_FILL_MODE_ENUM nFillMode, TMemoryElement nFillValue,
		std::ostream *msgFile, std::ostream *errFile) const
{
	UNUSED(disassembler);

	std::fstream theFile;

	theFile.open(strFilePathName, std::ios_base::out | std::ios_base::binary);
	if (theFile.is_open() == 0) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_OPENWRITE);

	bool bRetVal = WriteDataFile(theFile, aRange, nNewBase, aMemory, nDesc, bUsePhysicalAddr,
									nFillMode, nFillValue, msgFile, errFile);

	theFile.close();

	return bRetVal;
}

/////////////////////////////////////////////////////////////////////////////
// CDataFileConverters Implementation

CDataFileConverters *CDataFileConverters::instance()
{
	static CDataFileConverters DFCs;
	return &DFCs;
}

CDataFileConverters::CDataFileConverters()
{
}

CDataFileConverters::~CDataFileConverters()
{
}

const CDataFileConverter *CDataFileConverters::locateDFC(const std::string &strLibraryName) const
{
	for (unsigned int ndx = 0; ndx < size(); ++ndx) {
		if (compareNoCase(strLibraryName, at(ndx)->GetLibraryName()) == 0) return at(ndx);
		for (auto const & strAlias : at(ndx)->GetLibraryNameAliases()) {
			if (compareNoCase(strLibraryName, strAlias) == 0) return at(ndx);
		}
	}
	return nullptr;
}

void CDataFileConverters::registerDataFileConverter(const CDataFileConverter *pConverter)
{
	instance()->push_back(pConverter);
}
