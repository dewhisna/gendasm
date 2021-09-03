//
//	Generic Disassembly Class
//
//
//	Generic Code-Seeking Disassembler
//	Copyright(c)2021 by Donna Whisnant
//

#ifndef GENERIC_DISASSEMBLY_CLASS_H_
#define GENERIC_DISASSEMBLY_CLASS_H_

// ============================================================================

#include "memclass.h"
#include "stringhelp.h"
#include "enumhelp.h"

#include <stdint.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <utility>

#include <assert.h>

// ============================================================================

class ifstreamControlFile : public std::ifstream
{
public:
	ifstreamControlFile(const std::string &strFilename)
		:	std::ifstream(),
			m_strFilename(strFilename)
	{
		open(strFilename.c_str(), std::ios_base::in);
	}

	std::string getFilename() const { return m_strFilename; }

private:
	std::string m_strFilename;
};

// ============================================================================

typedef std::string TLabel;									// Basic Label type (should support basic_string)
typedef std::string TMnemonic;								// Basic Mnemonic type (should support basic_string)

typedef std::vector<TLabel> CLabelArray;					// Array of labels
typedef std::vector<TAddress> CAddressArray;				// Array of addresses
typedef std::set<TAddress> CAddressSet;						// Set of addresses as collection of addresses discovered
typedef std::map<TAddress, TAddress> CAddressMap;			// Mapping of Address to Address (used for objects where each member of the object points to the base address where the label is available)
typedef std::map<TAddress, CAddressArray> CAddressTableMap;	// Mapping of Address to Array of Addresses
typedef std::map<TAddress, TLabel> CAddressLabelMap;		// Mapping of Address to single Label
typedef std::map<TAddress, CLabelArray> CLabelTableMap;		// Mapping of Address to Array of Labels

// ----------------------------------------------------------------------------

// Type definitions for a specific disassembler:
template <	typename TOpcodeSymbol_,
			typename TGroupFlags_,
			typename TControlFlags_>
class TDisassemblerTypes
{
public:
	typedef TOpcodeSymbol_ TOpcodeSymbol;						// Opcode symbol type (should be numeric)
	typedef TGroupFlags_ TGroupFlags;							// Group Flags type (should be numeric)
	typedef TControlFlags_ TControlFlags;						// Control Flags type (should be numeric)
	// --------------------------------
	typedef std::vector<TOpcodeSymbol> COpcodeSymbolArray;		// Array of bytes or symbols in complete opcode


	// ----------------------------------------------------------------------------

	// The following bit-mask definitions define the bits in the m_Control field
	//	of each COpcodeEntry object that are processor independent and must be
	//	properly used in each opcode defined by all overrides (GDC's).  This is
	//	because these are used to control the overall disassembly operation.
	//	All other bit-fields in m_Control can be defined and used as needed by
	//	each override.  You will notice that the bit-fields in these definitions
	//	start on the upper-bit of the upper-byte in the TControlFlags type.
	//	It is recommended that processor specific additions are made starting
	//	at the other end -- that is the lower-bit of the lower-byte...  This
	//	will allow the insertion of additional processor independent bits at the
	//	top end in future updates and releases without necessarily running into
	//	and conflicting with those defined by overrides.  It is futher suggested
	//	that the entire upper-word be reserved for processor independent functions
	//	and the entire lower-word be reserved for dependent functions.  Also note
	//	that the usage need not be bit-by-bit only, but can be numerical values
	//	occupying certain bit positions -- with proper "and/or" masks and
	//	bit-shifting these can easily be converted to/from bit-field equivalents.
	//	(Names use OCTL_xxxx form)
	static constexpr TControlFlags OCTL_STOP = static_cast<TControlFlags>(0x80000000ul);		// Discontinue Disassembly - Upper-bit = 1
	static constexpr TControlFlags OCTL_HARDSTOP = static_cast<TControlFlags>(0x40000000ul);	// Discontinue Disassembly and Flag for functions that are hard stops (like rts, rti, ret, reti, etc, but NOT jmp, call, etc.)

	// Currently, all m_Group entries are used exclusively by processor
	//	dependent functions.  But, it too should follow the guidelines above
	//	so that in the future, if the group field is needed by the processor
	//	independent functions, then they can be added in without changing
	//	anything.  (Names use OGRP_xxxx form)

};

// ============================================================================

//	COpcodeEntry
//		This specifies exactly one opcode for the target processor.
template<typename TDisassembler>
class COpcodeEntry
{
public:
	typedef bool (*TOpcodeMatchFunc)(const COpcodeEntry &anOpcode,
									 const typename TDisassembler::COpcodeSymbolArray &arrOpMemory);

	COpcodeEntry()
	{ }
	COpcodeEntry(	const typename TDisassembler::COpcodeSymbolArray &opcode,
					const typename TDisassembler::COpcodeSymbolArray &opcodeMask,
					const typename TDisassembler::TGroupFlags &group,
					const typename TDisassembler::TControlFlags &control,
					const TMnemonic &mnemonic,
					const TOpcodeMatchFunc fncMatch = nullptr,
					const TUserData &userdata = {})
		:	m_Opcode(opcode),
			m_OpcodeMask(opcodeMask),
			m_Group(group),
			m_Control(control),
			m_Mnemonic(mnemonic),
			m_fncMatch(fncMatch),
			m_UserData(userdata)
	{ }

	const typename TDisassembler::COpcodeSymbolArray &opcode() const { return m_Opcode; }
	void setOpcode(const typename TDisassembler::COpcodeSymbolArray &opcode) { m_Opcode = opcode; }

	const typename TDisassembler::COpcodeSymbolArray &opcodeMask() const { return m_OpcodeMask; }
	void setOpcodeMask(const typename TDisassembler::COpcodeSymbolArray &opcodeMask) { m_OpcodeMask = opcodeMask; }

