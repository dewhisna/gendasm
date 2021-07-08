//
//	Function Descriptor Classes
//
//
//	Fuzzy Function Analyzer
//	for the Generic Code-Seeking Disassembler
//	Copyright(c)2021 by Donna Whisnant
//

//
//	Format of Function Output File:
//
//		Any line beginning with a ";" is considered to be a commment line and is ignored
//
//		FuncAnal Commands:
//			-cmd|args
//			  |   |_________  Comma separated list of arguments for the command (or empty field if none)
//			  |_____________  Command specifcally for funcanal:
//		                        memrangeoverlap -- If present, indicates the memory ranges overlap, and ROM/RAM/IO are independent, such as Harvard architecture, argument = true/false
//		                        opcodesymbolsize -- Indicates width (in bytes) of the opcodes, argument = width in bytes (used where opcodes are wider than a byte, like an AVR)
//
//		Memory Mapping:
//			#type|addr|size
//			   |    |    |____  Size of Mapped area (hex)
//			   |    |_________  Absolute Address of Mapped area (hex)
//			   |______________  Type of Mapped area (One of following: ROM, RAM, IO)
//
//		Label Definitions:
//			!addr|label
//			   |    |____ Label(s) for this address(comma separated)
//			   |_________ Absolute Address (hex)
//
//		Start of New Function:
//			@xxxx|name
//			   |    |____ Name(s) of Function (comma separated list)
//			   |_________ Absolute Address of Function Start relative to overall file (hex)
//
//		Mnemonic Line (inside function):
//			xxxx|xxxx|label|xxxxxxxxxx|xxxxxx|xxxx|DST|SRC|mnemonic|operands
//			  |    |    |        |        |     |   |   |     |        |____  Disassembled operand output (ascii)
//			  |    |    |        |        |     |   |   |     |_____________  Disassembled mnemonic (ascii)
//			  |    |    |        |        |     |   |___|___________________  See below for SRC/DST format
//			  |    |    |        |        |     |___________________________  Addressing/Data bytes from operand portion of instruction (hex)
//			  |    |    |        |        |_________________________________  Opcode bytes from instruction (hex)
//			  |    |    |        |__________________________________________  All bytes from instruction (hex)
//			  |    |    |___________________________________________________  Label(s) for this address (comma separated list)
//			  |    |________________________________________________________  Absolute address of instruction (hex)
//			  |_____________________________________________________________  Relative address of instruction to the function (hex)
//
//		Data Byte Line (inside function):
//			xxxx|xxxx|label|xx
//			  |    |    |    |____  Data Byte (hex)
//			  |    |    |_________  Label(s) for this address (comma separated)
//			  |    |______________  Absolute address of data byte (hex)
//			  |___________________  Relative address of data byte to the function (hex)
//
//		SRC and DST entries:
//			#xxxx			Immediate Data Value (xxxx=hex value)
//			C@xxxx			Absolute Code Address (xxxx=hex address)
//			C^n(xxxx)		Relative Code Address (n=signed offset in hex, xxxx=resolved absolute address in hex)
//			C&xx(r)			Register Code Offset (xx=hex offset, r=register number or name), ex: jmp 2,x -> "C$02(x)"
//			D@xxxx			Absolute Data Address (xxxx=hex address)
//			D@xxxx,b		Absolute Data Address (xxxx=hex address), (b=bit number, 0-7)
//			D^n(xxxx)		Relative Data Address (n=signed offset in hex, xxxx=resolved absolute address in hex)
//			D&xx(r)			Register Data Offset (xx=hex offset, r=register number or name), ex: ldaa 1,y -> "D$01(y)"
//			Rn				Register (n=register number, 0-31)
//			Rn,b			Register (n=register number, 0-31), (b=bit number, 0-7)
//			RX				X Register
//			RX+				X Register with post-increment
//			R-X				X Register with pre-decrement
//			RY				Y Register
//			RY+				Y Register with post-increment
//			R-Y				Y Register with pre-decrement
//			RY+q			Y Register with 'q' offset (in decimal)
//			RZ				Z Register
//			RZ+				Z Register with post-increment
//			R-Z				Z Register with pre-decrement
//			RZ+q			Z Register with 'q' offset (in decimal)
//
//			If any of the above also includes a mask, then the following will be added:
//			,Mxx			Value mask (xx=hex mask value)
//
//		Note: The address sizes are consistent with the particular process.  For example, an HC11 will
//			use 16-bit addresses (or 4 hex digits).  The size of immediate data entries, offsets, and masks will
//			reflect the actual value.  A 16-bit immediate value, offset, or mask, will be outputted as 4 hex
//			digits, but an 8-bit immediate value, offset, or mask, will be outputted as only 2 hex digits.
//

//#include "funcanal.h"
#include "funcdesc.h"
#include <errmsgs.h>

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <iterator>
#include <regex>
#include <filesystem>

#define CALC_FIELD_WIDTH(x) x

#ifndef UNUSED
	#define UNUSED(x) ((void)(x))
#endif

// ============================================================================

typedef std::map<std::string, int> TKeywordMap;

int parseKeyword(const TKeywordMap &map, const std::string &strKeyword)
{
	for (auto const & itr : map) {
		if (std::regex_match(strKeyword, std::regex(itr.first, std::regex::extended))) return itr.second;
	}

	return -1;
}

namespace {

	static const TKeywordMap g_mapParseMemType = {
		{ "^ROM$", CFuncDescFile::MEMORY_TYPE::MT_ROM },
		{ "^RAM$", CFuncDescFile::MEMORY_TYPE::MT_RAM },
		{ "^IO$", CFuncDescFile::MEMORY_TYPE::MT_IO },
	};

	// --------------------------------

	static const std::string g_arrstrMemRanges[CFuncDescFile::MEMORY_TYPE::NUM_MEMORY_TYPES] = {		// Both Human readable names for ranges for printing AND names used in Functions file!
		"ROM", "RAM", "IO",
	};

	// --------------------------------

	enum FUNC_ANAL_COMMANDS_ENUM {
		FACE_MemRangeOverlap = 0,
		FACE_OpcodeSymbolSize = 1,
	};

	static const TKeywordMap g_mapParseFuncAnalCmds = {
		{ "^memrangeoverlap$", FACE_MemRangeOverlap },
		{ "^opcodesymbolsize$", FACE_OpcodeSymbolSize },
	};

	enum TRUE_FALSE_ENUM {
		TFE_False = 0,
		TFE_True = 1,
	};

	static const TKeywordMap g_mapParseTrueFalse = {
		{ "^false|no|off|0$", TFE_False },
		{ "^true|yes|on|1$", TFE_True },
	};

	// --------------------------------

	const static char g_strUnexpectedError[] = "Unexpected Error";
	const static char g_strSyntaxError[] = "Syntax Error or Unexpected Entry";
	const static char g_strUnknownMemoryRangeName[] = "Unknown Memory Range Name";
	const static char g_strUnknownFuncAnalCommand[] = "Unknown FuncAnal Specific Command";
	const static char g_strInvalidTrueFalse[] = "Invalid True/False Specifier";
	const static char g_strInvalidOpcodeSymbolWidth[] = "Invalid Opcode Symbol Width";
	const static char g_strInvalidOpcodeLength[] = "Opcode Symbols not a multiple of OpcodeSymbolWidth";
}

//////////////////////////////////////////////////////////////////////
// Static Functions
//////////////////////////////////////////////////////////////////////

static std::string::size_type getLongestMemMapName()
{
	std::string::size_type nMax = 0;
	for (int nMemType = 0; nMemType < CDisassembler::NUM_MEMORY_TYPES; ++nMemType) {
		if (g_arrstrMemRanges[nMemType].size() > nMax) nMax = g_arrstrMemRanges[nMemType].size();
	}
	return nMax;
}


// ParseLine : Parses a line from the file and returns an array of
//				items from the specified separator.
static void ParseLine(const TString &line, TString::value_type cSepChar, CStringArray &argv)
{
	TString strLine = line;
	TString strTemp;
	TString::size_type pos;

	argv.clear();

	strLine += cSepChar;

	while (!strLine.empty()) {
		pos = strLine.find(cSepChar);
		if (pos != TString::npos) {
			strTemp = strLine.substr(0, pos);
			trim(strTemp);
			strLine = strLine.substr(pos+1);
		} else {
			strTemp = strLine;
			trim(strTemp);
			strLine.clear();
		}
		argv.push_back(strTemp);
	}
}


