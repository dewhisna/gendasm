//
//	Fuzzy Function Analyzer
//	for the Generic Code-Seeking Disassembler
//	Copyright(c)2021 by Donna Whisnant
//

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iterator>
#include <filesystem>
#include <set>
#include <utility>
#include <thread>

#include <stringhelp.h>

#include "funcanal.h"
#include "funcdesc.h"

#include <assert.h>

#define VERSION 0x200				// Function Analyzer Version number 2.00

#ifndef UNUSED
	#define UNUSED(x) ((void)(x))
#endif

// ============================================================================

#define MINIMUM_THREAD_COUNT 2		// Minimum number of worker threads to create if ideal number of threads is less than this value or ideal number of threads can't be determined

#ifdef OS_UNIX
#include <unistd.h>
static int idealThreadCount()
{
	int numCPU = sysconf(_SC_NPROCESSORS_ONLN);
	return ((numCPU > 0) ? numCPU : MINIMUM_THREAD_COUNT);
}
#elif defined (OS_WIN32)
#include <windows.h>
static int idealThreadCount()
{
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	return ((sysinfo.dwNumberOfProcessors > MINIMUM_THREAD_COUNT) ? sysinfo.dwNumberOfProcessors : MINIMUM_THREAD_COUNT);
}
#else
static int idealThreadCount()
{
	return MINIMUM_THREAD_COUNT;
}
#endif

static unsigned int threadCount()
{
	unsigned int nThreads = std::thread::hardware_concurrency();
	return (nThreads ? nThreads : idealThreadCount());
}

// ============================================================================

typedef std::vector<double> CCompResultArray;
typedef std::vector<CCompResultArray> CCompResultMatrix;

// ============================================================================

static std::string formatVersion(unsigned int nVersion)
{
	std::ostringstream ssTemp;
	ssTemp << ((nVersion / 256ul) & 0xFF);
	ssTemp << ".";
	ssTemp << std::setfill('0') << std::setw(2) << (nVersion & 0xFF);
	return ssTemp.str();
}

static bool promptFileOverwrite(const std::string &strFilename)
{
	if (std::filesystem::exists(strFilename)) {
		char ch;
		do {
			std::cout << "\nFile \"" << strFilename << "\" exists! -- Overwrite? (y/n): ";
			std::cout.flush();
			std::cin >> ch;
		} while (!std::cin.fail() && (ch != 'y') && (ch != 'n'));
		return (ch == 'y');
	}

	return true;
}

static bool openForWriting(bool bForceOverwrite, std::fstream &file, const TString &strFilename, const TString &strMessage)
{
	if (!strFilename.empty()) {
		if (!bForceOverwrite && !promptFileOverwrite(strFilename)) return false;
		file.open(strFilename, std::ios_base::out);
		if (!file.is_open()) {
			std::cerr << "\n*** Error: Opening " << strMessage << (!strMessage.empty() ? " " : "") << "Output File \"" << strFilename << "\" for writing...\n\n";
			return false;
		}
	}
	return true;
}

static CStringArray parseCSVLine(const std::string aLine)
{
	static const std::string strDelim = ",";
	static const std::string strWhitespace = "\x009\x00a\x00b\x00c\x00d\x020";
	CStringArray arrFields;
	std::string strLine = aLine;
	trim(strLine);

	while (!strLine.empty()) {
		std::string strString;

		bool bQuoted = false;
		bool bQuoteLiteral = false;
		bool bNonWhitespaceSeen = false;

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
			} else {
				strString.push_back(ch);
			}
		}

		arrFields.push_back(strString);
	}

	return arrFields;
}

// ----------------------------------------------------------------------------

static void dumpComparison(FUNC_COMPARE_TYPE nCompareType,
							std::fstream &fileComp, std::fstream &fileOES,
							bool bCompOESFlag,
							const CCompResultMatrix &matrixCompResult,
							FUNC_COMPARE_METHOD nCompMethod,
							std::shared_ptr<const CFuncDescFile> pFuncFile1, CFuncDescArray::size_type ndxFile1,
							std::shared_ptr<const CFuncDescFile> pFuncFile2, CFuncDescArray::size_type ndxFile2,
							OUTPUT_OPTIONS nOutputOptions,
							CSymbolMap *pSymbolMap)
{
	CStringArray oes;

	assert(ndxFile1 < ((nCompareType == FCT_FUNCTIONS) ? pFuncFile1->GetFuncCount() : pFuncFile1->GetDataBlockCount()));
	const CFuncDesc &function1 = ((nCompareType == FCT_FUNCTIONS) ? pFuncFile1->GetFunc(ndxFile1) : pFuncFile1->GetDataBlock(ndxFile1));
	assert(ndxFile2 < ((nCompareType == FCT_FUNCTIONS) ? pFuncFile2->GetFuncCount() : pFuncFile2->GetDataBlockCount()));
	const CFuncDesc &function2 = ((nCompareType == FCT_FUNCTIONS) ? pFuncFile2->GetFunc(ndxFile2) : pFuncFile2->GetDataBlock(ndxFile2));

	if (fileComp.is_open()) {
		fileComp << "--------------------------------------------------------------------------------\n";
		fileComp << ((nCompareType == FCT_FUNCTIONS) ? "    Left Function  : " : "    Left Data Block  : ")
											<< function1.GetMainName()
											<< " (" << (ndxFile1+1) << ")\n" <<
					((nCompareType == FCT_FUNCTIONS) ? "    Right Function : " : "    Right Data Block : ")
											<< function2.GetMainName()
											<< " (" << (ndxFile2+1) << ")\n" <<
					((nCompareType == FCT_FUNCTIONS) ? "    Matches by     : " : "    Matches by       : ")
											<< matrixCompResult[ndxFile1][ndxFile2]*100 << "%\n";
		fileComp << "--------------------------------------------------------------------------------\n";
	}

	double nMatchPercent;
	TString strDiff = DiffFunctions(nCompareType, nCompMethod, *pFuncFile1, ndxFile1, *pFuncFile2, ndxFile2,
									nOutputOptions, nMatchPercent, pSymbolMap);
	if (fileComp.is_open()) {
		if (bCompOESFlag) {
			if (GetLastEditScript(oes)) {
				std::copy(oes.cbegin(), oes.cend(), std::ostream_iterator<TString>(fileComp, "\n"));
			}
			fileComp << "--------------------------------------------------------------------------------\n";
		}
		fileComp << strDiff;
		fileComp << "--------------------------------------------------------------------------------\n";
	}

	if (fileOES.is_open() && (nCompareType == FCT_FUNCTIONS)) {
		if (GetLastEditScript(oes)) {
			fileOES << "\n";
			fileOES << "@" << function1.GetMainName() << "(" << (ndxFile1+1)
					<< ")|" << function2.GetMainName() << "(" << (ndxFile2+1) << ")\n";
			std::copy(oes.cbegin(), oes.cend(), std::ostream_iterator<TString>(fileOES, "\n"));
		}
	}
}

