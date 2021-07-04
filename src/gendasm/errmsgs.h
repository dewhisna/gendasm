//
//	Error Message Handler
//
//
//	Generic Code-Seeking Disassembler
//	Copyright(c)2021 by Donna Whisnant
//

#ifndef ERRMSGS_H
#define ERRMSGS_H

#include <string>

#define GET_MACRO3(_1,_2,_3,NAME,...) NAME
#define THROW_EXCEPTION_ERROR(...) GET_MACRO3(__VA_ARGS__, THROW_EXCEPTION_ERROR3, THROW_EXCEPTION_ERROR2, THROW_EXCEPTION_ERROR1)(__VA_ARGS__)

#define THROW_EXCEPTION_ERROR3(code, data, detail) \
	{	throw (EXCEPTION_ERROR(code, data, detail)); }

#define THROW_EXCEPTION_ERROR2(code, data) \
	{	throw (EXCEPTION_ERROR(code, data, std::string())); }

#define THROW_EXCEPTION_ERROR1(code) \
	{	throw (EXCEPTION_ERROR(code, 0, std::string())); }

class EXCEPTION_ERROR
{
public:
	enum ERR_CODE {
		ERR_NONE,
		ERR_OUT_OF_MEMORY,
		ERR_OUT_OF_RANGE,
		ERR_MAPPING_OVERLAP,
		ERR_OPENREAD,
		ERR_OPENWRITE,
		ERR_FILEEXISTS,
		ERR_CHECKSUM,
		ERR_UNEXPECTED_EOF,
		ERR_OVERFLOW,					// Either Memory Overflow (attempt to read into space not allocated) or File Overflow (attempt to write addressing/data to file that can't accept it)
		ERR_WRITEFAILED,
		ERR_READFAILED,
		ERR_INVALID_RECORD,				// DFC file record invalid (such as too short or wrong syntax)
		ERR_UNKNOWN_FILE_TYPE,			// Can't identify file type (such as elf not being an elf)
		ERR_LIBRARY_INIT_FAILED,		// Library initialization Error
		ERR_NOT_IMPLEMENTED,			// Operation not implemented
		ERR_CODE_COUNT
	};

	EXCEPTION_ERROR(ERR_CODE nErrorCode, unsigned int nData, const std::string &strDetail)
		:	m_nErrorCode(nErrorCode),
			m_nData(nData),
			m_strDetail(strDetail)
	{}

	ERR_CODE m_nErrorCode;			// Actual Error Code thrown
	unsigned int m_nData;			// User Defined Data for the error message
	std::string m_strDetail;		// Extended error message detail, like filename, etc.

	static const std::string &errorMessage(ERR_CODE nErrorCode);
	inline std::string errorMessage() const { return errorMessage(m_nErrorCode); }
};

#endif
