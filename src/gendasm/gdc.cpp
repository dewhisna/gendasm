//
//	Generic Disassembly Class
//
//
//	Generic Code-Seeking Disassembler
//	Copyright(c)2021 by Donna Whisnant
//

#include "gdc.h"
#include "dfc.h"
#include "errmsgs.h"

#include <ctime>
#include <ctype.h>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <regex>
#include <filesystem>

#define VERSION 0x300				// GDC Version number 3.00

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
	enum CTRL_COMMANDS_ENUM {
		CCE_ENTRY,
		CCE_LOAD,
		CCE_INPUT,
		CCE_OUTPUT,
		CCE_LABEL,
		CCE_ADDRESSES,
		CCE_INDIRECT,
		CCE_OPBYTES,
		CCE_ASCII,
		CCE_SPIT,
		CCE_BASE,
		CCE_MAXNONPRINT,
		CCE_MAXPRINT,
		CCE_DFC,
		CCE_TABS,
		CCE_TABWIDTH,
		CCE_ASCIIBYTES,
		CCE_DATAOPBYTES,
		CCE_EXITFUNCTION,
		CCE_MEMMAP,
		CCE_COMMENT,
		CCE_MCU,
		CCE_DATABLOCK,
		CCE_ASSEMBLER,
	};

	enum OUTPUT_TYPE_ENUM {
		OTE_DISASSEMBLY,
		OTE_FUNCTIONS,
	};

	// Setup commands to parse:
	static const TKeywordMap g_mapParseCommands = {
		{ "^ENTRY$", CCE_ENTRY},
		{ "^LOAD$", CCE_LOAD },
		{ "^INPUT$", CCE_INPUT },
		{ "^OUTPUT$", CCE_OUTPUT },
		{ "^LABEL$", CCE_LABEL },
		{ "^ADDRESSES$", CCE_ADDRESSES },
		{ "^INDIRECT$", CCE_INDIRECT },
		{ "^OPCODES|OPBYTES$", CCE_OPBYTES },
		{ "^ASCII$", CCE_ASCII },
		{ "^SPIT$", CCE_SPIT },
		{ "^BASE$", CCE_BASE },
		{ "^MAXNONPRINT$", CCE_MAXNONPRINT },
		{ "^MAXPRINT$", CCE_MAXPRINT },
		{ "^DFC$", CCE_DFC },
		{ "^TABS$", CCE_TABS },
		{ "^TABWIDTH$", CCE_TABWIDTH },
		{ "^ASCIIBYTES$", CCE_ASCIIBYTES },
		{ "^DATAOPBYTES$", CCE_DATAOPBYTES },
		{ "^EXITFUNCTION$", CCE_EXITFUNCTION },
		{ "^MEMMAP$", CCE_MEMMAP },
		{ "^COMMENT$", CCE_COMMENT },
		{ "^MCU|CPU$", CCE_MCU },
		{ "^DATABLOCK$", CCE_DATABLOCK },
		{ "^ASSEMBLER$", CCE_ASSEMBLER },
	};

	static const TKeywordMap g_mapParseTrueFalse = {
		{ "^OFF|FALSE|NO$", 0 },
		{ "^ON|TRUE|YES$", 1 },
	};

	static const TKeywordMap g_mapParseBase = {
		{ "^OFF|NONE|0$", 0 },
		{ "^BIN|BINARY|2$", 2 },
		{ "^OCT|OCTAL|8$", 8 },
		{ "^DEC|DECIMAL|10$", 10 },
		{ "^HEX|HEXADECIMAL|16$", 16 },
	};

	static const TKeywordMap g_mapParseRefType = {
		{ "^CODE$", CDisassembler::RT_CODE },
		{ "^DATA$", CDisassembler::RT_DATA },
	};

	static const TKeywordMap g_mapParseMemType = {
		{ "^ROM$", CDisassembler::MT_ROM },
		{ "^RAM$", CDisassembler::MT_RAM },
		{ "^IO$", CDisassembler::MT_IO },
		{ "^EE$", CDisassembler::MT_EE },
	};

	static const TKeywordMap g_mapParseOutputType = {
		{ "^DISASSEMBLY|DISASSEMBLE|DISASSEM|DISASM|DIS|DASM$", OTE_DISASSEMBLY },
		{ "^FUNCTION|FUNCTIONS|FUNC|FUNCS|FUNCT|FUNCTS$", OTE_FUNCTIONS },
	};

	static const TKeywordMap g_mapParseCommentType = {
		{ "^EQUATE$", CDisassembler::CTF_EQUATE },
		{ "^DATA$", CDisassembler::CTF_DATA },
		{ "^CODE$", CDisassembler::CTF_CODE },
		{ "^REF$", CDisassembler::CTF_REF },
	};

	// --------------------------------

	static const std::string g_arrstrMemRanges[CDisassembler::NUM_MEMORY_TYPES] = {		// Both Human readable names for ranges for printing AND names used in Functions file!
		"ROM", "RAM", "IO", "EE",
	};
};

// ----------------------------------------------------------------------------

CDisassembler::CDisassembler()
	:	m_bDeterministic(false),
		m_bAddrFlag(false),				// Set output defaults
		m_bOpcodeFlag(false),
		m_bAsciiFlag(false),
		m_bSpitFlag(false),
		m_bTabsFlag(true),
		m_bAsciiBytesFlag(true),
		m_bDataOpBytesFlag(false),
		// ----
		m_nMaxNonPrint(8),
		m_nMaxPrint(40),
		m_nTabWidth(4),
		// ----
		m_nBase(16),					// See ReadControlFile function before changing these here!
		m_nDefaultBase(16),
		m_sDFC("binary"),
		m_sDefaultDFC("binary"),
		m_nLoadAddress(0),
		m_bAllowMemRangeOverlap(false),
		m_bVBreakEquateLabels(false),
		m_bVBreakCodeLabels(false),
		m_bVBreakDataLabels(false),
		m_StartTime(time(nullptr)),
		m_nCtrlLine(0),
		m_nFilesLoaded(0),
		m_PC(0),
		m_LAdrDplyCnt(0)
{
}

CDisassembler::~CDisassembler()
{
}

// ----------------------------------------------------------------------------

unsigned int CDisassembler::GetVersionNumber() const
{
	return (VERSION << 16);
}

// ----------------------------------------------------------------------------

CStringArray CDisassembler::GetMCUList() const
{
	return CStringArray();
}

bool CDisassembler::SetMCU(const std::string &strMCUName)
{
	UNUSED(strMCUName);
	return false;
}

// ----------------------------------------------------------------------------

CStringArray CDisassembler::GetTargetAssemblerList() const
{
	return CStringArray();
}

bool CDisassembler::SetTargetAssembler(const std:: string &strTargetAssembler)
{
	UNUSED(strTargetAssembler);
	return false;
}

// ----------------------------------------------------------------------------

static std::string::size_type getLongestMemMapName()
{
	std::string::size_type nMax = 0;
	for (int nMemType = 0; nMemType < CDisassembler::NUM_MEMORY_TYPES; ++nMemType) {
		if (g_arrstrMemRanges[nMemType].size() > nMax) nMax = g_arrstrMemRanges[nMemType].size();
	}
	return nMax;
}

static void parseArgLine(const std::string aLine, CStringArray &args)
{
	static const std::string strDelim = "\x009\x00a\x00b\x00c\x00d\x020";
	static const std::string strWhitespace = "\x009\x00a\x00b\x00c\x00d\x020";
	std::string strLine = aLine;
	trim(strLine);

	while (!strLine.empty()) {
		std::string strString;

		bool bQuoted = false;
		bool bQuoteLiteral = false;
		bool bNonWhitespaceSeen = false;
		bool bDropField = false;

		std::string::value_type ch;

		while (!strLine.empty()) {
			ch = strLine.at(0);
			strLine = strLine.substr(1);

			if (!bNonWhitespaceSeen) {		// Quoting can only begin at the first character
				if (strWhitespace.find(ch) != std::string::npos) {
					continue; // ignore all leading whitespace
				} else {
					bNonWhitespaceSeen = true;
				}
				if (ch == '"') {
					strString.clear();		// not a null field anymore, but still an empty string
					bQuoted = true;
					continue;
				}
			}

			if (ch == '"') {
				if (!bQuoted) {
					// no special treatment in this case
				} else if (bQuoteLiteral) {
					bQuoteLiteral = false;
				} else {
					bQuoteLiteral = true;
					continue;
				}
			} else if (bQuoteLiteral) {		// Un-doubled " in quoted string, that's the end of the quoted portion
				bQuoted = false;
				bQuoteLiteral = true;
			}

			if ((!bQuoted) && (strDelim.find(ch) != std::string::npos)) {
				break;	// Hitting the delimiter ends the field
			} else if ((!bQuoted) && (ch == ';')) {
				// Unquoted comment marker ends the field and drops remainder of line
				strLine.clear();
				if (strString.empty()) bDropField = true;	// Drop the field if this comment was it
				break;
			} else {
				strString.push_back(ch);
			}
		}

		if (!bDropField) args.push_back(strString);
	}
}

bool CDisassembler::ReadControlFile(ifstreamControlFile& inFile, bool bLastFile, std::ostream *msgFile, std::ostream *errFile, int nStartLineCount)
{
	bool bRetVal = true;

	std::string aLine;
	CStringArray args;

	// Note: m_nFilesLoaded must be properly set/reset prior to calling this function
	//		This is typically done in the constructor -- but overrides, etc, doing
	//		preloading of files should properly set it as well.

	//	bLastFile causes this function to report the overall disassembly summary from
	//		control files -- such as "at least one input file" and "output file must
	//		be specified" type error messages and sets any default action that should
	//		be taken with entries, etc.  The reason for the boolean flag is to
	//		allow processing of several control files prior to reporting the summary.
	//		Typically, the last control file processed should be called with a "true"
	//		and all prior control files processed should be called with a "false".

	m_sControlFileList.push_back(std::filesystem::relative(inFile.getFilename()));

	if (msgFile) {
		(*msgFile) << "Reading and Parsing Control File: " << std::filesystem::relative(inFile.getFilename()) << "...\n";
	}

	m_nBase = m_nDefaultBase;			// Reset the default base before starting new file.. Therefore, each file has same default!
	m_sDFC = m_sDefaultDFC;				// Reset the default DFC format so every control file starts with same default!
	m_sInputFilename.clear();			// Reset the Input Filename so we won't try to reload file from previous control file if this control doesn't specify one

	m_nCtrlLine = nStartLineCount;		// In case several lines were read by outside process, it should pass in correct starting number so we display correct error messages!
	while (inFile.good() && !inFile.eof()) {
		std::getline(inFile, aLine);
		++m_nCtrlLine;
		m_ParseError = "*** Error: Unknown error";
		trim(aLine);
		if (aLine.empty()) continue;	// If it is a blank or null line, keep going
		args.clear();
		parseArgLine(aLine, args);
		if (args.empty()) continue;		// If we don't have any args, get next line (i.e. empty line or comment line)

		if (ParseControlLine(aLine, args, msgFile, errFile) == false) {		// Go parse it -- either internal or overrides
			if (errFile) {
				(*errFile) << m_ParseError;
				(*errFile) << " in Control File " << std::filesystem::relative(inFile.getFilename()) << " line " << m_nCtrlLine << "\n";
			}
		}
	}

	// If a Input File was specified by the control file, then open it and read it here:
	if (!m_sInputFilename.empty()) {
		ReadSourceFile(m_sInputFilename, m_nLoadAddress, m_sDFC, msgFile, errFile);
	}

	// Print report on last file:
	if (bLastFile) {
		if (m_nFilesLoaded == 0) {
			if (errFile) (*errFile) << "*** Error: At least one input file must be specified in the control file(s) and successfully loaded\n";
			bRetVal = false;
		}
		if (m_sOutputFilename.empty()) {
			if (errFile) (*errFile) << "*** Error: Disassembly Output file must be specified in the control file(s)\n";
			bRetVal = false;
		}
		if ((m_bSpitFlag == false) && (m_EntryTable.size() == 0) && (m_CodeIndirectTable.size() == 0)) {
			if (errFile) (*errFile) << "*** Error: No entry addresses or indirect code vectors have been specified in the control file(s)\n";
			bRetVal = false;
		}

		if (bRetVal) {
			if (msgFile) {
				(*msgFile) << "\n";
				(*msgFile) << "        " << m_sInputFileList.size() << " Source File"
										<< ((m_sInputFileList.size() != 1) ? "s" : "")
										<< ((m_sInputFileList.size() != 0) ? ":" : "");
				if (m_sInputFileList.size() == 1) {
					(*msgFile) << " " << m_sInputFileList.at(0) << "\n";
				} else {
					for (unsigned int i=0; i<m_sInputFileList.size(); ++i) {
						(*msgFile) << "                " << m_sInputFileList.at(i) << "\n";
					}
				}
				(*msgFile) << "\n";

				if (!m_sOutputFilename.empty()) {
					(*msgFile) << "        Disassembly Output File: " << m_sOutputFilename << "\n\n";
				}

				if (!m_sFunctionsFilename.empty()) {
					(*msgFile) << "        Functions Output File: " << m_sFunctionsFilename << "\n\n";
				}

				(*msgFile) << "        Memory Mappings:\n";

				for (int nMemType = 0; nMemType < NUM_MEMORY_TYPES; ++nMemType) {
					(*msgFile) << "            ";
					for (std::string::size_type i = (getLongestMemMapName() - g_arrstrMemRanges[nMemType].size()); i; --i) {
						(*msgFile) << " ";
					}
					(*msgFile) << g_arrstrMemRanges[nMemType] << " Memory Map:";

					if (m_MemoryRanges[nMemType].isNullRange()) {
						(*msgFile) << " <Not Defined>\n";
					} else {
						(*msgFile) << "\n";
						for (auto const & itr : m_MemoryRanges[nMemType]) {
							(*msgFile) << "                "
										<< GetHexDelim() << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << itr.startAddr()
										<< std::nouppercase << std::setbase(0) << " - "
										<< GetHexDelim() << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << (itr.startAddr() + itr.size() - 1)
										<< std::nouppercase << std::setbase(0) << "  (Size: "
										<< GetHexDelim() << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << itr.size()
										<< std::nouppercase << std::setbase(0) << ")\n";
						}
					}
				}

				(*msgFile) << "\n";
				(*msgFile) << "        " << m_EntryTable.size() << " Entry Point"
											<< ((m_EntryTable.size() != 1) ? "s" : "")
											<< ((m_EntryTable.size() != 0) ? ":" : "")
											<< "\n";
			}
			for (auto const & itr : m_EntryTable) {
				if (msgFile) {
					(*msgFile) << "                "
								<< GetHexDelim() << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << itr
								<< std::nouppercase << std::setbase(0) << "\n";
				}
				if (!IsAddressLoaded(MT_ROM, itr, 1)) {
					if (errFile) {
						(*errFile) << "    *** Warning: Entry Point Address "
									<< GetHexDelim() << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << itr
									<< std::nouppercase << std::setbase(0) << " is outside of loaded source file(s)...\n";
					}
				}
			}

			if (msgFile) {
				(*msgFile) << "\n";
				(*msgFile) << "        " << m_FuncExitAddresses.size()
							<< " Exit Function"
							<< ((m_FuncExitAddresses.size() != 1) ? "s" : "")
							<< " Defined"
							<< ((m_FuncExitAddresses.size() != 0) ? ":" : "")
							<< "\n";
			}
			for (auto const & itr : m_FuncExitAddresses) {
				if (msgFile) {
					(*msgFile) << "                "
								<< GetHexDelim() << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << itr
								<< std::nouppercase << std::setbase(0) << "\n";
				}
				if (!IsAddressLoaded(MT_ROM, itr, 1)) {
					if (errFile) {
						(*errFile) << "    *** Warning: Exit Function Address "
									<< GetHexDelim() << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << itr
									<< std::nouppercase << std::setbase(0) << " is outside of loaded source file(s)...\n";
					}
				}
			}

			if (msgFile) {
				for (int nMemType = 0; nMemType < NUM_MEMORY_TYPES; ++nMemType) {
					(*msgFile) << "\n";
					(*msgFile) << "        " << m_LabelTable[nMemType].size()
								<< " Unique " << g_arrstrMemRanges[nMemType] << " Label"
								<< ((m_LabelTable[nMemType].size() != 1) ? "s" : "")
								<< " Defined"
								<< ((m_LabelTable[nMemType].size() != 0) ? ":" : "")
								<< "\n";
					for (auto const & itrLabels : m_LabelTable[nMemType]) {
						(*msgFile) << "                "
									<< GetHexDelim() << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << itrLabels.first
									<< std::nouppercase << std::setbase(0) << "=";
						bool bFirst = true;
						for (auto const & itrNames : itrLabels.second) {
							if (!bFirst) (*msgFile) << ",";
							bFirst = false;
							(*msgFile) << itrNames;
						}
						(*msgFile) << "\n";
					}
				}

				(*msgFile) << "\n";

				if (m_bAddrFlag) (*msgFile) << "Writing program counter addresses to disassembly file.\n";
				if (m_bOpcodeFlag) (*msgFile) << "Writing opcode byte values to disassembly file.\n";
				if (m_bAsciiFlag) (*msgFile) << "Writing printable data as ASCII in disassembly file.\n";
				if (m_bSpitFlag) (*msgFile) << "Performing a code-dump (spit) disassembly instead of code-seeking.\n";
				if (m_bTabsFlag) {
					(*msgFile) << "Using tab characters in disassembly file.  Tab width set at: " << m_nTabWidth << "\n";
				}
				if (m_bAsciiBytesFlag) (*msgFile) << "Writing byte value comments for ASCII data in disassembly file.\n";
				if ((m_bOpcodeFlag) && (m_bDataOpBytesFlag)) (*msgFile) << "Writing OpBytes for data in disassembly file\n";
				(*msgFile) << "\n";
			}


			// Ok, now we resolve the indirect tables...  This process should create the indirect
			//		label name from the resolved address if the corresponding string in the hash table is "".
			//		This process requires the purely virtual ResolveIndirect function from overrides...
			if (msgFile) (*msgFile) << "Compiling Indirect Code (branch) Table as specified in Control File...\n";
			if (msgFile) {
				(*msgFile) << "        " << m_CodeIndirectTable.size() << " Indirect Code Vector"
							<< ((m_CodeIndirectTable.size() != 1) ? "s" : "")
							<< ((m_CodeIndirectTable.size() != 0) ? ":" : "")
							<< "\n";
			}
			for (auto const & itrIndirects : m_CodeIndirectTable) {
				TAddress nResolvedAddress;

				if (msgFile) {
					(*msgFile) << "                ["
								<< GetHexDelim() << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << itrIndirects.first
								<< std::nouppercase << std::setbase(0) << "] -> ";
				}
				if (!ResolveIndirect(MT_ROM, itrIndirects.first, nResolvedAddress, RT_CODE)) {
					if (msgFile) (*msgFile) << "ERROR\n";
					if (errFile) {
						(*errFile) << "    *** Warning: Vector Address "
									<< GetHexDelim() << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << itrIndirects.first
									<< std::nouppercase << std::setbase(0) << " is outside of loaded source file(s)...\n";
						(*errFile) << "                    Or the vector location conflicted with other analyzed areas.\n";
					}
				} else {
					if (msgFile) {
						(*msgFile) << GetHexDelim() << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << nResolvedAddress
									<< std::nouppercase << std::setbase(0);
					}
					AddLabel(MT_ROM, nResolvedAddress, false, 0, itrIndirects.second);	// Add label for resolved name.  If empty, add it so later we can resolve Lxxxx from it.
					m_FunctionEntryTable[nResolvedAddress] = FUNCF_INDIRECT;		// Resolved code indirects are also considered start-of functions
					if (!AddBranch(nResolvedAddress, true, itrIndirects.first)) {
						if (errFile) {
							(*errFile) << "    *** Warning: Indirect Address ["
										<< GetHexDelim() << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << itrIndirects.first
										<< std::nouppercase << std::setbase(0) << "] -> "
										<< GetHexDelim() << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << nResolvedAddress
										<< std::nouppercase << std::setbase(0) << " is outside of loaded source file(s)...\n";
						}
						if ((msgFile) && (msgFile != errFile)) {
							(*msgFile) << "\n";
						}
					} else {
						if (msgFile) {
							(*msgFile) << "\n";
						}
					}
				}
			}
			if (msgFile) (*msgFile) << "\n";
			// Now, repeat for Data indirects:
			if (msgFile) (*msgFile) << "Compiling Indirect Data Table as specified in Control File...\n";
			if (msgFile) {
				(*msgFile) << "        " << m_DataIndirectTable.size() << " Indirect Data Vector"
							<< ((m_DataIndirectTable.size() != 1) ? "s" : "")
							<< ((m_DataIndirectTable.size() != 0) ? ":" : "")
							<< "\n";
			}
			for (auto const & itrIndirects : m_DataIndirectTable) {
				TAddress nResolvedAddress;

				if (msgFile) {
					(*msgFile) << "                ["
								<< GetHexDelim() << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << itrIndirects.first
								<< std::nouppercase << std::setbase(0) << "] -> ";
				}
				if (!ResolveIndirect(MT_ROM, itrIndirects.first, nResolvedAddress, RT_DATA)) {
					if (msgFile) (*msgFile) << "ERROR\n";
					if (errFile) {
						(*errFile) << "    *** Warning: Vector Address "
									<< GetHexDelim() << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << itrIndirects.first
									<< std::nouppercase << std::setbase(0) << " is outside of loaded source file(s)...\n";
						(*errFile) << "                    Or the vector location conflicted with other analyzed areas.\n";
					}
				} else {
					if (msgFile) {
						(*msgFile) << GetHexDelim() << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << nResolvedAddress
									<< std::nouppercase << std::setbase(0) << "\n";
					}
					AddLabel(MT_ROM, nResolvedAddress, true, itrIndirects.first, itrIndirects.second);		// Add label for resolved name.  If empty, add it so later we can resolve Lxxxx from it.
				}
			}
			if (msgFile) (*msgFile) << "\n";
		}
	}

	return bRetVal;
}