//////////////////////////////////////////////////////////////////////
// CFuncObject Class
//////////////////////////////////////////////////////////////////////

CFuncObject::CFuncObject(std::shared_ptr<const CFuncDescFile> pParentFuncFile, std::shared_ptr<const CFuncDesc> pParentFunc, CStringArray &argv)
	:	m_pParentFuncFile(pParentFuncFile),
		m_pParentFunc(pParentFunc)
{
	assert(argv.size() >= 4);
	CStringArray arrstrLabels;

	if (argv.size() >= 1) m_nRelFuncAddress = strtoul(argv.at(0).c_str(), nullptr, 16);
	if (argv.size() >= 2) m_nAbsAddress = strtoul(argv.at(1).c_str(), nullptr, 16);
	if (argv.size() >= 3) {
		ParseLine(argv.at(2), ',', arrstrLabels);
		for (auto const &itrLabel : arrstrLabels) AddLabel(itrLabel);
	}
	if (argv.size() >= 4) {
		TString strTemp = argv.at(3);
		// TODO : Fix this to be TOpcodeSymbol instead of TMemoryElement
		for (TString::size_type ndx = 0; ndx < strTemp.size()/2; ++ndx) {
			m_Bytes.push_back(strtoul(strTemp.substr(ndx*2, 2).c_str(), nullptr, 16));
		}
	}
}

bool CFuncObject::isExactMatch(const CFuncObject &obj) const
{
	return (m_Bytes == obj.m_Bytes);
}

bool CFuncObject::AddLabel(const TLabel &strLabel)
{
	if (strLabel.empty()) return false;

	for (auto const & itrLabel : m_arrLabelTable) {
		if (compareNoCase(itrLabel, strLabel) == 0) return false;
	}
	m_arrLabelTable.push_back(strLabel);
	return true;
}

TString CFuncObject::GetBytes() const
{
	std::ostringstream ssTemp;
	for (auto const &itrByte : m_Bytes) {
		ssTemp << std::uppercase << std::setfill('0') << std::setw(2) << std::setbase(16) << static_cast<unsigned int>(itrByte);
	}
	return ssTemp.str();
}

CSymbolArray CFuncObject::GetSymbols() const
{
	//
	// This function, and its override, fills in the passed in array as follows:
	//		Lxxxxxx			-- Label for THIS object, "xxxxxx = Label"
	//		RSCxxxxxx		-- Referenced by THIS object -- Source, Code, "xxxxxx = Label"
	//		RSDxxxxxx		-- Referenced by THIS object -- Source, Data, "xxxxxx = Label"
	//		RDCxxxxxx		-- Referenced by THIS object -- Destination, Code, "xxxxxx = Label"
	//		RDDxxxxxx		-- Referenced by THIS object -- Destination, Data, "xxxxxx = Label"
	//

	CSymbolArray arrRetVal;

	TSymbol strTemp = m_pParentFuncFile->GetAnyPrimaryLabel(m_pParentFuncFile->allowMemRangeOverlap(), m_nAbsAddress);
	if (!strTemp.empty()) {
		arrRetVal.push_back("L" + strTemp);
	}
	return arrRetVal;
}

int CFuncObject::GetFieldWidth(FIELD_CODE nFC)
{
	switch (nFC) {
		case FIELD_CODE::FC_ADDRESS:	return CALC_FIELD_WIDTH(7);
		case FIELD_CODE::FC_OPBYTES:	return CALC_FIELD_WIDTH(11);
		case FIELD_CODE::FC_LABEL:		return CALC_FIELD_WIDTH(13);
		case FIELD_CODE::FC_MNEMONIC:	return CALC_FIELD_WIDTH(7);
		case FIELD_CODE::FC_OPERANDS:	return CALC_FIELD_WIDTH(21);
		case FIELD_CODE::FC_COMMENT:	return CALC_FIELD_WIDTH(60);
		default: break;
	}
	return 0;
}


//////////////////////////////////////////////////////////////////////
// CFuncAsmInstObject Class
//////////////////////////////////////////////////////////////////////

CFuncAsmInstObject::CFuncAsmInstObject(std::shared_ptr<const CFuncDescFile> pParentFuncFile, std::shared_ptr<const CFuncDesc> pParentFunc, CStringArray &argv)
	:	CFuncObject(pParentFuncFile, pParentFunc, argv)
{
	assert(argv.size() >= 10);

	if (argv.size() >= 5) {
		TString strTemp = argv.at(4);
		// TODO : Fix this to be TOpcodeSymbol instead of TMemoryElement
		for (TString::size_type ndx = 0; ndx < strTemp.size()/2; ++ndx) {
			m_OpCodeBytes.push_back(strtoul(strTemp.substr(ndx*2, 2).c_str(), nullptr, 16));
		}
	}
	if (argv.size() >= 6) {
		TString strTemp = argv.at(5);
		// TODO : Fix this to be TOpcodeSymbol instead of TMemoryElement
		for (TString::size_type ndx = 0; ndx < strTemp.size()/2; ++ndx) {
			m_OperandBytes.push_back(strtoul(strTemp.substr(ndx*2, 2).c_str(), nullptr, 16));
		}
	}
	if (argv.size() >= 7) m_strDstOperand = argv.at(6);
	if (argv.size() >= 8) m_strSrcOperand = argv.at(7);
	if (argv.size() >= 9) m_strOpCodeText = argv.at(8);
	if (argv.size() >= 10) m_strOperandText = argv.at(9);
}