	typename TDisassembler::TGroupFlags group() const { return m_Group; }
	void setGroup(const typename TDisassembler::TGroupFlags &group) { m_Group = group; }

	typename TDisassembler::TControlFlags control() const { return m_Control; }
	void setControl(const typename TDisassembler::TControlFlags &control) { m_Control = control; }

	TMnemonic mnemonic() const { return m_Mnemonic; }
	void setMnemonic(const TMnemonic &mnemonic) { m_Mnemonic = mnemonic; }

	TOpcodeMatchFunc matchFunc() const { return m_fncMatch; }
	void setMatchFunc(const TOpcodeMatchFunc fncMatch) { m_fncMatch = fncMatch; }

	TUserData userData() const { return m_UserData; }
	void setUserData(const TUserData &userdata) { m_UserData = userdata; }

private:
	typename TDisassembler::COpcodeSymbolArray	m_Opcode = {};			// Array of opcode symbols for this entry
	typename TDisassembler::COpcodeSymbolArray	m_OpcodeMask = {};		// Array of opcode symbol masks for this entry
	typename TDisassembler::TGroupFlags			m_Group = {};			// Group declaration
	typename TDisassembler::TControlFlags		m_Control = {};			// Flow control
	TMnemonic									m_Mnemonic = {};		// Printed mnemonic
	TOpcodeMatchFunc							m_fncMatch = nullptr;	// Optional matching function for this opcode
	TUserData									m_UserData = {};		// User defined addition data storage
};

template<typename TDisassembler> using COpcodeEntryArray = std::vector< COpcodeEntry<TDisassembler> >;


// ============================================================================

// COpcodeTableArray
//		This class manages a single table of opcodes by properly
//		adding and maintaining opcode arrays.  This version uses a
//		single flat array (unlike COpcodeTableMap which indexes by
//		first TOpcodeSymbol first).
//		It is also the users responsibility to not add duplicate or
//		conflicting opcodes!
template<typename TDisassembler>
class COpcodeTableArray : public COpcodeEntryArray<TDisassembler>
{
	using array_type = COpcodeEntryArray<TDisassembler>;
public:
	void AddOpcode(const COpcodeEntry<TDisassembler> & entry)
	{
		if (entry.opcode().empty() || entry.opcodeMask().empty()) {
			assert(false);
			return;
		}
		if (entry.opcode().size() != entry.opcodeMask().size()) {
			assert(false);
			return;
		}

		array_type::push_back(entry);
	}
};


//	COpcodeTableMap
//		This class manages a single table of opcodes by properly
//		adding and maintaining opcode arrays.  This class is a
//		mapping of opcode 'first symbols' to an array of COpcodeEntryArray.
//		When adding a new opcode, the AddOpcode function first checks to
//		see if an array exists for its 'first symbol', if not it creates it.
//		Then it adds the opcode either to the new array or the existing array.
//		Note that since arrays are used, iteration returns back the opcodes
//		for a 'first symbol' in the SAME order they were added.  Currently,
//		there is only an "Add" function and no remove!
//		It is also the users responsibility to not add duplicate or
//		conflicting opcodes!
template<typename TDisassembler>
class COpcodeTableMap : public std::map<typename TDisassembler::TOpcodeSymbol, COpcodeEntryArray<TDisassembler> >
{
	using map_type = std::map<typename TDisassembler::TOpcodeSymbol, COpcodeEntryArray<TDisassembler> >;
public:
	void AddOpcode(const COpcodeEntry<TDisassembler> & entry)
	{
		if (entry.opcode().empty() || entry.opcodeMask().empty()) {
			assert(false);
			return;
		}
		if (entry.opcode().size() != entry.opcodeMask().size()) {
			assert(false);
			return;
		}

		typename map_type::iterator itrArray = map_type::find(entry.opcode().at(0) & entry.opcodeMask().at(0));
		if (itrArray == map_type::end()) {
			COpcodeEntryArray<TDisassembler> anArray;
			anArray.push_back(entry);
			map_type::insert({ entry.opcode().at(0) & entry.opcodeMask().at(0), anArray});
		} else {
			itrArray->second.push_back(entry);
		}
	}
};

// ============================================================================

// CDisassemblerData
//		This class defines the data to store for the disassembler class.
//		It's kept separate so that it can be fully templated from the
//		origin disassembler typing and kept separate from the core
//		CDisassembler virtual base logic
template<typename TDisassemblerClass, typename TDisassembler, typename TOpcodeTable>
class CDisassemblerData
{
private:
	using TOpcodeTable_type = TOpcodeTable;
	using COpcodeSymbolArray_type = typename TDisassembler::COpcodeSymbolArray;

	friend TDisassemblerClass;

	TOpcodeTable m_Opcodes;		// Table of opcodes for this disassembler


	typename TDisassembler::COpcodeSymbolArray m_OpMemory;		// Array to hold the opcode currently being processed
	COpcodeEntry<TDisassembler> m_CurrentOpcode;				// Copy of COpcodeEntry object of the opcode last located in the ReadNextObj function
};

// ============================================================================