bool CDisassembler::ParseControlLine(const std::string & strLine, const CStringArray &argv, std::ostream *msgFile, std::ostream *errFile)
{
	UNUSED(strLine);

	bool bRetVal = true;
	std::string	strCmd;
	TAddress nAddress;
	TSize nSize;
	std::string strTemp;
	std::string strDfcLibrary;
	std::string strFileName;
	enum {
		ARGERR_NONE,
		ARGERR_Not_Enough_Args,
		ARGERR_Too_Many_Args,
		ARGERR_Illegal_Arg,
	} nArgError = ARGERR_NONE;

	auto &&fnSetBoolean = [&nArgError](const std::string &strArg, bool &bVar)->void {
		switch (parseKeyword(g_mapParseTrueFalse, makeUpperCopy(strArg))) {
			case 0:
				bVar = false;
				break;
			case 1:
				bVar = true;
				break;
			default:
				nArgError = ARGERR_Illegal_Arg;
				break;
		}
	};

	assert(argv.size() != 0);
	if (argv.size() == 0) return false;

	strCmd = argv.at(0);
	makeUpper(strCmd);

	switch (parseKeyword(g_mapParseCommands, strCmd)) {
		case CCE_ENTRY:			// ENTRY <addr> [<name>]
			if (argv.size() < 2) {
				nArgError = ARGERR_Not_Enough_Args;
				break;
			} else if (argv.size() > 3) {
				nArgError = ARGERR_Too_Many_Args;
				break;
			} else if ((argv.size() == 3) && !ValidateLabelName(argv.at(2))) {
				nArgError = ARGERR_Illegal_Arg;
				break;
			}
			nAddress = strtoul(argv.at(1).c_str(), nullptr, m_nBase);
			if (!m_MemoryRanges[MT_ROM].empty() && !m_MemoryRanges[MT_ROM].addressInRange(nAddress)) {
				bRetVal = false;
				m_ParseError = "*** Warning: Entry address outside of ROM memory range";
			} else if (HaveEntry(nAddress)) {
				bRetVal = false;
				m_ParseError = "*** Warning: Duplicate entry address";
			}
			AddEntry(nAddress);
			if (argv.size() == 3) {
				if (!AddLabel(MT_ROM, nAddress, false, 0, argv.at(2))) {
					bRetVal = false;
					m_ParseError = "*** Warning: Duplicate label";
				}
			}
			break;
		case CCE_LOAD:			// LOAD <addr> | <addr> <filename> [<library>]
		{
			if (argv.size() == 2) {
				m_nLoadAddress = strtoul(argv.at(1).c_str(), nullptr, m_nBase);
				break;
			} else if (argv.size() < 3) {
				nArgError = ARGERR_Not_Enough_Args;
				break;
			} else if (argv.size() > 4) {
				nArgError = ARGERR_Too_Many_Args;
				break;
			}
			nAddress = strtoul(argv.at(1).c_str(), nullptr, m_nBase);
			if (argv.size() == 4) {
				strDfcLibrary = argv.at(3);
			} else {
				strDfcLibrary = m_sDFC;
			}
			strFileName = argv.at(2);

			ReadSourceFile(strFileName, nAddress, strDfcLibrary, msgFile, errFile);
			// We don't set RetVal to FALSE here if there is an error on ReadSource,
			//	because errors have already been printed and are not in ParseError...
			break;
		}
		case CCE_INPUT:			// INPUT <filename>
			if (argv.size() != 2) {
				nArgError = (argv.size() < 2) ? ARGERR_Not_Enough_Args : ARGERR_Too_Many_Args;
				break;
			}
			if (!m_sInputFilename.empty()) {
				bRetVal = false;
				m_ParseError = "*** Warning: Input filename already defined";
			}
			m_sInputFilename = argv.at(1);
			break;
		case CCE_OUTPUT:		// OUTPUT [DISASSEMBLY | FUNCTIONS] <filename>
			if ((argv.size() != 2) && (argv.size() != 3)) {
				nArgError = (argv.size() < 2) ? ARGERR_Not_Enough_Args : ARGERR_Too_Many_Args;
				break;
			}
			switch ((argv.size() >= 3) ? parseKeyword(g_mapParseOutputType, makeUpperCopy(argv.at(1))) : OTE_DISASSEMBLY) {	// Assume DISASSEMBLY if no keyword specified
				case OTE_DISASSEMBLY:		// DISASSEMBLY
					if (!m_sOutputFilename.empty()) {
						bRetVal = false;
						m_ParseError = "*** Warning: Disassembly Output filename already defined";
					}
					m_sOutputFilename = argv.at((argv.size() >= 3) ? 2 : 1);
					break;
				case OTE_FUNCTIONS:			// FUNCTIONS
					if (!m_sFunctionsFilename.empty()) {
						bRetVal = false;
						m_ParseError = "*** Warning: Functions Output filename already defined";
					}
					m_sFunctionsFilename = argv.at((argv.size() >= 3) ? 2 : 1);
					break;
				default:
					nArgError = ARGERR_Illegal_Arg;
					break;
			}
			break;
		case CCE_LABEL:			// LABEL [ROM | RAM | IO] <addr> <name> [<comment>]      (note: use double quotes for whitespace in <comment>)
		{
			if ((argv.size() < 3) && (argv.size() > 5)) {
				nArgError = (argv.size() < 3) ? ARGERR_Not_Enough_Args : ARGERR_Too_Many_Args;
				break;
			}

			MEMORY_TYPE nType = MT_ROM;		// Assume ROM if not specified
			bool bHaveType = false;

			if (argv.size() > 3) {
				int nParsedType = parseKeyword(g_mapParseMemType, makeUpperCopy(argv.at(1)));
				if ((nParsedType == -1) && (argv.size() == 5)) {
					// If all args were specified, error out if type not valid
					nArgError = ARGERR_Illegal_Arg;
					break;
				} else {
					nType = static_cast<MEMORY_TYPE>(nParsedType);
					bHaveType = true;
				}
			}

			if (!ValidateLabelName(argv.at(bHaveType ? 3 : 2))) {
				nArgError = ARGERR_Illegal_Arg;
				break;
			}

			nAddress = strtoul(argv.at(bHaveType ? 2 : 1).c_str(), nullptr, m_nBase);
			if (!m_bAllowMemRangeOverlap && !bHaveType) {
				nType = MemoryTypeFromAddress(nAddress);
			}
			if (!IsAddressInRange(nType, nAddress)) {
				bRetVal = false;
				m_ParseError = "*** Warning: Specified Address is outside of specified Memory Range (" + g_arrstrMemRanges[nType] + ")";
			}
			if (!AddLabel(nType, nAddress, false, 0, argv.at(bHaveType ? 3 : 2))) {
				bRetVal = false;
				m_ParseError = "*** Warning: Duplicate label";
			}
			if ((argv.size() > (bHaveType ? 4 : 3)) && !AddComment(nType, nAddress, CComment(CTF_CODE, argv.at(bHaveType ? 4 : 3)))) {
				bRetVal = false;
				m_ParseError = "*** Warning: Failed to add <comment> field";
			}
			break;
		}
		case CCE_ADDRESSES:		// ADDRESSES [ON | OFF | TRUE | FALSE | YES | NO]
			if (argv.size() < 2) {
				m_bAddrFlag = true;		// Default is ON
				break;
			} else if (argv.size() > 2) {
				nArgError = ARGERR_Too_Many_Args;
				break;
			}
			fnSetBoolean(argv.at(1), m_bAddrFlag);
			break;
		case CCE_INDIRECT:		// INDIRECT [CODE | DATA] <addr> [<name>]
		{
			if ((argv.size() < 2) || (argv.size() > 4)) {
				nArgError = (argv.size() < 2) ? ARGERR_Not_Enough_Args : ARGERR_Too_Many_Args;
				break;
			}

			REFERENCE_TYPE nType = RT_CODE;		// Assume code if not specified
			int nParsedType = ((argv.size() >= 3) ? parseKeyword(g_mapParseRefType, makeUpperCopy(argv.at(1))) : -1);
			std::string strName;

			if (nParsedType == -1) {
				if (argv.size() == 4) {
					// If all args were specified, error out if type not valid
					nArgError = ARGERR_Illegal_Arg;
					break;
				} else {
					// if less than 4 args and didn't have a type, we have a name, but only if we have 3 args: (otherwise it's just command and address)
					if (argv.size() == 3) {
						strName = argv.at(2);
					}
					nAddress = strtoul(argv.at(1).c_str(), nullptr, m_nBase);
				}
			} else {
				nType = static_cast<REFERENCE_TYPE>(nParsedType);		// We did have a type
				if (argv.size() == 4) {
					strName = argv.at(3);		// If we had all args, last is a name, else we had no name
				}
				nAddress = strtoul(argv.at(2).c_str(), nullptr, m_nBase);
			}

			if (!strName.empty()) {
				if (!ValidateLabelName(strName)) {
					nArgError = ARGERR_Illegal_Arg;
					break;
				}
			}	// Note:  If we have no name, we'll set the name to "" so the Resolution function will create it from the resolved address...

			switch (nType) {
				case RT_CODE:
					if (HaveCodeIndirect(nAddress) || HaveDataIndirect(nAddress)) {
						bRetVal = false;
						m_ParseError = "*** Warning: Duplicate indirect";
					}
					AddCodeIndirect(nAddress, strName);			// Add a label to the code indirect table
					break;
				case RT_DATA:
					if (HaveDataIndirect(nAddress) || HaveCodeIndirect(nAddress)) {
						bRetVal = false;
						m_ParseError = "*** Warning: Duplicate indirect";
					}
					AddDataIndirect(nAddress, strName);			// Add a label to the data indirect table
					break;
				default:
					assert(false);
					nArgError = ARGERR_Illegal_Arg;
					break;
			}
			break;
		}
		case CCE_OPBYTES:		// OPCODES [ON | OFF | TRUE | FALSE | YES | NO]
								// OPBYTES [ON | OFF | TRUE | FALSE | YES | NO]
			if (argv.size() < 2) {
				m_bOpcodeFlag = true;		// Default is ON
				break;
			} else if (argv.size() > 2) {
				nArgError = ARGERR_Too_Many_Args;
				break;
			}
			fnSetBoolean(argv.at(1), m_bOpcodeFlag);
			break;
		case CCE_ASCII:			// ASCII [ON | OFF | TRUE | FALSE | YES | NO]
			if (argv.size() < 2) {
				m_bAsciiFlag = true;		// Default is ON
				break;
			} else if (argv.size() > 2) {
				nArgError = ARGERR_Too_Many_Args;
				break;
			}
			fnSetBoolean(argv.at(1), m_bAsciiFlag);
			break;
		case CCE_SPIT:			// SPIT [ON | OFF | TRUE | FALSE | YES | NO]
			if (argv.size() < 2) {
				m_bSpitFlag = true;		// Default is ON
				break;
			} else if (argv.size() > 2) {
				nArgError = ARGERR_Too_Many_Args;
				break;
			}
			fnSetBoolean(argv.at(1), m_bSpitFlag);
			break;
		case CCE_BASE:			// BASE [OFF | BIN | OCT | DEC | HEX]
		{
			if (argv.size() < 2) {
				m_nBase = 0;		// Default is OFF
				break;
			} else if (argv.size() > 2) {
				nArgError = ARGERR_Too_Many_Args;
				break;
			}
			int nBase = parseKeyword(g_mapParseBase, makeUpperCopy(argv.at(1)));
			if (nBase == -1) {
				nArgError = ARGERR_Illegal_Arg;
			} else {
				m_nBase = nBase;
			}
			break;
		}
		case CCE_MAXNONPRINT:	// MAXNONPRINT <value>
			if (argv.size() != 2) {
				nArgError = (argv.size() < 2) ? ARGERR_Not_Enough_Args : ARGERR_Too_Many_Args;
				break;
			}
			m_nMaxNonPrint = strtoul(argv.at(1).c_str(), nullptr, m_nBase);
			break;
		case CCE_MAXPRINT:		// MAXPRINT <value>
			if (argv.size() != 2) {
				nArgError = (argv.size() < 2) ? ARGERR_Not_Enough_Args : ARGERR_Too_Many_Args;
				break;
			}
			m_nMaxPrint = strtoul(argv.at(1).c_str(), nullptr, m_nBase);
			break;
		case CCE_DFC:			// DFC <library>
			if (argv.size() != 2) {
				nArgError = (argv.size() < 2) ? ARGERR_Not_Enough_Args : ARGERR_Too_Many_Args;
				break;
			}
			m_sDFC = argv.at(1);
			break;
		case CCE_TABS:			// TABS [ON | OFF | TRUE | FALSE | YES | NO]
			if (argv.size() < 2) {
				m_bTabsFlag = true;		// Default is ON
				break;
			} else if (argv.size() > 2) {
				nArgError = ARGERR_Too_Many_Args;
				break;
			}
			fnSetBoolean(argv.at(1), m_bTabsFlag);
			break;
		case CCE_TABWIDTH:		// TABWIDTH <value>
			if (argv.size() != 2) {
				nArgError = (argv.size() < 2) ? ARGERR_Not_Enough_Args : ARGERR_Too_Many_Args;
				break;
			}
			m_nTabWidth = strtoul(argv.at(1).c_str(), nullptr, m_nBase);
			break;
		case CCE_ASCIIBYTES:	// ASCIIBYTES [ON | OFF | TRUE | FALSE | YES | NO]
			if (argv.size() < 2) {
				m_bAsciiBytesFlag = true;		// Default is ON
				break;
			} else if (argv.size() > 2) {
				nArgError = ARGERR_Too_Many_Args;
				break;
			}
			fnSetBoolean(argv.at(1), m_bAsciiBytesFlag);
			break;
		case CCE_DATAOPBYTES:	// DATAOPBYTES [ON | OFF | TRUE | FALSE | YES | NO]
			if (argv.size() < 2) {
				m_bDataOpBytesFlag = true;		// Default is ON
				break;
			} else if (argv.size() > 2) {
				nArgError = ARGERR_Too_Many_Args;
				break;
			}
			fnSetBoolean(argv.at(1), m_bDataOpBytesFlag);
			break;
		case CCE_EXITFUNCTION:	// EXITFUNCTION <addr> [<name>]
			if (argv.size() < 2) {
				nArgError = ARGERR_Not_Enough_Args;
				break;
			} else if (argv.size() > 3) {
				nArgError = ARGERR_Too_Many_Args;
				break;
			} else if (argv.size() == 3) {
				if (!ValidateLabelName(argv.at(2))) {
					nArgError = ARGERR_Illegal_Arg;
					break;
				}
			}
			nAddress = strtoul(argv.at(1).c_str(), nullptr, m_nBase);
			if (m_FuncExitAddresses.contains(nAddress)) {
				bRetVal = false;
				m_ParseError = "*** Warning: Duplicate function exit address";
			}
			m_FunctionEntryTable[nAddress] = FUNCF_ENTRY;	// Exit Function Entry points are also considered start-of functions
			m_FuncExitAddresses.insert(nAddress);		// Add function exit entry
			if (argv.size() == 3) {
				if (!AddLabel(MT_ROM, nAddress, false, 0, argv.at(2))) {
					bRetVal = false;
					m_ParseError = "*** Warning: Duplicate label";
				}
			}
			break;
		case CCE_MEMMAP:		// MEMMAP [ROM | RAM | IO] <addr> <size>
		{
			if ((argv.size() != 3) && (argv.size() != 4)) {
				nArgError = (argv.size() < 3) ? ARGERR_Not_Enough_Args : ARGERR_Too_Many_Args;
				break;
			}

			MEMORY_TYPE nType = MT_ROM;		// Assume ROM if not specified

			if (argv.size() == 4) {
				int nParsedType = parseKeyword(g_mapParseMemType, makeUpperCopy(argv.at(1)));
				if (nParsedType == -1) {
					// If all args were specified, error out if type not valid
					nArgError = ARGERR_Illegal_Arg;
					break;
				}
				nType = static_cast<MEMORY_TYPE>(nParsedType);
				nAddress = strtoul(argv.at(2).c_str(), nullptr, m_nBase);
				nSize = strtoul(argv.at(3).c_str(), nullptr, m_nBase);
			} else {
				nAddress = strtoul(argv.at(1).c_str(), nullptr, m_nBase);
				nSize = strtoul(argv.at(2).c_str(), nullptr, m_nBase);
			}

			m_MemoryRanges[nType].push_back(CMemRange(nAddress, nSize));
			m_MemoryRanges[nType].compact();
			m_MemoryRanges[nType].removeOverlaps();
			m_MemoryRanges[nType].sort();

			if (!m_bAllowMemRangeOverlap) {
				for (int nMemType = 0; nMemType < NUM_MEMORY_TYPES; ++nMemType) {
					if (nType == nMemType) continue;
					if (m_MemoryRanges[nMemType].rangesOverlap(m_MemoryRanges[nType].back())) {
						bRetVal = false;
						m_ParseError = "*** Warning: Specified " + g_arrstrMemRanges[nType] +
										" Mapping conflicts with " + g_arrstrMemRanges[nMemType] + " Mapping";
						break;
					}
				}
			}
			break;
		}
		case CCE_COMMENT:		// COMMENT [ROM | RAM | IO] <address> [<Type> ...] <comment>
		{						//	Where <Type> is (EQUATE | DATA | CODE | REF)
								//		Can be specified multiple times and are OR'd together
								//		If omitted, "ALL" is assumed
			if (argv.size() < 3) {
				nArgError = ARGERR_Not_Enough_Args;
				break;
			}

			MEMORY_TYPE nType = MT_ROM;		// Assume ROM if not specified
			bool bHaveType = false;

			if (argv.size() > 3) {
				int nParsedType = parseKeyword(g_mapParseMemType, makeUpperCopy(argv.at(1)));
				if (nParsedType != -1) {
					nType = static_cast<MEMORY_TYPE>(nParsedType);
					bHaveType = true;
				}
			}

			COMMENT_TYPE_FLAGS ctfFlags = ((argv.size() == (bHaveType ? 4 : 3)) ? CTF_ALL : CTF_NONE);
			if (argv.size() > (bHaveType ? 4 : 3)) {
				bool bInvalidType = false;
				for (CStringArray::size_type ndx = (bHaveType ? 3 : 2); ndx < (argv.size()-1); ++ndx) {
					int nParsedType = parseKeyword(g_mapParseCommentType, makeUpperCopy(argv.at(ndx)));
					if (nParsedType == -1) {
						bInvalidType = true;
						break;
					}
					ctfFlags |= static_cast<COMMENT_TYPE_FLAGS>(nParsedType);
				}
				if (bInvalidType) {
					nArgError = ARGERR_Illegal_Arg;
					break;
				}
			}
			nAddress = strtoul(argv.at(bHaveType ? 2 : 1).c_str(), nullptr, m_nBase);
			if (!m_bAllowMemRangeOverlap && !bHaveType) {
				nType = MemoryTypeFromAddress(nAddress);
			}
			if (!IsAddressInRange(nType, nAddress)) {
				bRetVal = false;
				m_ParseError = "*** Warning: Specified Address is outside of specified Memory Range (" + g_arrstrMemRanges[nType] + ")";
			}
			if (!AddComment(nType, nAddress, CComment(ctfFlags, argv.at(argv.size()-1)))) {
				bRetVal = false;
				m_ParseError = "*** Warning: Failed to add <comment> field";
			}
			break;
		}
		case CCE_MCU:			// MCU <mcu>
								// CPU <mcu>
								//	Where <mcu> is GDC specific MPU subtype, like "m328pb", for example
			if (argv.size() != 2) {
				nArgError = (argv.size() < 2) ? ARGERR_Not_Enough_Args : ARGERR_Too_Many_Args;
				break;
			}
			if (!contains(GetMCUList(), argv.at(1))) {
				bRetVal = false;
				m_ParseError = "*** Warning: Invalid MCU type \"" + argv.at(1) + "\" specified";
			} else if (!SetMCU(argv.at(1))) {
				bRetVal = false;
				m_ParseError = "*** Warning: Failed to set MCU type \"" + argv.at(1) + "\" specified";
			} else {
				if (msgFile) {
					(*msgFile) << "Selected MCU and predefined labels and entry points for: " << argv.at(1) << "\n";
				}
			}
			break;
		case CCE_DATABLOCK:		// DATABLOCK <addr> <size>  (implies: MT_ROM)
		{
			if (argv.size() != 3) {
				nArgError = (argv.size() < 3) ? ARGERR_Not_Enough_Args : ARGERR_Too_Many_Args;
				break;
			}

			nAddress = strtoul(argv.at(1).c_str(), nullptr, m_nBase);
			nSize = strtoul(argv.at(2).c_str(), nullptr, m_nBase);

			CMemRange range(nAddress, nSize);

			if (m_rngDataBlocks.rangesOverlap(range)) {
				bRetVal = false;
				m_ParseError = "*** Warning: Data Block Range overlaps existing Data Blocks Ranges.  They will be merged.";
			}
			m_rngDataBlocks.push_back(range);
			m_rngDataBlocks.compact();
			m_rngDataBlocks.removeOverlaps();
			m_rngDataBlocks.sort();
			break;
		}
		case CCE_ASSEMBLER:		// ASSEMBLER <assembler>
								//	Where <assembler> is GDC specific Target Assembler name
			if (argv.size() != 2) {
				nArgError = (argv.size() < 2) ? ARGERR_Not_Enough_Args : ARGERR_Too_Many_Args;
				break;
			}
			if (!contains(GetTargetAssemblerList(), argv.at(1))) {
				bRetVal = false;
				m_ParseError = "*** Warning: Invalid Target Assembler name \"" + argv.at(1) + "\" specified";
			} else if (!SetTargetAssembler(argv.at(1))) {
				bRetVal = false;
				m_ParseError = "*** Warning: Failed to set Target Assembler name \"" + argv.at(1) + "\"";
			} else {
				if (msgFile) {
					(*msgFile) << "Setting Target Assembler to: " << argv.at(1) << "\n";
				}
			}
			break;

		default:
			m_ParseError = "*** Error: Unknown command";
			return false;
	}

	if (nArgError) {
		bRetVal = false;
		switch (nArgError) {
			case ARGERR_Not_Enough_Args:
				m_ParseError = "*** Error: Not enough arguments for '" + argv.at(0) + "' command";
				break;
			case ARGERR_Too_Many_Args:
				m_ParseError = "*** Error: Too many arguments for '" + argv.at(0) + "' command";
				break;
			case ARGERR_Illegal_Arg:
				m_ParseError = "*** Error: Illegal argument for '" + argv.at(0) + "' command";
				break;
			default:
				assert(false);
				break;
		}
	}

	return bRetVal;
}