TString CFuncAsmInstObject::ExportToDiff(FUNC_DIFF_LEVEL nLevel) const
{
	// TString strRetVal;
	std::ostringstream ssDiff;
	CMemRange zFuncRange(m_pParentFunc->GetMainAddress(), m_pParentFunc->GetFuncSize());

	char arrTemp[30];
	TString strTemp;
	TString strTemp2;
	TAddress nAddrTemp;
	TAddressOffset nAddrOffsetTemp;
	TString::size_type pos;

	ssDiff << "C|" << GetByteCount() << "|" << GetOpCodeBytes();

	for (int i=0; i<2; i++) {
		switch (i) {
			case 0:
				strTemp = m_strDstOperand;
				break;
			case 1:
				strTemp = m_strSrcOperand;
				break;
		}

		if (strTemp.empty()) continue;

		ssDiff << "|";

		switch (strTemp.at(0)) {
			case '#':				// Immediate Data
				ssDiff << strTemp;
				break;
			case 'C':				// Code addressing
				if (strTemp.size() < 2) continue;
				switch (strTemp.at(1)) {
					case '@':		// Absolute
						nAddrTemp = strtoul(strTemp.substr(2).c_str(), nullptr, 16);
						if (m_pParentFuncFile->AddrHasLabel(CFuncDescFile::MEMORY_TYPE::MT_ROM, nAddrTemp)) {
							// If the address has a user-defined file-level label, use it instead:
							ssDiff << "C=" + m_pParentFuncFile->GetPrimaryLabel(CFuncDescFile::MEMORY_TYPE::MT_ROM, nAddrTemp);
						} else {
							// If there isn't a label, see if this absolute address lies inside the function:
							if (zFuncRange.addressInRange(nAddrTemp)) {
								// If so, convert and treat as a relative address:
								nAddrOffsetTemp = nAddrTemp - (GetAbsAddress() + GetByteCount());
								std::sprintf(arrTemp, "C^%c%lX",
											((nAddrOffsetTemp != labs(nAddrOffsetTemp)) ? '-' : '+'),
											labs(nAddrOffsetTemp));
								ssDiff << arrTemp;
							} else {
								// Otherwise, it is an outside reference to something unknown:
								if (nLevel == FDL_1) {
									// Level 1 : Keep unknown
									ssDiff << "C?";
								} else {
									// Level 2 : Synthesize label
									if (!m_pParentFuncFile->allowMemRangeOverlap()) {
										std::sprintf(arrTemp, "C=L%04X", nAddrTemp);
									} else {
										std::sprintf(arrTemp, "C=CL%04X", nAddrTemp);
									}
									ssDiff << arrTemp;
								}
							}
						}
						// If there is a mask (or other appendage) add it:
						pos = strTemp.find(',');
						if (pos != strTemp.npos) ssDiff << strTemp.substr(pos);
						break;
					case '^':		// Relative
					{
						// Relative addressing is always relative to the "next byte" after the instruction:
						if (strTemp.size() < 3) continue;
						pos = strTemp.find('(', 3);
						if (pos != strTemp.npos) {
							strTemp2 = strTemp.substr(3, pos-3);
							pos = strTemp2.find(')');
							if (pos != strTemp2.npos) strTemp2 = strTemp2.substr(0, pos);
						} else {
							strTemp2 = strTemp.substr(3);
							pos = strTemp2.find(',');
							if (pos != strTemp2.npos) strTemp2 = strTemp2.substr(0, pos);
						}
						nAddrOffsetTemp = strtoul(strTemp2.c_str(), nullptr, 16);
						if (strTemp.at(2) == '-') nAddrOffsetTemp = 0 - nAddrOffsetTemp;
						nAddrTemp = GetAbsAddress() + GetByteCount() + nAddrOffsetTemp;		// Resolve address
						if (m_pParentFuncFile->AddrHasLabel(CFuncDescFile::MEMORY_TYPE::MT_ROM, nAddrTemp)) {
							// If the address has a user-defined file-level label, use it instead:
							ssDiff << "C=" << m_pParentFuncFile->GetPrimaryLabel(CFuncDescFile::MEMORY_TYPE::MT_ROM, nAddrTemp);
						} else {
							// If there isn't a label, see if this relative address lies inside the function:
							if (zFuncRange.addressInRange(nAddrTemp)) {
								// If so, continue to treat it as a relative address:
								std::sprintf(arrTemp, "C^%c%lX",
											((nAddrOffsetTemp != labs(nAddrOffsetTemp)) ? '-' : '+'),
											labs(nAddrOffsetTemp));
								ssDiff << arrTemp;
							} else {
								// Otherwise, it is an outside reference to something unknown:
								if (nLevel == FDL_1) {
									// Level 1 : Keep unknown
									ssDiff << "C?";
								} else {
									// Level 2 : Synthesize label
									if (!m_pParentFuncFile->allowMemRangeOverlap()) {
										std::sprintf(arrTemp, "C=L%04X", nAddrTemp);
									} else {
										std::sprintf(arrTemp, "C=CL%04X", nAddrTemp);
									}
									ssDiff << arrTemp;
								}
							}
						}
						// If there is a mask (or other appendage) add it:
						pos = strTemp.find(',');
						if (pos != strTemp.npos) ssDiff << strTemp.substr(pos);
						break;
					}
					case '&':		// Reg. Offset
						ssDiff << strTemp;
						break;
				}
				break;
			case 'D':				// Data addressing
			{
				TLabel strLabel;
				if (strTemp.size() < 2) continue;
				switch (strTemp.at(1)) {
					case '@':		// Absolute
						nAddrTemp = strtoul(strTemp.substr(2).c_str(), nullptr, 16);
						if (!m_pParentFuncFile->allowMemRangeOverlap()) {
							// Handle non-Harvard Architecture:
							strLabel = m_pParentFuncFile->GetAnyPrimaryLabel(false, nAddrTemp);
							if (!strLabel.empty()) {
								// If the address has a user-defined file-level label, use it instead:
								ssDiff << "D=" << strLabel;
							} else {
								// If there isn't a label, see if this absolute address lies inside the function:
								if (zFuncRange.addressInRange(nAddrTemp)) {
									// If so, convert and treat as a relative address:
									nAddrOffsetTemp = nAddrTemp - (GetAbsAddress() + GetByteCount());
									std::sprintf(arrTemp, "D^%c%lX",
												((nAddrOffsetTemp != labs(nAddrOffsetTemp)) ? '-' : '+'),
												labs(nAddrOffsetTemp));
									ssDiff << arrTemp;
								} else {
									// See if it is an I/O or NON-ROM/RAM location (or at >Level_1):
									if ((nLevel > FDL_1) ||
										(m_pParentFuncFile->isMemAddr(CFuncDescFile::MEMORY_TYPE::MT_IO, nAddrTemp)) ||
										(!m_pParentFuncFile->isMemAddr(CFuncDescFile::MEMORY_TYPE::MT_ROM, nAddrTemp) &&
										 !m_pParentFuncFile->isMemAddr(CFuncDescFile::MEMORY_TYPE::MT_RAM, nAddrTemp))) {
										// If so, create a label to reference it as it is significant:
										std::sprintf(arrTemp, "D=L%04X", nAddrTemp);
										ssDiff << arrTemp;
									} else {
										// Otherwise, it is an outside reference to something unknown in RAM/ROM
										//		and is probably a variable in memory that can move:
										ssDiff << "D?";
									}
								}
							}
						} else {
							// Handle Harvard-like Architecture (i.e. no code relative data):
							//	Like non-Harvard, but give priority to IO and RAM data (i.e. invert priority), and don't use
							//	function relative addressing:
							strLabel = m_pParentFuncFile->GetAnyPrimaryLabel(true, nAddrTemp);
							if (!strLabel.empty()) {
								// If the address has a user-defined file-level label, use it instead:
								ssDiff << "D=" << strLabel;
							} else {
								// See if it is an I/O or NON-ROM/RAM location (or at >Level_1):
								if ((nLevel > FDL_1) ||
									(m_pParentFuncFile->isMemAddr(CFuncDescFile::MEMORY_TYPE::MT_IO, nAddrTemp)) ||
									(!m_pParentFuncFile->isMemAddr(CFuncDescFile::MEMORY_TYPE::MT_ROM, nAddrTemp) &&
									 !m_pParentFuncFile->isMemAddr(CFuncDescFile::MEMORY_TYPE::MT_RAM, nAddrTemp))) {
									// If so, create a label to reference it as it is significant:
									std::sprintf(arrTemp, "D=DL%04X", nAddrTemp);
									ssDiff << arrTemp;
								} else {
									// Otherwise, it is an outside reference to something unknown in RAM/ROM
									//		and is probably a variable in memory that can move:
									ssDiff << "D?";
								}
							}
						}
						// If there is a mask (or other appendage like bit flags) add it:
						pos = strTemp.find(',');
						if (pos != strTemp.npos) ssDiff << strTemp.substr(pos);
						break;
					case '^':		// Relative
						// These will only be on non-Harvard architectures where we can have data relative to code
						// Relative addressing is always relative to the "next byte" after the instruction:
						if (strTemp.size() < 3) continue;
						pos = strTemp.find('(', 3);
						if (pos != strTemp.npos) {
							strTemp2 = strTemp.substr(3, pos-3);
							pos = strTemp2.find(')');
							if (pos != strTemp2.npos) strTemp2 = strTemp2.substr(0, pos);
						} else {
							strTemp2 = strTemp.substr(3);
							pos = strTemp2.find(',');
							if (pos != strTemp2.npos) strTemp2 = strTemp2.substr(0, pos);
						}
						nAddrOffsetTemp = strtoul(strTemp2.c_str(), nullptr, 16);
						if (strTemp.at(2) == '-') nAddrOffsetTemp = 0 - nAddrOffsetTemp;
						nAddrTemp = GetAbsAddress() + GetByteCount() + nAddrOffsetTemp;		// Resolve address
						strLabel = m_pParentFuncFile->GetAnyPrimaryLabel(m_pParentFuncFile->allowMemRangeOverlap(), nAddrTemp);
						if (!strLabel.empty()) {
							// If the address has a user-defined file-level label, use it instead:
							ssDiff << "D=" << strLabel;
						} else {
							// If there isn't a label, see if this relative address lies inside the function:
							if (zFuncRange.addressInRange(nAddrTemp)) {
								// If so, continue to treat it as a relative address:
								std::sprintf(arrTemp, "D^%c%lX",
											((nAddrOffsetTemp != labs(nAddrOffsetTemp)) ? '-' : '+'),
											labs(nAddrOffsetTemp));
								ssDiff << arrTemp;
							} else {
								// See if it is an I/O or NON-ROM/RAM location (or at >Level_1):
								if ((nLevel > FDL_1) ||
									(m_pParentFuncFile->isMemAddr(CFuncDescFile::MEMORY_TYPE::MT_IO, nAddrTemp)) ||
									(!m_pParentFuncFile->isMemAddr(CFuncDescFile::MEMORY_TYPE::MT_ROM, nAddrTemp) &&
									 !m_pParentFuncFile->isMemAddr(CFuncDescFile::MEMORY_TYPE::MT_RAM, nAddrTemp))) {
									// If so, treat create a label to reference it as it is significant:
									std::sprintf(arrTemp, "D=L%04X", nAddrTemp);
									ssDiff << arrTemp;
								} else {
									// Otherwise, it is an outside reference to something unknown in RAM/ROM
									//		and is probably a variable in memory that can move:
									ssDiff << "D?";
								}
							}
						}
						// If there is a mask (or other appendage) add it:
						pos = strTemp.find(',');
						if (pos != strTemp.npos) ssDiff << strTemp.substr(pos);
						break;
					case '&':		// Reg. Offset
						ssDiff << strTemp;
						break;
				}
				break;
			}
			case 'R':				// Registers
				ssDiff << strTemp;
				break;
		}
	}

	return ssDiff.str();
}