//	CDisassembler
//		This class describes a generic disassembler class to handle disassembly.
//		It contains no processor dependent information, therefore this class
//		must be overriden to handle processor dependent operations.  It is also
//		setup to use the DFC converters and memory objects so that these
//		functions are not dependent on input file types.
//
//		In the override, the constructor and destructor must be
//		defined to setup opcodes and memory for the target processor!
//
//		The function "ReadControlFile" takes an already open ifstreamControlFile
//		object and reads in the control file.  The base function calls the
//		function 'ParseControlLine' for every line in the input file (comment
//		lines are automatically removed).  The override (if any) should
//		return TRUE if all is OK with the line being processed, otherwise,
//		it should set the m_ParseError with an error message and return FALSE.
//		The default message, if not set, for m_ParseError is "*** Error: Unknown error in line xxx".
//		The ReadControlFile routine will automatically output the control
//		file line number information for each m_ParseError message.
//		If default functionality is desired for the line, then the override
//		should call the CDisassembler::ParseControlLine function -- that way,
//		for the normally accepted options, the override doesn't have to worry
//		about processing them.  Plus, it gives the override a chance to create
//		new behavior for each command.
//
//		When branches are encountered, they are added to the m_BranchTable
//		map -- m_BranchTable is a map of arrays -- each array is a CAddressArray that
//		contains every address that referenced the branch.  So, when a new branch is
//		found, the array is created -- when a branch is re-encountered, the address
//		that referenced the branch is added...
//
//		NOTE: Code references are added to the m_BranchTable arrays and data references
//		are added to the m_LabelRefTable arrays!!  This way, not only can we track
//		who referenced a particular address, but we can track how it was referenced.
//		All overrides should conform to this by making sure they add references to the
//		right place.  AddBranch function handles code references and AddLabel handles
//		data references.  When adding labels to code, add the reference using either
//		AddBranch (then call AddLabel without the reference) or
//		GenAddrLabel (which calls both AddBranch and AddLabel correctly).  When adding
//		data references, use either AddLabel or call GenDataLabel (GenDataLabel also
//		handles outputting of generated labels).
//
class CDisassembler
{
public:
	// The following define the function identification flags:
	enum FUNC_EXIT_FLAGS {
		FUNCF_HARDSTOP = 0,			// Hard Stop = RTS or RTI encountered
		FUNCF_EXITBRANCH = 1,		// Exit Branch = Branch to a specified exit address
		FUNCF_BRANCHOUT = 2,		// Execution branched out (JMP, BRA, etc)
		FUNCF_SOFTSTOP = 3,			// Soft Stop = Code execution flowed into another function definition
	};

	using CFuncExitMap = std::map<TAddress, FUNC_EXIT_FLAGS>;

	enum FUNC_ENTRY_FLAGS {
		FUNCF_ENTRY = 0,			// Entry Point Start
		FUNCF_INDIRECT = 1,			// Indirect Code Entry Point Start
		FUNCF_CALL = 2,				// Call Start Point (JSR, BSR, etc)
		FUNCF_BRANCHIN = 3,			// Entry from a branch into (BEQ, BNE, etc)
	};

	using CFuncEntryMap = std::map<TAddress, FUNC_ENTRY_FLAGS>;

	// The following define the memory descriptors for the disassembler memory.
	//		0 = The memory is unused/not-loaded
	//		10 = The memory is loaded, but hasn't been processed
	//		20 = The memory is loaded, has been processed, and found to be data, but not printable ASCII data
	//		21 = The memory is loaded, has been processed, and found to be printable ASCII data
	//		30 = The memory is loaded, has been processed, and is an indirect vector to Code
	//		31 = The memory is loaded, has been processed, and is an indirect vector to Data
	//		40 = The memory is loaded, has been processed, and found to be code
	//		41 = The memory is loaded, has been processed, and should be code, but was determined to be an illegal opcode
	//		50 = The memory is allocated, has been processed, but has no data associated with it
	//	NOTE: These should NOT be changed -- enough room has been allowed for future expansion!
	//		(Changing them will break functions like ResolveIndirect, for example)
	enum MEM_DESC {
		DMEM_NOTLOADED = 0,
		DMEM_LOADED = 10,
		DMEM_DATA = 20,
		DMEM_PRINTDATA = 21,
		DMEM_CODEINDIRECT = 30,
		DMEM_DATAINDIRECT = 31,
		DMEM_CODE = 40,
		DMEM_ILLEGALCODE = 41,
		DMEM_ALLOC = 50,
	};

	// The following define field positions for obtaining field widths from overrides.
	//	Additional entries can be added on overrides, but this order should NOT change
	//	in order to maintain backward compatibility at all times:
	enum FIELD_CODE { FC_ADDRESS, FC_OPBYTES, FC_LABEL, FC_MNEMONIC, FC_OPERANDS, FC_COMMENT, NUM_FIELD_CODES };

	// The following define mnemonic codes for use with the FormatMnemonic pure virtual
	//	function.  These are used to specify the retrieving of "special mnemonics" for
	//	things such as equates, data bytes, ascii text, etc.  Additional entries can be
	//	added on overrides, but this order should NOT change in order to maintain backward
	//	compatibility at all times:
	enum MNEMONIC_CODE { MC_OPCODE, MC_ILLOP, MC_EQUATE, MC_DATABYTE, MC_ASCII, MC_INDIRECT, NUM_MNEMONIC_CODES };

	// The following defines the label codes when formating output.  They are flags that
	//	determine where the label is being referenced.  It is used by the FormatLabel
	//	function to determine what type of delimiter to add, if any.  This enum can be
	//	added to in overrides, but the existing entries should not be changed:
	enum LABEL_CODE { LC_EQUATE, LC_DATA, LC_CODE, LC_REF, NUM_LABEL_CODES };

	// The following defines the reference types:
	enum REFERENCE_TYPE { RT_CODE, RT_DATA, NUM_REFERENCE_TYPES };

	// The following defines the memory types:
	enum MEMORY_TYPE { MT_ROM, MT_RAM, MT_IO, MT_EE, NUM_MEMORY_TYPES };

	// The following defines the commit type flags for determining when to output
	//	specific user-defined comments.  It is used by the FormatComments function.
	//	This enum can be added to in overrides, but the existing entries should not
	//	be changed.  Note: These are bit-flags.
	enum COMMENT_TYPE_FLAGS {
		CTF_NONE = 0,
		CTF_EQUATE = 1,			// Labels for things appearing outside Code and Data areas defined with equates (MC_EQUATE output)
		CTF_DATA = 2,			// Labels for things in Data (MC_DATABYTE and MC_ASCII output)
		CTF_CODE = 4,			// Labels for things in Code (MC_OPCODE and MC_ILLOP output)
		CTF_REF = 8,			// Labels for indirect references (MC_INDIRECT output)
		CTF_ALL = (CTF_EQUATE | CTF_DATA | CTF_CODE | CTF_REF)
	};