bool CDisassembler::ReadSourceFile(const std::string & strFilename, TAddress nLoadAddress, const std::string & strDFCLibrary, std::ostream *msgFile, std::ostream *errFile)
{
	if (strDFCLibrary.empty()) return false;
	if (strFilename.empty()) return false;

	const CDataFileConverter *pDFC = CDataFileConverters::instance()->locateDFC(strDFCLibrary);
	bool bRetVal = true;

	if (msgFile) {
		(*msgFile) << "Loading \"" << strFilename << "\" at offset "
					<< GetHexDelim() << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << nLoadAddress
					<< std::nouppercase << std::setbase(0) << " using " << strDFCLibrary << " library...\n";
	}
	if (pDFC == nullptr) {
		if (errFile) {
			(*errFile) << "*** Error: Can't open DFC Library \"" << strDFCLibrary << "\" to read \"" << strFilename << "\"\n";
		}
		bRetVal = false;
	} else {
		bool bStatus = true;
		++m_nFilesLoaded;
		m_sInputFileList.push_back(strFilename);

		try {
			if (!pDFC->RetrieveFileMapping(*this, strFilename, nLoadAddress, msgFile, errFile)) {
				if (errFile) {
					(*errFile) << "*** Unable to completely retrieve the file mapping for \"" << strFilename << "\"\n";
				}
			}

			bStatus = pDFC->ReadDataFile(*this, strFilename, nLoadAddress, DMEM_LOADED, msgFile, errFile);
		}
		catch (const EXCEPTION_ERROR &aErr) {
			bRetVal = false;
			--m_nFilesLoaded;
			m_sInputFileList.erase(m_sInputFileList.end()-1);
			if (errFile) {
				(*errFile) << "*** " << aErr.errorMessage();
				if (aErr.m_nData) {
					(*errFile) << " on Line " << aErr.m_nData;
				}
				if (!aErr.m_strDetail.empty()) {
					(*errFile) << " (" << aErr.m_strDetail << ")";
				}
				(*errFile) << ", reading file \"" << strFilename << "\"\n";
			}
		}
		if (!bStatus) {
			bRetVal = false;
			if (errFile) {
				(*errFile) << "*** Warning: Reading file \"" << strFilename << "\", overlaps previously loaded files\n";
			}
		}
	}

	return bRetVal;
}

// ----------------------------------------------------------------------------

bool CDisassembler::ScanEntries(std::ostream *msgFile, std::ostream *errFile)
{
	bool bRetVal = true;
	CAddressSet::size_type nCount = m_EntryTable.size();

	if (m_bSpitFlag) {
		m_PC = m_Memory[MT_ROM].lowestLogicalAddress();		// In spit mode, we start at first address and just spit
		return FindCode(msgFile, errFile);
	}

	CAddressSet::const_iterator itrEntry = m_EntryTable.cbegin();
	while (bRetVal && (itrEntry != m_EntryTable.cend())) {
		m_PC = *itrEntry;
		++itrEntry;

		bRetVal = bRetVal && FindCode(msgFile, errFile);

		if (nCount != m_EntryTable.size()) {
			nCount = m_EntryTable.size();
			itrEntry = m_EntryTable.cbegin();			// If one gets added during the find process, our iterator gets invalidated and we must start over...
		}
	}
	return bRetVal;
}

bool CDisassembler::ScanBranches(std::ostream *msgFile, std::ostream *errFile)
{
	bool bRetVal = true;
	CAddressTableMap::size_type nCount = m_BranchTable.size();

	if (m_bSpitFlag) return true;						// Don't do anything in spit mode cause we did it in ScanEntries!

	CAddressTableMap::const_iterator itrEntry = m_BranchTable.cbegin();
	while (bRetVal && (itrEntry != m_BranchTable.cend())) {
		m_PC = itrEntry->first;
		++itrEntry;

		bRetVal = bRetVal && FindCode(msgFile, errFile);

		if (nCount != m_BranchTable.size()) {
			nCount = m_BranchTable.size();
			itrEntry = m_BranchTable.cbegin();			// If one gets added during the find process, our iterator gets invalidated and we must start over...
		}
	}
	return bRetVal;
}

bool CDisassembler::ScanData(const std::string & strExcludeChars, std::ostream *msgFile, std::ostream *errFile)
{
	UNUSED(errFile);

	TMemoryElement c;

	// Note that we use an argument passed in so that the caller has a chance to change the
	//	list passed in by the GetExcludedPrintChars function!

	if (!m_bSpitFlag) {		// We've already done this in spit mode
		for (auto & itrDataBlock : m_rngDataBlocks) {
			m_PC = itrDataBlock.startAddr();
			for (TSize nSize = 0; nSize < itrDataBlock.size(); ++nSize, ++m_PC) {
				if (!m_Memory[MT_ROM].containsAddress(m_PC)) continue;

				if (m_Memory[MT_ROM].descriptor(m_PC) != DMEM_LOADED) {
					if (msgFile) {
						(*msgFile) << "*** Warning: Address "
									<< GetHexDelim() << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << m_PC
									<< std::nouppercase << std::setbase(0)
									<< " in declared Data Block Range "
									<< GetHexDelim() << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << itrDataBlock.startAddr()
									<< std::nouppercase << std::setbase(0)
									<< "-"
									<< GetHexDelim() << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << (itrDataBlock.startAddr()+itrDataBlock.size()-1)
									<< std::nouppercase << std::setbase(0)
									<< " conflicts with discovered code.\n";
					}
				} else {
					c = m_Memory[MT_ROM].element(m_PC);
					if (isprint(c) && (strExcludeChars.find(c) == std::string::npos)) {
						m_Memory[MT_ROM].setDescriptor(m_PC, DMEM_PRINTDATA);
					} else {
						m_Memory[MT_ROM].setDescriptor(m_PC, DMEM_DATA);
					}
				}
			}
		}
	}

	for (int nMemType = 0; nMemType < NUM_MEMORY_TYPES; ++nMemType) {
		if (m_Memory[nMemType].empty()) continue;

		for (auto & itrMemory : m_Memory[nMemType]) {
			m_PC = itrMemory.logicalAddr();
			for (TSize nSize = 0; nSize < itrMemory.size(); ++nSize, ++m_PC) {
				if (itrMemory.descriptor(m_PC) != DMEM_LOADED) continue;	// Only modify memory locations that have been loaded but not processed!
				c = itrMemory.element(m_PC);
				if (isprint(c) && (strExcludeChars.find(c) == std::string::npos)) {
					itrMemory.setDescriptor(m_PC, DMEM_PRINTDATA);
				} else {
					itrMemory.setDescriptor(m_PC, DMEM_DATA);
				}
			}
		}
	}
	return true;
}

bool CDisassembler::Disassemble(std::ostream *msgFile, std::ostream *errFile, std::ostream *outFile)
{
	bool bRetVal = true;
	std::ostream *theOutput;
	std::fstream aOutput;

	if (outFile) {
		theOutput = outFile;
	} else {
		aOutput.open(m_sOutputFilename.c_str(), std::ios_base::out | std::ios_base::trunc);
		if (!aOutput.is_open()) {
			if (errFile) {
				(*errFile) << "\n*** Error: Opening file \"" << m_sOutputFilename << "\" for writing...\n";
			}
			return false;
		}
		theOutput = &aOutput;
	}

	while (1) {		// Setup dummy endless loop so we can use 'break' instead of 'goto'
		if (msgFile) (*msgFile) << "\nPass 1 - Finding Code, Data, and Labels...\n";
		bRetVal = Pass1(*theOutput, msgFile, errFile);
		if (!bRetVal) break;

		if (msgFile) {
			(*msgFile) << "\n\nPass 2 - Disassembling to Output File...\n";
		}
		m_LAdrDplyCnt = 0;
		bRetVal = Pass2(*theOutput, msgFile, errFile);
		if (!bRetVal) break;

		if (!m_sFunctionsFilename.empty()) {
			if (*msgFile) {
				(*msgFile) << "\n\nPass 3 - Creating Functions Output File...\n";
			}
			m_LAdrDplyCnt = 0;
			bRetVal = Pass3(*theOutput, msgFile, errFile);
			if (!bRetVal) break;
		}

		if (msgFile) {
			(*msgFile) << "\nDisassembly Complete\n";
		}

		// Add additional Passes here and end this loop with a break
		break;
	}

	if (bRetVal == false) {
		(*theOutput) << "\n*** Internal error encountered while disassembling.\n";
		if (errFile) (*errFile) << "\n*** Internal error encountered while disassembling.\n";
	}

	if (aOutput.is_open()) aOutput.close();		// Close the output file if we opened it

	return bRetVal;
}

// ----------------------------------------------------------------------------