// ----------------------------------------------------------------------------

static void dumpSymbols(std::fstream &fileSym,
						const CSymbolMap &aSymbolMap,
						CSymbolArray (*fncGetSymbolList)(const CSymbolMap *pSymbolMap),
						THitCount (*fncGetHitList)(const CSymbolMap *pSymbolMap, const TSymbol &aSymbol, CSymbolArray &aSymbolArray, CHitCountArray &aHitCountArray),
						const TString &strHeaderText)
{
	// Dump Symbol Tables:
	CSymbolArray arrSymbolList;
	CSymbolArray arrHitList;
	CHitCountArray arrHitCount;
	THitCount nTotalHits;
	TSymbol::size_type nSymWidth;

	std::cout << strHeaderText;
	if (fileSym.is_open()) {
		fileSym << strHeaderText;
	}
	arrSymbolList = fncGetSymbolList(&aSymbolMap);
	nSymWidth = 0;
	for (auto const & symbol : arrSymbolList) if (symbol.size() > nSymWidth) nSymWidth = symbol.size();
	for (auto const & symbol : arrSymbolList) {
		nTotalHits = fncGetHitList(&aSymbolMap, symbol, arrHitList, arrHitCount);
		assert(arrHitList.size() == arrHitCount.size());
		TString strTemp = symbol;
		padString(strTemp, nSymWidth-symbol.size());
		std::cout << strTemp + " : ";
		if (fileSym.is_open()) fileSym << strTemp + " : ";
		if (!arrHitList.empty()) {
			if (nTotalHits == 0) nTotalHits = 1;	// Safe-guard for divide by zero
			for (CSymbolArray::size_type ndxHits = 0; ndxHits < arrHitList.size(); ++ndxHits) {
				if (arrHitList.at(ndxHits).empty()) arrHitList[ndxHits] = "???";		// Special case for unnamed functions
				char arrTemp[120];
				std::sprintf(arrTemp, "%s%s (%f%%)", (ndxHits ? ", " : ""), arrHitList.at(ndxHits).c_str(),
									(static_cast<double>(arrHitCount.at(ndxHits))/nTotalHits)*100.0);
				std::cout << arrTemp;
				if (fileSym.is_open()) fileSym << arrTemp;
			}
			std::cout << "\n";
			if (fileSym.is_open()) fileSym << "\n";
		} else {
			std::cout << "<none>\n";
			if (fileSym.is_open()) fileSym << "<none>\n";
		}
	}
}