	struct CComment {
		CComment() {}
		CComment(COMMENT_TYPE_FLAGS nFlags, const std::string &strComment)
			:	m_nFlags(nFlags),
				m_strComment(strComment)
		{ }

		COMMENT_TYPE_FLAGS m_nFlags;	// Flags of where to use it
		std::string m_strComment;		// Comment text
	};

	typedef std::vector<CComment> CCommentArray;					// Array of Comments
	typedef std::map<TAddress, CCommentArray> CCommentTableMap;		// Mapping of Address or Array of Comments

public:
	CDisassembler();
	virtual ~CDisassembler();

	virtual unsigned int GetVersionNumber() const;		// Base function returns GDC version is most-significant word.  Overrides should call this parent to get the GDC version and then set the least-significant word with the specific disassembler version.
	virtual std::string GetGDCLongName() const = 0;		// Pure virtual.  Defines the long name for this disassembler
	virtual std::string GetGDCShortName() const = 0;	// Pure virtual.  Defines the short name for this disassembler

	virtual CStringArray GetMCUList() const;
	virtual bool SetMCU(const std::string &strMCUName);

	virtual CStringArray GetTargetAssemblerList() const;	// List of target assemblers
	virtual bool SetTargetAssembler(const std:: string &strTargetAssembler);

	virtual bool ReadControlFile(ifstreamControlFile& inFile, bool bLastFile = true, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr, int nStartLineCount = 0);	// Read already open control file 'infile', outputs messages to 'msgFile' and errors to 'errFile', nStartLineCount = initial m_nCtrlLine value
	virtual bool ReadSourceFile(const std::string & strFilename, TAddress nLoadAddress, const std::string & strDFCLibrary, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);	// Reads source file named strFilename using DFC strDFCLibrary
	virtual bool Disassemble(std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr, std::ostream *outFile = nullptr);		// Disassembles entire loaded memory and outputs info to outFile if non-nullptr or opens and writes the file specified by m_sOutputFilename -- calls Pass1 and Pass2 to perform this processing

	// --------------------------------

	virtual bool deterministic() const { return m_bDeterministic; }
	virtual void setDeterministic(bool bDeterministic) { m_bDeterministic = bDeterministic; }

	virtual bool flagAddr() const { return m_bAddrFlag; }
	virtual void setFlagAddr(bool bFlag) { m_bAddrFlag = bFlag; }

	virtual bool flagOpcode() const { return m_bOpcodeFlag; }
	virtual void setFlagOpcode(bool bFlag) { m_bOpcodeFlag = bFlag; }

	virtual bool flagAscii() const { return m_bAsciiFlag; }
	virtual void setFlagAscii(bool bFlag) { m_bAsciiFlag = bFlag; }

	virtual bool flagSpit() const { return m_bSpitFlag; }
	virtual void setFlagSpit(bool bFlag) { m_bSpitFlag = bFlag; }

	virtual bool flagTabs() const { return m_bTabsFlag; }
	virtual void setFlagTabs(bool bFlag) { m_bTabsFlag = bFlag; }

	virtual bool flagAsciiBytes() const { return m_bAsciiBytesFlag; }
	virtual void setFlagAsciiBytes(bool bFlag) { m_bAsciiBytesFlag = bFlag; }

	virtual bool flagDataOpBytes() const { return m_bDataOpBytesFlag; }
	virtual void setFlagDataOpBytes(bool bFlag) { m_bDataOpBytesFlag = bFlag; }

	virtual int maxNonPrint() const { return m_nMaxNonPrint; }
	virtual void setMaxNonPrint(int nValue) { m_nMaxNonPrint = nValue; }

	virtual int maxPrint() const { return m_nMaxPrint; }
	virtual void setMaxPrint(int nValue) { m_nMaxPrint = nValue; }

	virtual int tabWidth() const { return m_nTabWidth; }
	virtual void setTabWidth(int nValue) { m_nTabWidth = nValue; }

	virtual int defaultBase() const { return m_nDefaultBase; }
	virtual void setDefaultBase(int nValue) { m_nDefaultBase = nValue; }

	virtual std::string defaultDFC() const { return m_sDefaultDFC; }
	virtual void setDefaultDFC(const std::string &strDFC) { m_sDefaultDFC = strDFC; }

	virtual bool allowMemRangeOverlap() const { return m_bAllowMemRangeOverlap; }

	// ------------------------------------------------------------------------

	virtual size_t memoryTypeCount() const { return NUM_MEMORY_TYPES; }
	virtual CMemBlocks &memory(MEMORY_TYPE nMemoryType) { return m_Memory[nMemoryType]; }
	virtual CMemRanges &memoryRanges(MEMORY_TYPE nMemoryType) { return m_MemoryRanges[nMemoryType]; }

	// ------------------------------------------------------------------------

protected:
	virtual bool ParseControlLine(const std::string & strLine, const CStringArray& argv, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);		// Parses a line from the control file -- strLine is full line, argv is array of whitespace delimited args.  Should return false ONLY if ReadControlFile should print the ParseError string to errFile with line info

	virtual bool ScanSymbols(std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);		// Scans through the symbols to generate labels, indirects, comments, etc., based on symbols from the file sources (such as an elf file)
	virtual bool ScanIndirects(std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);	// Scans through the indirect lists and handles resolving and labeling them
	virtual bool ScanEntries(std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);		// Scans through the entry list looking for code
	virtual bool ScanBranches(std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);	// Scans through the branch lists looking for code
	virtual bool ScanData(const std::string & strExcludeChars, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);			// Scans through the data that remains and tags it as printable or non-printable