bool CDisassembler::Pass1(std::ostream& outFile, std::ostream *msgFile, std::ostream *errFile)
{
	UNUSED(outFile);

	bool bRetVal = true;

	const std::string strExcludeChars = GetExcludedPrintChars();

	// If in "spit" mode, first tag everything explicitly declared as a Data Block
	//	so that it doesn't get converted incorrectly to code:
	if (m_bSpitFlag) {
		for (auto & itrDataBlock : m_rngDataBlocks) {
			m_PC = itrDataBlock.startAddr();
			for (TSize nSize = 0; nSize < itrDataBlock.size(); ++nSize, ++m_PC) {
				if (!m_Memory[MT_ROM].containsAddress(m_PC)) continue;
				TMemoryElement c = m_Memory[MT_ROM].element(m_PC);
				if (isprint(c) && (strExcludeChars.find(c) == std::string::npos)) {
					m_Memory[MT_ROM].setDescriptor(m_PC, DMEM_PRINTDATA);
				} else {
					m_Memory[MT_ROM].setDescriptor(m_PC, DMEM_DATA);
				}

			}
		}
	}

	// Note that Short-Circuiting will keep following process stages from being called in the event of an error!
	bRetVal = bRetVal && ScanEntries(msgFile, errFile);
	bRetVal = bRetVal && ScanBranches(msgFile, errFile);
	bRetVal = bRetVal && ScanData(strExcludeChars, msgFile, errFile);

	return bRetVal;
}

bool CDisassembler::Pass2(std::ostream& outFile, std::ostream *msgFile, std::ostream *errFile)
{
	bool bRetVal = true;

	// Note that Short-Circuiting will keep following process stages from being called in the event of an error!
	bRetVal = bRetVal && WriteHeader(outFile, msgFile, errFile);
	bRetVal = bRetVal && WriteEquates(outFile, msgFile, errFile);

	MEMORY_TYPE arrMemTypes[NUM_MEMORY_TYPES] = { MT_IO, MT_RAM, MT_EE, MT_ROM };
	for (int ndxType = 0; ndxType < NUM_MEMORY_TYPES; ++ndxType) {
		MEMORY_TYPE nMemoryType = arrMemTypes[ndxType];
		bRetVal = bRetVal && WriteDisassembly(static_cast<MEMORY_TYPE>(nMemoryType), outFile, msgFile, errFile);
	}

	return bRetVal;
}

bool CDisassembler::Pass3(std::ostream& outFile, std::ostream *msgFile, std::ostream *errFile)
{
	UNUSED(outFile);

	const MEMORY_TYPE nMemType = MT_ROM;

	bool bRetVal = true;
	std::fstream aFunctionsFile;
	bool bTempFlag;
	bool bTempFlag2;

	aFunctionsFile.open(m_sFunctionsFilename.c_str(), std::ios_base::out | std::ios_base::trunc);
	if (!aFunctionsFile.is_open()) {
		if (errFile) {
			(*errFile) << "\n*** Error: Opening file \"" << m_sFunctionsFilename << "\" for writing...\n";
		}
		return false;
	}

	// Output File Header Comments:
	aFunctionsFile << ";\n";
	aFunctionsFile << "; GenDasm - Generic Code-Seeking Disassembler\n";
	aFunctionsFile << "; " << GetGDCLongName() << " Generated Function Information File\n";
	aFunctionsFile << ";\n";
	aFunctionsFile << ";    Control File" << ((m_sControlFileList.size() > 1) ? "s:" : ": ") << " ";
	for (unsigned int i=0; i<m_sControlFileList.size(); ++i) {
		if (i==0) {
			aFunctionsFile << m_sControlFileList.at(i) << "\n";
		} else {
			aFunctionsFile << ";                  " << m_sControlFileList.at(i) << "\n";
		}
	}
	aFunctionsFile << ";      Input File" << ((m_sInputFileList.size() > 1) ? "s:" : ": ") << " ";
	for (unsigned int i=0; i<m_sInputFileList.size(); ++i) {
		if (i==0) {
			aFunctionsFile << m_sInputFileList.at(i) << "\n";
		} else {
			aFunctionsFile << ";                  " << m_sInputFileList.at(i) << "\n";
		}
	}
	aFunctionsFile << ";     Output File:  " << m_sOutputFilename << "\n";
	aFunctionsFile << ";  Functions File:  " << m_sFunctionsFilename << "\n";
	aFunctionsFile << ";\n";

	aFunctionsFile << "; Memory Mappings:";

	for (int nMemType = 0; nMemType < NUM_MEMORY_TYPES; ++nMemType) {
		aFunctionsFile << ((nMemType == 0) ? "  " : ";                   ");
		for (std::string::size_type i = (getLongestMemMapName() - g_arrstrMemRanges[nMemType].size()); i; --i) {
			aFunctionsFile << " ";
		}
		aFunctionsFile << g_arrstrMemRanges[nMemType] << " Memory Map:";

		if (m_MemoryRanges[nMemType].isNullRange()) {
			aFunctionsFile << " <Not Defined>\n";
		} else {
			aFunctionsFile << "\n";
			for (auto const & itrMemRange : m_MemoryRanges[nMemType]) {
				if (itrMemRange.isNullRange()) continue;
				aFunctionsFile << ";                       "
								<< GetHexDelim() << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << itrMemRange.startAddr()
								<< std::nouppercase << std::setbase(0) << " - "
								<< GetHexDelim() << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << (itrMemRange.startAddr() + itrMemRange.size() - 1)
								<< std::nouppercase << std::setbase(0) << "  (Size: "
								<< GetHexDelim() << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << itrMemRange.size()
								<< std::nouppercase << std::setbase(0) << ")\n";
			}
		}
	}

	if (!m_bDeterministic) {
		aFunctionsFile << ";\n";
		aFunctionsFile << ";       Generated:  " << std::ctime(&m_StartTime);		// Note: std::ctime adds extra \n character, so no need to add our own
	}
	aFunctionsFile << ";\n\n";

	// Output FuncAnal Specific Commands:
	bTempFlag = false;
	if (allowMemRangeOverlap()) {
		aFunctionsFile << "-memrangeoverlap|true\n";
		bTempFlag = true;
	}
	if (opcodeSymbolSize() > 1) {
		aFunctionsFile << "-opcodesymbolsize|" << opcodeSymbolSize() << "\n";
		bTempFlag = true;
	}
	if (bTempFlag) aFunctionsFile << "\n";

	// Output Memory Mapping:
	bTempFlag = false;
	for (int nMemType = 0; nMemType < NUM_MEMORY_TYPES; ++nMemType) {
		if (m_MemoryRanges[nMemType].isNullRange()) continue;
		bTempFlag = true;

		for (auto const & itrMemRange : m_MemoryRanges[nMemType]) {
			std::ostringstream sstrTemp;

			if (itrMemRange.isNullRange()) continue;
			sstrTemp << "#" << g_arrstrMemRanges[nMemType] << "|";
			sstrTemp << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << itrMemRange.startAddr();
			sstrTemp << "|"  << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << itrMemRange.size();
			sstrTemp << "\n";
			m_sFunctionalOpcode = sstrTemp.str();
			aFunctionsFile << m_sFunctionalOpcode;
		}
	}
	if (bTempFlag) aFunctionsFile << "\n";

	// Output Labels:
	for (int nMemType = 0; nMemType < NUM_MEMORY_TYPES; ++nMemType) {
		CMemRanges ranges = m_Memory[nMemType].ranges();
		ranges.insert(ranges.end(), m_MemoryRanges[nMemType].cbegin(), m_MemoryRanges[nMemType].cend());
		ranges.consolidate();		// Use consolidation of actual memory and specified ranges to make sure we output everything

		CLabelTableMap mapMissing = m_LabelTable[nMemType];		// Copy of map to find labels missed during output (because they are out of range)

		bTempFlag = false;
		for (auto const & itrRange : ranges) {
			m_PC = itrRange.startAddr();
			for (TSize nSize = 0; nSize < itrRange.size(); ++nSize, ++m_PC) {
				mapMissing.erase(m_PC);
				CLabelTableMap::const_iterator itrLabels = m_LabelTable[nMemType].find(m_PC);
				if (itrLabels != m_LabelTable[nMemType].cend()) {
					std::ostringstream sstrTemp;
					bTempFlag2 = false;
					sstrTemp << "!" << g_arrstrMemRanges[nMemType] << "|";
					sstrTemp << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << m_PC;
					sstrTemp << std::nouppercase << std::setbase(0) << "|";
					for (CLabelArray::size_type i = 0; i < itrLabels->second.size(); ++i) {
						if (bTempFlag2) sstrTemp << ",";
						if (!itrLabels->second.at(i).empty()) {
							sstrTemp << itrLabels->second.at(i);
							bTempFlag2 = true;
						}
					}
					sstrTemp << "\n";
					m_sFunctionalOpcode = sstrTemp.str();
					if (bTempFlag2) {
						aFunctionsFile << m_sFunctionalOpcode;
						bTempFlag = true;
					}
				}
			}
		}
		for (auto const & itrMissing : mapMissing) {
			std::ostringstream sstrTemp;
			bTempFlag2 = false;
			sstrTemp << "!" << g_arrstrMemRanges[nMemType] << "|";
			sstrTemp << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << itrMissing.first;
			sstrTemp << std::nouppercase << std::setbase(0) << "|";
			for (CLabelArray::size_type i = 0; i < itrMissing.second.size(); ++i) {
				if (bTempFlag2) sstrTemp << ",";
				if (!itrMissing.second.at(i).empty()) {
					sstrTemp << itrMissing.second.at(i);
					bTempFlag2 = true;
				}
			}
			sstrTemp << "\n";
			m_sFunctionalOpcode = sstrTemp.str();
			if (bTempFlag2) {
				aFunctionsFile << m_sFunctionalOpcode;
				bTempFlag = true;
			}
		}
		if (bTempFlag) aFunctionsFile << "\n";
	}

	// Output Functions and Data Blocks:
	TAddress nFuncAddr = 0;			// Start PC of current function or Data Block
	TAddress nSavedPC = 0;			// nSavedPC will be the last PC for the last memory evaluated below
	bool bInFunc = false;			// In Function
	bool bInDataBlock = false;		// In Data Block
	CMemRange rngDataBlock;			// Current declared Data Block matching current Data Block being traversed (or null range if none matches)
	m_PC = 0;						// init to zero for test below that clears bInFunc and bInDataBlock (as we can't be "in a function" or "in a data block" across discontiguous memory)
	for (auto const & itrMemory : m_Memory[nMemType]) {
		if (m_PC != itrMemory.logicalAddr()) {
			bInFunc = false;		// Can't be "in a function" across discontiguous memory
			bInDataBlock = false;	// Can't be "in a data block" across discontiguous memory
			rngDataBlock = CMemRange();
		}
		m_PC = itrMemory.logicalAddr();
		for (TSize nSize = 0; ((nSize < itrMemory.size()) && bRetVal);  ) {
			bool bLastFlag = false;			// Flag for last instruction of a function
			bool bBranchOutFlag = false;	// Flag for branch out of a function

			// Check for function start flags:
			CFuncEntryMap::const_iterator itrFunctionEntry = m_FunctionEntryTable.find(m_PC);
			if (itrFunctionEntry != m_FunctionEntryTable.cend()) {
				switch (itrFunctionEntry->second) {
					case FUNCF_BRANCHIN:
						if (bInFunc) break;		// Continue if already inside a function
						// Else, fall-through and setup for new function:
					case FUNCF_ENTRY:
					case FUNCF_INDIRECT:
					case FUNCF_CALL:
					{
						if (bInDataBlock) {
							aFunctionsFile << "\n";
							bInDataBlock = false;
							rngDataBlock = CMemRange();
						}

						std::ostringstream sstrTemp;
						sstrTemp << "@" << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << m_PC << "|";
						m_sFunctionalOpcode = sstrTemp.str();

						CLabelTableMap::const_iterator itrLabels = m_LabelTable[nMemType].find(m_PC);
						if (itrLabels != m_LabelTable[nMemType].cend()) {
							for (CLabelArray::size_type i = 0; i < itrLabels->second.size(); ++i) {
								if (i != 0) m_sFunctionalOpcode += ",";
								m_sFunctionalOpcode += FormatLabel(nMemType, LC_REF, itrLabels->second.at(i), m_PC);
							}
						} else {
							m_sFunctionalOpcode += "???";
						}

						aFunctionsFile << m_sFunctionalOpcode;
						aFunctionsFile << "\n";

						nFuncAddr = m_PC;
						bInFunc = true;
						break;
					}

					default:
						assert(false);			// Unexpected Function Flag!!  Check Code!!
						bRetVal = false;
						continue;
				}
			}

			// Check for function end flags:
			//	This has to be done after the start flags above so
			//	that BranchIn works correctly
			CFuncExitMap::const_iterator itrFunctionExit = m_FunctionExitTable.find(m_PC);
			if (itrFunctionExit != m_FunctionExitTable.cend()) {
				switch (itrFunctionExit->second) {
					case FUNCF_HARDSTOP:
					case FUNCF_EXITBRANCH:
					case FUNCF_SOFTSTOP:
						bLastFlag = true;
						bInFunc = false;
						break;

					case FUNCF_BRANCHOUT:
						bBranchOutFlag = true;
						break;

					default:
						assert(false);			// Unexpected Function Flag!!  Check Code!!
						bRetVal = false;
						continue;
				}
			}

			nSavedPC = m_PC;
			m_sFunctionalOpcode.clear();
			switch (itrMemory.descriptor(m_PC)) {
				case DMEM_NOTLOADED:
					++m_PC;
					++nSize;
					if (bInDataBlock && !bInFunc) aFunctionsFile << "\n";
					bLastFlag = bInFunc;
					bInFunc = false;
					bInDataBlock = false;
					rngDataBlock = CMemRange();
					break;
				case DMEM_LOADED:
					assert(false);		// WARNING!  All loaded code should have been evaluated.  Check override code!
					bRetVal = false;
					++m_PC;
					++nSize;
					break;
				case DMEM_CODEINDIRECT:
				case DMEM_DATAINDIRECT:
					if (!bInFunc && !bLastFlag) {
						if (bInDataBlock) aFunctionsFile << "\n";
						bInDataBlock = false;
						rngDataBlock = CMemRange();

						std::ostringstream sstrTemp;
						sstrTemp << "=" << ((itrMemory.descriptor(m_PC) == DMEM_CODEINDIRECT) ? "C" : "D") << "|"
										<< std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << m_PC << "|";
						m_sFunctionalOpcode = sstrTemp.str();

						CLabelTableMap::const_iterator itrLabels = m_LabelTable[nMemType].find(m_PC);
						if (itrLabels != m_LabelTable[nMemType].cend()) {
							for (CLabelArray::size_type i=0; i<itrLabels->second.size(); ++i) {
								if (i != 0) m_sFunctionalOpcode += ",";
								m_sFunctionalOpcode += FormatLabel(nMemType, LC_REF, itrLabels->second.at(i), m_PC);
							}
						}
						m_sFunctionalOpcode += "|";

						RetrieveIndirect(nMemType, msgFile, errFile);		// This increments m_PC and populates m_OpMemory
						nSize += (m_PC-nSavedPC);

						aFunctionsFile << m_sFunctionalOpcode;
						aFunctionsFile << "\n\n";
						break;
					}
					// If inside of function, output it as inline function data
					//	as before the Data Block reorg:
					[[fallthrough]];
				case DMEM_DATA:
				case DMEM_PRINTDATA:
					if (!bInFunc && !bLastFlag) {
						if (bInDataBlock && !rngDataBlock.isNullRange() && !rngDataBlock.addressInRange(m_PC)) {
							aFunctionsFile << "\n";
							bInDataBlock = false;
							rngDataBlock = CMemRange();
						}
						if (!bInDataBlock) {
							std::ostringstream sstrTemp;
							sstrTemp << "$" << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << m_PC << "|";
							m_sFunctionalOpcode = sstrTemp.str();

							CLabelTableMap::const_iterator itrLabels = m_LabelTable[nMemType].find(m_PC);
							if (itrLabels != m_LabelTable[nMemType].cend()) {
								for (CLabelArray::size_type i = 0; i < itrLabels->second.size(); ++i) {
									if (i != 0) m_sFunctionalOpcode += ",";
									m_sFunctionalOpcode += FormatLabel(nMemType, LC_REF, itrLabels->second.at(i), m_PC);
								}
							} else {
								m_sFunctionalOpcode += "???";
							}

							aFunctionsFile << m_sFunctionalOpcode;
							aFunctionsFile << "\n";

							nFuncAddr = m_PC;
							bInDataBlock = true;
							if (nMemType == MT_ROM) {
								rngDataBlock = m_rngDataBlocks.firstMatchingRange(m_PC);
							}
						}
					}
					[[fallthrough]];
				case DMEM_ILLEGALCODE:
				{
					static const std::set<MEM_DESC> setData = { DMEM_DATA, DMEM_PRINTDATA };
					std::ostringstream sstrTemp;
					sstrTemp << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << m_PC << "|";
					m_sFunctionalOpcode = sstrTemp.str();

					CLabelTableMap::const_iterator itrLabels = m_LabelTable[nMemType].find(m_PC);
					if (itrLabels != m_LabelTable[nMemType].cend()) {
						for (CLabelArray::size_type i=0; i<itrLabels->second.size(); ++i) {
							if (i != 0) m_sFunctionalOpcode += ",";
							m_sFunctionalOpcode += FormatLabel(nMemType, LC_REF, itrLabels->second.at(i), m_PC);
						}
					}
					m_sFunctionalOpcode += "|";

					do {
						sstrTemp.str(std::string());
						sstrTemp << std::uppercase << std::setfill('0') << std::setw(2) << std::setbase(16) << static_cast<unsigned int>(m_Memory[nMemType].element(m_PC));
						m_sFunctionalOpcode += sstrTemp.str();

						++m_PC;
						++nSize;
					} while ((nSize < itrMemory.size())
							 && (!m_LabelTable[nMemType].contains(m_PC))
							 && ((itrMemory.descriptor(m_PC) == itrMemory.descriptor(nSavedPC)) ||
								 (setData.contains(static_cast<MEM_DESC>(itrMemory.descriptor(m_PC))) && setData.contains(static_cast<MEM_DESC>(itrMemory.descriptor(nSavedPC)))))
							 && ((m_PC-nSavedPC) < opcodeSymbolSize())
							 && (rngDataBlock.isNullRange() || rngDataBlock.addressInRange(m_PC)));
					break;
				}
				case DMEM_CODE:
					bRetVal = ReadNextObj(nMemType, false, msgFile, errFile);
					nSize += (m_PC - nSavedPC);		// NOTE: ReadNextObj automatically advanced m_PC while reading object!
					break;
				default:
					bInFunc = false;
					bInDataBlock = false;
					rngDataBlock = CMemRange();
					bRetVal = false;
					break;
			}

			if (!m_sFunctionalOpcode.empty()) {
				std::ostringstream sstrTemp;
				sstrTemp << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << (nSavedPC - nFuncAddr) << "|";
				m_sFunctionalOpcode = sstrTemp.str() + m_sFunctionalOpcode;
			}

			if (bBranchOutFlag) {
				itrFunctionEntry = m_FunctionEntryTable.find(m_PC);
				if (itrFunctionEntry != m_FunctionEntryTable.cend()) {
					switch (itrFunctionEntry->second) {
						case FUNCF_ENTRY:
						case FUNCF_INDIRECT:
						case FUNCF_CALL:
							bLastFlag = true;
							bInFunc = false;
							break;
						default:
							break;
					}
				} else {
					bLastFlag = true;
					bInFunc = false;
				}
			}

			if (bLastFlag) {
				itrFunctionEntry = m_FunctionEntryTable.find(m_PC);
				if (itrFunctionEntry != m_FunctionEntryTable.cend()) {
					switch (itrFunctionEntry->second) {
						case FUNCF_BRANCHIN:
							bLastFlag = false;
							bInFunc = true;
							break;
						default:
							break;
					}
				}
			}

			if (bInFunc || bLastFlag || bInDataBlock) {
				if (!m_sFunctionalOpcode.empty()) {
					aFunctionsFile << m_sFunctionalOpcode;
					aFunctionsFile << "\n";
				}
				if (bLastFlag) aFunctionsFile << "\n";
			}
		}
	}

	aFunctionsFile.close();

	return bRetVal;
}

