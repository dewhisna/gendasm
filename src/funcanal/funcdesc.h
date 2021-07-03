//
//	Function Descriptor Classes
//
//
//	Fuzzy Function Analyzer
//	for the Generic Code-Seeking Disassembler
//	Copyright(c)2021 by Donna Whisnant
//

#ifndef FUNC_DESC_H
#define FUNC_DESC_H

// ============================================================================

#include <memclass.h>
#include <gdc.h>
#include "funccomp.h"

#include "stringhelp.h"

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <memory>

// ============================================================================

// Foward Declarations:
class CFuncDesc;
class CFuncDescFile;
class CSymbolMap;

typedef std::string TSymbol;
typedef std::vector<TSymbol> CSymbolArray;
typedef std::map<TSymbol, CSymbolArray> CSymbolArrayMap;

typedef std::size_t THitCount;
typedef std::vector<THitCount> CHitCountArray;
typedef std::map<TSymbol, THitCount> CSymbolHitMap;

// ============================================================================

class ifstreamFuncDescFile : public std::ifstream
{
public:
	ifstreamFuncDescFile(const std::string &strFilename)
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

//////////////////////////////////////////////////////////////////////
// Callback Functions
//////////////////////////////////////////////////////////////////////

// Function Analysis Progress Callback -- Used in functions that take
//		a while to do processing (like reading a file).  Passes
//		current progress position, maximum progress value, a flag
//		indicating if the user/client can currently cancel operation,
//		and a client defined lParam.  Returns True/False Cancel Status.
//		True=Cancel, False=Continue.  If progress indexes aren't supported
//		then nProgressPos will always be 0 and nProgressMax always 1.
typedef bool (*TFN_FuncAnalProgressCallback)(int nProgressPos, int nProgressMax, bool bAllowCancel, TUserData nUserData);


// ============================================================================

//////////////////////////////////////////////////////////////////////
// CFuncObject Class
//////////////////////////////////////////////////////////////////////
//		This specifies a pure virtual base class for defining
//		Function Objects:
class CFuncObject
{
public:
	CFuncObject(std::shared_ptr<const CFuncDescFile> pParentFuncFile, std::shared_ptr<const CFuncDesc> pParentFunc, CStringArray &argv);

	virtual bool isExactMatch(const CFuncObject &obj) const;

	virtual bool AddLabel(const TLabel &strLabel);

	TAddress GetRelFuncAddress() const { return m_nRelFuncAddress; }
	TAddress GetAbsAddress() const { return m_nAbsAddress; }
	CLabelArray::size_type GetLabelCount() const { return m_arrLabelTable.size(); }
	TLabel GetLabel(CLabelArray::size_type nIndex) const { return m_arrLabelTable.at(nIndex); }
	TString GetBytes() const;
	CMemoryArray::size_type GetByteCount() const { return m_Bytes.size(); }

	virtual TString ExportToDiff(FUNC_DIFF_LEVEL nLevel) const = 0;

	virtual TString CreateOutputLine(OUTPUT_OPTIONS nOutputOptions = OO_NONE) const = 0;

	virtual CSymbolArray GetSymbols() const;		// Returns specially encoded symbol array (see implementation comments)

	// The following define field positions for obtaining field widths from overrides.
	//	Additional entries can be added on overrides, but this order should NOT change
	//	in order to maintain backward compatibility at all times:
	using FIELD_CODE = CDisassembler::FIELD_CODE;

	static int GetFieldWidth(FIELD_CODE nFC);

protected:
	std::shared_ptr<const CFuncDescFile> m_pParentFuncFile;
	std::shared_ptr<const CFuncDesc> m_pParentFunc;

	TAddress	m_nRelFuncAddress;		// Address of object relative to function start
	TAddress	m_nAbsAddress;			// Absolute Address of object
	CLabelArray		m_arrLabelTable;	// Labels assigned to this object
	CMemoryArray	m_Bytes;			// Array of bytes for this object (TODO : Change this to TDisassembler::COpcodeSymbolArray)
};

// ============================================================================

//////////////////////////////////////////////////////////////////////
// CFuncAsmInstObject Class
//////////////////////////////////////////////////////////////////////
//		This class specifies an assembly instruction object
class CFuncAsmInstObject : public CFuncObject
{
public:
	CFuncAsmInstObject(std::shared_ptr<const CFuncDescFile> pParentFuncFile, std::shared_ptr<const CFuncDesc> pParentFunc, CStringArray &argv);

	virtual TString ExportToDiff(FUNC_DIFF_LEVEL nLevel) const override;

