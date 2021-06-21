//
//	Generic Code-Seeking Disassembler
//	Copyright(c)2021 by Donna Whisnant
//

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <iomanip>

#include "gdc.h"
#include "dfc.h"
#include "stringhelp.h"

#include <dfc/binary/binarydfc.h>
#include <dfc/intel/inteldfc.h>

#include <cpu/m6811/m6811gdc.h>
#include <cpu/avr/avrgdc.h>

// ============================================================================

static std::string formatGDCVersion(unsigned int nVersion)
{
	std::ostringstream ssTemp;
	ssTemp << (((nVersion/65536ul) / 256ul) & 0xFF);
	ssTemp << ".";
	ssTemp << std::setfill('0') << std::setw(2) << ((nVersion/65536ul) & 0xFF);
	return ssTemp.str();
}

static std::string formatDisassemblerVersion(unsigned int nVersion)
{
	std::ostringstream ssTemp;
	ssTemp << ((nVersion / 256ul) & 0xFF);
	ssTemp << ".";
	ssTemp << std::setfill('0') << std::setw(2) << (nVersion & 0xFF);
	return ssTemp.str();
}

int main(int argc, char *argv[])
{
	// Data File Converters:
	CBinaryDataFileConverter dfcBinary;
	CDataFileConverters::registerDataFileConverter(&dfcBinary);
	CIntelDataFileConverter dfcIntel;
	CDataFileConverters::registerDataFileConverter(&dfcIntel);

	// Disassemblers:
	CDisassemblers disassemblers;
	CM6811Disassembler m6811dis;
	disassemblers.registerDisassembler(&m6811dis);
	CAVRDisassembler avrdis;
	disassemblers.registerDisassembler(&avrdis);

	CDisassembler *pDisassembler = nullptr;
	bool bDeterministic = false;
	CStringArray arrControlFiles;
	bool bNeedDisassembler = true;
	bool bNeedUsage = false;

	for (int ndx = 1; ndx < argc; ++ndx) {
		std::string strArg = argv[ndx];
		if (!starts_with(strArg, "-")) {
			if (bNeedDisassembler) {
				pDisassembler = disassemblers.locateDisassembler(strArg);
				bNeedDisassembler = false;
			} else {
				arrControlFiles.push_back(strArg);
			}
		} else if (strArg == "--deterministic") {
			bDeterministic = true;
		} else {
			bNeedUsage = true;
		}
	}
	if (pDisassembler == nullptr) bNeedUsage = true;
	if (arrControlFiles.empty()) bNeedUsage = true;

	if (!bDeterministic) {
		std::cout << "GenDasm - Generic Code-Seeking Disassembler v" << formatGDCVersion(m6811dis.GetVersionNumber()) << std::endl;
		std::cout << "Copyright(c)2021 by Donna Whisnant" << std::endl;
	} else {
		std::cout << "GenDasm - Generic Code-Seeking Disassembler" << std::endl;
	}
	std::cout << std::endl;
	if (bNeedUsage) {
		std::cout << "Usage: gendasm <disassembler> <ctrl-filename1> [<ctrl-filename2> <ctrl-filename3> ...]" << std::endl;
		std::cout << std::endl;
		std::cout << "Valid <disassembler> types:" << std::endl;
		for (auto const & itrDisassemblers : disassemblers) {
			std::cout << "    " << itrDisassemblers->GetGDCShortName() << " - " << itrDisassemblers->GetGDCLongName() <<
						" (v" << formatDisassemblerVersion(itrDisassemblers->GetVersionNumber()) << ")" << std::endl;
		}
		std::cout << std::endl;
		std::cout << "Supported Data File Converters:" << std::endl;
		for (auto const & itrDFCs : *CDataFileConverters::instance()) {
			CStringArray arrAliases = itrDFCs->GetLibraryNameAliases();
			std::cout << "    " << itrDFCs->GetLibraryName() << " - " << itrDFCs->GetShortDescription();
			if (!arrAliases.empty()) std::cout << " (" << itrDFCs->GetLibraryName();
			for (auto const & strAlias : arrAliases) std::cout << ", " << strAlias;
			if (!arrAliases.empty()) std::cout << ")";
			std::cout << std::endl;
		}
		std::cout << std::endl;
		return -1;
	}

	std::cout << "Using: " << pDisassembler->GetGDCLongName() << std::endl;
	std::cout << std::endl;

	pDisassembler->setDeterministic(bDeterministic);

	bool bOkFlag = true;

	for (CStringArray::size_type ndx = 0; (bOkFlag && (ndx < arrControlFiles.size())); ++ndx) {
		ifstreamControlFile CtrlFile(arrControlFiles.at(ndx));
		if (!CtrlFile.is_open()) {
			std::cerr << "*** Error: Opening control file \"" << arrControlFiles.at(ndx) << "\" for reading..." << std::endl;
			return -2;
		}
		bOkFlag = pDisassembler->ReadControlFile(CtrlFile, (ndx == (arrControlFiles.size()-1)), &std::cout, &std::cerr);
		CtrlFile.close();
	}

	if (bOkFlag) pDisassembler->Disassemble(&std::cout, &std::cerr);

	return 0;
}