// ----------------------------------------------------------------------------

bool CDisassembler::FindCode(std::ostream *msgFile, std::ostream *errFile)
{
	const MEMORY_TYPE nMemType = MT_ROM;

	bool bDoneFlag = false;

	TAddress nHighestAddress = m_Memory[nMemType].highestLogicalAddress();

	while (!bDoneFlag) {
		// See if m_PC is in an area of memory that doesn't exist:
		if (!m_Memory[nMemType].containsAddress(m_PC)) {
			if (m_bSpitFlag) {
				// In spit mode, find next address inside memory that hasn't been "looked at":
				while (!m_Memory[nMemType].containsAddress(m_PC) && (m_PC <= nHighestAddress)) ++m_PC;	// Skip non-existant addresses
				while (m_Memory[nMemType].containsAddress(m_PC) && (m_PC <= nHighestAddress) &&
					   (m_Memory[nMemType].descriptor(m_PC) != DMEM_LOADED)) {
					++m_PC;		// Skip "observed" locations (such as those from declared Data Blocks)
					while (!m_Memory[nMemType].containsAddress(m_PC) && (m_PC <= nHighestAddress)) ++m_PC;	// Skip non-existant addresses
				}
			} else {
				bDoneFlag = true;			// We are done with this find if there's nothing here in memory
				continue;
			}
		}
		if (m_PC > nHighestAddress) {
			bDoneFlag = true;				// If we run out of memory, we are done
			continue;
		}
		if ((m_Memory[nMemType].descriptor(m_PC) != DMEM_LOADED) && !m_bSpitFlag) {
			bDoneFlag = true;				// Exit when we hit an area we've already looked at
			continue;
		}
		if (ReadNextObj(nMemType, true, msgFile, errFile)) {		// Read next opcode and tag memory since we are finding code here, NOTE: This increments m_PC!
			if (CurrentOpcodeIsStop() && !m_bSpitFlag) bDoneFlag = true;		// Done when we hit a code exit point, unless we're just spitting code out
		}	// If the ReadNextObj returns false, that means we hit an illegal opcode byte.  The ReadNextObj will have incremented the m_PC past that byte, so we'll keep processing
	}

	return true;
}

// ----------------------------------------------------------------------------

std::string CDisassembler::FormatAddress(TAddress nAddress) const
{
	std::ostringstream sstrTemp;

	sstrTemp << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << nAddress;
	return sstrTemp.str();
}

TAddress CDisassembler::UnformatAddress(const std::string strAddress) const
{
	return strtoul(strAddress.c_str(), nullptr, 16);
}

std::string CDisassembler::FormatLabel(MEMORY_TYPE nMemoryType, LABEL_CODE nLC, const TLabel & strLabel, TAddress nAddress)
{
	UNUSED(nLC);

	TLabel strTemp;

	if (strLabel.empty()) {
		strTemp = GenLabel(nMemoryType, nAddress);
	} else {
		strTemp = strLabel;
	}

	return strTemp;
}

std::string CDisassembler::FormatReferences(MEMORY_TYPE nMemoryType, MNEMONIC_CODE nMCCode, TAddress nAddress)
{
	UNUSED(nMCCode);

	std::string strRetVal;
	bool bFlag = false;

	if (!allowMemRangeOverlap() || (nMemoryType == MT_ROM)) {
		CAddressTableMap::const_iterator itrBranches = m_BranchTable.find(nAddress);
		if (itrBranches != m_BranchTable.cend()) {
			if (itrBranches->second.size() != 0) {
				strRetVal += "CRef: ";
				bFlag = true;
				for (CAddressArray::size_type i=0; i<itrBranches->second.size(); ++i) {
					if (i != 0) strRetVal += ",";
					std::ostringstream sstrTemp;
					sstrTemp << GetHexDelim() << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << itrBranches->second.at(i);
					strRetVal += sstrTemp.str();
				}
			}
		}
	}

	if (m_LabelTable[nMemoryType].contains(nAddress)) {
		CAddressTableMap::const_iterator itrLabelRef = m_LabelRefTable[nMemoryType].find(nAddress);
		if (itrLabelRef == m_LabelRefTable[nMemoryType].cend()) {
			assert(false);		// Should also have a ref entry!
			return strRetVal;
		}

		if (itrLabelRef->second.size() != 0) {
			if (bFlag) strRetVal += "; ";
			strRetVal += "DRef: ";
			bFlag = true;
			for (CAddressArray::size_type i=0; i<itrLabelRef->second.size(); ++i) {
				if (i != 0) strRetVal += ",";
				std::ostringstream sstrTemp;
				sstrTemp << GetHexDelim() << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << itrLabelRef->second.at(i);
				strRetVal += sstrTemp.str();
			}
		}
	}

	return strRetVal;
}

std::string CDisassembler::FormatUserComments(MEMORY_TYPE nMemoryType, MNEMONIC_CODE nMCCode, TAddress nAddress)
{
	CCommentTableMap::const_iterator itrUserComments = m_CommentTable[nMemoryType].find(nAddress);
	if (itrUserComments == m_CommentTable[nMemoryType].cend()) return std::string();

	COMMENT_TYPE_FLAGS ctfType = CTF_NONE;
	switch (nMCCode) {
		case MC_OPCODE:
		case MC_ILLOP:
			ctfType = CTF_CODE;
			break;
		case MC_EQUATE:
			ctfType = CTF_EQUATE;
			break;
		case MC_DATABYTE:
		case MC_ASCII:
			ctfType = CTF_DATA;
			break;
		case MC_INDIRECT:
			ctfType = CTF_REF;
			break;
		default:
			assert(false);
			break;
	}

	std::ostringstream ssUserComments;

	for (auto const & itrComment : itrUserComments->second) {
		if (itrComment.m_nFlags & ctfType) {
			ssUserComments << itrComment.m_strComment << "\n";
		}
	}

	std::string strRetVal = ssUserComments.str();
	rtrim(strRetVal);				// Trim extra trailing '\n' from above

	return strRetVal;
}

std::string CDisassembler::FormatFunctionFlagComments(MEMORY_TYPE nMemoryType, MNEMONIC_CODE nMCCode, TAddress nStartAddress)
{
	std::string strFuncFlags;

#ifdef FUNC_FLAG_COMMENT
	if ((nMemoryType == MT_ROM) && ((nMCCode == MC_OPCODE) || (nMCCode == MC_ILLOP))) {
		std::ostringstream ssFuncFlags;
		CStringArray arrFuncFlags;
		for (TAddressOffset nOffset = 0; nOffset < static_cast<TAddressOffset>(getOpMemorySize()*opcodeSymbolSize()); ++nOffset) {
			std::string strFlags;

			CFuncEntryMap::const_iterator itrFunctionEntry = m_FunctionEntryTable.find(nStartAddress + nOffset);
			if (itrFunctionEntry != m_FunctionEntryTable.cend()) {
				switch (itrFunctionEntry->second) {
					case FUNCF_BRANCHIN:
						strFlags += "BRANCHIN";
						break;
					case FUNCF_ENTRY:
						strFlags += "ENTRY";
						break;
					case FUNCF_INDIRECT:
						strFlags += "INDIRECT";
						break;
					case FUNCF_CALL:
						strFlags += "CALL";
						break;
					default:
						strFlags += "???";
						break;
				}
			} else {
				strFlags += "---";
			}

			strFlags += "|";

			CFuncExitMap::const_iterator itrFunctionExit = m_FunctionExitTable.find(nStartAddress + nOffset);
			if (itrFunctionExit != m_FunctionExitTable.cend()) {
				switch (itrFunctionExit->second) {
					case FUNCF_HARDSTOP:
						strFlags += "HARDSTOP";
						break;
					case FUNCF_EXITBRANCH:
						strFlags += "EXITBRANCH";
						break;
					case FUNCF_SOFTSTOP:
						strFlags += "SOFTSTOP";
						break;
					case FUNCF_BRANCHOUT:
						strFlags += "BRANCHOUT";
						break;
					default:
						strFlags += "???";
						break;
				}
			} else {
				strFlags += "---";
			}

			arrFuncFlags.push_back(strFlags);
		}
		std::copy(arrFuncFlags.cbegin(), arrFuncFlags.cend(), std::ostream_iterator<TString>(ssFuncFlags, ", "));
		strFuncFlags = ssFuncFlags.str();
		assert(strFuncFlags.size() >= 2);
		strFuncFlags.pop_back();
		strFuncFlags.pop_back();
	}
#endif

	return strFuncFlags;
}

// ----------------------------------------------------------------------------

TAddressOffset CDisassembler::GetOpBytesFWAddressOffset() const
{
	return (GetFieldWidth(FC_OPBYTES)+2)/3;				// Use +2 /3 for rounding, since a field width of 11 and 12, for example, should both be 4 bytes
}

int CDisassembler::GetFieldWidth(FIELD_CODE nFC) const
{
	switch (nFC) {
		case FC_ADDRESS:	return calcFieldWidth(8);
		case FC_OPBYTES:	return calcFieldWidth(12);
		case FC_LABEL:		return calcFieldWidth(14);
		case FC_MNEMONIC:	return calcFieldWidth(8);
		case FC_OPERANDS:	return calcFieldWidth(44);
		case FC_COMMENT:	return calcFieldWidth(60);
		default:
			break;
	}
	return 0;
}

std::string CDisassembler::MakeOutputLine(CStringArray& saOutputData) const
{
	std::string strOutput;
	std::string strTemp;
	std::string strTemp2;
	std::string strItem;
	std::string::size_type fw = 0;
	std::string::size_type pos = 0;
	std::string::size_type tfw = 0;
	std::string::size_type nTempPos;
	std::string strOpBytePart;
	std::string strCommentPart;
	CStringArray saWrapSave;
	int nToss;
	bool bLabelBreak = false;
	std::string strSaveLabel;

	// This function will "wrap" the FC_OPBYTES and FC_COMMENT fields and create a
	//	result that spans multiple lines if they are longer than the field width.
	//	Wrapping is first attempted via white-space and/or one of ",;:.!?" symbols.  If
	//	that fails, the line is truncated at the specified length.  All other
	//	fields are not wrapped.  Note that embedded tabs count as 1 character for the
	//	wrapping mechanism!  Also, a '\n' can be used in the FC_COMMENT field to force
	//	a break.  All other fields should NOT have a '\n' character!
	//
	//	A '\v' (vertical tab or 0x0B) at the end of FC_LABEL will cause the label to
	//	appear on a line by itself -- the '\v' for labels should appear only at the end
	//	of the label -- not in the middle of it!.
	//

	for (CStringArray::size_type i=0; i<saOutputData.size(); ++i) {
		strItem = saOutputData[i];

		tfw += fw;
		fw = GetFieldWidth(static_cast<FIELD_CODE>(i));

		if (!m_bAddrFlag && (i == FC_ADDRESS)) {
			pos += fw;
			continue;
		}
		if (!m_bOpcodeFlag && (i == FC_OPBYTES)) {
			pos += fw;
			continue;
		}

		if (i == FC_LABEL) {			// Check for Label Break:
			if (strItem.find('\v') != std::string::npos) {
				while (strItem.find('\v') != std::string::npos) {
					strItem = strItem.substr(0, strItem.find('\v')) + strItem.substr(strItem.find('\v')+1);
				}
				strSaveLabel = strItem;
				strItem.clear();
				bLabelBreak = true;
			}
		}

		if (i == FC_OPBYTES) {			// Check OpBytes "wrap"
			if (strItem.size() > fw) {
				strTemp = strItem.substr(0, fw);
				nTempPos = strTemp.find_last_of(" \t,;:.!?");
				if (nTempPos != std::string::npos) {
					nToss = ((strTemp.find_last_of(" \t")==nTempPos) ? 1 : 0);
					strTemp = strItem.substr(0, nTempPos + 1 - nToss);	// Throw ' ' and '\t' away
					strOpBytePart = strItem.substr(nTempPos + nToss);
					strItem = strTemp;
				} else {
					strOpBytePart = strItem.substr(fw+1);
					strItem = strItem.substr(0, fw);
				}
			}
		}
		if (i == FC_COMMENT) {			// Check comment "wrap"
			strTemp = GetCommentStartDelim() + " " + strItem + " " + GetCommentEndDelim();
			strTemp2 = GetCommentStartDelim() + "  " + GetCommentEndDelim();
			if ((strTemp.size() > fw) || (strTemp.find('\n') != std::string::npos)) {
				bool bNeedsTossing = false;
				strTemp = strItem.substr(0, fw - strTemp2.size());
				nTempPos = strTemp.find('\n');			// We have to treat '\n' special to always break on the first
				if (nTempPos == std::string::npos) {
					nTempPos = strTemp.find_last_of(" \t,;:.!?");		// No need to check '\n' since we just did so above
					if ((nTempPos != std::string::npos) &&
						(strTemp.find_last_of(" \t") == nTempPos)) {
						bNeedsTossing = true;			// Characters to toss on the wrap instead of keeping
					}
				}
				if (nTempPos != std::string::npos) {
					nToss = (bNeedsTossing ? 1 : 0);
					strTemp = strItem.substr(0, nTempPos + 1 - nToss);	// Throw ' ' and '\t' away
					if ((nTempPos+1+nToss) < strItem.size()) {	// If it's the very last character, we need to clear or we'll crash trying to access off the end
						strCommentPart = "  " + strItem.substr(nTempPos+1 + nToss);
					} else {
						strCommentPart.clear();
					}
					strItem = strTemp;
				} else {
					strCommentPart = "  " + strItem.substr(fw+1-strTemp2.size());
					strItem = strItem.substr(0, fw-strTemp2.size());
				}
			}
		}

		if (i == FC_COMMENT) {
			if (!strItem.empty())
				strItem = GetCommentStartDelim() + " " + strItem + " " + GetCommentEndDelim();
		}

		if (m_bTabsFlag) {
			while (pos < tfw) {
				if (m_nTabWidth != 0) {
					while ((tfw - pos) % m_nTabWidth) {
						strOutput += " ";
						++pos;
					}
					while ((tfw - pos) / m_nTabWidth) {
						strOutput += "\t";
						pos += m_nTabWidth;
					}
				} else {
					strOutput += " ";
					++pos;
				}
			}
			strOutput += strItem;
		} else {
			strTemp.clear();
			for (int j=0; j<m_nTabWidth; ++j) strTemp += " ";
			while (strItem.find('\t') != std::string::npos) {
				nTempPos = strItem.find('\t');
				strItem = strItem.substr(0, nTempPos) + strTemp + strItem.substr(nTempPos+1);
			}
			while (pos < tfw) {
				strOutput += " ";
				++pos;
			}
			strOutput += strItem;
		}
		pos += strItem.size();
		if (pos >= (tfw+fw)) pos = tfw+fw-1;		// Sub 1 to force at least 1 space between fields
	}

	// Remove all extra white space on right-hand side:
	rtrim(strOutput);

	if ((!strOpBytePart.empty()) || (!strCommentPart.empty()) || (bLabelBreak)) {
		// Save data:
		saWrapSave = saOutputData;
		if (saOutputData.size() < NUM_FIELD_CODES)
			for (unsigned int i=saOutputData.size(); i<NUM_FIELD_CODES; ++i) saOutputData.push_back(std::string());

		if (bLabelBreak) {
			saOutputData[FC_OPBYTES].clear();
			saOutputData[FC_LABEL] = strSaveLabel;
			saOutputData[FC_MNEMONIC].clear();
			saOutputData[FC_OPERANDS].clear();
			saOutputData[FC_COMMENT].clear();
			strOutput = MakeOutputLine(saOutputData) + "\n" + strOutput;	// Call recursively
		}
		if ((!strOpBytePart.empty()) || (!strCommentPart.empty())) {
			if (!strOpBytePart.empty()) {
				saOutputData[FC_ADDRESS] = FormatAddress(UnformatAddress(saOutputData[FC_ADDRESS]) + GetOpBytesFWAddressOffset());
			}
			saOutputData[FC_OPBYTES] = strOpBytePart;
			saOutputData[FC_LABEL].clear();
			saOutputData[FC_MNEMONIC].clear();
			saOutputData[FC_OPERANDS].clear();
			saOutputData[FC_COMMENT] = strCommentPart;
			strOutput += "\n";
			strOutput += MakeOutputLine(saOutputData);		// Call recursively
		}

		// Restore data:
		saOutputData = saWrapSave;
	}

	return strOutput;
}