	virtual TString CreateOutputLine(OUTPUT_OPTIONS nOutputOptions = OO_NONE) const override;

	virtual CSymbolArray GetSymbols() const override;	// Returns specially encoded symbol array (see implementation comments)

	TString GetOpCodeBytes() const;
	CMemoryArray::size_type GetOpCodeByteCount() const { return m_OpCodeBytes.size(); }

	TString GetOperandBytes() const;
	CMemoryArray::size_type GetOperandByteCount() const { return m_OperandBytes.size(); }

protected:
	CMemoryArray m_OpCodeBytes;			// Array of OpCode Bytes for this instruction  (TODO : Change this to TDisassembler::COpcodeSymbolArray)
	CMemoryArray m_OperandBytes;		// Array of Operand Bytes for ths instruction  (TODO : Change this to TDisassembler::COpcodeSymbolArray)
	TString m_strDstOperand;			// Encoded Destination Operand
	TString m_strSrcOperand;			// Encoded Source Operand
	TString m_strOpCodeText;			// Textual OpCode mnemonic
	TString m_strOperandText;			// Textual Operands
};

// ============================================================================

//////////////////////////////////////////////////////////////////////
// CFuncDataByteObject Class
//////////////////////////////////////////////////////////////////////
//		This class specifies a function embedded data byte object
class CFuncDataByteObject : public CFuncObject
{
public:
	CFuncDataByteObject(std::shared_ptr<const CFuncDescFile> pParentFuncFile, std::shared_ptr<const CFuncDesc> pParentFunc, CStringArray &argv)
		:	CFuncObject(pParentFuncFile, pParentFunc, argv)
	{ }

	virtual TString ExportToDiff(FUNC_DIFF_LEVEL nLevel) const override;

	virtual TString CreateOutputLine(OUTPUT_OPTIONS nOutputOptions = OO_NONE) const override;
};

// ============================================================================

//////////////////////////////////////////////////////////////////////
// CFuncDesc Class
//////////////////////////////////////////////////////////////////////
//		This specifies exactly one function as an array of CFuncObject
//		objects:
class CFuncDesc : public std::vector< std::shared_ptr<CFuncObject> >
{
public:
	CFuncDesc(TAddress nAddress, const TString &strNames);

	virtual bool AddName(TAddress nAddress, const TLabel &strName);
	virtual TLabel GetMainName() const;
	virtual TAddress GetMainAddress() const { return m_nMainAddress; }

	virtual bool AddLabel(TAddress nAddress, const TLabel &strLabel);
	virtual bool AddrHasLabel(TAddress nAddress) const;
	virtual TLabel GetPrimaryLabel(TAddress nAddress) const;
	virtual CLabelArray GetLabelList(TAddress nAddress) const;

	virtual TSize GetFuncSize() const;		// Returns size of function in byte counts (used for address calculation)

	virtual void ExportToDiff(FUNC_DIFF_LEVEL nLevel, CStringArray &anArray) const;

	void Add(std::shared_ptr<CFuncObject>pObj);


	// Warning: This object supports ADDing only!  Removing an item does NOT remove labels
	//			that have been added nor decrement the function size!  To modify a function
	//			description, create a new function object with only the new elements desired!!
	//			This has been done to improve speed efficiency!

protected:
	TAddress m_nMainAddress;		// Main Address is the address defined at instantiation time.  Additional relocated declarations might be defined in the name list
	TSize m_nFunctionSize;			// Count of bytes in function -- used to calculate addressing inside the function

	// Note: The File's Label Table only contains those labels defined
	//			internally in this function and may not directly
	//			correspond to that of the file level.  These might
	//			contain 'L' labels.
	CLabelTableMap m_mapFuncNameTable;				// Table of names for this function.  First entry is typical default
	CLabelTableMap m_mapLabelTable;					// Table of labels in this function.  First entry is typical default
};
typedef std::vector< std::shared_ptr<CFuncDesc> > CFuncDescArray;

// ============================================================================

//////////////////////////////////////////////////////////////////////
// CFuncDescFile Class
//////////////////////////////////////////////////////////////////////
//		This specifies all of the information from a single Func
//		Description File.  It contains a CFuncDescArray of all of
//		the functions in the file, but also contains the file mapping
//		and label information:
class CFuncDescFile
{
public:
	using MEMORY_TYPE = CDisassembler::MEMORY_TYPE;