TString CFuncAsmInstObject::CreateOutputLine(OUTPUT_OPTIONS nOutputOptions) const
{
	TString strRetVal;

	if (nOutputOptions & OO_ADD_ADDRESS) {
		char arrTemp[30];
		std::sprintf(arrTemp, "%04X ", m_nAbsAddress);
		strRetVal += padString(arrTemp, GetFieldWidth(FIELD_CODE::FC_ADDRESS));
	}

	TString strTemp = m_pParentFunc->GetPrimaryLabel(m_nAbsAddress);
	strRetVal += padString(strTemp + ((!strTemp.empty()) ? ": " : " "), GetFieldWidth(FIELD_CODE::FC_LABEL));
	strRetVal += padString(m_strOpCodeText + " ", GetFieldWidth(FIELD_CODE::FC_MNEMONIC));
	strRetVal += padString(m_strOperandText, GetFieldWidth(FIELD_CODE::FC_OPERANDS));

	return strRetVal;
}

CSymbolArray CFuncAsmInstObject::GetSymbols() const
{
	char arrTemp[30];
	TString strTemp;
	TString strTemp2;
	TAddress nAddrTemp;
	TAddressOffset nAddrOffsetTemp;
	TString::size_type pos;

	TString strLabelPrefix;

	// Call parent to build initial list:
	CSymbolArray aSymbolArray = CFuncObject::GetSymbols();

	for (int i=0; i<2; i++) {
		switch (i) {
			case 0:
				strTemp = m_strDstOperand;
				strLabelPrefix = "RD";
				break;
			case 1:
				strTemp = m_strSrcOperand;
				strLabelPrefix = "RS";
				break;
		}

		if (strTemp.empty()) continue;

		switch (strTemp.at(0)) {
			case '#':				// Immediate Data
				break;
			case 'C':				// Code addressing
				if (strTemp.size() < 2) continue;
				switch (strTemp.at(1)) {
					case '@':		// Absolute
						nAddrTemp = strtoul(strTemp.substr(2).c_str(), nullptr, 16);
						if (m_pParentFuncFile->AddrHasLabel(CFuncDescFile::MEMORY_TYPE::MT_ROM, nAddrTemp)) {
							// If the address has a user-defined file-level label, use it instead:
							aSymbolArray.push_back(strLabelPrefix + "C" + m_pParentFuncFile->GetPrimaryLabel(CFuncDescFile::MEMORY_TYPE::MT_ROM, nAddrTemp));
						} else {
							// Else, build a label:
							std::sprintf(arrTemp, "L%04X", nAddrTemp);
							aSymbolArray.push_back(strLabelPrefix + "C" + arrTemp);
						}
						break;
					case '^':		// Relative
						// Relative addressing is always relative to the "next byte" after the instruction:
						if (strTemp.size() < 3) continue;
						pos = strTemp.find('(', 3);
						if (pos != strTemp.npos) {
							strTemp2 = strTemp.substr(3, pos-3);
							pos = strTemp2.find(')');
							if (pos != strTemp2.npos) strTemp2 = strTemp2.substr(0, pos);
						} else {
							strTemp2 = strTemp.substr(3);
							pos = strTemp2.find(',');
							if (pos != strTemp2.npos) strTemp2 = strTemp2.substr(0, pos);
						}
						nAddrOffsetTemp = strtoul(strTemp2.c_str(), nullptr, 16);
						if (strTemp.at(2) == '-') nAddrOffsetTemp = 0 - nAddrOffsetTemp;
						nAddrTemp = GetAbsAddress() + GetByteCount() + nAddrOffsetTemp;		// Resolve address
						if (m_pParentFuncFile->AddrHasLabel(CFuncDescFile::MEMORY_TYPE::MT_ROM, nAddrTemp)) {
							// If the address has a user-defined file-level label, use it instead:
							aSymbolArray.push_back(strLabelPrefix + "C" + m_pParentFuncFile->GetPrimaryLabel(CFuncDescFile::MEMORY_TYPE::MT_ROM, nAddrTemp));
						} else {
							// Else, build a label:
							std::sprintf(arrTemp, "L%04X", nAddrTemp);
							aSymbolArray.push_back(strLabelPrefix + "C" + arrTemp);
						}
						break;
					case '&':		// Reg. Offset
						break;
				}
				break;
			case 'D':				// Data addressing
			{
				TLabel strLabel;
				if (strTemp.size() < 2) continue;
				switch (strTemp.at(1)) {
					case '@':		// Absolute
						nAddrTemp = strtoul(strTemp.substr(2).c_str(), nullptr, 16);
						strLabel = m_pParentFuncFile->GetAnyPrimaryLabel(m_pParentFuncFile->allowMemRangeOverlap(), nAddrTemp);
						if (!strLabel.empty()) {
							// If the address has a user-defined file-level label, use it instead:
							aSymbolArray.push_back(strLabelPrefix + "D" + strLabel);
						} else {
							// Else, build a label:
							std::sprintf(arrTemp, "L%04X", nAddrTemp);
							aSymbolArray.push_back(strLabelPrefix + "D" + arrTemp);
						}
						break;
					case '^':		// Relative
						// Relative addressing is always relative to the "next byte" after the instruction:
						if (strTemp.size() < 3) continue;
						pos = strTemp.find('(', 3);
						if (pos != strTemp.npos) {
							strTemp2 = strTemp.substr(3, pos-3);
							pos = strTemp2.find(')');
							if (pos != strTemp2.npos) strTemp2 = strTemp2.substr(0, pos);
						} else {
							strTemp2 = strTemp.substr(3);
							pos = strTemp2.find(',');
							if (pos != strTemp2.npos) strTemp2 = strTemp2.substr(0, pos);
						}
						nAddrOffsetTemp = strtoul(strTemp2.c_str(), nullptr, 16);
						if (strTemp.at(2) == '-') nAddrOffsetTemp = 0 - nAddrOffsetTemp;
						nAddrTemp = GetAbsAddress() + GetByteCount() + nAddrOffsetTemp;		// Resolve address
						strLabel = m_pParentFuncFile->GetAnyPrimaryLabel(m_pParentFuncFile->allowMemRangeOverlap(), nAddrTemp);
						if (!strLabel.empty()) {
							// If the address has a user-defined file-level label, use it instead:
							aSymbolArray.push_back(strLabelPrefix + "D" + strLabel);
						} else {
							// Else, build a label:
							std::sprintf(arrTemp, "L%04X", nAddrTemp);
							aSymbolArray.push_back(strLabelPrefix + "D" + arrTemp);
						}
						break;
					case '&':		// Reg. Offset
						break;
				}
				break;
			}
			case 'R':				// Registers
				break;
		}
	}

	return aSymbolArray;
}

TString CFuncAsmInstObject::GetOpCodeBytes() const
{
	std::ostringstream ssTemp;
	for (auto const &itrByte : m_OpCodeBytes) {
		ssTemp << std::uppercase << std::setfill('0') << std::setw(2) << std::setbase(16) << static_cast<unsigned int>(itrByte);
	}
	return ssTemp.str();
}

