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
#include <string_view>
#include <vector>

typedef std::string TString;
typedef std::vector<TString> CStringArray;

// ============================================================================

static inline bool contains(const CStringArray &arr, const TString &s)
{
	return (std::find(arr.cbegin(), arr.cend(), s) != arr.cend());
}

// trim from start
static inline TString &ltrim(TString &s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

// trim from end
static inline TString &rtrim(TString &s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

// trim from both ends
static inline TString &trim(TString &s)
{
	return ltrim(rtrim(s));
}

static inline TString &makeUpper(TString &s)
{
	std::transform(s.begin(), s.end(), s.begin(), ::toupper);
	return s;
}

static inline TString makeUpperCopy(const TString &s)
{
	TString strCopy = s;
	std::transform(strCopy.begin(), strCopy.end(), strCopy.begin(), ::toupper);
	return strCopy;
}

static inline int compareNoCase(const TString &s1, const TString &s2)
{
	TString strUpper1 = s1;
	makeUpper(strUpper1);
	TString strUpper2 = s2;
	makeUpper(strUpper2);
	return ((strUpper1 < strUpper2) ? -1 : ((strUpper1 > strUpper2) ? 1 : 0));
}

static bool equals(std::string_view s1, std::string_view s2) {
	return (s1.size() == s2.size())
			&& std::equal(s1.begin(),s1.end(),s2.begin());
}

static bool starts_with(std::string_view s, std::string_view prefix) {
	return equals(s.substr(0,prefix.size()), prefix);
}

// padString : Pads a string with spaces up to the specified length.
static TString padString(const TString &s, TString::size_type nWidth, TString::value_type chrPadChar = ' ', bool bPrepend = false)
{
	TString strRetVal = s;

	if (!bPrepend) {
		if (strRetVal.size() < nWidth) strRetVal.append(nWidth-strRetVal.size(), chrPadChar);
	} else {
		if (strRetVal.size() < nWidth) strRetVal = std::string().append(nWidth-strRetVal.size(), chrPadChar) + strRetVal;
	}
	return strRetVal;
}

// ============================================================================

#endif		// STRING_HELP_H