	virtual bool ReadFuncDescFile(std::shared_ptr<CFuncDescFile> pThis, ifstreamFuncDescFile &inFile, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr, int nStartLineCount = 0);			// Read already open control file 'infile', outputs messages to 'msgFile' and errors to 'errFile', nStartLineCount = initial line counter value

	virtual bool AddLabel(MEMORY_TYPE nMemoryType, TAddress nAddress, const TLabel &strLabel);
	virtual bool AddrHasLabel(MEMORY_TYPE nMemoryType, TAddress nAddress) const;
	virtual TLabel GetPrimaryLabel(MEMORY_TYPE nMemoryType, TAddress nAddress) const;
	virtual TLabel GetAnyPrimaryLabel(bool bInvertPriority, TAddress nAddress) const;
	virtual CLabelArray GetLabelList(MEMORY_TYPE nMemoryType, TAddress nAddress) const;

	virtual CFuncDescArray::size_type GetFuncCount() const { return m_arrFunctions.size(); }
	virtual const CFuncDesc &GetFunc(CFuncDescArray::size_type nIndex) const { return *m_arrFunctions.at(nIndex); }

	TString GetFuncPathName() const { return m_strFilePathName; }
	TString GetFuncFileName() const { return m_strFileName; }

	virtual bool isMemAddr(MEMORY_TYPE nMemType, TAddress nAddress) const;

	virtual void SetProgressCallback(TFN_FuncAnalProgressCallback pfnCallback, const TUserData &nUserData = {})
	{
		m_pfnProgressCallback = pfnCallback;
		m_nUserDataProgressCallback = nUserData;
	}

	virtual bool allowMemRangeOverlap() const { return m_bAllowMemRangeOverlap; }
	virtual size_t opcodeSymbolSize() const { return m_nOpcodeSymbolSize; }

protected:
	TString		m_strFilePathName;
	TString		m_strFileName;

	CMemRanges		m_MemoryRanges[MEMORY_TYPE::NUM_MEMORY_TYPES];	// ROM, RAM, I/O Ranges/Mapping

	bool			m_bAllowMemRangeOverlap = false;	// Set to true on Harvard (and similar) architectures where ROM/RAM/IO/etc can overlap in address range since they are on separate buses
	size_t			m_nOpcodeSymbolSize = 1;			// Width of opcode symbols

	TFN_FuncAnalProgressCallback m_pfnProgressCallback = nullptr;
	TUserData m_nUserDataProgressCallback = {};

	// Note: The File's Label Table only contains those labels defined
	//			at the file level by "!" entries.  NOT those down at the
	//			individual function level.  Specifically it doesn't
	//			include 'L' names.
	CLabelTableMap m_mapLabelTable[MEMORY_TYPE::NUM_MEMORY_TYPES];		// Table of labels.  First entry is typical default
	CFuncDescArray m_arrFunctions;						// Array of functions in the file
};

// ============================================================================

//////////////////////////////////////////////////////////////////////
// CFuncDescFileArray Class
//////////////////////////////////////////////////////////////////////
//		This specifies a cluster of Func Definition Files as an array
//		of CFuncDescFile Objects.
class CFuncDescFileArray : public std::vector< std::shared_ptr<CFuncDescFile> >
{
public:
	virtual CFuncDescArray::size_type GetFuncCount() const;

	virtual double CompareFunctions(FUNC_COMPARE_METHOD nMethod,
									size_type nFile1Ndx, std::size_t nFile1FuncNdx,
									size_type nFile2Ndx, std::size_t nFile2FuncNdx,
									bool bBuildEditScript) const;
	virtual TString DiffFunctions(FUNC_COMPARE_METHOD nMethod,
									int nFile1Ndx, int nFile1FuncNdx,
									int nFile2Ndx, int nFile2FuncNdx,
									OUTPUT_OPTIONS nOutputOptions,
									double &nMatchPercent,
									CSymbolMap *pSymbolMap = nullptr) const;

	virtual void SetProgressCallback(TFN_FuncAnalProgressCallback pfnCallback, const TUserData &nUserData = {})
	{
		m_pfnProgressCallback = pfnCallback;
		m_nUserDataProgressCallback = nUserData;
	}

protected:
	TFN_FuncAnalProgressCallback m_pfnProgressCallback = nullptr;
	TUserData m_nUserDataProgressCallback = {};
};

// ============================================================================

//////////////////////////////////////////////////////////////////////
// CSymbolMap Class
//////////////////////////////////////////////////////////////////////
//		This class stores mapping between two sets of functions by
//		hashing and storing address accesses and finding highest
//		hit matches.
class CSymbolMap
{
public:
	bool empty() const;