TString CFuncAsmInstObject::GetOperandBytes() const
{
	std::ostringstream ssTemp;
	for (auto const &itrByte : m_OperandBytes) {
		ssTemp << std::uppercase << std::setfill('0') << std::setw(2) << std::setbase(16) << static_cast<unsigned int>(itrByte);
	}
	return ssTemp.str();
}


//////////////////////////////////////////////////////////////////////
// CFuncDataByteObject Class
//////////////////////////////////////////////////////////////////////
TString CFuncDataByteObject::ExportToDiff(FUNC_DIFF_LEVEL nLevel) const
{
	UNUSED(nLevel);

	std::ostringstream ssDiff;

	ssDiff << "D|" << GetByteCount() << "|" << GetBytes();
	return ssDiff.str();
}

TString CFuncDataByteObject::CreateOutputLine(OUTPUT_OPTIONS nOutputOptions) const
{
	TString strRetVal;
	char arrTemp[30];
	TString strTemp;

	if (nOutputOptions & OO_ADD_ADDRESS) {
		std::sprintf(arrTemp, "%04X ", m_nAbsAddress);
		strRetVal += padString(arrTemp, GetFieldWidth(FIELD_CODE::FC_ADDRESS));
	}

	strTemp = m_pParentFunc->GetPrimaryLabel(m_nAbsAddress);
	strRetVal += padString(strTemp + ((!strTemp.empty()) ? ": " : " "), GetFieldWidth(FIELD_CODE::FC_LABEL));
	strRetVal += padString(".data ", GetFieldWidth(FIELD_CODE::FC_MNEMONIC));
	strTemp.clear();
	for (auto const &itrBytes : m_Bytes) {
		if (!strTemp.empty()) strTemp += ", ";
		std::sprintf(arrTemp, "0x%02X", itrBytes);
		strTemp += arrTemp;
	}
	strRetVal += padString(strTemp, GetFieldWidth(FIELD_CODE::FC_OPERANDS));

	return strRetVal;
}


//////////////////////////////////////////////////////////////////////
// CFuncDesc Class
//////////////////////////////////////////////////////////////////////

CFuncDesc::CFuncDesc(TAddress nAddress, const TString &strNames)
	:	m_nMainAddress(nAddress),
		m_nFunctionSize(0)
{
	CStringArray argv;

	ParseLine(strNames, ',', argv);
	for (auto const &name : argv) AddName(nAddress, name);
}

bool CFuncDesc::AddName(TAddress nAddress, const TLabel &strName)
{
	if (strName.empty()) return false;

	CLabelArray &arrNames = m_mapFuncNameTable[nAddress];
	for (auto const &itrNames : arrNames) {
		if (compareNoCase(strName, itrNames) == 0) return false;	// Skip if we have the label already
	}
	arrNames.push_back(strName);
	return true;
}

TLabel CFuncDesc::GetMainName() const
{
	TString strRetVal;
	char arrTemp[30];

	// Ideally, these would be "L" or "CL" based on CFuncDescFile::allowMemRangeOverlap(),
	//	but we don't have a pointer to the parent function.  So just default to the
	//	unambiguous "CL":
	std::sprintf(arrTemp, "CL%0X", m_nMainAddress);
	strRetVal = arrTemp;

	CLabelTableMap::const_iterator itrFuncNames = m_mapFuncNameTable.find(m_nMainAddress);
	if (itrFuncNames != m_mapFuncNameTable.cend()) {
		if (itrFuncNames->second.empty()) return strRetVal;
		if (itrFuncNames->second.at(0) == "???") return strRetVal;		// Special case for unnamed functions
		return itrFuncNames->second.at(0);
	}

	return strRetVal;
}

bool CFuncDesc::AddLabel(TAddress nAddress, const TLabel &strLabel)
{
	if (strLabel.empty()) return false;

	CLabelArray &arrLabels = m_mapLabelTable[nAddress];
	for (auto const &itrLabels : arrLabels) {
		if (compareNoCase(strLabel, itrLabels) == 0) return false;	// Skip if we have the label already
	}
	arrLabels.push_back(strLabel);
	return true;
}

bool CFuncDesc::AddrHasLabel(TAddress nAddress) const
{
	return m_mapLabelTable.contains(nAddress);
}

TLabel CFuncDesc::GetPrimaryLabel(TAddress nAddress) const
{
	CLabelArray arrLabels = GetLabelList(nAddress);
	if (arrLabels.empty()) return TLabel();
	return arrLabels.at(0);
}

CLabelArray CFuncDesc::GetLabelList(TAddress nAddress) const
{
	CLabelTableMap::const_iterator itrMap = m_mapLabelTable.find(nAddress);
	if (itrMap == m_mapLabelTable.cend()) return CLabelArray();
	return itrMap->second;
}

TSize CFuncDesc::GetFuncSize() const
{
//	CFuncObject *pFuncObject;
//	int i;
//	DWORD nRetVal;
//
//	nRetVal = 0;
//
//	// Add byte counts to get size:
//	for (i=0; i<GetSize(); i++) {
//		pFuncObject = GetAt(i);
//		if (pFuncObject) nRetVal += pFuncObject->GetByteCount();
//	}
//
//	return nRetVal;

	// For efficiency, the above has been replaced with the following:
	return m_nFunctionSize;
}

void CFuncDesc::ExportToDiff(FUNC_DIFF_LEVEL nLevel, CStringArray &anArray) const
{
	anArray.clear();

	for (auto const &itrObject : *this) {
		anArray.push_back(itrObject->ExportToDiff(nLevel));
	}
}

void CFuncDesc::Add(std::shared_ptr<CFuncObject> pObj)
{
	for (CLabelArray::size_type i=0; i<pObj->GetLabelCount(); i++) {
		AddLabel(pObj->GetAbsAddress(), pObj->GetLabel(i));
	}
	m_nFunctionSize += pObj->GetByteCount();
	push_back(pObj);
}


//////////////////////////////////////////////////////////////////////
// CFuncDescFile Class
//////////////////////////////////////////////////////////////////////