	virtual bool Pass1(std::ostream& outFile, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);	// Performs Pass1 which finds code and data -- i.e. calls Scan functions
	virtual bool Pass2(std::ostream& outFile, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);	// Performs Pass2 which is the actual disassemble stage
	virtual bool Pass3(std::ostream& outFile, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);	// Performs Pass3 which creates the function output file

	virtual bool FindCode(std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);		// Iterates through memory using m_PC finding and marking code.  It should add branches and labels as necessary and return when it hits an end-of-code or runs into other code found
	virtual bool ReadNextObj(MEMORY_TYPE nMemoryType, bool bTagMemory, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr) = 0;	// Pure Virtual as it's depenent on processor.  Procedure to read next object code from memory.  The memory type is flagged as code (or illegal code).  Returns True if current code legal, else False.  OpMemory = object code from memory.  CurrentOpcode = Copy of COpcodeEntry for the opcode located.  PC is automatically advanced.
	virtual bool CompleteObjRead(MEMORY_TYPE nMemoryType, bool bAddLabels = true, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr) = 0;	// Pure Virtual that finishes the opcode reading process as it's dependent on processor.  It should increment m_PC and add bytes to m_OpMemory, but it should NOT flag memory descriptors!  If the result produces an invalid opcode, the routine should return FALSE.  The ReadNextObj func will tag the memory bytes based off of m_OpMemory!  If bAddLabels = FALSE, then labels and branches aren't added -- disassemble only!
	virtual bool CurrentOpcodeIsStop() const = 0;	// Checks the m_CurrentOpcode in the DisassemblerData and returns True if the opcode's Control flags signals a 'Stop' condition.  Pure virtual because DisassemblerData is only known by derived classes

	virtual bool RetrieveIndirect(MEMORY_TYPE nMemoryType, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr) = 0;	// Pure Virtual. Retrieves the indirect address specified by m_PC and places it m_OpMemory for later formatting.  It is a pure virtual because of length and indian notation differences

	virtual std::string FormatOpBytes(MEMORY_TYPE nMemoryType, MNEMONIC_CODE nMCCode, TAddress nStartAddress) = 0;	// Pure Virtual.  This function creates a opcode byte string from the bytes in m_OpMemory.
	virtual std::string FormatMnemonic(MEMORY_TYPE nMemoryType, MNEMONIC_CODE nMCCode, TAddress nStartAddress) = 0;	// Pure Virtual.  This function should provide the specified mnemonic.  For normal opcodes, MC_OPCODE is used -- for which the override should return the mnemonic in the opcode table.  This is done to provide the assembler/processor dependent module opportunity to change/set the case of the mnemonic and to provide special opcodes.
	virtual std::string FormatOperands(MEMORY_TYPE nMemoryType, MNEMONIC_CODE nMCCode, TAddress nStartAddress) = 0;	// Pure Virtual.  This function should create the operands for the current opcode if MC_OPCODE is issued.  For others, it should format the data in m_OpMemory to the specified form!
	virtual std::string FormatComments(MEMORY_TYPE nMemoryType, MNEMONIC_CODE nMCCode, TAddress nStartAddress) = 0;	// Pure Virtual.  This function should create any needed comments for the disassembly output for the current opcode or other special MC_OPCODE function.  This is where "undetermined branches" and "out of source branches" can get flagged by the specific disassembler.  The suggested minimum is to call FormatReferences to put references in the comments.

	virtual std::string FormatAddress(TAddress nAddress) const;				// This function creates the address field of the disassembly output.  Default is "xxxx" hex value.  Override for other formats.
	virtual TAddress UnformatAddress(const std::string strAddress) const;	// Returns the Address from a formatted address (used for address increment in Opbyte wrapping of MakeOutputLine.
	virtual std::string FormatLabel(MEMORY_TYPE nMemoryType, LABEL_CODE nLC, const TLabel & strLabel, TAddress nAddress);	// This function modifies the specified label to be in the Lxxxx format for the nAddress if strLabel is null. If strLabel is not empty no changes are made.  This function should be overridden to add the correct suffix delimiters as needed!
	virtual std::string FormatReferences(MEMORY_TYPE nMemoryType, MNEMONIC_CODE nMCCode, TAddress nAddress);		// Makes a string to place in the comment field that contains all references for the specified address
	virtual std::string FormatUserComments(MEMORY_TYPE nMemoryType, MNEMONIC_CODE nMCCode, TAddress nAddress);		// Makes a string to place in the comment field that contains all user comments for the specified address
	virtual bool AddressHasUserComments(MEMORY_TYPE nMemoryType, COMMENT_TYPE_FLAGS nFlags, TAddress nAddress);		// Check the specified address to see if it has comments for the specified usage
	virtual std::string FormatFunctionFlagComments(MEMORY_TYPE nMemoryType, MNEMONIC_CODE nMCCode, TAddress nStartAddress);	// Generate debug comments from the function flags if it's enabled

	virtual TAddressOffset GetOpBytesFWAddressOffset() const;				// Returns the number of bytes to increment FC_ADDRESS by for the field width of OpcodeByte display.  That is, if OpcodeByte field width is 12 and Opbyte Output is "XX XX XX XX ", then this would return 4.
	virtual int GetFieldWidth(FIELD_CODE nFC) const;						// Defines the widths of each output field.  Can be overridden to alter output formatting.  To eliminate space/tab mixing, these should typically be a multiple of the tab width
	virtual std::string MakeOutputLine(CStringArray& saOutputData) const;	// Formats the data in saOutputData, which should have indicies corresponding to FIELD_CODE enum, to a string that can be sent to the output file
	virtual void ClearOutputLine(CStringArray& saOutputData) const;

	virtual bool WriteHeader(std::ostream& outFile, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);		// Writes the disassembly header comments to the output file.  Override in subclasses for non-default header

