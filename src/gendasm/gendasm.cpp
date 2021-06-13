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

#include <dfc/binary/binarydfc.h>
#include <cpu/m6811/m6811gdc.h>

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

	// Disassemblers:
	CDisassemblers disassemblers;
	CM6811Disassembler m6811dis;
	disassemblers.registerDisassembler(&m6811dis);

	CDisassembler *pDisassembler = ((argc >= 2) ? disassemblers.locateDisassembler(argv[1]) : nullptr);

	std::cout << "GenDasm - Generic Code-Seeking Disassembler v" << formatGDCVersion(m6811dis.GetVersionNumber()) << std::endl;
	std::cout << "Copyright(c)2021 by Donna Whisnant" << std::endl;
	std::cout << std::endl;
	if ((argc<3) || (pDisassembler == nullptr)) {
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
			std::cout << "    " << itrDFCs->GetLibraryName() << " - " << itrDFCs->GetShortDescription() << std::endl;
		}
		std::cout << std::endl;
		return -1;
	}

	std::cout << "Using: " << pDisassembler->GetGDCLongName() << std::endl;
	std::cout << std::endl;

	bool bOkFlag = true;
	int nArgIndex = 2;

	while ((bOkFlag) && (nArgIndex < argc)) {
		ifstreamControlFile CtrlFile(argv[nArgIndex]);
		if (!CtrlFile.is_open()) {
			std::cerr << "*** Error: Opening control file \"" << argv[nArgIndex] << "\" for reading..." << std::endl;
			return -2;
		}
		bOkFlag = pDisassembler->ReadControlFile(CtrlFile, (nArgIndex == argc-1), &std::cout, &std::cerr);
		CtrlFile.close();

		++nArgIndex;
	}

	if (bOkFlag) pDisassembler->Disassemble(&std::cout, &std::cerr);

	return 0;
}