bool CFuncDescFile::ReadFuncDescFile(std::shared_ptr<CFuncDescFile> pThis, ifstreamFuncDescFile &inFile, std::ostream *msgFile, std::ostream *errFile, int nStartLineCount)
{
	bool bRetVal = true;
	TString strError = g_strUnexpectedError;
	int nLineCount = nStartLineCount;
	TString strLine;
	CStringArray argv;
	TAddress nAddress;
	TSize nSize;
	std::shared_ptr<CFuncDesc> pCurrentFunction;
	CFuncDescArray::size_type nCurrentFunctionIndex = 0;

	constexpr int BUSY_CALLBACK_RATE = 50;

	if (msgFile) {
		(*msgFile) << "Reading Function Definition File " << std::filesystem::relative(inFile.getFilename()) << "...\n";
	}

	m_strFilePathName = std::filesystem::path(inFile.getFilename()).relative_path();
	m_strFileName = std::filesystem::path(inFile.getFilename()).filename();

	while (bRetVal && inFile.good() && !inFile.eof()) {
		std::getline(inFile, strLine);
		nLineCount++;
		trim(strLine);

		if ((m_pfnProgressCallback) && ((nLineCount % BUSY_CALLBACK_RATE) == 0))
			m_pfnProgressCallback(0, 1, false, m_nUserDataProgressCallback);

		// Empty lines are ignored:
		if (strLine.empty()) continue;

		switch (strLine.at(0)) {
			case ';':			// Comments
				// Ignore comment lines:
				break;

			case '-':				// FuncAnal Command
			{
				if (pCurrentFunction) {
					m_mapSortedFunctionMap.insert(std::pair<CFuncDesc::size_type, CFuncDescArray::size_type>(pCurrentFunction->size(), nCurrentFunctionIndex));
				}
				pCurrentFunction = nullptr;

				ParseLine(strLine.substr(1), '|', argv);
				if (argv.size() != 2) {
					strError = g_strSyntaxError;
					bRetVal = false;
					break;
				}

				int nParseCmd = parseKeyword(g_mapParseFuncAnalCmds, argv.at(0));
				if (nParseCmd == -1) {
					strError = g_strUnknownFuncAnalCommand;
					bRetVal = false;
					break;
				}

				ParseLine(argv.at(1), ',', argv);	// Parse Command Arguments

				switch (static_cast<FUNC_ANAL_COMMANDS_ENUM>(nParseCmd)) {
					case FACE_MemRangeOverlap:
						if (argv.size() != 1) {
							strError = g_strSyntaxError;
							bRetVal = false;
						} else {
							int nParseTF = parseKeyword(g_mapParseTrueFalse, argv.at(0));
							if (nParseTF == -1) {
								strError = g_strInvalidTrueFalse;
								bRetVal = false;
							} else {
								m_bAllowMemRangeOverlap = (nParseTF != TFE_False);
							}
						}
						break;

					case FACE_OpcodeSymbolSize:
						if (argv.size() != 1) {
							strError = g_strSyntaxError;
							bRetVal = false;
						} else {
							m_nOpcodeSymbolSize = strtoul(argv[0].c_str(), nullptr, 0);
							if (m_nOpcodeSymbolSize == 0) {		// Width can't be zero
								strError = g_strInvalidOpcodeSymbolWidth;
								bRetVal = false;
							}
						}
						break;
				}

				break;
			}

			case '#':			// Memory Mapping
			{
				if (pCurrentFunction) {
					m_mapSortedFunctionMap.insert(std::pair<CFuncDesc::size_type, CFuncDescArray::size_type>(pCurrentFunction->size(), nCurrentFunctionIndex));
				}
				pCurrentFunction = nullptr;

				ParseLine(strLine.substr(1), '|', argv);
				if (argv.size() != 3) {
					strError = g_strSyntaxError;
					bRetVal = false;
					break;
				}

				int nParseVal = parseKeyword(g_mapParseMemType, argv.at(0));
				if (nParseVal == -1) {
					strError = g_strUnknownMemoryRangeName;
					bRetVal = false;
					break;
				}

				nAddress = strtoul(argv.at(1).c_str(), nullptr, 16);
				nSize = strtoul(argv.at(2).c_str(), nullptr, 16);

				m_MemoryRanges[nParseVal].push_back(CMemRange(nAddress, nSize));
				m_MemoryRanges[nParseVal].compact();
				m_MemoryRanges[nParseVal].removeOverlaps();
				m_MemoryRanges[nParseVal].sort();

				if (!allowMemRangeOverlap()) {
					for (int nMemType = 0; nMemType < MEMORY_TYPE::NUM_MEMORY_TYPES; ++nMemType) {
						if (nParseVal == nMemType) continue;
						if (m_MemoryRanges[nMemType].rangesOverlap(m_MemoryRanges[nParseVal].back())) {
							if (errFile) {
								(*errFile) << "*** Warning: Specified " + g_arrstrMemRanges[nParseVal] +
											" Mapping conflicts with " + g_arrstrMemRanges[nMemType] + " Mapping\n";
							}
						}
					}
				}

				break;
			}

			case '!':			// Label
			{
				if (pCurrentFunction) {
					m_mapSortedFunctionMap.insert(std::pair<CFuncDesc::size_type, CFuncDescArray::size_type>(pCurrentFunction->size(), nCurrentFunctionIndex));
				}
				pCurrentFunction = nullptr;

				ParseLine(strLine.substr(1), '|', argv);
				if (argv.size() != 3) {
					strError = g_strSyntaxError;
					bRetVal = false;
					break;
				}

				int nParseVal = parseKeyword(g_mapParseMemType, argv.at(0));
				if (nParseVal == -1) {
					strError = g_strSyntaxError;
					bRetVal = false;
					break;
				}

				nAddress = strtoul(argv.at(1).c_str(), nullptr, 16);
				ParseLine(argv.at(2), ',', argv);

				for (auto const & label : argv) AddLabel(static_cast<CFuncDescFile::MEMORY_TYPE>(nParseVal), nAddress, label);
				break;
			}

			case '@':			// New Function declaration:
				if (pCurrentFunction) {
					m_mapSortedFunctionMap.insert(std::pair<CFuncDesc::size_type, CFuncDescArray::size_type>(pCurrentFunction->size(), nCurrentFunctionIndex));
				}
				pCurrentFunction = nullptr;

				ParseLine(strLine.substr(1), '|', argv);
				if (argv.size() != 2) {
					strError = g_strSyntaxError;
					bRetVal = false;
					break;
				}

				nAddress = strtoul(argv.at(0).c_str(), nullptr, 16);

				nCurrentFunctionIndex = m_arrFunctions.size();
				m_arrFunctions.push_back(std::make_shared<CFuncDesc>(nAddress, argv.at(1)));
				pCurrentFunction = m_arrFunctions.back();
				break;

			default:
				// See if we are in the middle of a function declaration:
				if (pCurrentFunction == nullptr) {
					// If we aren't in a function, it's a syntax error:
					strError = g_strSyntaxError;
					bRetVal = false;
					break;
				}

				// If we are in a function, parse entry:
				if (isxdigit(strLine.at(0))) {
					ParseLine(strLine, '|', argv);
					if ((argv.size() != 4) &&
						(argv.size() != 10)) {
						strError = g_strSyntaxError;
						bRetVal = false;
						break;
					}

					if (argv.size() == 4) {
						pCurrentFunction->Add(std::make_shared<CFuncDataByteObject>(pThis, pCurrentFunction, argv));
					} else {
						std::shared_ptr<CFuncAsmInstObject> pAsmInst = std::make_shared<CFuncAsmInstObject>(pThis, pCurrentFunction, argv);
						pCurrentFunction->Add(pAsmInst);
						assert(pThis->opcodeSymbolSize() > 0);
						if (pAsmInst->GetOpCodeByteCount() % pThis->opcodeSymbolSize()) {
							strError = g_strInvalidOpcodeLength;
							bRetVal = false;
						}
					}
				} else {
					strError = g_strSyntaxError;
					bRetVal = false;
				}
				break;
		}
	}
	if (pCurrentFunction) {
		m_mapSortedFunctionMap.insert(std::pair<CFuncDesc::size_type, CFuncDescArray::size_type>(pCurrentFunction->size(), nCurrentFunctionIndex));
	}
	assert(m_mapSortedFunctionMap.size() == m_arrFunctions.size());

	if ((bRetVal) && (msgFile)) {
		if (allowMemRangeOverlap()) {
			(*msgFile) << "\n    Allowing Memory Range Overlaps\n";
		}
		if (opcodeSymbolSize() > 1) {
			(*msgFile) << "\n    Opcode Symbol Size: " << opcodeSymbolSize() << "\n";
		}

		(*msgFile) << "\n    Memory Mappings:\n";

		for (int nMemType = 0; nMemType < MEMORY_TYPE::NUM_MEMORY_TYPES; ++nMemType) {
			(*msgFile) << "        ";
			for (std::string::size_type i = (getLongestMemMapName() - g_arrstrMemRanges[nMemType].size()); i; --i) {
				(*msgFile) << " ";
			}
			(*msgFile) << g_arrstrMemRanges[nMemType] << " Memory Map:";

			if (m_MemoryRanges[nMemType].isNullRange()) {
				(*msgFile) << " <Not Defined>\n";
			} else {
				(*msgFile) << "\n";
				for (auto const & itr : m_MemoryRanges[nMemType]) {
					(*msgFile) << "            0x"
								<< std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << itr.startAddr()
								<< std::nouppercase << std::setbase(0) << " - 0x"
								<< std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << (itr.startAddr() + itr.size() - 1)
								<< std::nouppercase << std::setbase(0) << "  (Size: 0x"
								<< std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << itr.size()
								<< std::nouppercase << std::setbase(0) << ")\n";
				}
			}
		}

		(*msgFile) << "\n    " << m_arrFunctions.size() << " Function"
					<< ((m_arrFunctions.size() != 1) ? "s" : "") << " Defined"
					<< ((m_arrFunctions.size() != 0) ? ":" : "") << "\n";

		for (auto const & func : m_arrFunctions) {
			(*msgFile) << "        0x"
						<< std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << func->GetMainAddress()
						<< std::nouppercase << std::setbase(0)
						<< " -> " << func->GetMainName() << "\n";
		}

		for (int nMemType = 0; nMemType < MEMORY_TYPE::NUM_MEMORY_TYPES; ++nMemType) {
			(*msgFile) << "\n";
			(*msgFile) << "    " << m_mapLabelTable[nMemType].size()
						<< " Unique " << g_arrstrMemRanges[nMemType] << " Label"
						<< ((m_mapLabelTable[nMemType].size() != 1) ? "s" : "") << " Defined"
						<< ((m_mapLabelTable[nMemType].size() != 0) ? ":" : "") << "\n";
			for (auto const & labelList : m_mapLabelTable[nMemType]) {
				(*msgFile) << "        0x"
							<< std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << labelList.first
							<< std::nouppercase << std::setbase(0)
							<< "=";
				bool bFirst = true;
				for (auto const & label : labelList.second) {
					(*msgFile) << (!bFirst ? "," : "") << label;
					bFirst = false;
				}
				(*msgFile) << "\n";
			}
		}

		(*msgFile) << "\n";
	}

	if (!bRetVal && errFile) {
		(*errFile) << "*** Error: " << strError << " : on line " << nLineCount << " of file\n"
					  "           \"" << std::filesystem::relative(inFile.getFilename()) << "\"\n";
	}

	return bRetVal;
}