void CDisassembler::ClearOutputLine(CStringArray& saOutputData) const
{
	saOutputData.clear();
	saOutputData.reserve(NUM_FIELD_CODES);
	for (CStringArray::size_type i=0; i<NUM_FIELD_CODES; ++i) saOutputData.push_back(std::string());
}

// ----------------------------------------------------------------------------

bool CDisassembler::WriteHeader(std::ostream& outFile, std::ostream *msgFile, std::ostream *errFile)
{
	UNUSED(msgFile);
	UNUSED(errFile);

	CStringArray saOutLine;

	ClearOutputLine(saOutLine);
	saOutLine[FC_ADDRESS] = FormatAddress(0);
	saOutLine[FC_LABEL] = GetCommentStartDelim() + GetCommentEndDelim() + "\n";
	outFile << MakeOutputLine(saOutLine) << "\n";
	saOutLine[FC_LABEL] = GetCommentStartDelim() + " GenDasm - Generic Code-Seeking Disassembler" + GetCommentEndDelim() + "\n";
	outFile << MakeOutputLine(saOutLine) << "\n";
	saOutLine[FC_LABEL] = GetCommentStartDelim() + " " + GetGDCLongName() + " Generated Source Code " + GetCommentEndDelim() + "\n";
	outFile << MakeOutputLine(saOutLine) << "\n";
	saOutLine[FC_LABEL] = GetCommentStartDelim() + GetCommentEndDelim() + "\n";
	outFile << MakeOutputLine(saOutLine) << "\n";
	saOutLine[FC_LABEL] = GetCommentStartDelim() + "    Control File";
	saOutLine[FC_LABEL] += ((m_sControlFileList.size() > 1) ? "s: " : ":  ");
	for (CStringArray::size_type i=0; i<m_sControlFileList.size(); ++i) {
		if (i==0) {
			saOutLine[FC_LABEL] += m_sControlFileList.at(i) + "  " + GetCommentEndDelim() + "\n";
		} else {
			saOutLine[FC_LABEL] = GetCommentStartDelim() + "                  " + m_sControlFileList.at(i) + "  " + GetCommentEndDelim() + "\n";
		}
		outFile << MakeOutputLine(saOutLine) << "\n";
	}
	saOutLine[FC_LABEL] = GetCommentStartDelim() + "      Input File";
	saOutLine[FC_LABEL] += ((m_sInputFileList.size() > 1) ? "s: " : ":  ");
	for (CStringArray::size_type i=0; i<m_sInputFileList.size(); ++i) {
		if (i==0) {
			saOutLine[FC_LABEL] += m_sInputFileList.at(i) +  "  " + GetCommentEndDelim() + "\n";
		} else {
			saOutLine[FC_LABEL] = GetCommentStartDelim() + "                  " + m_sInputFileList.at(i) + "  " + GetCommentEndDelim() + "\n";
		}
		outFile << MakeOutputLine(saOutLine) << "\n";
	}
	saOutLine[FC_LABEL] = GetCommentStartDelim() + "     Output File:  " + m_sOutputFilename + "  " + GetCommentEndDelim() + "\n";
	outFile << MakeOutputLine(saOutLine) << "\n";
	if (!m_sFunctionsFilename.empty()) {
		saOutLine[FC_LABEL] = GetCommentStartDelim() + "  Functions File:  " + m_sFunctionsFilename + "  " + GetCommentEndDelim() + "\n";
		outFile << MakeOutputLine(saOutLine) << "\n";
	}
	saOutLine[FC_LABEL] = GetCommentStartDelim() + GetCommentEndDelim() + "\n";
	outFile << MakeOutputLine(saOutLine) << "\n";

	for (int nMemType = 0; nMemType < NUM_MEMORY_TYPES; ++nMemType) {
		saOutLine[FC_LABEL] = GetCommentStartDelim();
		saOutLine[FC_LABEL] += ((nMemType == 0) ? " Memory Mappings:  " : "                   ");
		for (std::string::size_type i = (getLongestMemMapName() - g_arrstrMemRanges[nMemType].size()); i; --i) {
			saOutLine[FC_LABEL] += " ";
		}
		saOutLine[FC_LABEL] += g_arrstrMemRanges[nMemType] + " Memory Map:";

		if (m_MemoryRanges[nMemType].isNullRange()) {
			saOutLine[FC_LABEL] += " <Not Defined>" + GetCommentEndDelim() + "\n";
			outFile << MakeOutputLine(saOutLine) << "\n";
		} else {
			saOutLine[FC_LABEL] += GetCommentEndDelim() + "\n";
			outFile << MakeOutputLine(saOutLine) << "\n";

			for (auto const & itr : m_MemoryRanges[nMemType]) {
				std::ostringstream sstrTemp;
				sstrTemp << GetCommentStartDelim() << "                       "
					<< GetHexDelim() << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << itr.startAddr()
					<< std::nouppercase << std::setbase(0) << " - "
					<< GetHexDelim() << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << (itr.startAddr() + itr.size() - 1)
					<< std::nouppercase << std::setbase(0) << "  (Size: "
					<< GetHexDelim() << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << itr.size()
					<< std::nouppercase << std::setbase(0) << ")  " << GetCommentEndDelim() << "\n";
					saOutLine[FC_LABEL] = sstrTemp.str();
					outFile << MakeOutputLine(saOutLine) << "\n";
			}
		}
	}

	if (!m_bDeterministic) {
		saOutLine[FC_LABEL] = GetCommentStartDelim() + GetCommentEndDelim() + "\n";
		outFile << MakeOutputLine(saOutLine) << "\n";
		saOutLine[FC_LABEL] = GetCommentStartDelim() + "       Generated:  ";
		saOutLine[FC_LABEL] += std::ctime(&m_StartTime);
		saOutLine[FC_LABEL].erase(saOutLine[FC_LABEL].end()-1);		// Note: std::ctime adds extra \n character
		saOutLine[FC_LABEL] += GetCommentEndDelim() + "\n";
		outFile << MakeOutputLine(saOutLine) << "\n";
	}

	saOutLine[FC_LABEL] = GetCommentStartDelim() + GetCommentEndDelim() + "\n";
	outFile << MakeOutputLine(saOutLine) << "\n";

	saOutLine[FC_LABEL].clear();
	outFile << MakeOutputLine(saOutLine) << "\n";
	outFile << MakeOutputLine(saOutLine) << "\n";
	return true;
}

// ----------------------------------------------------------------------------

bool CDisassembler::WritePreEquates(std::ostream& outFile, std::ostream *msgFile, std::ostream *errFile)
{
	UNUSED(outFile);
	UNUSED(msgFile);
	UNUSED(errFile);
	return true;		// Don't do anything by default
}

bool CDisassembler::WritePreEquatesRange(MEMORY_TYPE nMemoryType, std::ostream& outFile, std::ostream *msgFile, std::ostream *errFile)
{
	UNUSED(nMemoryType);
	UNUSED(outFile);
	UNUSED(msgFile);
	UNUSED(errFile);
	return true;		// Don't do anything by default
}

bool CDisassembler::WriteEquates(std::ostream& outFile, std::ostream *msgFile, std::ostream *errFile)
{
	CStringArray saOutLine;
	bool bRetVal;

	clearOpMemory();				// This is needed so that m_OpMemory works correctly for Equates in functions like FormatComments that recalculate m_PC

	bRetVal = WritePreEquates(outFile, msgFile, errFile);
	if (bRetVal) {
		ClearOutputLine(saOutLine);

		MEMORY_TYPE arrMemTypes[NUM_MEMORY_TYPES] = { MT_IO, MT_RAM, MT_EE, MT_ROM };
		for (int ndxType = 0; ndxType < NUM_MEMORY_TYPES; ++ndxType) {
			MEMORY_TYPE nMemoryType = arrMemTypes[ndxType];

			if (WritePreEquatesRange(nMemoryType, outFile, msgFile, errFile)) {
				bool bTypeHeader = false;
				CMemRanges ranges = m_Memory[nMemoryType].ranges();
				ranges.insert(ranges.end(), m_MemoryRanges[nMemoryType].cbegin(), m_MemoryRanges[nMemoryType].cend());
				ranges.consolidate();		// Use consolidation of actual memory and specified ranges to make sure we output everything

				CLabelTableMap mapMissing = m_LabelTable[nMemoryType];		// Copy of map to find labels missed during output (because they are out of range)

				for (auto const & itrRange : ranges) {		// This loop technically not necessary as there should only be a single range after the consolidation
					m_PC = itrRange.startAddr();
					for (TSize nSize = 0; nSize < itrRange.size(); ++nSize, ++m_PC) {
						assert(m_Memory[nMemoryType].descriptor(m_PC) != DMEM_LOADED);		// Find Routines didn't find and analyze all memory!!  Fix it!

						if (m_Memory[nMemoryType].descriptor(m_PC) != DMEM_NOTLOADED) {
							mapMissing.erase(m_PC);		// Remove from our track of missing
							continue;		// Loaded addresses will get outputted during main part
						}
						if ((m_PC != 0) && m_Memory[nMemoryType].containsAddress(m_PC-1) &&
							(m_Memory[nMemoryType].descriptor(m_PC-1) != DMEM_NOTLOADED)) {
							// If previous address was part of a main section that will be
							//	outputted (either code or data) then remove it because it
							//	means it has to be at the end of a given section and we will
							//	output it as a label at the end of that section instead...
							mapMissing.erase(m_PC-1);
							continue;
						}

						CLabelTableMap::const_iterator itrLabels = m_LabelTable[nMemoryType].find(m_PC);
						if (itrLabels != m_LabelTable[nMemoryType].cend()) {
							mapMissing.erase(itrLabels->first);		// Remove from our track of missing

							ClearOutputLine(saOutLine);
							saOutLine[FC_ADDRESS] = FormatAddress(m_PC);

							if (!bTypeHeader) {
								saOutLine[FC_LABEL] = GetCommentStartDelim() + " Equates for " + g_arrstrMemRanges[nMemoryType] + ": " + GetCommentEndDelim() + "\n";
								outFile << MakeOutputLine(saOutLine) << "\n";
								bTypeHeader = true;
							}

							for (CLabelArray::size_type i=0; i<itrLabels->second.size(); ++i) {
								saOutLine[FC_LABEL] = FormatLabel(nMemoryType, LC_EQUATE, itrLabels->second.at(i), m_PC);
								if (m_bVBreakEquateLabels && (saOutLine[FC_LABEL].size() >= static_cast<size_t>(GetFieldWidth(FC_LABEL)))) saOutLine[FC_LABEL] += '\v';
								saOutLine[FC_MNEMONIC] = FormatMnemonic(nMemoryType, MC_EQUATE, m_PC);
								saOutLine[FC_OPERANDS] = FormatOperands(nMemoryType, MC_EQUATE, m_PC);
								saOutLine[FC_COMMENT] = FormatComments(nMemoryType, MC_EQUATE, m_PC);
								outFile << MakeOutputLine(saOutLine) << "\n";
							}
						}
					}
				}
				if (bTypeHeader) {
					ClearOutputLine(saOutLine);
					saOutLine[FC_ADDRESS] = FormatAddress(0);
					outFile << MakeOutputLine(saOutLine) << "\n";
				}

				// Output missed labels for this memory type:
				bTypeHeader = false;
				for (auto const & itrMissing : mapMissing) {
					if ((itrMissing.first != 0) && m_Memory[nMemoryType].containsAddress(itrMissing.first-1) &&
						(m_Memory[nMemoryType].descriptor(itrMissing.first-1) != DMEM_NOTLOADED)) {
						// If previous address was part of a main section that will be
						//	outputted (either code or data) then remove it because it
						//	means it has to be at the end of a given section and we will
						//	output it as a label at the end of that section instead...
						continue;
					}

					if (!bTypeHeader) {
						saOutLine[FC_LABEL] = GetCommentStartDelim() + " Out-of-Range Equates for " + g_arrstrMemRanges[nMemoryType] + ": " + GetCommentEndDelim() + "\n";
						outFile << MakeOutputLine(saOutLine) << "\n";
						bTypeHeader = true;
					}

					ClearOutputLine(saOutLine);
					saOutLine[FC_ADDRESS] = FormatAddress(itrMissing.first);
					for (CLabelArray::size_type i=0; i<itrMissing.second.size(); ++i) {
						saOutLine[FC_LABEL] = FormatLabel(nMemoryType, LC_EQUATE, itrMissing.second.at(i), itrMissing.first);
						if (m_bVBreakEquateLabels && (saOutLine[FC_LABEL].size() >= static_cast<size_t>(GetFieldWidth(FC_LABEL)))) saOutLine[FC_LABEL] += '\v';
						saOutLine[FC_MNEMONIC] = FormatMnemonic(nMemoryType, MC_EQUATE, itrMissing.first);
						saOutLine[FC_OPERANDS] = FormatOperands(nMemoryType, MC_EQUATE, itrMissing.first);
						saOutLine[FC_COMMENT] = FormatComments(nMemoryType, MC_EQUATE, itrMissing.first);
						outFile << MakeOutputLine(saOutLine) << "\n";
					}
				}
				if (bTypeHeader) {
					ClearOutputLine(saOutLine);
					saOutLine[FC_ADDRESS] = FormatAddress(0);
					outFile << MakeOutputLine(saOutLine) << "\n";
				}

				WritePostEquatesRange(nMemoryType, outFile, msgFile, errFile);
			}
		}
		bRetVal = bRetVal && WritePostEquates(outFile, msgFile, errFile);
	}
	return bRetVal;
}

bool CDisassembler::WritePostEquatesRange(MEMORY_TYPE nMemoryType, std::ostream& outFile, std::ostream *msgFile, std::ostream *errFile)
{
	UNUSED(nMemoryType);
	UNUSED(outFile);
	UNUSED(msgFile);
	UNUSED(errFile);
	return true;		// Don't do anything by default
}

bool CDisassembler::WritePostEquates(std::ostream& outFile, std::ostream *msgFile, std::ostream *errFile)
{
	UNUSED(outFile);
	UNUSED(msgFile);
	UNUSED(errFile);
	return true;		// Don't do anything by default
}

// ----------------------------------------------------------------------------

bool CDisassembler::WritePreDisassembly(MEMORY_TYPE nMemoryType, std::ostream& outFile, std::ostream *msgFile, std::ostream *errFile)
{
	UNUSED(nMemoryType);
	UNUSED(outFile);
	UNUSED(msgFile);
	UNUSED(errFile);
	return true;
}

bool CDisassembler::WriteDisassembly(MEMORY_TYPE nMemoryType, std::ostream& outFile, std::ostream *msgFile, std::ostream *errFile)
{
	bool bRetVal = WritePreDisassembly(nMemoryType, outFile, msgFile, errFile);

	if (bRetVal) {
		for (auto const & itrMemory : m_Memory[nMemoryType]) {
			m_PC = itrMemory.logicalAddr();
			for (TSize nSize = 0; ((nSize < itrMemory.size()) && bRetVal);  ) {		// WARNING!! WriteSection MUST increment m_PC (and we increment nSize from it)
				switch (m_Memory[nMemoryType].descriptor(m_PC)) {
					case DMEM_NOTLOADED:
						++m_PC;
						++nSize;
						break;
					case DMEM_LOADED:
						assert(false);		// WARNING!  All loaded code should have been evaluated.  Check override code!
						bRetVal = false;
						++m_PC;
						++nSize;
						break;
					case DMEM_DATA:
					case DMEM_PRINTDATA:
					case DMEM_CODEINDIRECT:
					case DMEM_DATAINDIRECT:
					case DMEM_CODE:
					case DMEM_ILLEGALCODE:
					{
						TAddress nSavedPC = m_PC;
						bRetVal = bRetVal && WriteSection(nMemoryType, itrMemory, outFile, msgFile, errFile);			// WARNING!! WriteSection MUST properly increment m_PC!!
						nSize += (m_PC - nSavedPC);
						break;
					}
				}
			}
		}

		bRetVal = bRetVal && WritePostDisassembly(nMemoryType, outFile, msgFile, errFile);
	}

	return bRetVal;
}

bool CDisassembler::WritePostDisassembly(MEMORY_TYPE nMemoryType, std::ostream& outFile, std::ostream *msgFile, std::ostream *errFile)
{
	UNUSED(nMemoryType);
	UNUSED(outFile);
	UNUSED(msgFile);
	UNUSED(errFile);
	return true;		// Don't do anything by default
}

// ----------------------------------------------------------------------------

