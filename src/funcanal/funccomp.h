//
//	Fuzzy Function Comparison Logic
//
//
//	Fuzzy Function Analyzer
//	for the Generic Code-Seeking Disassembler
//	Copyright(c)2021 by Donna Whisnant
//

#ifndef FUNC_COMP_H_
#define FUNC_COMP_H_

#include <vector>

#include <stringhelp.h>
#include <enumhelp.h>

class CFuncDescFile;
class CSymbolMap;

enum FUNC_COMPARE_METHOD {
	FCM_DYNPROG_XDROP = 0,
	FCM_DYNPROG_GREEDY = 1,
	FCM_COUNT
};

enum FUNC_DIFF_LEVEL {
	FDL_1 = 0,				// Function Diff Level #1 -- Most generic
	FDL_2 = 1,				// Function Diff Level #2
	// ----					//                        -- Least generic
	NUM_FUNC_DIFF_LEVELS,
};

// Output Options:
//		These are OR'd bit fields of output-options used
//		in diff and CreateOutputLine methods
enum OUTPUT_OPTIONS {
	OO_NONE = 0,
	OO_ADD_ADDRESS = 1,
};
DEFINE_ENUM_FLAG_OPERATORS(OUTPUT_OPTIONS);

double CompareFunctions(FUNC_COMPARE_METHOD nMethod,
						const CFuncDescFile &file1, std::size_t nFile1FuncNdx,
						const CFuncDescFile &file2, std::size_t nFile2FuncNdx,
						bool bBuildEditScript);
bool GetLastEditScript(CStringArray &anArray);
TString DiffFunctions(FUNC_COMPARE_METHOD nMethod,
						const CFuncDescFile &file1, std::size_t nFile1FuncNdx,
						const CFuncDescFile &file2, std::size_t nFile2FuncNdx,
						OUTPUT_OPTIONS nOutputOptions,
						double &nMatchPercent,
						CSymbolMap *pSymbolMap = nullptr);

#endif	// FUNC_COMP_H_