// ----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
	TString m_strMatrixInFilename;
	TString m_strMatrixOutFilename;
	TString m_strDFROFilename;
	TString m_strCompFilename;
	TString m_strOESFilename;
	TString m_strSymFilename;
	CStringArray m_arrInputFilenames;
	bool m_bForceOverwrite = false;
	bool m_bOutputOptionAddAddress = false;

	bool bNeedUsage = false;
	CStringArray::size_type nMinReqInputFiles = 1;				// Must have at least one input filename
	double nMinCompLimit = 0;
	FUNC_COMPARE_METHOD nCompMethod = FCM_DYNPROG_XDROP;
	FUNC_DIFF_LEVEL nFDL = FDL_1;
	bool bDFROWithCodeFlag = false;
	bool bCompOESFlag = false;
	bool bDeterministic = false;
	bool bSingleThreaded = false;

	// Parse Arguments:
	for (int ndx = 1; ((ndx < argc) && !bNeedUsage); ++ndx) {
		std::string strArg = argv[ndx];
		if (strArg.starts_with("/")) strArg[0] = '-';
		if (!strArg.starts_with("-")) {
			// If not prefixed by an option, assume it's a filename:
			m_arrInputFilenames.push_back(strArg);
		} else if (strArg == "-st") {
			bSingleThreaded = true;
		} else if (strArg == "--deterministic") {
			bDeterministic = true;
		} else if (strArg.starts_with("-mi")) {			// Matrix Input File
			if (!m_strMatrixInFilename.empty()) {
				bNeedUsage = true;
				continue;
			} else if (strArg.size() > 3) {
				m_strMatrixInFilename = strArg.substr(3);
			} else if ((ndx+1) < argc) {
				++ndx;
				m_strMatrixInFilename = argv[ndx];
			} else {
				bNeedUsage = true;
				continue;
			}
		} else if (strArg.starts_with("-mo")) {			// Matrix Output File
			if (!m_strMatrixOutFilename.empty()) {
				bNeedUsage = true;
				continue;
			} else if (strArg.size() > 3) {
				m_strMatrixOutFilename = strArg.substr(3);
			} else if ((ndx+1) < argc) {
				++ndx;
				m_strMatrixOutFilename = argv[ndx];
			} else {
				bNeedUsage = true;
				continue;
			}
			nMinReqInputFiles = 2;		// Must have 2 input files
		} else if (strArg.starts_with("-do") ||			// Normal DFRO
				   strArg.starts_with("-dc")) {			// DFRO with Assembly Code
			if (!m_strDFROFilename.empty()) {
				bNeedUsage = true;
				continue;
			}
			if (strArg.starts_with("-dc")) bDFROWithCodeFlag = true;
			if (strArg.size() > 3) {
				m_strDFROFilename = strArg.substr(3);
			} else if ((ndx+1) < argc) {
				++ndx;
				m_strDFROFilename = argv[ndx];
			} else {
				bNeedUsage = true;
				continue;
			}
		} else if (strArg.starts_with("-dl")) {			// Diff Level
			int nDiffLevel = 0;
			if (strArg.size() > 3) {
				nDiffLevel = atol(&strArg.c_str()[3]);
			} else if ((ndx+1) < argc) {
				++ndx;
				nDiffLevel = atol(argv[ndx]);
			} else {
				bNeedUsage = true;
				continue;
			}
			if ((nDiffLevel < 1) || (nDiffLevel > NUM_FUNC_DIFF_LEVELS)) {
				bNeedUsage = true;
				continue;
			} else {
				nFDL = static_cast<FUNC_DIFF_LEVEL>(nDiffLevel-1);
			}
		} else if (strArg.starts_with("-cn") ||			// Normal Diff
				   strArg.starts_with("-ce")) {			// Normal Diff with OES inline
			if (!m_strCompFilename.empty()) {
				bNeedUsage = true;
				continue;
			}
			if (strArg.starts_with("-ce")) bCompOESFlag = true;
			if (strArg.size() > 3) {
				m_strCompFilename = strArg.substr(3);
			} else if ((ndx+1) < argc) {
				++ndx;
				m_strCompFilename = argv[ndx];
			} else {
				bNeedUsage = true;
				continue;
			}
		} else if (strArg.starts_with("-e")) {
			if (!m_strOESFilename.empty()) {
				bNeedUsage = true;
				continue;
			} else if (strArg.size() > 2) {
				m_strOESFilename = strArg.substr(2);
			} else if ((ndx+1) < argc) {
				++ndx;
				m_strOESFilename = argv[ndx];
			} else {
				bNeedUsage = true;
				continue;
			}
			nMinReqInputFiles = 2;		// Must have 2 input files
		} else if (strArg.starts_with("-s")) {
			if (!m_strSymFilename.empty()) {
				bNeedUsage = true;
				continue;
			} else if (strArg.size() > 2) {
				m_strSymFilename = strArg.substr(2);
			} else if ((ndx+1) < argc) {
				++ndx;
				m_strSymFilename = argv[ndx];
			} else {
				bNeedUsage = true;
				continue;
			}
			nMinReqInputFiles = 2;		// Must have 2 input files
		} else if (strArg == "-f") {
			m_bForceOverwrite = true;
		} else if (strArg == "-ooa") {
			m_bOutputOptionAddAddress = true;
		} else if (strArg.starts_with("-l")) {
			if (strArg.size() > 2) {
				nMinCompLimit = atof(&strArg.c_str()[2])/100;
			} else if ((ndx+1) < argc) {
				++ndx;
				nMinCompLimit = atof(argv[ndx])/100;
			} else {
				bNeedUsage = true;
				continue;
			}
		} else if (strArg.starts_with("-a")) {
			if (strArg.size() > 2) {
				nCompMethod = static_cast<FUNC_COMPARE_METHOD>(strtoul(&strArg.c_str()[2], nullptr, 10));
			} else if ((ndx+1) < argc) {
				++ndx;
				nCompMethod = static_cast<FUNC_COMPARE_METHOD>(strtoul(argv[ndx], nullptr, 10));
			} else {
				bNeedUsage = true;
				continue;
			}
			if ((nCompMethod < 0) || (nCompMethod >= FCM_COUNT)) {
				bNeedUsage = true;
				continue;
			}
		} else {
			bNeedUsage = true;
		}
	}

	if (m_arrInputFilenames.size() < nMinReqInputFiles) bNeedUsage = true;

	if (!m_strMatrixInFilename.empty() &&
		!m_strMatrixOutFilename.empty()) bNeedUsage = true;		// Can only have matrix in or matrix out (not both)

	if (m_strMatrixOutFilename.empty() &&
		m_strDFROFilename.empty() &&
		m_strCompFilename.empty() &&
		m_strOESFilename.empty() &&
		m_strSymFilename.empty()) {
		std::cerr << std::endl << std::endl << "Nothing to do..." << std::endl << std::endl;
		bNeedUsage = true;
	}

	std::fstream fileFunc;
	std::fstream fileMatrixIn;
	std::fstream fileMatrixOut;
	std::fstream fileDFRO;
	std::fstream fileComp;
	std::fstream fileOES;
	std::fstream fileSym;

	CSymbolMap aSymbolMap;
	CFuncDescFileArray m_arrFuncFiles;
	CCompResultMatrix m_matrixFuncCompResult;
	CCompResultMatrix m_matrixDataCompResult;
	double nMaxCompResult;
	bool bFlag;

	if (!bDeterministic) {
		std::cout << "Generic Code-Seeking Disassembler" << std::endl;
		std::cout << "Fuzzy Function Analyzer v" << formatVersion(VERSION) << std::endl;
		std::cout << "Copyright(c)2021 by Donna Whisnant" << std::endl;
	} else {
		std::cout << "Generic Code-Seeking Disassembler" << std::endl;
		std::cout << "Fuzzy Function Analyzer" << std::endl;
	}

	if (bNeedUsage) {
		std::cerr <<"Usage:\n"
					"funcanal [--deterministic] [-st] [-ooa] [-a <alg>] [-f] [-e <oes-fn>] [-s <sym-fn>] [-mi <mtx-fn> | -mo <mtx-fn>] [[-do <dro-fn> | -dc <dro-fn>] -dl <fdl>] [-cn <cmp-fn> | -ce <cmp-fn>] [-l <limit>] <func-fn1> [<func-fn2>]\n"
					"\n"
					"Where:\n\n"
					"    <oes-fn>   = Output Optimal Edit Script Filename to generate\n\n"
					"    <mtx-fn>   = Input or Output Filename of a CSV file to read or to generate\n"
					"                 that denotes percentage of function cross-similarity.\n\n"
					"    <dro-fn>   = Output Filename of a file to generate that contains the\n"
					"                 diff-ready version all of the functions from the input file(s)\n\n"
					"    <cmp-fn>   = Output Filename of a file to generate that contains the full\n"
					"                 cross-functional comparisons.\n\n"
					"    <func-fn1> = Input Filename of the primary functions-definition-file.\n\n"
					"    <func-fn2> = Input Filename of the secondary functions-definition-file.\n"
					"                 (Optional only if not using -mo, -cX, -e, or -s)\n\n"
					"    <alg>      = Comparison algorithm to use (see below).\n\n"
					"    <fdl>      = Function Diff Level (for diff-ready-output, see below).\n\n"
					"    <limit>    = Lower-Match Limit Percentage.\n\n"
					"\n"
					"At least one of the following switches must be used:\n"
					"    -mo <mtx-fn> Perform cross comparison of files and output a matrix of\n"
					"                 percent match (requires 2 input files). Cannot be used\n"
					"                 with the -mi switch.\n\n"
					"    -do <dro-fn> Dump the functions definition file(s) in Diff-Ready notation\n\n"
					"    -dc <dro-fn> Dump the functions definition file(s) in Diff-Ready notation\n"
					"                 with assembly code output side-by-side\n\n"
					"    -cn <cmp-fn> Perform cross comparison of files and output a side-by-side\n"
					"                 diff of most similar functions (Normal Output)\n\n"
					"    -ce <cmp-fn> Perform cross comparison of files and output a side-by-side\n"
					"                 diff of most similar functions (Include inline OES)\n\n"
					"    -e <oes-fn>  Perform cross comparison of files and output an optimal edit\n"
					"                 script file that details the most optimal way of editing the\n"
					"                 left-most file to get the right-most file\n\n"
					"    -s <sym-fn>  Perform cross comparison of files and output a cross-map\n"
					"                 probability based symbol table\n\n"
					"\n"
					"The following switches can be specified but are optional:\n"
					"    --deterministic  Skip output like dates and version numbers so that the\n"
					"                     output can be compared with other content for tests.\n\n"
					"    -mi <mtx-fn> Reads the specified matrix file to get function cross\n"
					"                 comparison information rather than recalculating it.\n"
					"                 Cannot be used with the -mo switch.\n\n"
					"    -f           Force output file overwrite without prompting\n\n"
					"    -l <limit>   Minimum-Match Limit.  This option is only useful with the -cX,\n"
					"                 -e, and -s options and limits output to functions having a\n"
					"                 match percentage greater than or equal to this value.  If not\n"
					"                 specified, the minimum required match is anything greater than\n"
					"                 0%. This value should be specified as a percentage or\n"
					"                 fraction of a percent.  For example: -l 50 matches anything\n"
					"                 50% or higher.  -l 23.7 matches anything 23.7% or higher.\n\n"
					"    -a <alg>     Select a specific comparison algorithm to use.  Where <alg> is\n"
					"                 one of the following:\n"
					"                       0 = Dynamic Programming X-Drop Algorithm\n"
					"                       1 = Dynamic Programming Greedy Algorithm\n"
					"                 If not specified, the X-Drop algorithm will be used.\n\n"
					"    -dl <fdl>    Function Diff Level when generating DRO.\n"
					"                 <fdl> Levels 1-" << NUM_FUNC_DIFF_LEVELS << "\n\n"
					"    -ooa         Output-Option Add Address to diff create line output.\n\n"
					"    -st          Run Single Threaded when computing comparison matrix.\n\n"
					"\n";
		return -1;
	}

	switch (nCompMethod) {
		case FCM_DYNPROG_XDROP:
			std::cout << "Using Comparison Algorithm: DynProg X-Drop\n\n";
			break;
		case FCM_DYNPROG_GREEDY:
			std::cout << "Using Comparison Algorithm: DynProg Greedy\n\n";
			break;
		default:
			break;
	}

	// Open output files:
	if (!openForWriting(m_bForceOverwrite, fileMatrixOut, m_strMatrixOutFilename, "Matrix") ||
		!openForWriting(m_bForceOverwrite, fileDFRO, m_strDFROFilename, "Diff-Ready") ||
		!openForWriting(m_bForceOverwrite, fileComp, m_strCompFilename, "Compare") ||
		!openForWriting(m_bForceOverwrite, fileOES, m_strOESFilename, "Optimal Edit Script") ||
		!openForWriting(m_bForceOverwrite, fileSym, m_strSymFilename, "Symbol Table")) return -2;

	// Read function files:
	for (auto const & strFilename : m_arrInputFilenames) {
		std::shared_ptr<CFuncDescFile> pFuncDescFile = std::make_shared<CFuncDescFile>();
		ifstreamFuncDescFile fileFunc(strFilename);
		if (!fileFunc.is_open()) {
			std::cerr << "*** Error: Opening Function Definition File \"" << strFilename << "\" for reading...\n";
			return -3;
		}
		pFuncDescFile->ReadFuncDescFile(pFuncDescFile, fileFunc, &std::cout, &std::cerr);
		fileFunc.close();

		if (fileDFRO.is_open()) {
			fileDFRO << std::string(pFuncDescFile->GetFuncFileName().size()+7, '=') + "\n";
			fileDFRO << "File \"" << pFuncDescFile->GetFuncFileName() << "\"\n";
			fileDFRO << std::string(pFuncDescFile->GetFuncFileName().size()+7, '=') + "\n";
			for (CFuncDescArray::size_type ndx = 0; ndx < pFuncDescFile->GetFuncCount(); ++ndx) {
				const CFuncDesc &zFunc = pFuncDescFile->GetFunc(ndx);

				CStringArray arrDFRO;
				CStringArray arrFuncLines;

				std::string::size_type nDFROMax = 0;
				std::string::size_type nFuncMax = 0;
				for (auto const &funcObj : zFunc) {
					TString strTemp = funcObj->ExportToDiff(nFDL);
					nDFROMax = std::max(nDFROMax, strTemp.size());
					arrDFRO.push_back(strTemp);

					strTemp = funcObj->CreateOutputLine(m_bOutputOptionAddAddress ? OO_ADD_ADDRESS : OO_NONE);
					nFuncMax = std::max(nFuncMax, strTemp.size());
					arrFuncLines.push_back(strTemp);
				}

				TString strFuncHeader = "Function \"" + zFunc.GetMainName() + "\" ("
										+ std::to_string(ndx+1) + "):";
				fileDFRO << strFuncHeader << "\n" << std::string(strFuncHeader.size(), '-') << "\n";
				if (!bDFROWithCodeFlag ) {
					std::copy(arrDFRO.cbegin(), arrDFRO.cend(), std::ostream_iterator<TString>(fileDFRO, "\n"));
				} else {
					for (CStringArray::size_type ndx = 0; ndx < arrDFRO.size(); ++ndx) {
						fileDFRO << padString(arrDFRO.at(ndx), nDFROMax) << "  ->  " << arrFuncLines.at(ndx) << "\n";
					}
				}
				fileDFRO << "\n\n";
			}
		}

		m_arrFuncFiles.push_back(pFuncDescFile);
	}