bool CDisassembler::WritePreSection(MEMORY_TYPE nMemoryType, const CMemBlock &memBlock, std::ostream& outFile, std::ostream *msgFile, std::ostream *errFile)
{
	UNUSED(nMemoryType);
	UNUSED(msgFile);
	UNUSED(errFile);

	if (memBlock.size() != 0) {
		CStringArray saOutLine;

		ClearOutputLine(saOutLine);
		saOutLine[FC_ADDRESS] = FormatAddress(0);
		outFile << MakeOutputLine(saOutLine) << "\n";

		saOutLine[FC_LABEL] = GetCommentStartDelim();
		saOutLine[FC_LABEL] += " ";
		for (int i=(GetFieldWidth(FC_LABEL)+
				GetFieldWidth(FC_MNEMONIC)+
				GetFieldWidth(FC_OPERANDS)); i; --i) saOutLine[FC_LABEL] += "=";
		saOutLine[FC_LABEL] += " ";
		saOutLine[FC_LABEL] += GetCommentEndDelim();
		outFile << MakeOutputLine(saOutLine) << "\n";

		ClearOutputLine(saOutLine);
		saOutLine[FC_ADDRESS] = FormatAddress(0);
		outFile << MakeOutputLine(saOutLine) << "\n";
	}

	return true;		// Don't do anything by default
}

bool CDisassembler::WriteSection(MEMORY_TYPE nMemoryType, const CMemBlock &memBlock, std::ostream& outFile, std::ostream *msgFile, std::ostream *errFile)
{
	bool bRetVal = WritePreSection(nMemoryType, memBlock, outFile, msgFile, errFile);

	CStringArray saOutLine;

	auto const &&fnWriteLabels = [&](TAddress nAddress)->void {
		CLabelTableMap::const_iterator itrLabels = m_LabelTable[nMemoryType].find(nAddress);
		if (itrLabels != m_LabelTable[nMemoryType].cend()) {
			for (CLabelArray::size_type i=1; i<itrLabels->second.size(); ++i) {
				saOutLine[FC_LABEL] = FormatLabel(nMemoryType, LC_CODE, itrLabels->second.at(i), nAddress);
				outFile << MakeOutputLine(saOutLine) << "\n";
			}
			if (itrLabels->second.size()) {
				saOutLine[FC_LABEL] = FormatLabel(nMemoryType, LC_CODE, itrLabels->second.at(0), nAddress);
				if (m_bVBreakCodeLabels && (saOutLine[FC_LABEL].size() >= static_cast<size_t>(GetFieldWidth(FC_LABEL)))) saOutLine[FC_LABEL] += '\v';
			}
		}
	};

	if (bRetVal) {
		bool bDone = false;
		TAddress nLastAddr = memBlock.logicalAddr() + memBlock.size();
		while ((m_PC < nLastAddr) && !bDone && bRetVal) {
			// WARNING!! Write Data and Code section functions MUST properly increment m_PC!!
			switch (m_Memory[nMemoryType].descriptor(m_PC)) {
				case DMEM_DATA:
				case DMEM_PRINTDATA:
				case DMEM_CODEINDIRECT:
				case DMEM_DATAINDIRECT:
					bRetVal = bRetVal && WriteDataSection(nMemoryType, memBlock, outFile, msgFile, errFile);
					break;
				case DMEM_CODE:
				case DMEM_ILLEGALCODE:
					bRetVal = bRetVal && WriteCodeSection(nMemoryType, memBlock, outFile, msgFile, errFile);
					break;
				default:
					bDone = true;
					break;
			}
		}

		// Output any non-loaded labels at the trail of this section:
		if (m_Memory[nMemoryType].descriptor(m_PC) == DMEM_NOTLOADED) {
			ClearOutputLine(saOutLine);
			fnWriteLabels(m_PC);
			if (!saOutLine[FC_LABEL].empty()) {
				outFile << MakeOutputLine(saOutLine) << "\n";
			}
		}

		bRetVal = bRetVal && WritePostSection(nMemoryType, memBlock, outFile, msgFile, errFile);
	}

	return bRetVal;
}

bool CDisassembler::WritePostSection(MEMORY_TYPE nMemoryType, const CMemBlock &memBlock, std::ostream& outFile, std::ostream *msgFile, std::ostream *errFile)
{
	UNUSED(nMemoryType);
	UNUSED(memBlock);
	UNUSED(outFile);
	UNUSED(msgFile);
	UNUSED(errFile);
	return true;		// Don't do anything by default
}

// ----------------------------------------------------------------------------

bool CDisassembler::WritePreDataSection(MEMORY_TYPE nMemoryType, const CMemBlock &memBlock, std::ostream& outFile, std::ostream *msgFile, std::ostream *errFile)
{
	UNUSED(nMemoryType);
	UNUSED(memBlock);
	UNUSED(outFile);
	UNUSED(msgFile);
	UNUSED(errFile);
	return true;		// Don't do anything by default
}

bool CDisassembler::WriteDataSection(MEMORY_TYPE nMemoryType, const CMemBlock &memBlock, std::ostream& outFile, std::ostream *msgFile, std::ostream *errFile)
{
	bool bRetVal;
	CStringArray saOutLine;
	bool bDone;
	TAddress nSavedPC;
	int nCount;
	bool bFlag;
	MEM_DESC nDesc;
	TLabel strTempLabel;
	CMemoryArray maTempOpMemory;
	// Note: Even though maTempOpMemory is supposed to mirror COpcodeSymbolArray from TDisassembler,
	//	we don't have a good way to get access to that type name.  That means that this CMemoryArray
	//	will technically be the wrong type (or at least mismatched) when on processors with a
	//	TOpcodeSymbol type that doesn't match TMemoryElement.  Not a major problem since this
	//	is the 'data' section and specifically working on bytes.

	auto const &&fnWriteLabels = [&](TAddress nAddress)->void {
		CLabelTableMap::const_iterator itrLabels = m_LabelTable[nMemoryType].find(nAddress);
		if (itrLabels != m_LabelTable[nMemoryType].cend()) {
			for (CLabelArray::size_type i=1; i<itrLabels->second.size(); ++i) {
				saOutLine[FC_LABEL] = FormatLabel(nMemoryType, LC_DATA, itrLabels->second.at(i), nAddress);
				outFile << MakeOutputLine(saOutLine) << "\n";
			}
			if (itrLabels->second.size()) {
				saOutLine[FC_LABEL] = FormatLabel(nMemoryType, LC_DATA, itrLabels->second.at(0), nAddress);
				if (m_bVBreakDataLabels && (saOutLine[FC_LABEL].size() >= static_cast<size_t>(GetFieldWidth(FC_LABEL)))) saOutLine[FC_LABEL] += '\v';
			}
		}
	};

	ClearOutputLine(saOutLine);

	bRetVal = WritePreDataSection(nMemoryType, memBlock, outFile, msgFile, errFile);
	if (bRetVal) {
		bDone = false;
		TAddress nLastAddr = memBlock.logicalAddr() + memBlock.size();
		while ((m_PC < nLastAddr) && !bDone && bRetVal) {
			ClearOutputLine(saOutLine);
			saOutLine[FC_ADDRESS] = FormatAddress(m_PC);

			CMemRange rngDataBlock = ((nMemoryType == MT_ROM) ? m_rngDataBlocks.firstMatchingRange(m_PC) : CMemRange());
			nSavedPC = m_PC;		// Keep a copy of the PC for this line because some calls will be incrementing our m_PC
			nDesc = static_cast<MEM_DESC>(m_Memory[nMemoryType].descriptor(m_PC));
			if (!m_bAsciiFlag && (nDesc == DMEM_PRINTDATA)) nDesc = DMEM_DATA;		// If not doing ASCII, treat print data as data
			switch (nDesc) {
				case DMEM_DATA:
					fnWriteLabels(m_PC);	// Must do this before bumping m_PC
					nCount = 0;
					clearOpMemory();
					bFlag = false;
					while (!bFlag) {
						pushBackOpMemory(m_PC, m_Memory[nMemoryType].element(m_PC));
						++m_PC;
						++nCount;
						// Stop on this line when we've either run out of data,
						//	hit the specified line limit, hit another label, or
						//	hit another user-declared DataBlock range:
						if (nCount >= m_nMaxNonPrint) bFlag = true;
						if (m_LabelTable[nMemoryType].contains(m_PC)) bFlag = true;
						nDesc = static_cast<MEM_DESC>(m_Memory[nMemoryType].descriptor(m_PC));
						if (!m_bAsciiFlag && (nDesc == DMEM_PRINTDATA)) nDesc = DMEM_DATA;		// If not doing ASCII, treat print data as data
						if (nDesc != DMEM_DATA) bFlag = true;
						if (m_PC >= nLastAddr) bFlag = true;
						if (!rngDataBlock.isNullRange() && !rngDataBlock.addressInRange(m_PC)) bFlag = true;
					}
					if (m_bDataOpBytesFlag) saOutLine[FC_OPBYTES] = FormatOpBytes(nMemoryType, MC_DATABYTE, nSavedPC);
					saOutLine[FC_MNEMONIC] = FormatMnemonic(nMemoryType, MC_DATABYTE, nSavedPC);
					saOutLine[FC_OPERANDS] = FormatOperands(nMemoryType, MC_DATABYTE, nSavedPC);
					saOutLine[FC_COMMENT] = FormatComments(nMemoryType, MC_DATABYTE, nSavedPC);
					outFile << MakeOutputLine(saOutLine) << "\n";
					break;
				case DMEM_PRINTDATA:
					fnWriteLabels(m_PC);	// Must do this before bumping m_PC
					nCount = 0;
					maTempOpMemory.clear();
					bFlag = false;
					while (!bFlag) {
						maTempOpMemory.push_back(m_Memory[nMemoryType].element(m_PC));
						++m_PC;
						++nCount;
						// Stop on this line when we've either run out of data,
						//	hit the specified line limit, hit another label, or
						//	hit another user-declared DataBlock range:
						if (nCount >= m_nMaxPrint) bFlag = true;
						if (m_LabelTable[nMemoryType].contains(m_PC)) bFlag = true;
						if (m_Memory[nMemoryType].descriptor(m_PC) != DMEM_PRINTDATA) bFlag = true;
						if (!rngDataBlock.isNullRange() && !rngDataBlock.addressInRange(m_PC)) bFlag = true;
					}
					// First, print a line of the output bytes for reference:
					if (m_bAsciiBytesFlag) {
						strTempLabel = saOutLine[FC_LABEL];

						int nIndex = 0;		// Index into maTempOpMemory, and offset from nSavedPC
						assert(static_cast<CMemoryArray::size_type>(nCount) == maTempOpMemory.size());
						while (nCount) {
							TAddress nCurrentPC = nSavedPC + nIndex;
							clearOpMemory();

							for (int i=0; ((nCount > 0) && (i<m_nMaxNonPrint)); ++i, ++nIndex, --nCount)
								pushBackOpMemory(nSavedPC+nIndex, maTempOpMemory.at(nIndex));

							saOutLine[FC_LABEL] = GetCommentStartDelim() + " " + strTempLabel +
												((!strTempLabel.empty()) ? " " : "") +
												FormatOperands(nMemoryType, MC_DATABYTE, nSavedPC) + " " + GetCommentEndDelim();
							saOutLine[FC_ADDRESS] = FormatAddress(nCurrentPC);
							saOutLine[FC_MNEMONIC].clear();
							saOutLine[FC_OPERANDS].clear();
							saOutLine[FC_COMMENT].clear();
							outFile << MakeOutputLine(saOutLine) << "\n";
						}

						saOutLine[FC_LABEL] = strTempLabel;
					}
					clearOpMemory();
					pushBackOpMemory(nSavedPC, maTempOpMemory);

					// Then, print the line as it should be in the ASCII equivalent:
					saOutLine[FC_ADDRESS] = FormatAddress(nSavedPC);
					if (m_bDataOpBytesFlag) saOutLine[FC_OPBYTES] = FormatOpBytes(nMemoryType, MC_ASCII, nSavedPC);
					saOutLine[FC_MNEMONIC] = FormatMnemonic(nMemoryType, MC_ASCII, nSavedPC);
					saOutLine[FC_OPERANDS] = FormatOperands(nMemoryType, MC_ASCII, nSavedPC);
					saOutLine[FC_COMMENT] = FormatComments(nMemoryType, MC_ASCII, nSavedPC);
					outFile << MakeOutputLine(saOutLine) << "\n";
					break;
				case DMEM_CODEINDIRECT:
				case DMEM_DATAINDIRECT:
					fnWriteLabels(m_PC);	// Must do this before bumping m_PC
					// If the following call returns false, that means that the user (or
					//	special indirect detection logic) erroneously specified (or detected)
					//	the indirect.  So instead of throwing an error or causing problems,
					//	we will treat it as a data byte instead:
					if (RetrieveIndirect(nMemoryType, msgFile, errFile) == false) {		// Bumps PC and fills in m_OpMemory
						if (m_bDataOpBytesFlag) saOutLine[FC_OPBYTES] = FormatOpBytes(nMemoryType, MC_DATABYTE, nSavedPC);
						saOutLine[FC_MNEMONIC] = FormatMnemonic(nMemoryType, MC_DATABYTE, nSavedPC);
						saOutLine[FC_OPERANDS] = FormatOperands(nMemoryType, MC_DATABYTE, nSavedPC);
						saOutLine[FC_COMMENT] = FormatComments(nMemoryType, MC_DATABYTE, nSavedPC) + " -- Erroneous Indirect Stub";
						outFile << MakeOutputLine(saOutLine) << "\n";
						break;
					}

					if (m_bDataOpBytesFlag) saOutLine[FC_OPBYTES] = FormatOpBytes(nMemoryType, MC_INDIRECT, nSavedPC);
					saOutLine[FC_MNEMONIC] = FormatMnemonic(nMemoryType, MC_INDIRECT, nSavedPC);
					saOutLine[FC_OPERANDS] = FormatOperands(nMemoryType, MC_INDIRECT, nSavedPC);
					saOutLine[FC_COMMENT] = FormatComments(nMemoryType, MC_INDIRECT, nSavedPC);
					outFile << MakeOutputLine(saOutLine) << "\n";
					break;

				default:
					bDone = true;
			}
		}

		bRetVal = bRetVal && WritePostDataSection(nMemoryType, memBlock, outFile, msgFile, errFile);
	}

	return bRetVal;
}

bool CDisassembler::WritePostDataSection(MEMORY_TYPE nMemoryType, const CMemBlock &memBlock, std::ostream& outFile, std::ostream *msgFile, std::ostream *errFile)
{
	UNUSED(nMemoryType);
	UNUSED(memBlock);
	UNUSED(outFile);
	UNUSED(msgFile);
	UNUSED(errFile);
	return true;		// Don't do anything by default
}

// ----------------------------------------------------------------------------

bool CDisassembler::WritePreCodeSection(MEMORY_TYPE nMemoryType, const CMemBlock &memBlock, std::ostream& outFile, std::ostream *msgFile, std::ostream *errFile)
{
	UNUSED(nMemoryType);
	UNUSED(memBlock);
	UNUSED(outFile);
	UNUSED(msgFile);
	UNUSED(errFile);
	return true;		// Don't do anything by default
}