	virtual bool WritePreEquates(std::ostream& outFile, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);	// Writes any info needed in the disassembly file prior to the equates section.  Default is do nothing.  Override as needed
	virtual bool WritePreEquatesRange(MEMORY_TYPE nMemoryType, std::ostream& outFile, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);	// Writes any info needed in the disassembly file prior to the equates section range.  Default is do nothing.  Override as needed
	virtual bool WriteEquates(std::ostream& outFile, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);		// Writes the equates table to the disassembly file.
	virtual bool WritePostEquatesRange(MEMORY_TYPE nMemoryType, std::ostream& outFile, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);	// Writes any info needed in the disassembly file after the equates section range.  Default is do nothing.  Override as needed
	virtual bool WritePostEquates(std::ostream& outFile, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);	// Writes any info needed in the disassembly file after the equates section.  Default is do nothing.  Override as needed

	virtual bool WritePreDisassembly(MEMORY_TYPE nMemoryType, std::ostream& outFile, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);		// Writes any info needed in the disassembly file prior to the main disassembly section.  Default writes several blank lines.  Override as needed
	virtual bool WriteDisassembly(MEMORY_TYPE nMemoryType, std::ostream& outFile, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);		// Writes the disassembly to the output file.  Default iterates through memory and calls correct functions.  Override as needed.
	virtual bool WritePostDisassembly(MEMORY_TYPE nMemoryType, std::ostream& outFile, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);	// Writes any info needed in the disassembly file after the main disassembly section, such as ".end".  Default is do nothing.  Override as needed

	virtual bool WritePreSection(MEMORY_TYPE nMemoryType, const CMemBlock& memBlock, std::ostream& outFile, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);	// Writes any info needed in the disassembly file prior to a new section.  That is, a section that is loaded and analyzed.  A part of memory containing "not_loaded" areas would cause a new section to be generated.  This should be things like "org", etc.
	virtual bool WriteSection(MEMORY_TYPE nMemoryType, const CMemBlock& memBlock, std::ostream& outFile, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);		// Writes the current section to the disassembly file.  Override as needed.
	virtual bool WritePostSection(MEMORY_TYPE nMemoryType, const CMemBlock& memBlock, std::ostream& outFile, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);	// Writes any info needed in the disassembly file after a new section.  Default is do nothing, override as needed.

	virtual bool WritePreDataSection(MEMORY_TYPE nMemoryType, const CMemBlock& memBlock, std::ostream& outFile, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);	// Writes any info needed prior to a data section -- that is a section that contains only DMEM_DATA directives.  This would be things like "DSEG", etc.  Default is do nothing. Override as needed.
	virtual bool WriteDataSection(MEMORY_TYPE nMemoryType, const CMemBlock& memBlock, std::ostream& outFile, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);		// Writes a data section.
	virtual bool WritePostDataSection(MEMORY_TYPE nMemoryType, const CMemBlock& memBlock, std::ostream& outFile, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);	// Writes any info needed after a data section.  Default is do nothing.  Typically this would be something like "ends"

	virtual bool WritePreCodeSection(MEMORY_TYPE nMemoryType, const CMemBlock& memBlock, std::ostream& outFile, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);	// Writes any info needed prior to a code section -- such as "CSEG", etc.  Default is do nothing.  Override as needed.
	virtual bool WriteCodeSection(MEMORY_TYPE nMemoryType, const CMemBlock& memBlock, std::ostream& outFile, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);		// Writes a code section.
	virtual bool WritePostCodeSection(MEMORY_TYPE nMemoryType, const CMemBlock& memBlock, std::ostream& outFile, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);	// Writes any info needed after a code section.  Default is do nothing.  Typically this would be something like "ends"

	virtual bool WritePreFunction(MEMORY_TYPE nMemoryType, std::ostream& outFile, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);		// Writes any info needed prior to a new function -- such as "proc".  The default simply writes a separator line "===".  Override as needed
	virtual bool WriteIntraFunctionSep(MEMORY_TYPE nMemoryType, std::ostream& outFile, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);	// Writes a function's intra code separation.  The default simply writes a separator line "---".  Override as needed
	virtual bool WritePostFunction(MEMORY_TYPE nMemoryType, std::ostream& outFile, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);		// Writes any info needed after a function -- such as "endp".  The default simply writes a separator line "===".  Override as needed

public:
	virtual bool ValidateLabelName(const TLabel & aName);		// Parses a specified label to make sure it meets label rules

	virtual bool ResolveIndirect(MEMORY_TYPE nMemoryType, TAddress nAddress, TAddress& nResAddress, REFERENCE_TYPE nType) = 0;	// Pure Virtual that Resolves the indirect address specified by nAddress of type nType where nType is the reference type (i.e. Code locations vs. Data locations).  It returns the resolved address in nResAddress and T/F for mem load status
	//		NOTE: This ResolveIndirect function should set the m_Memory type code for the affected bytes to
	//				either DMEM_CODEINDIRECT or DMEM_DATAINDIRECT!!!!   It should set
	//				nResAddress to the resolved address and return T/F -- True if indirect
	//				vector word lies completely inside the loaded space.  False if indirect
	//				vector word (or part of it) is outside.  This function is NOT to check
	//				the destination for loaded status -- that is checked by the routine
	//				calling this function...  nType specifies if this for Code or for Data...
	//				Typically nType can be added to DMEM_CODEINDIRECT to determine the correct
	//				code to store.  This is because the DMEM enumeration was created with
	//				this expansion and uniformity in mind.
	//				It is also good for this function to check that all bytes of the indirect
	//				vector itself have previously not been "looked at" or tagged.  Routines
	//				calling this function for previously tagged addresses should reset the
	//				vector bytes to DMEM_LOADED.