//m_arrFuncFiles.CompareFunctions(FCM_DYNPROG_GREEDY, 0, 31, 1, 32, true);
//return;

	if (fileMatrixOut.is_open() ||
		fileComp.is_open() ||
		fileOES.is_open() ||
		fileSym.is_open()) {

		if (m_strMatrixInFilename.empty()) {
			std::cout << "Cross-Comparing Functions...\n";
		} else {
			std::cout << "Using specified Cross-Comparison Matrix File: \"" << m_strMatrixInFilename << "\"...\n";
			fileMatrixIn.open(m_strMatrixInFilename, std::ios_base::in);
			if (!fileMatrixIn.is_open()) {
				std::cerr << "*** Error: Opening Matrix Input File \"" << m_strMatrixInFilename << "\" for reading...\n";
				return -4;
			}
		}

		assert(m_arrFuncFiles.size() >= 2);
		std::shared_ptr<const CFuncDescFile> pFuncFile1 = m_arrFuncFiles.at(0);
		std::shared_ptr<const CFuncDescFile> pFuncFile2 = m_arrFuncFiles.at(1);


		// Allocate Memory:
		for (CFuncDescArray::size_type ndx = 0; ndx < pFuncFile1->GetFuncCount(); ++ndx) {
			m_matrixFuncCompResult.push_back(CCompResultArray(pFuncFile2->GetFuncCount()));
		}
		for (CFuncDescArray::size_type ndx = 0; ndx < pFuncFile1->GetDataBlockCount(); ++ndx) {
			m_matrixDataCompResult.push_back(CCompResultArray(pFuncFile2->GetDataBlockCount()));
		}

		// Read Matrix file if using it for cross-compare info:
		if (fileMatrixIn.is_open()) {
			TString strLine;
			std::getline(fileMatrixIn, strLine);
			bool bMatrixMatchesFunctions = true;		// True if the matrix input file matches the function files

			CStringArray arrMatrixSize = parseCSVLine(strLine);
			CFuncDescArray::size_type nTempFunc1Size = 0;
			CFuncDescArray::size_type nTempFunc2Size = 0;

			if (arrMatrixSize.size() == 2) {
				nTempFunc1Size = atoi(trim(arrMatrixSize[0]).c_str());
				nTempFunc2Size = atoi(trim(arrMatrixSize[1]).c_str());
			}

			if ((nTempFunc1Size != pFuncFile1->GetFuncCount()) ||
				(nTempFunc2Size != pFuncFile2->GetFuncCount())) {
				bMatrixMatchesFunctions = false;
			} else {
				CStringArray arrMatrixLine;
				bool bReadGood = true;
				std::getline(fileMatrixIn, strLine);	// Read and compare function names on break-points line
				int nLine = 2;		// We already read the first two lines
				arrMatrixLine = parseCSVLine(strLine);
				if (arrMatrixLine.size() != (nTempFunc2Size+1)) {
					bReadGood = false;
				} else {
					// Check the function names to make sure they match:
					for (CFuncDescArray::size_type ndxFile2 = 0; ((ndxFile2 < nTempFunc2Size) && bMatrixMatchesFunctions); ++ndxFile2) {
						if (arrMatrixLine[ndxFile2+1] != pFuncFile2->GetFunc(ndxFile2).GetMainName()) {
							std::cerr <<"*** Expected function \"" << pFuncFile2->GetFunc(ndxFile2).GetMainName() <<
										"\" on line " << nLine << ", column " << static_cast<int>(ndxFile2+1) << " of matrix file, but found \"" << arrMatrixLine[ndxFile2+1] << "\"\n";
							bMatrixMatchesFunctions = false;
						}
					}
				}
				for (CFuncDescArray::size_type ndxFile1 = 0; ((ndxFile1 < nTempFunc1Size) && bReadGood && bMatrixMatchesFunctions); ++ndxFile1) {
					if (!fileMatrixIn.good() || fileMatrixIn.eof()) {
						bReadGood = false;
						continue;
					}

					std::getline(fileMatrixIn, strLine);
					++nLine;
					arrMatrixLine = parseCSVLine(strLine);

					if (arrMatrixLine.size() != (nTempFunc2Size+1)) {	// Will be +1 due to 'Y' break-points
						bReadGood = false;
						continue;
					} else if (arrMatrixLine[0] != pFuncFile1->GetFunc(ndxFile1).GetMainName()) {
						std::cerr <<"*** Expected function \"" << pFuncFile1->GetFunc(ndxFile1).GetMainName() <<
									"\" on line " << nLine << " of matrix file, but found \"" << arrMatrixLine[0] << "\"\n";
						bMatrixMatchesFunctions = false;
						continue;
					}
					for (CFuncDescArray::size_type ndxFile2 = 0; ndxFile2 < nTempFunc2Size; ++ndxFile2) {
						m_matrixFuncCompResult[ndxFile1][ndxFile2] = atof(trim(arrMatrixLine[ndxFile2+1]).c_str());	// +1 to skip Y breakpoint
					}
				}
				if (!bReadGood && bMatrixMatchesFunctions) {	// Only print bad file warning if things are matching, otherwise print the mismatch error below
					std::cerr << "*** Warning: Failed to read Input Matrix File.\n"
									"        Bad line at " << nLine << ".\n"
									"        Reverting to perform a full cross-comparison.\n";
					m_strMatrixInFilename.clear();
				}
			}

			fileMatrixIn.close();

			if (!bMatrixMatchesFunctions) {
				std::cerr << "*** Warning: Specified Input Matrix File doesn't match\n"
								"        the specified function description files.  A full\n"
								"        cross-comparison will be performed!\n\n";
				m_strMatrixInFilename.clear();
			}
		}

		// If no MatrixIn file was specified or we failed to read it,
		//	do complete cross comparison:
		if (m_strMatrixInFilename.empty()) {
			std::cerr << "Computing Function Comparison : Please Wait";
			if (fileMatrixOut.is_open()) {
				// Write 'X' breakpoints:
				fileMatrixOut << pFuncFile1->GetFuncCount() << "," << pFuncFile2->GetFuncCount() << "\n";
				for (CFuncDescArray::size_type ndxFile2 = 0; ndxFile2 < pFuncFile2->GetFuncCount(); ++ndxFile2) {
					fileMatrixOut << "," << pFuncFile2->GetFunc(ndxFile2).GetMainName();
				}
				fileMatrixOut << "\n";
			}

			// Compute the Function Comparison Results:
			if (bSingleThreaded) {
				// Single-Threaded Version:
				for (CFuncDescArray::size_type ndxFile1 = 0; ndxFile1 < pFuncFile1->GetFuncCount(); ++ndxFile1) {
					std::cerr << ".";
					if (fileMatrixOut.is_open()) {
						fileMatrixOut << pFuncFile1->GetFunc(ndxFile1).GetMainName();		// Y breakpoint
					}
					for (CFuncDescArray::size_type ndxFile2 = 0; ndxFile2 < pFuncFile2->GetFuncCount(); ++ndxFile2) {
						m_matrixFuncCompResult[ndxFile1][ndxFile2] = CompareFunctions(FCT_FUNCTIONS, nCompMethod, *pFuncFile1, ndxFile1, *pFuncFile2, ndxFile2, false);
						if (fileMatrixOut.is_open()) {
							char arrTemp[30];
							std::sprintf(arrTemp, ",%.12g", m_matrixFuncCompResult[ndxFile1][ndxFile2]);
							fileMatrixOut << arrTemp;
						}
					}
					if (fileMatrixOut.is_open()) {
						fileMatrixOut << "\n";
					}
				}
			} else {
				// Multi-Threaded Version:
				CFuncDescArray::size_type nThreadGroupSize = pFuncFile1->GetFuncCount() / threadCount();	// How many things each thread will be processing
				CFuncDescArray::size_type nRemainingSize = pFuncFile1->GetFuncCount() - (nThreadGroupSize * threadCount());		// Remaining functions not covered by threads
				std::vector<std::unique_ptr<std::thread>> arrThreads;
				// "stripe" the function list giving an equal number of large and small functions
				//		to each thread.  The SortedFunctionMap of the function files have them
				//		in order, so we assign the first function to the first thread, the second
				//		to the second thread, and so on.  The main thread will bring up the rear,
				//		handling the last "stripe" and any remaining functions not evenly divisible
				//		by the number of threads:
				auto const &&fnCompare = [&](CFuncDescArray::size_type nStartOffset, CFuncDescArray::size_type nAdvance, CFuncDescArray::size_type nCount)->void {
					CFunctionSizeMultimap::const_iterator itrFuncMap = pFuncFile1->GetSortedFunctionMap().cbegin();
					std::advance(itrFuncMap, nStartOffset);
					while (nCount > 0) {
						std::cerr << ".";		// On C++20, these are synchronized for multi-thread writes, only the data order isn't guaranteed, but just printing "." should be fine
						for (CFuncDescArray::size_type ndxFile2 = 0; ndxFile2 < pFuncFile2->GetFuncCount(); ++ndxFile2) {
							m_matrixFuncCompResult[itrFuncMap->second][ndxFile2] = CompareFunctions(FCT_FUNCTIONS, nCompMethod, *pFuncFile1, itrFuncMap->second, *pFuncFile2, ndxFile2, false);
						}
						std::advance(itrFuncMap, nAdvance);
						--nCount;
					}
				};

				// Spawn the threads:
				unsigned int nThreadCount = threadCount() - 1;
				for (unsigned int nThread = 0; nThread < nThreadCount; ++nThread) {
					arrThreads.push_back(std::make_unique<std::thread>(fnCompare, nThread, nThreadCount+1, nThreadGroupSize));
				}
				// Compute remaining on main-thread:
				fnCompare(nThreadCount, nThreadCount+1, nThreadGroupSize);
				if (nRemainingSize) fnCompare((nThreadCount+1)*nThreadGroupSize, 1, nRemainingSize);
				// Wait for other threads to finish:
				for (auto &pThread : arrThreads) {
					pThread->join();
				}

				// Then output it to the file:
				if (fileMatrixOut.is_open()) {
					char arrTemp[30];
					for (CFuncDescArray::size_type ndxFile1 = 0; ndxFile1 < pFuncFile1->GetFuncCount(); ++ndxFile1) {
						fileMatrixOut << pFuncFile1->GetFunc(ndxFile1).GetMainName();		// Y breakpoint
						for (CFuncDescArray::size_type ndxFile2 = 0; ndxFile2 < pFuncFile2->GetFuncCount(); ++ndxFile2) {
							std::sprintf(arrTemp, ",%.12g", m_matrixFuncCompResult[ndxFile1][ndxFile2]);
							fileMatrixOut << arrTemp;
						}
						fileMatrixOut << "\n";
					}
				}
			}

			std::cerr << "\n\n";
		}

		// Compute the Data Block Comparison Results:
		//		Currently, Data Block results are not written to
		//		the matrix file (should it?).  So, we must always
		//		calculate it:
		std::cerr << "Computing Data Block Comparison : Please Wait";
		for (CFuncDescArray::size_type ndxFile1 = 0; ndxFile1 < pFuncFile1->GetDataBlockCount(); ++ndxFile1) {
			std::cerr << ".";
			for (CFuncDescArray::size_type ndxFile2 = 0; ndxFile2 < pFuncFile2->GetDataBlockCount(); ++ndxFile2) {
				m_matrixDataCompResult[ndxFile1][ndxFile2] = CompareFunctions(FCT_DATABLOCKS, nCompMethod, *pFuncFile1, ndxFile1, *pFuncFile2, ndxFile2, false);
			}
		}


		if (fileComp.is_open()) {
			fileComp << "Left Filename  : ";
			fileComp << pFuncFile1->GetFuncPathName() + "\n";
			fileComp << "Right Filename : ";
			fileComp << pFuncFile2->GetFuncPathName() + "\n";
			fileComp << "\n";
		}

		if (fileOES.is_open()) {
			fileOES << "; Left Filename  : ";
			fileOES << pFuncFile1->GetFuncPathName() + "\n";
			fileOES << "; Right Filename : ";
			fileOES << pFuncFile2->GetFuncPathName() + "\n";
		}

		if (fileSym.is_open()) {
			fileSym << "; Left Filename  : ";
			fileSym << pFuncFile1->GetFuncPathName() + "\n";
			fileSym << "; Right Filename : ";
			fileSym << pFuncFile2->GetFuncPathName() + "\n";
		}

		// --------------------------------------------------------------------

		typedef std::pair<TLabel, TLabel> TFunctionPair;
		std::set<TFunctionPair> setCompsWritten;		// Pairing of Left/Right functions already outputted
		std::ostringstream ssCompResult;

		// Output Function comparison of Left->Right
		std::cout << "\nBest Function Matches (Left->Right):\n";
		if (fileComp.is_open()) {
			fileComp << "================================================================================\n";
			fileComp << "                      Best Function Matches (Left->Right):\n";
			fileComp << "================================================================================\n\n";
		}
		for (CFuncDescArray::size_type ndxFile1 = 0; ndxFile1 < pFuncFile1->GetFuncCount(); ++ndxFile1) {
			nMaxCompResult = 0.0;
			for (CFuncDescArray::size_type ndxFile2 = 0; ndxFile2 < pFuncFile2->GetFuncCount(); ++ndxFile2) nMaxCompResult = std::max(nMaxCompResult, m_matrixFuncCompResult[ndxFile1][ndxFile2]);
			bFlag = false;
			ssCompResult.str(std::string());
			if ((nMaxCompResult > 0.0) && (nMaxCompResult >= nMinCompLimit)) {
				ssCompResult << "    " << padString(pFuncFile1->GetFunc(ndxFile1).GetMainName(), CFuncObject::GetFieldWidth(CFuncObject::FIELD_CODE::FC_LABEL)) << " : ";
				for (CFuncDescArray::size_type ndxFile2 = 0; ndxFile2 < pFuncFile2->GetFuncCount(); ++ndxFile2) {
					if (m_matrixFuncCompResult[ndxFile1][ndxFile2] < nMaxCompResult) continue;

					TFunctionPair thisFunctionPair(pFuncFile1->GetFunc(ndxFile1).GetMainName(),
														 pFuncFile2->GetFunc(ndxFile2).GetMainName());
					if (setCompsWritten.contains(thisFunctionPair)) continue;

					if (bFlag) {
						if (fileComp.is_open()) {
							fileComp << "\n\n";
						}
						ssCompResult << ", ";
					} else {
						if (fileComp.is_open()) {
							fileComp << "================================================================================\n";
						}
					}
					bFlag = true;

					ssCompResult << pFuncFile2->GetFunc(ndxFile2).GetMainName();

					dumpComparison(FCT_FUNCTIONS, fileComp, fileOES, bCompOESFlag, m_matrixFuncCompResult,
									nCompMethod, pFuncFile1, ndxFile1, pFuncFile2, ndxFile2,
									(m_bOutputOptionAddAddress ? OO_ADD_ADDRESS : OO_NONE),
									&aSymbolMap);

					setCompsWritten.insert(thisFunctionPair);
				}
			}
			if (bFlag) {
				if (fileComp.is_open()) {
					fileComp << "================================================================================\n\n\n";
				}
				ssCompResult << " : (" << nMaxCompResult*100 << "%)\n";
				std::cout << ssCompResult.str();
			}
		}

		// Output Function comparison of Right->Left : This is needed because some functions
		//		on left may match multiple functions on the right, but not "optimally".
		//		This will add the right-side's best matches on the left so that all
		//		best matches are shown:
		std::cout << "\nBest Function Matches (Right->Left):\n";
		if (fileComp.is_open()) {
			fileComp << "================================================================================\n";
			fileComp << "                      Best Function Matches (Right->Left):\n";
			fileComp << "================================================================================\n\n";
		}
		for (CFuncDescArray::size_type ndxFile2 = 0; ndxFile2 < pFuncFile2->GetFuncCount(); ++ndxFile2) {
			nMaxCompResult = 0.0;
			for (CFuncDescArray::size_type ndxFile1 = 0; ndxFile1 < pFuncFile1->GetFuncCount(); ++ndxFile1) nMaxCompResult = std::max(nMaxCompResult, m_matrixFuncCompResult[ndxFile1][ndxFile2]);
			bFlag = false;
			ssCompResult.str(std::string());
			if ((nMaxCompResult > 0.0) && (nMaxCompResult >= nMinCompLimit)) {
				ssCompResult << "    " << padString(pFuncFile2->GetFunc(ndxFile2).GetMainName(), CFuncObject::GetFieldWidth(CFuncObject::FIELD_CODE::FC_LABEL)) << " : ";
				for (CFuncDescArray::size_type ndxFile1 = 0; ndxFile1 < pFuncFile1->GetFuncCount(); ++ndxFile1) {
					if (m_matrixFuncCompResult[ndxFile1][ndxFile2] < nMaxCompResult) continue;

					TFunctionPair thisFunctionPair(pFuncFile1->GetFunc(ndxFile1).GetMainName(),
														 pFuncFile2->GetFunc(ndxFile2).GetMainName());
					if (setCompsWritten.contains(thisFunctionPair)) continue;

					if (bFlag) {
						if (fileComp.is_open()) {
							fileComp << "\n\n";
						}
						ssCompResult << ", ";
					} else {
						if (fileComp.is_open()) {
							fileComp << "================================================================================\n";
						}
					}
					bFlag = true;

					ssCompResult << pFuncFile1->GetFunc(ndxFile1).GetMainName();

					dumpComparison(FCT_FUNCTIONS, fileComp, fileOES, bCompOESFlag, m_matrixFuncCompResult,
									nCompMethod, pFuncFile1, ndxFile1, pFuncFile2, ndxFile2,
									(m_bOutputOptionAddAddress ? OO_ADD_ADDRESS : OO_NONE),
									&aSymbolMap);

					setCompsWritten.insert(thisFunctionPair);
				}
			}
			if (bFlag) {
				if (fileComp.is_open()) {
					fileComp << "================================================================================\n\n\n";
				}
				ssCompResult << " : (" << nMaxCompResult*100 << "%)\n";
				std::cout << ssCompResult.str();
			}
		}

		// --------------------------------------------------------------------

		std::cout << "\nFunctions in \"" << pFuncFile1->GetFuncPathName() << "\" with No Matches:\n";

		bFlag = true;
		for (CFuncDescArray::size_type ndxFile1 = 0; ndxFile1 < pFuncFile1->GetFuncCount(); ++ndxFile1) {
			nMaxCompResult = 0.0;
			for (CFuncDescArray::size_type ndxFile2 = 0; ndxFile2 < pFuncFile2->GetFuncCount(); ++ndxFile2) nMaxCompResult = std::max(nMaxCompResult, m_matrixFuncCompResult[ndxFile1][ndxFile2]);
			if ((nMaxCompResult <= 0.0) || (nMaxCompResult < nMinCompLimit)) {
				std::cout << "    " << pFuncFile1->GetFunc(ndxFile1).GetMainName() << "\n";
				bFlag = false;
			}
		}
		if (bFlag) std::cout << "    <None>\n";

		std::cout << "\nFunctions in \"" << pFuncFile2->GetFuncPathName() << "\" with No Matches:\n";

		bFlag = true;
		for (CFuncDescArray::size_type ndxFile2 = 0; ndxFile2 < pFuncFile2->GetFuncCount(); ++ndxFile2) {
			nMaxCompResult = 0.0;
			for (CFuncDescArray::size_type ndxFile1 = 0; ndxFile1 < pFuncFile1->GetFuncCount(); ++ndxFile1) nMaxCompResult = std::max(nMaxCompResult, m_matrixFuncCompResult[ndxFile1][ndxFile2]);
			if ((nMaxCompResult <= 0.0) || (nMaxCompResult < nMinCompLimit)) {
				std::cout << "    " << pFuncFile2->GetFunc(ndxFile2).GetMainName() << "\n";
				bFlag = false;
			}
		}
		if (bFlag) std::cout << "    <None>\n";

		// --------------------------------------------------------------------

		setCompsWritten.clear();

		// Output Data Block comparison of Left->Right
		std::cout << "\nBest Data Block Matches (Left->Right):\n";
		if (fileComp.is_open()) {
			fileComp << "================================================================================\n";
			fileComp << "                     Best Data Block Matches (Left->Right):\n";
			fileComp << "================================================================================\n\n";
		}
		for (CFuncDescArray::size_type ndxFile1 = 0; ndxFile1 < pFuncFile1->GetDataBlockCount(); ++ndxFile1) {
			nMaxCompResult = 0.0;
			for (CFuncDescArray::size_type ndxFile2 = 0; ndxFile2 < pFuncFile2->GetDataBlockCount(); ++ndxFile2) nMaxCompResult = std::max(nMaxCompResult, m_matrixDataCompResult[ndxFile1][ndxFile2]);
			bFlag = false;
			ssCompResult.str(std::string());
			if ((nMaxCompResult > 0.0) && (nMaxCompResult >= nMinCompLimit)) {
				ssCompResult << "    " << padString(pFuncFile1->GetDataBlock(ndxFile1).GetMainName(), CFuncObject::GetFieldWidth(CFuncObject::FIELD_CODE::FC_LABEL)) << " : ";
				for (CFuncDescArray::size_type ndxFile2 = 0; ndxFile2 < pFuncFile2->GetDataBlockCount(); ++ndxFile2) {
					if (m_matrixDataCompResult[ndxFile1][ndxFile2] < nMaxCompResult) continue;

					TFunctionPair thisFunctionPair(pFuncFile1->GetDataBlock(ndxFile1).GetMainName(),
														 pFuncFile2->GetDataBlock(ndxFile2).GetMainName());
					if (setCompsWritten.contains(thisFunctionPair)) continue;

					if (bFlag) {
						if (fileComp.is_open()) {
							fileComp << "\n\n";
						}
						ssCompResult << ", ";
					} else {
						if (fileComp.is_open()) {
							fileComp << "================================================================================\n";
						}
					}
					bFlag = true;

					ssCompResult << pFuncFile2->GetDataBlock(ndxFile2).GetMainName();

					dumpComparison(FCT_DATABLOCKS, fileComp, fileOES, bCompOESFlag, m_matrixDataCompResult,
									nCompMethod, pFuncFile1, ndxFile1, pFuncFile2, ndxFile2,
									(m_bOutputOptionAddAddress ? OO_ADD_ADDRESS : OO_NONE),
									&aSymbolMap);

					setCompsWritten.insert(thisFunctionPair);
				}
			}
			if (bFlag) {
				if (fileComp.is_open()) {
					fileComp << "================================================================================\n\n\n";
				}
				ssCompResult << " : (" << nMaxCompResult*100 << "%)\n";
				std::cout << ssCompResult.str();
			}
		}

		// Output Data Block comparison of Right->Left : This is needed because some functions
		//		on left may match multiple functions on the right, but not "optimally".
		//		This will add the right-side's best matches on the left so that all
		//		best matches are shown:
		std::cout << "\nBest Data Block Matches (Right->Left):\n";
		if (fileComp.is_open()) {
			fileComp << "================================================================================\n";
			fileComp << "                     Best Data Block Matches (Right->Left):\n";
			fileComp << "================================================================================\n\n";
		}
		for (CFuncDescArray::size_type ndxFile2 = 0; ndxFile2 < pFuncFile2->GetDataBlockCount(); ++ndxFile2) {
			nMaxCompResult = 0.0;
			for (CFuncDescArray::size_type ndxFile1 = 0; ndxFile1 < pFuncFile1->GetDataBlockCount(); ++ndxFile1) nMaxCompResult = std::max(nMaxCompResult, m_matrixDataCompResult[ndxFile1][ndxFile2]);
			bFlag = false;
			ssCompResult.str(std::string());
			if ((nMaxCompResult > 0.0) && (nMaxCompResult >= nMinCompLimit)) {
				ssCompResult << "    " << padString(pFuncFile2->GetDataBlock(ndxFile2).GetMainName(), CFuncObject::GetFieldWidth(CFuncObject::FIELD_CODE::FC_LABEL)) << " : ";
				for (CFuncDescArray::size_type ndxFile1 = 0; ndxFile1 < pFuncFile1->GetDataBlockCount(); ++ndxFile1) {
					if (m_matrixDataCompResult[ndxFile1][ndxFile2] < nMaxCompResult) continue;

					TFunctionPair thisFunctionPair(pFuncFile1->GetDataBlock(ndxFile1).GetMainName(),
														 pFuncFile2->GetDataBlock(ndxFile2).GetMainName());
					if (setCompsWritten.contains(thisFunctionPair)) continue;

					if (bFlag) {
						if (fileComp.is_open()) {
							fileComp << "\n\n";
						}
						ssCompResult << ", ";
					} else {
						if (fileComp.is_open()) {
							fileComp << "================================================================================\n";
						}
					}
					bFlag = true;

					ssCompResult << pFuncFile1->GetDataBlock(ndxFile1).GetMainName();

					dumpComparison(FCT_DATABLOCKS, fileComp, fileOES, bCompOESFlag, m_matrixDataCompResult,
									nCompMethod, pFuncFile1, ndxFile1, pFuncFile2, ndxFile2,
									(m_bOutputOptionAddAddress ? OO_ADD_ADDRESS : OO_NONE),
									&aSymbolMap);

					setCompsWritten.insert(thisFunctionPair);
				}
			}
			if (bFlag) {
				if (fileComp.is_open()) {
					fileComp << "================================================================================\n\n\n";
				}
				ssCompResult << " : (" << nMaxCompResult*100 << "%)\n";
				std::cout << ssCompResult.str();
			}
		}

		// --------------------------------------------------------------------

		std::cout << "\nData Blocks in \"" << pFuncFile1->GetFuncPathName() << "\" with No Matches:\n";

		bFlag = true;
		for (CFuncDescArray::size_type ndxFile1 = 0; ndxFile1 < pFuncFile1->GetDataBlockCount(); ++ndxFile1) {
			nMaxCompResult = 0.0;
			for (CFuncDescArray::size_type ndxFile2 = 0; ndxFile2 < pFuncFile2->GetDataBlockCount(); ++ndxFile2) nMaxCompResult = std::max(nMaxCompResult, m_matrixDataCompResult[ndxFile1][ndxFile2]);
			if ((nMaxCompResult <= 0.0) || (nMaxCompResult < nMinCompLimit)) {
				std::cout << "    " << pFuncFile1->GetDataBlock(ndxFile1).GetMainName() << "\n";
				bFlag = false;
			}
		}
		if (bFlag) std::cout << "    <None>\n";

		std::cout << "\nData Blocks in \"" << pFuncFile2->GetFuncPathName() << "\" with No Matches:\n";

		bFlag = true;
		for (CFuncDescArray::size_type ndxFile2 = 0; ndxFile2 < pFuncFile2->GetDataBlockCount(); ++ndxFile2) {
			nMaxCompResult = 0.0;
			for (CFuncDescArray::size_type ndxFile1 = 0; ndxFile1 < pFuncFile1->GetDataBlockCount(); ++ndxFile1) nMaxCompResult = std::max(nMaxCompResult, m_matrixDataCompResult[ndxFile1][ndxFile2]);
			if ((nMaxCompResult <= 0.0) || (nMaxCompResult < nMinCompLimit)) {
				std::cout << "    " << pFuncFile2->GetDataBlock(ndxFile2).GetMainName() << "\n";
				bFlag = false;
			}
		}
		if (bFlag) std::cout << "    <None>\n";

		// --------------------------------------------------------------------

		std::cout << "\nCross-Comparing Symbol Tables...\n";

		dumpSymbols(fileSym, aSymbolMap, CSymbolMap::GetLeftSideCodeSymbolList, CSymbolMap::GetLeftSideCodeHitList,
					"\nLeft-Side Code Symbol Matches:\n"
					"------------------------------\n");

		dumpSymbols(fileSym, aSymbolMap, CSymbolMap::GetLeftSideDataSymbolList, CSymbolMap::GetLeftSideDataHitList,
					"\nLeft-Side Data Symbol Matches:\n"
					"------------------------------\n");

		dumpSymbols(fileSym, aSymbolMap, CSymbolMap::GetRightSideCodeSymbolList, CSymbolMap::GetRightSideCodeHitList,
					"\nRight-Side Code Symbol Matches:\n"
					"-------------------------------\n");

		dumpSymbols(fileSym, aSymbolMap, CSymbolMap::GetRightSideDataSymbolList, CSymbolMap::GetRightSideDataHitList,
					"\nRight-Side Data Symbol Matches:\n"
					"-------------------------------\n");
	}

	printf("\nFunction Analysis Complete...\n\n");

	if (fileMatrixOut.is_open()) fileMatrixOut.close();
	if (fileDFRO.is_open()) fileDFRO.close();
	if (fileComp.is_open()) fileComp.close();
	if (fileOES.is_open()) fileOES.close();
	if (fileSym.is_open()) fileSym.close();

	return 0;
}

// ============================================================================