bool CDisassembler::WriteCodeSection(MEMORY_TYPE nMemoryType, const CMemBlock &memBlock, std::ostream& outFile, std::ostream *msgFile, std::ostream *errFile)
{
	bool bRetVal;
	CStringArray saOutLine;
	bool bDone;
	TAddress nSavedPC;
	bool bInFunc;
	bool bLastFlag;
	bool bBranchOutFlag;

	auto const &&fnWriteLabels = [&](TAddress nAddress)->void {
		CLabelTableMap::const_iterator itrLabels = m_LabelTable[nMemoryType].find(nAddress);
		if (itrLabels != m_LabelTable[nMemoryType].cend()) {
			for (CLabelArray::size_type i=1; i<itrLabels->second.size(); ++i) {
				saOutLine[FC_LABEL] = FormatLabel(nMemoryType, LC_CODE, itrLabels->second.at(i), nAddress);
				outFile << MakeOutputLine(saOutLine) << "\n";
			}
			if (itrLabels->second.size()) {
				saOutLine[FC_LABEL] = FormatLabel(nMemoryType, LC_CODE, itrLabels->second.at(0), nAddress);
				if (m_bVBreakCodeLabels && (saOutLine[FC_LABEL].size() >= static_cast<size_t>(GetFieldWidth(FC_LABEL)))) saOutLine[FC_LABEL] += '\v';
			}
		}
	};

	ClearOutputLine(saOutLine);

	bRetVal = WritePreCodeSection(nMemoryType, memBlock, outFile, msgFile, errFile);
	if (bRetVal) {
		bInFunc = false;
		bDone = false;

		TAddress nLastAddr = memBlock.logicalAddr() + memBlock.size();
		while ((m_PC < nLastAddr) && !bDone && bRetVal) {
			bLastFlag = false;
			bBranchOutFlag = false;

			// Check for function start flags:
			CFuncEntryMap::const_iterator itrFunctionEntry = m_FunctionEntryTable.find(m_PC);
			if (itrFunctionEntry != m_FunctionEntryTable.cend()) {
				switch (itrFunctionEntry->second) {

					case FUNCF_BRANCHIN:
						if (bInFunc) {
							bRetVal = bRetVal && WriteIntraFunctionSep(nMemoryType, outFile, msgFile, errFile);
							break;		// Continue if already inside a function (after printing a soft-break)
						}
						// Else, fall-through and setup for new function:
					case FUNCF_ENTRY:
					case FUNCF_INDIRECT:
					case FUNCF_CALL:
						bRetVal = bRetVal && WritePreFunction(nMemoryType, outFile, msgFile, errFile);
						bInFunc = true;
						break;

					default:
						assert(false);			// Unexpected Function Flag!!  Check Code!!
						bRetVal = false;
						continue;
				}
			}

			// Check for function end flags:
			//	This has to be done after the start flags above so
			//	that BranchIn works correctly
			CFuncExitMap::const_iterator itrFunctionExit = m_FunctionExitTable.find(m_PC);
			if (itrFunctionExit != m_FunctionExitTable.cend()) {
				switch (itrFunctionExit->second) {
					case FUNCF_HARDSTOP:
					case FUNCF_EXITBRANCH:
					case FUNCF_SOFTSTOP:
						bLastFlag = true;
						bInFunc = false;
						break;

					case FUNCF_BRANCHOUT:
						bBranchOutFlag = true;
						break;

					default:
						assert(false);			// Unexpected Function Flag!!  Check Code!!
						bRetVal = false;
						continue;
				}
			}

			ClearOutputLine(saOutLine);
			saOutLine[FC_ADDRESS] = FormatAddress(m_PC);

			nSavedPC = m_PC;		// Keep a copy of the PC for this line because some calls will be incrementing our m_PC
			switch (m_Memory[nMemoryType].descriptor(m_PC)) {
				case DMEM_ILLEGALCODE:
					fnWriteLabels(m_PC);	// Must do this before bumping m_PC
					clearOpMemory();
					for (size_t ndx = 0; ndx < opcodeSymbolSize(); ++ndx) {		// Must be a multiple of the opcode symbol size
						pushBackOpMemory(m_PC, m_Memory[nMemoryType].element(m_PC));
						++m_PC;
					}
					saOutLine[FC_OPBYTES] = FormatOpBytes(nMemoryType, MC_ILLOP, nSavedPC);
					saOutLine[FC_MNEMONIC] = FormatMnemonic(nMemoryType, MC_ILLOP, nSavedPC);
					saOutLine[FC_OPERANDS] = FormatOperands(nMemoryType, MC_ILLOP, nSavedPC);
					saOutLine[FC_COMMENT] = FormatComments(nMemoryType, MC_ILLOP, nSavedPC);
					outFile << MakeOutputLine(saOutLine) << "\n";
					break;
				case DMEM_CODE:
					fnWriteLabels(m_PC);	// Must do this before bumping m_PC
					// If the following call returns false, that means that we erroneously
					//	detected code in the first pass so instead of throwing an error or
					//	causing problems, we will treat it as an illegal opcode:
					if (ReadNextObj(nMemoryType, false, msgFile, errFile) == false) {		// Bumps PC and update m_OpMemory
						// Flag it as illegal and then process it as such:
						for (size_t ndx = 0; ndx < opcodeSymbolSize(); ++ndx) {		// Must be a multiple of the opcode symbol size
							m_Memory[nMemoryType].setDescriptor(nSavedPC+ndx, DMEM_ILLEGALCODE);
						}
						saOutLine[FC_OPBYTES] = FormatOpBytes(nMemoryType, MC_ILLOP, nSavedPC);
						saOutLine[FC_MNEMONIC] = FormatMnemonic(nMemoryType, MC_ILLOP, nSavedPC);
						saOutLine[FC_OPERANDS] = FormatOperands(nMemoryType, MC_ILLOP, nSavedPC);
						saOutLine[FC_COMMENT] = FormatComments(nMemoryType, MC_ILLOP, nSavedPC);
						outFile << MakeOutputLine(saOutLine) << "\n";
						break;
					}
					saOutLine[FC_OPBYTES] = FormatOpBytes(nMemoryType, MC_OPCODE, nSavedPC);
					saOutLine[FC_MNEMONIC] = FormatMnemonic(nMemoryType, MC_OPCODE, nSavedPC);
					saOutLine[FC_OPERANDS] = FormatOperands(nMemoryType, MC_OPCODE, nSavedPC);
					saOutLine[FC_COMMENT] = FormatComments(nMemoryType, MC_OPCODE, nSavedPC);
					outFile << MakeOutputLine(saOutLine) << "\n";
					break;
				default:
					bDone = true;
			}

			if (bBranchOutFlag) {
				itrFunctionEntry = m_FunctionEntryTable.find(m_PC);
				if (itrFunctionEntry != m_FunctionEntryTable.cend()) {
					switch (itrFunctionEntry->second) {
						case FUNCF_ENTRY:
						case FUNCF_INDIRECT:
						case FUNCF_CALL:
							bInFunc = false;
							bLastFlag = true;
							break;
						default:
							break;
					}
				} else {
					bInFunc = false;
					bLastFlag = true;
				}
			}

			if (bLastFlag) {
				itrFunctionEntry = m_FunctionEntryTable.find(m_PC);
				if (itrFunctionEntry != m_FunctionEntryTable.cend()) {
					switch (itrFunctionEntry->second) {
						case FUNCF_BRANCHIN:
							bLastFlag = false;
							bInFunc = true;
							break;
						default:
							break;
					}
				}
			}

			if (/*(bInFunc) || */ (bLastFlag)) {
				bRetVal = bRetVal & WritePostFunction(nMemoryType, outFile, msgFile, errFile);
			}
		}

		bRetVal = bRetVal && WritePostCodeSection(nMemoryType, memBlock, outFile, msgFile, errFile);
	}

	return bRetVal;
}

bool CDisassembler::WritePostCodeSection(MEMORY_TYPE nMemoryType, const CMemBlock &memBlock, std::ostream& outFile, std::ostream *msgFile, std::ostream *errFile)
{
	UNUSED(nMemoryType);
	UNUSED(memBlock);
	UNUSED(outFile);
	UNUSED(msgFile);
	UNUSED(errFile);
	return true;		// Don't do anything by default
}

// ----------------------------------------------------------------------------

bool CDisassembler::WritePreFunction(MEMORY_TYPE nMemoryType, std::ostream& outFile, std::ostream *msgFile, std::ostream *errFile)
{
	UNUSED(nMemoryType);
	UNUSED(msgFile);
	UNUSED(errFile);

	// Default is to write a newline and '=' separator bar
	CStringArray saOutLine;

	ClearOutputLine(saOutLine);
	saOutLine[FC_ADDRESS] = FormatAddress(m_PC);
	outFile << MakeOutputLine(saOutLine) << "\n";

	saOutLine[FC_LABEL] = GetCommentStartDelim();
	saOutLine[FC_LABEL] += " ";
	for (int i=(GetFieldWidth(FC_LABEL)+
			GetFieldWidth(FC_MNEMONIC)+
			GetFieldWidth(FC_OPERANDS)); i; --i) saOutLine[FC_LABEL] += "=";
	saOutLine[FC_LABEL] += " ";
	saOutLine[FC_LABEL] += GetCommentEndDelim();
	outFile << MakeOutputLine(saOutLine) << "\n";

	return true;
}

bool CDisassembler::WriteIntraFunctionSep(MEMORY_TYPE nMemoryType, std::ostream& outFile, std::ostream *msgFile, std::ostream *errFile)
{
	UNUSED(nMemoryType);
	UNUSED(msgFile);
	UNUSED(errFile);

	// Default is to write a '-' separator bar
	CStringArray saOutLine;

	ClearOutputLine(saOutLine);
	saOutLine[FC_ADDRESS] = FormatAddress(m_PC);
	saOutLine[FC_LABEL] = GetCommentStartDelim();
	saOutLine[FC_LABEL] += " ";
	for (int i=(GetFieldWidth(FC_LABEL)+
			GetFieldWidth(FC_MNEMONIC)+
			GetFieldWidth(FC_OPERANDS)); i; --i) saOutLine[FC_LABEL] += "-";
	saOutLine[FC_LABEL] += " ";
	saOutLine[FC_LABEL] += GetCommentEndDelim();
	outFile << MakeOutputLine(saOutLine) << "\n";

	return true;
}

bool CDisassembler::WritePostFunction(MEMORY_TYPE nMemoryType, std::ostream& outFile, std::ostream *msgFile, std::ostream *errFile)
{
	UNUSED(nMemoryType);
	UNUSED(msgFile);
	UNUSED(errFile);

	// Default is to write a '=' separator bar and newline
	CStringArray saOutLine;

	ClearOutputLine(saOutLine);
	saOutLine[FC_ADDRESS] = FormatAddress(m_PC);
	saOutLine[FC_LABEL] = GetCommentStartDelim();
	saOutLine[FC_LABEL] += " ";
	for (int i=(GetFieldWidth(FC_LABEL)+
			GetFieldWidth(FC_MNEMONIC)+
			GetFieldWidth(FC_OPERANDS)); i; --i) saOutLine[FC_LABEL] += "=";
	saOutLine[FC_LABEL] += " ";
	saOutLine[FC_LABEL] += GetCommentEndDelim();
	outFile << MakeOutputLine(saOutLine) << "\n";

	ClearOutputLine(saOutLine);
	saOutLine[FC_ADDRESS] = FormatAddress(m_PC);
	outFile << MakeOutputLine(saOutLine) << "\n";

	return true;
}

// ----------------------------------------------------------------------------

bool CDisassembler::ValidateLabelName(const TLabel & aName)
{
	if (aName.empty()) return false;					// Must have a length
	if ((!isalpha(aName.at(0))) && (aName.at(0) != '_')) return false;	// Must start with alpha or '_'
	for (TLabel::size_type i=1; i<aName.size(); ++i) {
		if ((!isalnum(aName.at(i))) && (aName.at(i) != '_')) return false;	// Each character must be alphanumeric or '_'
	}
	return true;
}

// ----------------------------------------------------------------------------

bool CDisassembler::IsAddressLoaded(MEMORY_TYPE nMemoryType, TAddress nAddress, TSize nSize)
{
	bool bRetVal = true;

	for (TSize i=0; ((i<nSize) && (bRetVal)); ++i) {
		bRetVal = bRetVal && (m_Memory[nMemoryType].descriptor(nAddress + i) != DMEM_NOTLOADED);
	}
	return bRetVal;
}

bool CDisassembler::IsAddressInRange(MEMORY_TYPE nRange, TAddress nAddress)
{
	return m_MemoryRanges[nRange].addressInRange(nAddress);
}

// ----------------------------------------------------------------------------

bool CDisassembler::AddLabel(MEMORY_TYPE nMemoryType, TAddress nAddress, bool bAddRef, TAddress nRefAddress, const TLabel & strLabel)
{
	CLabelTableMap::iterator itrLabelTable = m_LabelTable[nMemoryType].find(nAddress);
	CAddressTableMap::iterator itrRefTable = m_LabelRefTable[nMemoryType].find(nAddress);
	if (itrLabelTable != m_LabelTable[nMemoryType].end()) {
		if (itrRefTable == m_LabelRefTable[nMemoryType].end()) {
			assert(false);			// We should have an array for both label strings and addresses -- check adding!
			return false;
		}
		if (bAddRef) {				// Search and add reference, even if we find the string
			bool bFound = false;
			for (CAddressArray::size_type i=0; i<itrRefTable->second.size(); ++i) {
				if (itrRefTable->second.at(i) == nRefAddress) {
					bFound = true;
					break;
				}
			}
			if (!bFound) itrRefTable->second.push_back(nRefAddress);
		}
		for (unsigned int i=0; i<itrLabelTable->second.size(); ++i) {
			if (compareNoCase(strLabel, itrLabelTable->second.at(i)) == 0) return false;
		}
	} else {
		assert(itrRefTable == m_LabelRefTable[nMemoryType].end());		// If we don't have an entry in the label tabel, we shouldn't have any reference label table
		m_LabelTable[nMemoryType][nAddress] = CLabelArray();
		itrLabelTable = m_LabelTable[nMemoryType].find(nAddress);
		m_LabelRefTable[nMemoryType][nAddress] = CAddressArray();
		itrRefTable = m_LabelRefTable[nMemoryType].find(nAddress);
		if (bAddRef) {				// If we are just now adding the label, we can add the ref without searching because it doesn't exist either
			itrRefTable->second.push_back(nRefAddress);
		}
	}
	if (itrLabelTable->second.size()) {
		if (!strLabel.empty()) {
			for (CLabelArray::iterator itrLabels = itrLabelTable->second.begin(); itrLabels != itrLabelTable->second.end(); ++itrLabels) {
				if (itrLabels->empty()) {
					itrLabelTable->second.erase(itrLabels);
					break;							// If adding a non-Lxxxx entry, remove previous Lxxxx entry if it exists!
				}
			}
		} else {
			return false;		// Don't set Lxxxx entry if we already have at least 1 label!
		}
	}
	itrLabelTable->second.push_back(strLabel);
	return true;
}

bool CDisassembler::AddBranch(TAddress nAddress, bool bAddRef, TAddress nRefAddress)
{
	const MEMORY_TYPE nMemType = MT_ROM;

	CAddressTableMap::iterator itrRefList = m_BranchTable.find(nAddress);
	if (itrRefList == m_BranchTable.end()) {
		m_BranchTable[nAddress] = CAddressArray();
		itrRefList = m_BranchTable.find(nAddress);
		assert(itrRefList != m_BranchTable.end());
	}
	if (bAddRef) {				// Search and add reference
		bool bFound = false;
		for (unsigned int i = 0; i < itrRefList->second.size(); ++i) {
			if (itrRefList->second.at(i) == nRefAddress) {
				bFound = true;
				break;
			}
		}
		if (!bFound) itrRefList->second.push_back(nRefAddress);
	}
	return IsAddressLoaded(nMemType, nAddress, 1);
}

bool CDisassembler::AddComment(MEMORY_TYPE nMemoryType, TAddress nAddress, const CComment &strComment)
{
	CCommentTableMap::iterator itrComments = m_CommentTable[nMemoryType].find(nAddress);
	if (itrComments == m_CommentTable[nMemoryType].end()) {
		m_CommentTable[nMemoryType][nAddress] = CCommentArray();
		itrComments = m_CommentTable[nMemoryType].find(nAddress);
		assert(itrComments != m_CommentTable[nMemoryType].end());
	}
	itrComments->second.push_back(strComment);
	return true;
}

// ----------------------------------------------------------------------------

bool CDisassembler::AddEntry(TAddress nAddress)
{
	m_EntryTable.insert(nAddress);					// Add an entry to the entry table
	m_FunctionEntryTable[nAddress] = FUNCF_ENTRY;	// Entries are also considered start-of functions
	return true;
}

bool CDisassembler::AddCodeIndirect(TAddress nAddress, const TLabel &strLabel)
{
	m_CodeIndirectTable[nAddress] = strLabel;
	return true;
}

bool CDisassembler::AddDataIndirect(TAddress nAddress, const TLabel &strLabel)
{
	m_DataIndirectTable[nAddress] = strLabel;
	return true;
}

// ----------------------------------------------------------------------------

void CDisassembler::GenDataLabel(MEMORY_TYPE nMemoryType, TAddress nAddress, TAddress nRefAddress, const TLabel & strLabel, std::ostream *msgFile, std::ostream *errFile)
{
	UNUSED(strLabel);
	UNUSED(errFile);
	if (AddLabel(nMemoryType, nAddress, true, nRefAddress)) OutputGenLabel(nMemoryType, nAddress, msgFile);
}

void CDisassembler::GenAddrLabel(TAddress nAddress, TAddress nRefAddress, const TLabel & strLabel, std::ostream *msgFile, std::ostream *errFile)
{
	const MEMORY_TYPE nMemType = MT_ROM;

	UNUSED(strLabel);
	if (AddLabel(nMemType, nAddress, false, nRefAddress)) OutputGenLabel(nMemType, nAddress, msgFile);
	if (!AddBranch(nAddress, true, nRefAddress)) {
		if (errFile) {
			if ((errFile == msgFile) || (m_LAdrDplyCnt != 0)) {
				(*errFile) << "\n";
				m_LAdrDplyCnt = 0;
			}
			(*errFile) << "     *** Warning:  Branch Ref: "
						<< GetHexDelim() << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << nAddress
						<< std::nouppercase << std::setbase(0) << " is outside of Loaded Source File.\n";
		}
	}
}

void CDisassembler::OutputGenLabel(MEMORY_TYPE nMemoryType, TAddress nAddress, std::ostream *msgFile)
{
	std::string strTemp;

	if (msgFile == nullptr) return;
	strTemp = GenLabel(nMemoryType, nAddress);
	strTemp += ' ';
	if (strTemp.size() < 7) {
		strTemp += "       ";
		strTemp = strTemp.substr(0, 7);
	}
	if (m_LAdrDplyCnt >= 9) {
		std::size_t nPos = strTemp.find(' ');
		if (nPos != std::string::npos) strTemp = strTemp.substr(0, nPos);
		strTemp += '\n';
		m_LAdrDplyCnt=0;
	} else {
		++m_LAdrDplyCnt;
	}

	(*msgFile) << strTemp;
}

TLabel CDisassembler::GenLabel(MEMORY_TYPE nMemoryType, TAddress nAddress)
{
	UNUSED(nMemoryType);

	std::ostringstream sstrTemp;

	sstrTemp << "L" << std::uppercase << std::setfill('0') << std::setw(4) << std::setbase(16) << nAddress;
	return sstrTemp.str();
}

// ----------------------------------------------------------------------------

CDisassembler::MEMORY_TYPE CDisassembler::MemoryTypeFromAddress(TAddress nAddress)
{
	MEMORY_TYPE nType = MT_ROM;		// Default back to ROM

	for (int nMemType = 0; nMemType < NUM_MEMORY_TYPES; ++nMemType) {
		if (IsAddressInRange(static_cast<MEMORY_TYPE>(nMemType), nAddress)) {
			nType = static_cast<MEMORY_TYPE>(nMemType);
			break;
		}
	}

	return nType;
}

// ============================================================================

CDisassemblers::CDisassemblers()
{
}

CDisassemblers::~CDisassemblers()
{
}

CDisassembler *CDisassemblers::locateDisassembler(const std::string &strGDCName) const
{
	for (unsigned int ndx = 0; ndx < size(); ++ndx) {
		if (compareNoCase(strGDCName, at(ndx)->GetGDCShortName()) == 0) return at(ndx);
	}
	return nullptr;
}

void CDisassemblers::registerDisassembler(CDisassembler *pDisassembler)
{
	push_back(pDisassembler);
}

// ============================================================================