	virtual bool IsAddressLoaded(MEMORY_TYPE nMemoryType, TAddress nAddress, TSize nSize);	// Checks to see if the nSize bytes starting at address nAddress are loaded.  True only if all the bytes are loaded!
	virtual bool IsAddressInRange(MEMORY_TYPE nRange, TAddress nAddress);	// Checks to see if the specified address resides in the specified range

	virtual bool AddLabel(MEMORY_TYPE nMemoryType, TAddress nAddress, bool bAddRef = false, TAddress nRefAddress = 0, const TLabel & strLabel = TLabel());		// Sets strLabel string as the label for nAddress.  If nAddress is already set with that string, returns FALSE else returns TRUE or all OK.  If address has a label and strLabel = empty or zero length, nothing is added!  If strLabel is empty or zero length, an empty string is added to later get resolved to Lxxxx form.  If bAddRef, then nRefAddress is added to the reference list
	virtual bool AddBranch(TAddress nAddress, bool bAddRef = false, TAddress nRefAddress = 0);	// (Always MT_ROM) Adds nAddress to the branch table with nRefAddress as the referring address.  Returns T if all ok, F if branch address is outside of loaded space.  If nAddRef is false, a branch is added without a reference
	virtual bool AddComment(MEMORY_TYPE nMemoryType, TAddress nAddress, const CComment &strComment);

	virtual bool HaveEntry(TAddress nAddress) const { return m_EntryTable.contains(nAddress); }
	virtual bool AddEntry(TAddress nAddress);
	virtual bool HaveCodeIndirect(TAddress nAddress) const { return m_CodeIndirectTable.contains(nAddress); }
	virtual bool AddCodeIndirect(TAddress nAddress, const TLabel &strLabel = TLabel());
	virtual bool HaveDataIndirect(TAddress nAddress) const { return m_DataIndirectTable.contains(nAddress); }
	virtual bool AddDataIndirect(TAddress nAddress, const TLabel &strLabel = TLabel());

	virtual bool AddSymbol(MEMORY_TYPE nMemoryType, TAddress nAddress, const TLabel &strLabel);
	virtual bool AddObjectMapping(MEMORY_TYPE nMemoryType, TAddress nBaseObjectAddress, TSize nSize);

protected:
	virtual void GenDataLabel(MEMORY_TYPE nMemoryType, TAddress nAddress, TAddress nRefAddress, const TLabel & strLabel = TLabel(), std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);	// Calls AddLabel to create a label for nAddress -- unlike calling direct, this function outputs the new label to msgFile if specified...
	virtual void GenAddrLabel(TAddress nAddress, TAddress nRefAddress, const TLabel & strLabel = TLabel(), std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr);	// (Always MT_ROM) Calls AddLabel to create a label for nAddress, then calls AddBranch to add a branch address -- unlike calling direct, this function outputs the new label to msgFile if specified and displays "out of source" errors for the branches to errFile...
	virtual void OutputGenLabel(MEMORY_TYPE nMemoryType, TAddress nAddress, std::ostream *msgFile);		// Outputs a generated label to the msgFile -- called by GenAddrLabel and GenDataLabel
	virtual TLabel GenLabel(MEMORY_TYPE nMemoryType, TAddress nAddress);		// Creates a default generated label.  Base form is Lxxxx, but can be overridden in derived classes for other formats

	virtual MEMORY_TYPE MemoryTypeFromAddress(TAddress nAddress);	// Searches the ranges to find the first match for the specified address -- useful for flat memory models of the non-Harvard architectures (i.e. when m_bAllowMemRangeOverlap is false)

	virtual std::string GetExcludedPrintChars() const = 0;		// Pure Virtual. Returns a list of characters excluded during datascan for printable scan
	virtual std::string GetHexDelim() const = 0;				// Pure Virtual. Returns hexadecimal delimiter for specific assembler.  Typically "0x" or "$"
	virtual std::string GetCommentStartDelim() const = 0;		// Pure Virtual. Returns start of comment delimiter string
	virtual std::string	GetCommentEndDelim() const = 0;			// Pure Virtual. Returns end of comment delimiter string

	virtual void clearOpMemory() = 0;							// Pure Virtual to clear the data->OpMemory for the processor
	virtual size_t opcodeSymbolSize() const = 0;				// Pure Virtual to return the sizeof TOpcodeSymbol or element size of m_OpMemory i.e. sizeof(TDisassemblerTypes::TOpcodeSymbol) or sizeof(decltype(m_OpMemory)::value_type)
	virtual size_t getOpMemorySize() const = 0;					// Pure Virtual to return number of elements in the OpMemory array
	virtual void pushBackOpMemory(TAddress nLogicalAddress, TMemoryElement nValue) = 0;			// Pure Virtual to push back a TMemoryElement for the specified address.  The address is mostly for architecture purposes (like alignment, etc) if memory element type doesn't match CPU opcode type
	virtual void pushBackOpMemory(TAddress nLogicalAddress, const CMemoryArray &maValues) = 0;	// Pure Virtual to push back an array of TMemoryElement (see single value version above)

	// The following provides an easy method for GetFieldWidth to round field widths to the next tab-stop to prevent tab/space mixture:
	inline int calcFieldWidth(int n) const
	{
		return (((m_nTabWidth != 0) && (m_bTabsFlag)) ? ((n/m_nTabWidth)+(((n%m_nTabWidth)!=0) ? 1 : 0))*m_nTabWidth : n);
	}

	// --------------------------------

protected:
	// Output Flags and Control Values:
	bool	m_bDeterministic;		// TRUE = skip writing date/time stamps and version detail in output streams, useful for automated testing