	void AddObjectMapping(const CFuncObject &aLeftObject, const CFuncObject &aRightObject);

	CSymbolArray GetLeftSideCodeSymbolList() const;		// Returns sorted list of left-side code symbols
	CSymbolArray GetRightSideCodeSymbolList() const;	// Returns sorted list of right-side code symbols
	CSymbolArray GetLeftSideDataSymbolList() const;		// Returns sorted list of left-side data symbols
	CSymbolArray GetRightSideDataSymbolList() const;	// Returns sorted list of right-side data symbols

	THitCount GetLeftSideCodeHitList(const TSymbol &aSymbol, CSymbolArray &aSymbolArray, CHitCountArray &aHitCountArray) const;		// Returns sorted array of target hit symbols and corresponding counts, function returns total hit count
	THitCount GetRightSideCodeHitList(const TSymbol &aSymbol, CSymbolArray &aSymbolArray, CHitCountArray &aHitCountArray) const;	// Returns sorted array of target hit symbols and corresponding counts, function returns total hit count
	THitCount GetLeftSideDataHitList(const TSymbol &aSymbol, CSymbolArray &aSymbolArray, CHitCountArray &aHitCountArray) const;		// Returns sorted array of target hit symbols and corresponding counts, function returns total hit count
	THitCount GetRightSideDataHitList(const TSymbol &aSymbol, CSymbolArray &aSymbolArray, CHitCountArray &aHitCountArray) const;	// Returns sorted array of target hit symbols and corresponding counts, function returns total hit count

	// Static helpers:
	static CSymbolArray GetLeftSideCodeSymbolList(const CSymbolMap *pThis) { return pThis->GetLeftSideCodeSymbolList(); }
	static CSymbolArray GetRightSideCodeSymbolList(const CSymbolMap *pThis) { return pThis->GetRightSideCodeSymbolList(); }
	static CSymbolArray GetLeftSideDataSymbolList(const CSymbolMap *pThis) { return pThis->GetLeftSideDataSymbolList(); }
	static CSymbolArray GetRightSideDataSymbolList(const CSymbolMap *pThis) { return pThis->GetRightSideDataSymbolList(); }

	static THitCount GetLeftSideCodeHitList(const CSymbolMap *pThis, const TSymbol &aSymbol, CSymbolArray &aSymbolArray, CHitCountArray &aHitCountArray) { return pThis->GetLeftSideCodeHitList(aSymbol, aSymbolArray, aHitCountArray); }
	static THitCount GetRightSideCodeHitList(const CSymbolMap *pThis, const TSymbol &aSymbol, CSymbolArray &aSymbolArray, CHitCountArray &aHitCountArray) { return pThis->GetRightSideCodeHitList(aSymbol, aSymbolArray, aHitCountArray); }
	static THitCount GetLeftSideDataHitList(const CSymbolMap *pThis, const TSymbol &aSymbol, CSymbolArray &aSymbolArray, CHitCountArray &aHitCountArray) { return pThis->GetLeftSideDataHitList(aSymbol, aSymbolArray, aHitCountArray); }
	static THitCount GetRightSideDataHitList(const CSymbolMap *pThis, const TSymbol &aSymbol, CSymbolArray &aSymbolArray, CHitCountArray &aHitCountArray) { return pThis->GetRightSideDataHitList(aSymbol, aSymbolArray, aHitCountArray); }

private:
	CSymbolArray GetSymbolList(const CSymbolArrayMap &mapSymbolArrays) const;
	THitCount GetHitList(const CSymbolArrayMap &mapSymbolArrays,
						const TSymbol &aSymbol, CSymbolArray &aSymbolArray, CHitCountArray &aHitCountArray) const;

protected:
	void AddLeftSideCodeSymbol(const TSymbol &aLeftSymbol, const TSymbol &aRightSymbol);
	void AddRightSideCodeSymbol(const TSymbol &aRightSymbol, const TSymbol &aLeftSymbol);
	void AddLeftSideDataSymbol(const TSymbol &aLeftSymbol, const TSymbol &aRightSymbol);
	void AddRightSideDataSymbol(const TSymbol &aRightSymbol, const TSymbol &aLeftSymbol);

	CSymbolArrayMap m_LeftSideCodeSymbols;
	CSymbolArrayMap m_RightSideCodeSymbols;
	CSymbolArrayMap m_LeftSideDataSymbols;
	CSymbolArrayMap m_RightSideDataSymbols;
};

// ============================================================================

#endif	// FUNC_DESC_H