bool CFuncDescFile::AddLabel(MEMORY_TYPE nMemoryType, TAddress nAddress, const TLabel &strLabel)
{
	if (strLabel.empty()) return false;

	CLabelArray &arrLabels = m_mapLabelTable[nMemoryType][nAddress];
	for (auto const &itrLabels : arrLabels) {
		if (compareNoCase(strLabel, itrLabels) == 0) return false;	// Skip if we have the label already
	}
	arrLabels.push_back(strLabel);
	return true;
}

bool CFuncDescFile::AddrHasLabel(MEMORY_TYPE nMemoryType, TAddress nAddress) const
{
	return (m_mapLabelTable[nMemoryType].contains(nAddress));
}

TLabel CFuncDescFile::GetPrimaryLabel(MEMORY_TYPE nMemoryType, TAddress nAddress) const
{
	if (!AddrHasLabel(nMemoryType, nAddress)) return TLabel();
	CLabelTableMap::const_iterator itrMap = m_mapLabelTable[nMemoryType].find(nAddress);
	if (itrMap == m_mapLabelTable[nMemoryType].cend()) return TLabel();
	return itrMap->second.at(0);
}

TLabel CFuncDescFile::GetAnyPrimaryLabel(bool bInvertPriority, TAddress nAddress) const
{
	TLabel strLabel;

	if (!bInvertPriority) {
		strLabel = GetPrimaryLabel(MEMORY_TYPE::MT_ROM, nAddress);
		if (strLabel.empty()) strLabel = GetPrimaryLabel(MEMORY_TYPE::MT_RAM, nAddress);
		if (strLabel.empty()) strLabel = GetPrimaryLabel(MEMORY_TYPE::MT_IO, nAddress);
	} else {
		strLabel = GetPrimaryLabel(MEMORY_TYPE::MT_IO, nAddress);
		if (strLabel.empty()) strLabel = GetPrimaryLabel(MEMORY_TYPE::MT_RAM, nAddress);
		if (strLabel.empty()) strLabel = GetPrimaryLabel(MEMORY_TYPE::MT_ROM, nAddress);
	}
	return strLabel;
}

CLabelArray CFuncDescFile::GetLabelList(MEMORY_TYPE nMemoryType, TAddress nAddress) const
{
	CLabelTableMap::const_iterator itrMap = m_mapLabelTable[nMemoryType].find(nAddress);
	if (itrMap == m_mapLabelTable[nMemoryType].cend()) return CLabelArray();
	return itrMap->second;
}

bool CFuncDescFile::isMemAddr(MEMORY_TYPE nMemType, TAddress nAddress) const
{
	return m_MemoryRanges[nMemType].addressInRange(nAddress);
}


//////////////////////////////////////////////////////////////////////
// CFuncDescFileArray Class
//////////////////////////////////////////////////////////////////////

CFuncDescArray::size_type CFuncDescFileArray::GetFuncCount() const
{
	CFuncDescArray::size_type nRetVal = 0;

	for (auto const & itrDescFile : *this) {
		nRetVal += itrDescFile->GetFuncCount();
	}

	return nRetVal;
}

double CFuncDescFileArray::CompareFunctions(FUNC_COMPARE_METHOD nMethod,
											size_type nFile1Ndx, std::size_t nFile1FuncNdx,
											size_type nFile2Ndx, std::size_t nFile2FuncNdx,
											bool bBuildEditScript) const
{
	try
	{
		return ::CompareFunctions(nMethod, *at(nFile1Ndx), nFile1FuncNdx, *at(nFile2Ndx), nFile2FuncNdx, bBuildEditScript);
	}
	catch (const EXCEPTION_ERROR &aErr)
	{
		std::cerr << "\n*** Error: " << aErr.errorMessage();
		if (!aErr.m_strDetail.empty()) {
			std::cerr << " (" << aErr.m_strDetail << ")";
		}
		std::cerr << "\n";
	}
	return 0.0;
}

TString CFuncDescFileArray::DiffFunctions(FUNC_COMPARE_METHOD nMethod,
									int nFile1Ndx, int nFile1FuncNdx,
									int nFile2Ndx, int nFile2FuncNdx,
									OUTPUT_OPTIONS nOutputOptions,
									double &nMatchPercent,
									CSymbolMap *pSymbolMap) const
{
	return ::DiffFunctions(nMethod, *at(nFile1Ndx), nFile1FuncNdx, *at(nFile2Ndx), nFile2FuncNdx,
							nOutputOptions, nMatchPercent, pSymbolMap);
}


//////////////////////////////////////////////////////////////////////
// CSymbolMap Class
//////////////////////////////////////////////////////////////////////

bool CSymbolMap::empty() const
{
	return (m_LeftSideCodeSymbols.empty() &&
			m_RightSideCodeSymbols.empty() &&
			m_LeftSideDataSymbols.empty() &&
			m_RightSideDataSymbols.empty());
}