	bool	m_bAddrFlag;			// CmdFileToggle, TRUE = Write addresses on disassembly
	bool	m_bOpcodeFlag;			// CmdFileToggle, TRUE = Write opcode listing as comment in disassembly file
	bool	m_bAsciiFlag;			// CmdFileToggle, TRUE = Convert printable data to ASCII strings
	bool	m_bSpitFlag;			// CmdFileToggle, Code-Seeking/Code-Dump Mode, TRUE = Code-Dump, FALSE = Code-Seek
	bool	m_bTabsFlag;			// CmdFileToggle, TRUE = Use tabs in output file.  Default is TRUE.
	bool	m_bAsciiBytesFlag;		// CmdFileToggle, TRUE = Outputs byte values of ASCII text as comment prior to ascii line (default)  FALSE = ASCII line only
	bool	m_bDataOpBytesFlag;		// CmdFileToggle, TRUE = Outputs byte values for data fields in OpBytes field for both printable and non-printable data, FALSE = no opbytes -- Default is FALSE

	int		m_nMaxNonPrint;			// CmdFile Value, Maximum number of non-printable characters per line
	int		m_nMaxPrint;			// CmdFile Value, Maximum number of printable characters per line
	int		m_nTabWidth;			// CmdFile Value, Width of tabs for output, default is 4

	int		m_nBase;				// CmdFile Value, Current number base for control file input
	int		m_nDefaultBase;			// CmdFile Value, Default number base for control file input (settable)

	std::string		m_sDFC;				// CmdFile Value, Current DFC Library -- used with single input file and on multi-file when not specified
	std::string		m_sDefaultDFC;		// CmdFile Value, Default DFC Library -- used with single input file and on multi-file when not specified (settable)

	TAddress		m_nLoadAddress;		// CmdFile Value, Load address for input file -- used only with single input file!
	std::string		m_sInputFilename;	// CmdFile Value, Filename for the input file -- used only with single input file!
	std::string		m_sOutputFilename;	// CmdFile Value, Filename for the disassembly output file
	std::string		m_sFunctionsFilename;	// CmdFile Value, Filename for the function output file
	CStringArray	m_sControlFileList;	// This list is appended with each control file read.  The only purpose for this list is for generating comments in the output file
	CStringArray	m_sInputFileList;	// This list is appended with each source file read (even with the single input name).  The only purpose for this list is for generating comments in the output file

	bool			m_bAllowMemRangeOverlap;	// Set to true on Harvard (and similar) architectures where ROM/RAM/IO/etc can overlap in address range since they are on separate buses
	bool			m_bVBreakEquateLabels;		// Set if the assembler allows labels in Equates to be broken to next line
	bool			m_bVBreakCodeLabels;		// Set if the assembler allows labels in Code to be broken to next line
	bool			m_bVBreakDataLabels;		// Set if the assembler allows labels in Data to be broken to next line

	// --------------------------------

	// Processing variables:
	std::string		m_sFunctionalOpcode;	// Functions File export opcode.  Cleared by ReadNextObj and set by CompleteObjRead.
	time_t			m_StartTime;			// Set to system time at instantiated

	int				m_nCtrlLine;	// Line Count while processing control file
	int				m_nFilesLoaded;	// Count of number of files successfully loaded by the control file
	std::string		m_ParseError;	// Set during the control file parsing to indicate error messages

	CAddressSet m_EntryTable;		// (Always MT_ROM) Table of Entry values (start addresses) from control file

	CFuncEntryMap	m_FunctionEntryTable;	// (Always MT_ROM) Table of start-of functions
	CFuncExitMap	m_FunctionExitTable;	// (Always MT_ROM) Table of end-of functions

	CAddressSet m_FuncExitAddresses;	// (Always MT_ROM) Table of address that are equivalent to function exit like RTS or RTI.  Any JMP or BRA or execution into one of these addresses will equate to a function exit
	CAddressTableMap m_BranchTable;		// (Always MT_ROM) Table mapping branch addresses encountered with the address that referenced it in disassembly.
	CLabelTableMap m_LabelTable[NUM_MEMORY_TYPES];		// Table of labels both specified by the user and from disassembly.  Entry is pointer to array of labels.  An empty entry equates back to Lxxxx.  First entry is default for "Get" function.
	CCommentTableMap m_CommentTable[NUM_MEMORY_TYPES];	// Table of comments by address

	CAddressTableMap m_LabelRefTable[NUM_MEMORY_TYPES];	// Table of reference addresses for labels.  User specified labels have no reference added.
	CAddressLabelMap m_CodeIndirectTable;	// (Always MT_ROM) Table of indirect code vectors with labels specified by the user and from disassembly
	CAddressLabelMap m_DataIndirectTable;	// (Always MT_ROM) Table of indirect data vectors with labels specified by the user and from disassembly

	CAddressLabelMap m_SymbolTable[NUM_MEMORY_TYPES];	// Table of symbols generated from ELF file.  Is like the equivalent added to m_LabelTable, but is unaltered so that demangling will work correctly
	CAddressMap		m_ObjectMap[NUM_MEMORY_TYPES];		// Mapping of addresses in objects to the object base address from which the label can be derived

	CMemRanges		m_rngDataBlocks;		// Explicitly added Data Block declarations from the Control File, used to split discovered Data Blocks from declared Data Blocks

	TAddress		m_PC;				// Program counter

	CMemBlocks		m_Memory[NUM_MEMORY_TYPES];			// Memory object for the processor.
	CMemRanges		m_MemoryRanges[NUM_MEMORY_TYPES];	// ROM, RAM, I/O Ranges/Mapping

private:

	int m_LAdrDplyCnt;					// Label address display count -- used to format output
};

DEFINE_ENUM_FLAG_OPERATORS(CDisassembler::COMMENT_TYPE_FLAGS);

// ============================================================================

class CDisassemblers : public std::vector<CDisassembler *>
{
public:
	CDisassemblers();
	~CDisassemblers();

	CDisassembler *locateDisassembler(const std::string &strGDCName) const;

	void registerDisassembler(CDisassembler *pDisassembler);
};

// ============================================================================

#endif	//  GENERIC_DISASSEMBLY_CLASS_H_

