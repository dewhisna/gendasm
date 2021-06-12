//
//	Error Message Handler
//
//
//	Generic Code-Seeking Disassembler
//	Copyright(c)2021 by Donna Whisnant
//

#include "errmsgs.h"
#include <string>


const std::string &EXCEPTION_ERROR::errorMessage(ERR_CODE nErrorCode)
{
	static const std::string arrErrorMessages[ERR_CODE_COUNT] =
	{
		"No error",								//		ERR_NONE
		"Error: Out of Memory",					//		ERR_OUT_OF_MEMORY
		"Error: Outside Memory Range",			//		ERR_OUT_OF_RANGE
		"Error: Memory Mapping Overlap",		//		ERR_MAPPING_OVERLAP
		"Error: Opening File for Reading",		//		ERR_OPENREAD
		"Error: Opening File for Writing",		//		ERR_OPENWRITE
		"Error: File Exists",					//		ERR_FILEEXISTS
		"Error: Bad checksum",					//		ERR_CHECKSUM
		"Error: Unexpected End-of-File",		//		ERR_UNEXPECTED_EOF
		"Error: Overflow",						//		ERR_OVERFLOW
		"Error: Write Failed",					//		ERR_WRITEFAILED
		"Error: Read Failed",					//		ERR_READFAILED
	};

	return arrErrorMessages[nErrorCode];
}

