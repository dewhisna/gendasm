//
//	String Helper Functions
//
//
//	Generic Code-Seeking Disassembler
//	Copyright(c)2021 by Donna Whisnant
//

#ifndef STRING_HELP_H
#define STRING_HELP_H

#include <string>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>

// ============================================================================

// trim from start
static inline std::string &ltrim(std::string &s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

// trim from end
static inline std::string &rtrim(std::string &s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

// trim from both ends
static inline std::string &trim(std::string &s)
{
	return ltrim(rtrim(s));
}

static inline std::string &makeUpper(std::string &s)
{
	std::transform(s.begin(), s.end(), s.begin(), ::toupper);
	return s;
}

static inline std::string makeUpperCopy(const std::string &s)
{
	std::string strCopy = s;
	std::transform(strCopy.begin(), strCopy.end(), strCopy.begin(), ::toupper);
	return strCopy;
}

static inline int compareNoCase(const std::string &s1, const std::string &s2)
{
	std::string strUpper1 = s1;
	makeUpper(strUpper1);
	std::string strUpper2 = s2;
	makeUpper(strUpper2);
	return ((strUpper1 < strUpper2) ? -1 : ((strUpper1 > strUpper2) ? 1 : 0));
}

// ============================================================================

#endif		// STRING_HELP_H
