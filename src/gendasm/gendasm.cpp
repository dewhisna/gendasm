//
//	Generic Code-Seeking Disassembler
//	Copyright(c)2021 by Donna Whisnant
//

#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "gdc.h"

// ============================================================================

int main(int argc, char *argv[])
{
	bool bOkFlag;
	int nCount;

	printf("GenDasm Generic Code-Seeking Disassembler\n");
	printf("Copyright(c)2021 by Donna Whisnant\n");
	printf("\n");
	if (argc<2) {
		printf("Usage: gendasm <ctrl-filename1> [<ctrl-filename2> <ctrl-filename3> ...]\n");
		return -1;
	}

	bOkFlag = true;
	nCount = 1;
	while ((bOkFlag) && (nCount < argc)) {
//		ifstreamControlFile CtrlFile(argv[Count]);
//		if (!CtrlFile.is_open()) {
//			printf("*** Error: Opening control file \"%s\" for reading...\n", argv[Count]);
//			return -2;
//		}
//		bOkFlag = theDisassembler.ReadControlFile(CtrlFile, (Count == argc-1), &std::cout, &std::cerr);
//		CtrlFile.close();
		nCount++;
	}

	if (bOkFlag) ; //theDisassembler.Disassemble(&std::cout, &std::cerr);

	return 0;
}