void CSymbolMap::AddObjectMapping(const CFuncObject &aLeftObject, const CFuncObject &aRightObject)
{
	CSymbolArray arrLeftSymbols;
	CSymbolArray arrRightSymbols;
	int nMode;		// 0 = Source, 1 = Destination
	int nType;		// 0 = Code, 1 = Data

	arrLeftSymbols = aLeftObject.GetSymbols();
	arrRightSymbols = aRightObject.GetSymbols();

	// Since we are comparing functions, any "L" and "CL" entries are automatically "Code" entries:
	for (auto const & leftSymbol : arrLeftSymbols) {
		if (leftSymbol.empty() || ((leftSymbol.at(0) != 'L') && !leftSymbol.starts_with("CL"))) continue;
		if (leftSymbol.size() < 2) continue;
		bool bFlag = false;
		for (auto const & rightSymbol : arrRightSymbols) {
			if (rightSymbol.empty() || ((rightSymbol.at(0) != 'L') && !rightSymbol.starts_with("CL"))) continue;
			if (rightSymbol.size() < 2) continue;
			AddLeftSideCodeSymbol(leftSymbol.substr(1), rightSymbol.substr(1));
			bFlag = true;
		}
		if (!bFlag) AddLeftSideCodeSymbol(leftSymbol.substr(1), TSymbol());
	}

	for (auto const & rightSymbol : arrRightSymbols) {
		if (rightSymbol.empty() || ((rightSymbol.at(0) != 'L') && !rightSymbol.starts_with("CL"))) continue;
		if (rightSymbol.size() < 2) continue;
		bool bFlag = false;
		for (auto const & leftSymbol : arrLeftSymbols) {
			if (leftSymbol.empty() || ((leftSymbol.at(0) != 'L') && !leftSymbol.starts_with("CL"))) continue;
			if (leftSymbol.size() < 2) continue;
			AddRightSideCodeSymbol(rightSymbol.substr(1), leftSymbol.substr(1));
			bFlag = true;
		}
		if (!bFlag) AddRightSideCodeSymbol(rightSymbol.substr(1), TSymbol());
	}

	// Add Left Source/Destination entries:
	for (auto const & leftSymbol : arrLeftSymbols) {
		if (leftSymbol.empty() || (leftSymbol.at(0) != 'R')) continue;
		if (leftSymbol.size() < 4) continue;
		switch (leftSymbol.at(1)) {
			case 'S':
				nMode = 0;
				break;
			case 'D':
				nMode = 1;
				break;
			default:
				continue;
		}
		switch (leftSymbol.at(2)) {
			case 'C':
				nType = 0;
				break;
			case 'D':
				nType = 1;
				break;
			default:
				continue;
		}
		bool bFlag = false;
		for (auto const & rightSymbol : arrRightSymbols) {
			if (rightSymbol.empty() || (rightSymbol.at(0) != 'R')) continue;
			if (rightSymbol.size() < 4) continue;
			bool bFlag2 = false;
			switch (rightSymbol.at(1)) {
				case 'S':
					if (nMode == 0) {
						bFlag2 = true;
					}
					break;
				case 'D':
					if (nMode == 1) {
						bFlag2 = true;
					}
					break;
				default:
					continue;
			}
			if (bFlag2) {
				switch (rightSymbol.at(2)) {
					case 'C':
						if (nType == 0) {
							AddLeftSideCodeSymbol(leftSymbol.substr(3), rightSymbol.substr(3));
							bFlag = true;
						}
						break;
					case 'D':
						if (nType == 1) {
							AddLeftSideDataSymbol(leftSymbol.substr(3), rightSymbol.substr(3));
							bFlag = true;
						}
						break;
					default:
						continue;
				}
			}
		}
		if (!bFlag) {
			switch (nType) {
				case 0:
					AddLeftSideCodeSymbol(leftSymbol.substr(3), TSymbol());
					break;
				case 1:
					AddLeftSideDataSymbol(leftSymbol.substr(3), TSymbol());
					break;
			}
		}
	}

	// Add Right Source/Destination entries:
	for (auto const & rightSymbol : arrRightSymbols) {
		if (rightSymbol.empty() || (rightSymbol.at(0) != 'R')) continue;
		if (rightSymbol.size() < 4) continue;
		switch (rightSymbol.at(1)) {
			case 'S':
				nMode = 0;
				break;
			case 'D':
				nMode = 1;
				break;
			default:
				continue;
		}
		switch (rightSymbol.at(2)) {
			case 'C':
				nType = 0;
				break;
			case 'D':
				nType = 1;
				break;
			default:
				continue;
		}
		bool bFlag = false;
		for (auto const & leftSymbol : arrLeftSymbols) {
			if (leftSymbol.empty() || (leftSymbol.at(0) != 'R')) continue;
			if (leftSymbol.size() < 4) continue;
			bool bFlag2 = false;
			switch (leftSymbol.at(1)) {
				case 'S':
					if (nMode == 0) {
						bFlag2 = true;
					}
					break;
				case 'D':
					if (nMode == 1) {
						bFlag2 = true;
					}
					break;
				default:
					continue;
			}
			if (bFlag2) {
				switch (leftSymbol.at(2)) {
					case 'C':
						if (nType == 0) {
							AddRightSideCodeSymbol(rightSymbol.substr(3), leftSymbol.substr(3));
							bFlag = true;
						}
						break;
					case 'D':
						if (nType == 1) {
							AddRightSideDataSymbol(rightSymbol.substr(3), leftSymbol.substr(3));
							bFlag = true;
						}
						break;
					default:
						continue;
				}
			}
		}
		if (!bFlag) {
			switch (nType) {
				case 0:
					AddRightSideCodeSymbol(rightSymbol.substr(3), TSymbol());
					break;
				case 1:
					AddRightSideDataSymbol(rightSymbol.substr(3), TSymbol());
					break;
			}
		}
	}
}

CSymbolArray CSymbolMap::GetLeftSideCodeSymbolList() const
{
	return GetSymbolList(m_LeftSideCodeSymbols);
}

CSymbolArray CSymbolMap::GetRightSideCodeSymbolList() const
{
	return GetSymbolList(m_RightSideCodeSymbols);
}

CSymbolArray CSymbolMap::GetLeftSideDataSymbolList() const
{
	return GetSymbolList(m_LeftSideDataSymbols);
}

CSymbolArray CSymbolMap::GetRightSideDataSymbolList() const
{
	return GetSymbolList(m_RightSideDataSymbols);
}

CSymbolArray CSymbolMap::GetSymbolList(const CSymbolArrayMap &mapSymbolArrays) const
{
	CSymbolArray arrRetVal;

	std::transform(mapSymbolArrays.cbegin(), mapSymbolArrays.cend(), std::back_inserter(arrRetVal),
					[](const CSymbolArrayMap::value_type &val)->CSymbolArrayMap::key_type { return val.first; });
	return arrRetVal;
}

THitCount CSymbolMap::GetLeftSideCodeHitList(const TSymbol &aSymbol, CSymbolArray &aSymbolArray, CHitCountArray &aHitCountArray) const
{
	return GetHitList(m_LeftSideCodeSymbols, aSymbol, aSymbolArray, aHitCountArray);
}

THitCount CSymbolMap::GetRightSideCodeHitList(const TSymbol &aSymbol, CSymbolArray &aSymbolArray, CHitCountArray &aHitCountArray) const
{
	return GetHitList(m_RightSideCodeSymbols, aSymbol, aSymbolArray, aHitCountArray);
}

THitCount CSymbolMap::GetLeftSideDataHitList(const TSymbol &aSymbol, CSymbolArray &aSymbolArray, CHitCountArray &aHitCountArray) const
{
	return GetHitList(m_LeftSideDataSymbols, aSymbol, aSymbolArray, aHitCountArray);
}

THitCount CSymbolMap::GetRightSideDataHitList(const TSymbol &aSymbol, CSymbolArray &aSymbolArray, CHitCountArray &aHitCountArray) const
{
	return GetHitList(m_RightSideDataSymbols, aSymbol, aSymbolArray, aHitCountArray);
}

THitCount CSymbolMap::GetHitList(const CSymbolArrayMap &mapSymbolArrays, const TSymbol &aSymbol,
									CSymbolArray &aSymbolArray, CHitCountArray &aHitCountArray) const
{
	CSymbolHitMap mapSymbolHits;
	THitCount nTotal = 0;

	aSymbolArray.clear();
	aHitCountArray.clear();

	CSymbolArrayMap::const_iterator itrSymbolMap = mapSymbolArrays.find(aSymbol);
	if (itrSymbolMap != mapSymbolArrays.cend()) {
		for (auto const &itrSymbols : itrSymbolMap->second) {
			CSymbolHitMap::iterator itrHitMap = mapSymbolHits.find(itrSymbols);
			if (itrHitMap != mapSymbolHits.end()) {
				++itrHitMap->second;
			} else {
				mapSymbolHits[itrSymbols] = 1;
			}
		}

		for (auto const &itrHits : mapSymbolHits) {
			CHitCountArray::size_type ndx;
			for (ndx = 0; ndx < aHitCountArray.size(); ++ndx) {
				if (itrHits.second > aHitCountArray.at(ndx)) break;
				if (itrHits.second == aHitCountArray.at(ndx)) {
					if (aSymbolArray.at(ndx).empty()) break;
					if ((itrHits.first < aSymbolArray.at(ndx)) &&
						!itrHits.first.empty()) break;
				}
			}
			aSymbolArray.insert(aSymbolArray.begin() + ndx, itrHits.first);
			aHitCountArray.insert(aHitCountArray.begin() + ndx, itrHits.second);
		}

		nTotal = itrSymbolMap->second.size();
	}

	return nTotal;
}

void CSymbolMap::AddLeftSideCodeSymbol(const TSymbol &aLeftSymbol, const TSymbol &aRightSymbol)
{
	m_LeftSideCodeSymbols[aLeftSymbol].push_back(aRightSymbol);
}

void CSymbolMap::AddRightSideCodeSymbol(const TSymbol &aRightSymbol, const TSymbol &aLeftSymbol)
{
	m_RightSideCodeSymbols[aRightSymbol].push_back(aLeftSymbol);
}

void CSymbolMap::AddLeftSideDataSymbol(const TSymbol &aLeftSymbol, const TSymbol &aRightSymbol)
{
	m_LeftSideDataSymbols[aLeftSymbol].push_back(aRightSymbol);
}

void CSymbolMap::AddRightSideDataSymbol(const TSymbol &aRightSymbol, const TSymbol &aLeftSymbol)
{
	m_RightSideDataSymbols[aRightSymbol].push_back(aLeftSymbol);
}

