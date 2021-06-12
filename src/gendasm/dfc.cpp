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
	}
	return nullptr;
}

void CDataFileConverters::registerDataFileConverter(const CDataFileConverter *pConverter)
{
	instance()->push_back(pConverter);
}
