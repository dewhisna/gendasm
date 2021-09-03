//
//	ELF Data File Converter Class
//
//
//	Generic Code-Seeking Disassembler
//	Copyright(c)2021 by Donna Whisnant
//

#include <iostream>
#include <string>
#include <time.h>
#include <cstdio>
#include <iterator>
#include <algorithm>
#include <utility>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

#include "elfdfc.h"
#include "gdc.h"

#include <stringhelp.h>

#include <assert.h>

#ifdef __GLIBCXX__
// GCC extension to get file descripter from i/o stream:
#include <ext/stdio_filebuf.h>
#endif

#include <libelf.h>
#include <gelf.h>
#include <elfutils/version.h>
#include <nlist.h>

#ifndef UNUSED
	#define UNUSED(x) ((void)(x))
#endif

#define DEBUG_ELF_FILE 0			// Set to '1' (or non-zero) to debug ELF header reading
#define REPORT_SYMTABLE 1			// Set to '1' (or non-zero) to output symbol tables in output stream

// ============================================================================

namespace {
	typedef std::map<GElf_Word, TString> CELFInfoMap;

	static const CELFInfoMap g_mapPHdrType = {
		{ PT_NULL, "NULL" },
		{ PT_LOAD, "LOAD" },
		{ PT_DYNAMIC, "DYNAMIC" },
		{ PT_INTERP, "INTERP" },
		{ PT_NOTE, "NOTE" },
		{ PT_SHLIB, "SHLIB" },
		{ PT_PHDR, "PHDR" },
		{ PT_TLS, "TLS" },
	};

	static const CELFInfoMap g_mapSHdrType = {
		{ SHT_NULL, "NULL" },
		{ SHT_PROGBITS, "PROGBITS" },
		{ SHT_SYMTAB, "SYMTAB" },
		{ SHT_STRTAB, "STRTAB" },
		{ SHT_RELA, "RELA" },
		{ SHT_HASH, "HASH" },
		{ SHT_DYNAMIC, "DYNAMIC" },
		{ SHT_NOTE, "NOTE" },
		{ SHT_NOBITS, "NOBITS" },
		{ SHT_REL, "REL" },
		{ SHT_SHLIB, "SHLIB" },
		{ SHT_DYNSYM, "DYNSYM" },
		{ SHT_INIT_ARRAY, "INIT_ARRAY" },
		{ SHT_FINI_ARRAY, "FINI_ARRAY" },
		{ SHT_PREINIT_ARRAY, "PREINIT_ARRAY" },
		{ SHT_GROUP, "GROUP" },
		{ SHT_SYMTAB_SHNDX, "SYMTAB_SHNDX" },
	};

	static const CELFInfoMap g_mapSymType = {
		{ STT_NOTYPE, "NOTYPE" },
		{ STT_OBJECT, "OBJECT" },
		{ STT_FUNC, "FUNC" },
		{ STT_SECTION, "SECTION" },
		{ STT_FILE, "FILE" },
		{ STT_COMMON, "COMMON" },
		{ STT_TLS, "TLS" },
		{ STT_GNU_IFUNC, "IFUNC" },
	};

	static const CELFInfoMap g_mapSymBind = {
		{ STB_LOCAL, "LOCAL" },
		{ STB_GLOBAL, "GLOBAL" },
		{ STB_WEAK, "WEAK" },
		{ STB_GNU_UNIQUE, "UNIQUE" },
	};

	static const CELFInfoMap g_mapSymVis = {
		{ STV_DEFAULT, "DEFAULT" },
		{ STV_INTERNAL, "INTERNAL" },
		{ STV_HIDDEN, "HIDDEN" },
		{ STV_PROTECTED, "PROTECTED" },
	};


	// Main Machine Types:
	// ===================

	// TODO : Keep this list updated with the processors/machines that we
	//		have GDC support for:
	static const CELFInfoMap g_mapMachineType = {
		{ EM_68HC11, "Motorola MC68HC11 microcontroller" },
		{ EM_AVR, "Atmel AVR 8-bit microcontroller" },
	};


	// AVR Processor Specific Flags:
	// =============================

	static constexpr TAddress E_AVR_RAM_VirtualBase = 0x00800100ul;		// Virtual Starting Address in ELF files for AVR RAM
	static constexpr TAddress E_AVR_RAM_VirtualBaseAdj = 0x00800000ul;	// Adjustment value for AVR RAM (amount to subtract from E_AVR_RAM_VirtualBase)
	static constexpr TAddress E_AVR_EE_VirtualBase = 0x00810000ul;		// Virtual Starting Address in ELF files for AVR EE
	static constexpr TAddress E_AVR_EE_VirtualBaseAdj = 0x00810000ul;	// Adjustment value for AVR EE (amount to subtract from E_AVR_EE_VirtualBase)

	/* Processor specific flags for the ELF header e_flags field.  */
	#define EF_AVR_MACH 0x7F

	/* If bit #7 is set, it is assumed that the elf file uses local symbols
	   as reference for the relocations so that linker relaxation is possible.  */
	#define EF_AVR_LINKRELAX_PREPARED 0x80

	#define E_AVR_MACH_AVR1     1
	#define E_AVR_MACH_AVR2     2
	#define E_AVR_MACH_AVR25   25
	#define E_AVR_MACH_AVR3     3
	#define E_AVR_MACH_AVR31   31
	#define E_AVR_MACH_AVR35   35
	#define E_AVR_MACH_AVR4     4
	#define E_AVR_MACH_AVR5     5
	#define E_AVR_MACH_AVR51   51
	#define E_AVR_MACH_AVR6     6
	#define E_AVR_MACH_AVRTINY 100
	#define E_AVR_MACH_XMEGA1  101
	#define E_AVR_MACH_XMEGA2  102
	#define E_AVR_MACH_XMEGA3  103
	#define E_AVR_MACH_XMEGA4  104
	#define E_AVR_MACH_XMEGA5  105
	#define E_AVR_MACH_XMEGA6  106
	#define E_AVR_MACH_XMEGA7  107

	static const CELFInfoMap g_mapAVRType = {
		{	E_AVR_MACH_AVR1,	"avr1  - classic AVR core without data RAM" },
		{	E_AVR_MACH_AVR2,	"avr2  - classic AVR core with up to 8K program memory" },
		{	E_AVR_MACH_AVR25,	"avr25 - classic AVR core with up to 8K program memory plus the MOVW instruction" },
		{	E_AVR_MACH_AVR3,	"avr3  - classic AVR core with up to 64K program memory" },
		{	E_AVR_MACH_AVR31,	"avr31 - classic AVR core with up to 128K program memory" },
		{	E_AVR_MACH_AVR35,	"avr35 - classic AVR core with up to 64K program memory plus the MOVW instruction" },
		{	E_AVR_MACH_AVR4,	"avr4  - enhanced AVR core with up to 8K program memory" },
		{	E_AVR_MACH_AVR5,	"avr5  - enhanced AVR core with up to 64K program memory" },
		{	E_AVR_MACH_AVR51,	"avr51 - enhanced AVR core with up to 128K program memory" },
		{	E_AVR_MACH_AVR6,	"avr6  - enhanced AVR core with up to 256K program memory" },
		{	E_AVR_MACH_XMEGA1,	"avrxmega1 - XMEGA" },		// Can't find more details on this one as it's not in the binutils list
		{	E_AVR_MACH_XMEGA2,	"avrxmega2 - XMEGA, > 8K, < 64K FLASH, < 64K RAM" },
		{	E_AVR_MACH_XMEGA3,	"avrxmega3 - XMEGA, RAM + FLASH < 64K, Flash visible in RAM" },
		{	E_AVR_MACH_XMEGA4,	"avrxmega4 - XMEGA, > 64K, <= 128K FLASH, <= 64K RAM" },
		{	E_AVR_MACH_XMEGA5,	"avrxmega5 - XMEGA, > 64K, <= 128K FLASH, > 64K RAM" },
		{	E_AVR_MACH_XMEGA6,	"avrxmega6 - XMEGA, > 128K, <= 256K FLASH, <= 64K RAM" },
		{	E_AVR_MACH_XMEGA7,	"avrxmega7 - XMEGA, > 128K, <= 256K FLASH, > 64K RAM" },
		{	E_AVR_MACH_AVRTINY,	"avrtiny   - AVR Tiny core with 16 gp registers" },
	};

};

// ----------------------------------------------------------------------------

static void printField(std::ostream &outFile, const TString &strName, int nFieldSize, uint64_t nVal, bool bDecimal = false)
{
	std::string strPrefix = (!strName.empty() ? ("    " + strName + " : ") : "");
	if (!bDecimal) {
		outFile << strPrefix << "0x" << std::uppercase << std::setfill('0') << std::setw(nFieldSize*2) << std::setbase(16)
					<< nVal << std::nouppercase << std::setbase(0);
	} else {
		outFile << strPrefix << nVal;
	}
	if (!strPrefix.empty()) outFile << "\n";
};

static TString getPHdrType(GElf_Word nValue)
{
	CELFInfoMap::const_iterator itrPT = g_mapPHdrType.find(nValue);
	if (itrPT == g_mapPHdrType.cend()) return "unknown";
	return itrPT->second;
};

static TString getSHdrType(GElf_Word nValue)
{
	CELFInfoMap::const_iterator itrST = g_mapSHdrType.find(nValue);
	if (itrST == g_mapSHdrType.cend()) return "unknown";
	return itrST->second;
};

static TString getSymType(unsigned char info)
{
	CELFInfoMap::const_iterator itrST = g_mapSymType.find(GELF_ST_TYPE(info));
	if (itrST == g_mapSymType.cend()) return "unknown";
	return itrST->second;
};

static TString getSymBind(unsigned char info)
{
	CELFInfoMap::const_iterator itrSB = g_mapSymBind.find(GELF_ST_BIND(info));
	if (itrSB == g_mapSymBind.cend()) return "unknown";
	return itrSB->second;
};

static TString getSymVis(unsigned char other)
{
	CELFInfoMap::const_iterator itrSV = g_mapSymVis.find(GELF_ST_VISIBILITY(other));
	if (itrSV == g_mapSymVis.cend()) return "unknown";
	return itrSV->second;
};

// ----------------------------------------------------------------------------

size_t getStrTabSectionIndex(Elf *pElf)
{
	size_t nSHdrNum = 0;
	size_t nSHdrStrNdx = 0;

	if (elf_getshdrnum(pElf, &nSHdrNum) != 0) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
												std::string("getshdrnum() failed: ") + std::string(elf_errmsg(-1)));
	if (elf_getshdrstrndx(pElf, &nSHdrStrNdx) != 0) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
												std::string("getshdrstrndx() failed: ") + std::string(elf_errmsg(-1)));

	size_t nStrTabNdx = 0;		// String table section index
	for (size_t ndx = 0; ndx < nSHdrNum; ++ndx) {
		Elf_Scn *pScn = nullptr;
		GElf_Shdr shdr;
		char *pName;
		pScn = elf_getscn(pElf, ndx);
		if (pScn == nullptr) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
									std::string("getscn() failed: ") + std::string(elf_errmsg(-1)));
		if (gelf_getshdr(pScn, &shdr) != &shdr) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
									std::string("getshdr() failed: ") + std::string(elf_errmsg(-1)));
		pName = elf_strptr(pElf, nSHdrStrNdx, shdr.sh_name);
		if (pName == nullptr) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
									std::string("strptr() failed for string index ") + std::to_string(shdr.sh_name) + ": " + std::string(elf_errmsg(-1)));
		if (compareNoCase(pName, ".strtab") != 0) continue;		// ??? necessary
		if (shdr.sh_type != SHT_STRTAB) continue;
		if (ndx == nSHdrStrNdx) continue;			// Not the .shstrtab
		nStrTabNdx = ndx;
		break;
	}

	return nStrTabNdx;
}

// ----------------------------------------------------------------------------

std::string CELFDataFileConverter::GetShortDescription() const
{
	TString strVersion = std::to_string(_ELFUTILS_VERSION%1000);
	return "ELF Data File Converter (libelf " + std::to_string(_ELFUTILS_VERSION/1000) + "." + padString(strVersion, 3, '0', true) + ")";
}

// ============================================================================

bool CELFDataFileConverter::_ReadDataFile(ELF_READ_MODE_ENUM nReadMode, CDisassembler *pDisassembler, struct Elf *pElf, TAddress nNewBase,
					CMemBlocks *pMemory, CMemRanges *pRange, TDescElement nDesc,
					std::ostream *msgFile, std::ostream *errFile) const
{
	bool bRetVal = true;

	if (pRange) pRange->clear();

	// Basic ELF Header Verification:
	// ------------------------------
	if (elf_kind(pElf) != ELF_K_ELF) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_UNKNOWN_FILE_TYPE, 0, "Not an ELF object file");

	GElf_Ehdr ehdr;

	if (gelf_getehdr(pElf, &ehdr) == nullptr) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
												std::string("getehdr() failed: ") + std::string(elf_errmsg(-1)));

	int nClass = gelf_getclass(pElf);
	if (nClass == ELFCLASSNONE) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
												std::string("getclass() failed: ") + std::string(elf_errmsg(-1)));
	if (nReadMode == ERM_Mapping) {
		if (msgFile) (*msgFile) << ((nClass == ELFCLASS32) ? "32-bit" : "64-bit") << " ELF object\n";
	}

#if DEBUG_ELF_FILE
	if (nReadMode == ERM_Mapping) {
		if (msgFile) {
			(*msgFile) << "Elf Header:\n";

			(*msgFile) << "    e_ident: ";
			for (auto const & byte : std::vector<unsigned char>(ehdr.e_ident, &ehdr.e_ident[std::min(static_cast<size_t>(EI_ABIVERSION), sizeof(ehdr.e_ident))])) {
				(*msgFile) << " " << std::uppercase << std::setfill('0') << std::setw(2) << std::setbase(16)
							<< static_cast<unsigned int>(byte) << std::nouppercase << std::setbase(0);
			}
			(*msgFile) << "\n";

			printField(*msgFile, "Object File Type", sizeof(ehdr.e_type), ehdr.e_type);
			printField(*msgFile, "Machine", sizeof(ehdr.e_machine), ehdr.e_machine);
			printField(*msgFile, "Object File Version:", sizeof(ehdr.e_version), ehdr.e_version);
			printField(*msgFile, "Entry Point Virtual Address", std::min(4ul, sizeof(ehdr.e_entry)), ehdr.e_entry);
			printField(*msgFile, "Program Header Table File Offset", std::min(4ul, sizeof(ehdr.e_phoff)), ehdr.e_phoff);
			printField(*msgFile, "Section Header Table File Offset", std::min(4ul, sizeof(ehdr.e_shoff)), ehdr.e_shoff);
			printField(*msgFile, "Processor-Specific Flags", sizeof(ehdr.e_flags), ehdr.e_flags);
			printField(*msgFile, "ELF Header Size in Bytes", sizeof(ehdr.e_ehsize), ehdr.e_ehsize, true);
			printField(*msgFile, "Program Header Table Entry Size", sizeof(ehdr.e_phentsize), ehdr.e_phentsize, true);
			printField(*msgFile, "Program Header Table Entry Count", sizeof(ehdr.e_phnum), ehdr.e_phnum, true);
			printField(*msgFile, "Section Header Table Entry Size", sizeof(ehdr.e_shentsize), ehdr.e_shentsize, true);
			printField(*msgFile, "Section Header Table Entry Count", sizeof(ehdr.e_shnum), ehdr.e_shnum, true);
			printField(*msgFile, "Section Header String Table Index", sizeof(ehdr.e_shstrndx), ehdr.e_shstrndx, true);
		}
	}
#endif

	// Identify Machine Type:
	// ----------------------
	if (nReadMode == ERM_Mapping) {
		if (msgFile) {
			CELFInfoMap::const_iterator itrMachine = g_mapMachineType.find(ehdr.e_machine);
			if (itrMachine == g_mapMachineType.cend()) {
				(*msgFile) << "ELF Machine Type: Unknown/Unsupported ELF Machine Type\n";
			} else {
				(*msgFile) << "ELF Machine Type: " << itrMachine->second << "\n";

				// TODO : Keep this list updated with the processors/machines that we
				//		have GDC support for:
				switch (ehdr.e_machine) {
					case EM_68HC11:
						// There are subtypes for 68HC12, but no other distinguishing features
						break;

					case EM_AVR:
					{
						CELFInfoMap::const_iterator itrSubmachine = g_mapAVRType.find(ehdr.e_flags & EF_AVR_MACH);
						if (itrSubmachine == g_mapAVRType.cend()) {
							(*msgFile) << "AVR Type: Unknown/Unsupported\n";
						} else {
							(*msgFile) << "AVR Type: " << itrSubmachine->second << "\n";
						}
						break;
					}
				}
			}
		}
	}

	// Get Header Counts:
	// ------------------
	size_t nPHdrNum = 0;
	size_t nSHdrNum = 0;
	size_t nSHdrStrNdx = 0;

	if (elf_getphdrnum(pElf, &nPHdrNum) != 0) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
												std::string("getphdrnum() failed: ") + std::string(elf_errmsg(-1)));
	if (elf_getshdrnum(pElf, &nSHdrNum) != 0) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
												std::string("getshdrnum() failed: ") + std::string(elf_errmsg(-1)));
	if (elf_getshdrstrndx(pElf, &nSHdrStrNdx) != 0) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
												std::string("getshdrstrndx() failed: ") + std::string(elf_errmsg(-1)));

#if DEBUG_ELF_FILE
	if (nReadMode == ERM_Mapping) {
		if (msgFile) {
			(*msgFile) << "    Program Header Table Entry Count: " << nPHdrNum << "\n";
			(*msgFile) << "    Section Header Table Entry Count: " << nSHdrNum << "\n";
			(*msgFile) << "    Section Header String Table Index: " << nSHdrStrNdx << "\n";
		}
	}
#endif

	// Get Program Headers, Map Ranges, and Allocate Memory:
	// -----------------------------------------------------
	if (nReadMode == ERM_Mapping) {
		if (msgFile) {
			(*msgFile) << "Program Headers:\n";
			(*msgFile) << "    Type           Offset   VirtAddr   PhysAddr   FileSize MemSize  Flg Align  Section\n";
		}
	}

	CMemRanges ranges[CDisassembler::NUM_MEMORY_TYPES];
	CMemRanges rangesRAMInit;		// Ranges for RAM init

	for (size_t ndxP = 0; ndxP < nPHdrNum; ++ndxP) {
		GElf_Phdr phdr;
		if (gelf_getphdr(pElf, ndxP, &phdr) != &phdr) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
												std::string("getphdr() failed on index ") + std::to_string(ndxP) +
															": " + std::string(elf_errmsg(-1)));

		// Get mapping for all marked "LOAD":
		if (phdr.p_type == PT_LOAD) {
			// Handle Read/Execute sections as CODE:
			if ((phdr.p_flags & PF_R) &&
				(phdr.p_flags & PF_X)) {
				if (nReadMode == ERM_Mapping) {
					if (pRange) {
						// If only a single range is given, it's taken to be
						//	the MT_ROM range:
						pRange->push_back(CMemRange(phdr.p_vaddr + nNewBase, phdr.p_memsz));
					}
				}

				ranges[CDisassembler::MT_ROM].push_back(CMemRange(phdr.p_vaddr + nNewBase, phdr.p_memsz));
			} else // Handle Read/Write sections as RAM/EE (DATA):
				if ((phdr.p_flags & PF_R) &&
					(phdr.p_flags & PF_W) &&
					((phdr.p_flags & PF_X) == 0)) {
				// Note: nNewBase is for ROM-area only
				if ((ehdr.e_machine == EM_AVR) && (phdr.p_vaddr >= E_AVR_EE_VirtualBase)) {		// Do EE first since it's >RAM
					ranges[CDisassembler::MT_EE].push_back(CMemRange(phdr.p_vaddr - E_AVR_EE_VirtualBaseAdj, phdr.p_memsz));
				} else if ((ehdr.e_machine == EM_AVR) && (phdr.p_vaddr >= E_AVR_RAM_VirtualBase)) {
					ranges[CDisassembler::MT_RAM].push_back(CMemRange(phdr.p_vaddr - E_AVR_RAM_VirtualBaseAdj, phdr.p_memsz));
					rangesRAMInit.push_back(CMemRange(phdr.p_vaddr - E_AVR_RAM_VirtualBaseAdj, phdr.p_filesz));
				} else {
					ranges[CDisassembler::MT_RAM].push_back(CMemRange(phdr.p_vaddr, phdr.p_memsz));
					rangesRAMInit.push_back(CMemRange(phdr.p_vaddr, phdr.p_filesz));
				}
			} else // Handle Read-only sections as ROM/EE (DATA):
				if ((phdr.p_flags & PF_R) &&
					((phdr.p_flags & PF_W) == 0) &&
					((phdr.p_flags & PF_X) == 0)) {
				if ((ehdr.e_machine == EM_AVR) && (phdr.p_vaddr >= E_AVR_EE_VirtualBase)) {		// Do EE first since it's >RAM
					// Note: nNewBase is for ROM-area only
					ranges[CDisassembler::MT_EE].push_back(CMemRange(phdr.p_vaddr - E_AVR_EE_VirtualBaseAdj, phdr.p_memsz));
				} else if ((ehdr.e_machine == EM_AVR) && (phdr.p_vaddr >= E_AVR_RAM_VirtualBase)) {
					ranges[CDisassembler::MT_ROM].push_back(CMemRange(phdr.p_vaddr + nNewBase - E_AVR_RAM_VirtualBaseAdj, phdr.p_memsz));
					if (nReadMode == ERM_Mapping) {
						if (pRange) {
							// If only a single range is given, it's taken to be
							//	the MT_ROM range:
							pRange->push_back(CMemRange(phdr.p_vaddr + nNewBase - E_AVR_RAM_VirtualBaseAdj, phdr.p_memsz));
						}
					}
				} else {
					ranges[CDisassembler::MT_ROM].push_back(CMemRange(phdr.p_vaddr + nNewBase, phdr.p_memsz));
					if (nReadMode == ERM_Mapping) {
						if (pRange) {
							// If only a single range is given, it's taken to be
							//	the MT_ROM range:
							pRange->push_back(CMemRange(phdr.p_vaddr + nNewBase, phdr.p_memsz));
						}
					}
				}
			}
		}

		if (nReadMode == ERM_Mapping) {
			// Print Header Detail:
			if (msgFile) {
				(*msgFile) << "    ";
				(*msgFile) << padString(getPHdrType(phdr.p_type), 15);
				printField(*msgFile, TString(), 3, phdr.p_offset); (*msgFile) << " ";
				printField(*msgFile, TString(), 4, phdr.p_vaddr); (*msgFile) << " ";
				printField(*msgFile, TString(), 4, phdr.p_paddr); (*msgFile) << " ";
				printField(*msgFile, TString(), 3, phdr.p_filesz); (*msgFile) << " ";
				printField(*msgFile, TString(), 3, phdr.p_memsz); (*msgFile) << " ";
				(*msgFile) << ((phdr.p_flags & PF_R) ? "R" : " ");
				(*msgFile) << ((phdr.p_flags & PF_W) ? "W" : " ");
				(*msgFile) << ((phdr.p_flags & PF_X) ? "X" : " ");
				(*msgFile) << " ";
				printField(*msgFile, TString(), 1, phdr.p_align);(*msgFile) << " ";
			}

			for (size_t ndxS = 0; ndxS < nSHdrNum; ++ndxS) {
				Elf_Scn *pScn = nullptr;
				GElf_Shdr shdr;
				pScn = elf_getscn(pElf, ndxS);
				if (pScn == nullptr) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
															std::string("getscn() failed: ") + std::string(elf_errmsg(-1)));
				if (gelf_getshdr(pScn, &shdr) != &shdr) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
															std::string("getshdr() failed: ") + std::string(elf_errmsg(-1)));
				if ((shdr.sh_offset == phdr.p_offset) &&
					(shdr.sh_addr == phdr.p_vaddr)) {
					char *pName;
					pName = elf_strptr(pElf, nSHdrStrNdx, shdr.sh_name);
					if (pName == nullptr) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
												std::string("strptr() failed for string index ") + std::to_string(shdr.sh_name) + ": " + std::string(elf_errmsg(-1)));
					if (msgFile) {
						(*msgFile) << " " << pName;
					}
					break;
				}
			}

			if (msgFile) (*msgFile) << "\n";
		}
	}

	if (nReadMode == ERM_Mapping) {
		if (msgFile) (*msgFile) << "\n";
	}

	if (nReadMode == ERM_Mapping) {
		// Allocate Memory if mapping:
		if (pDisassembler) {
			for (int nMemType = 0; nMemType < CDisassembler::NUM_MEMORY_TYPES; ++nMemType) {
				if (nMemType == CDisassembler::MT_RAM) {
					// Skip RAM and init below when we determine the flash shadow,
					//	but clear any existing allocation so we can just pushback
					//	our new allocations:
					pDisassembler->memory(CDisassembler::MT_RAM).clear();
					continue;
				}
				if (!ranges[nMemType].isNullRange()) {
					pDisassembler->memory(static_cast<CDisassembler::MEMORY_TYPE>(nMemType)).initFromRanges(ranges[nMemType], 0, true, 0, nDesc);
				}
			}
		}
	}

	// End of Flash contains init bytes for the .data section:
	assert(ranges[CDisassembler::MT_RAM].size() == rangesRAMInit.size());	// We should have made these parallel arrays above!
	TAddress nDataStartAddr = ranges[CDisassembler::MT_ROM].highestAddress()+1;		// Note: Already had nNewBase added in
	TAddress nDataAddr = nDataStartAddr;
	for (CMemRanges::size_type ndx = 0; ndx < rangesRAMInit.size(); ++ndx) {
		ranges[CDisassembler::MT_ROM].push_back(CMemRange(nDataAddr, rangesRAMInit.at(ndx).size()));
		if (nReadMode == ERM_Mapping) {
			if (pRange) {
				// If only a single range is given, it's taken to be
				//	the MT_ROM range:
				pRange->push_back(CMemRange(nDataAddr, rangesRAMInit.at(ndx).size()));
			}

			// Allocate memory for flash shadow and cross-link with RAM:
			if (pDisassembler && !rangesRAMInit.at(ndx).isNullRange()) {
				pDisassembler->memory(CDisassembler::MT_ROM).push_back(
							CMemBlock(nDataAddr, ranges[CDisassembler::MT_RAM].at(ndx).startAddr(), true,
										rangesRAMInit.at(ndx).size(), 0, nDesc));
			}
			// Allocate memory for RAM and cross-link with ROM:
			//	Note: While the number of the ranges are the same, the size
			//		of each range may be different!
			if (pDisassembler && !ranges[CDisassembler::MT_RAM].isNullRange()) {
				pDisassembler->memory(CDisassembler::MT_RAM).push_back(
							CMemBlock(ranges[CDisassembler::MT_RAM].at(ndx).startAddr(), nDataAddr, true,
										ranges[CDisassembler::MT_RAM].at(ndx).size(), 0, nDesc));

			}
		}
		nDataAddr += rangesRAMInit.at(ndx).size();
	}


	// Print Section Headers in Mapping Mode:
	// --------------------------------------
	if (nReadMode == ERM_Mapping) {
		if (msgFile) {
			(*msgFile) << "Section Headers:\n";
			(*msgFile) << "  [Nr]  Name                     Type            Addr       Off      Size     EntS Flg Lk Inf Al\n";
		}

		for (size_t ndx = 0; ndx < nSHdrNum; ++ndx) {
			Elf_Scn *pScn = nullptr;
			GElf_Shdr shdr;
			char *pName;
			pScn = elf_getscn(pElf, ndx);
			if (pScn == nullptr) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
														std::string("getscn() failed: ") + std::string(elf_errmsg(-1)));
			if (gelf_getshdr(pScn, &shdr) != &shdr) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
														std::string("getshdr() failed: ") + std::string(elf_errmsg(-1)));
			pName = elf_strptr(pElf, nSHdrStrNdx, shdr.sh_name);
			if (pName == nullptr) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
										std::string("strptr() failed for string index ") + std::to_string(shdr.sh_name) + ": " + std::string(elf_errmsg(-1)));

			if (msgFile) {
				(*msgFile) << "  ";
				TString strTemp = "[" + padString(std::to_string(elf_ndxscn(pScn)), 2, ' ', true) + "]";
				(*msgFile) << padString(strTemp, 5) << " ";
				(*msgFile) << padString(pName, 24).substr(0, 24) << " ";
				(*msgFile) << padString(getSHdrType(shdr.sh_type), 15) << " ";
				printField(*msgFile, TString(), 4, shdr.sh_addr); (*msgFile) << " ";
				printField(*msgFile, TString(), 3, shdr.sh_offset); (*msgFile) << " ";
				printField(*msgFile, TString(), 3, shdr.sh_size); (*msgFile) << " ";
				printField(*msgFile, TString(), 1, shdr.sh_entsize); (*msgFile) << " ";
				TString strFlags;
				if (shdr.sh_flags & SHF_WRITE) strFlags += 'W';
				if (shdr.sh_flags & SHF_ALLOC) strFlags += 'A';
				if (shdr.sh_flags & SHF_EXECINSTR) strFlags += 'X';
				if (shdr.sh_flags & SHF_MERGE) strFlags += 'M';
				if (shdr.sh_flags & SHF_STRINGS) strFlags += 'S';
				if (shdr.sh_flags & SHF_INFO_LINK) strFlags += 'I';
				if (shdr.sh_flags & SHF_LINK_ORDER) strFlags += 'L';
				if (shdr.sh_flags & SHF_OS_NONCONFORMING) strFlags += 'O';
				if (shdr.sh_flags & SHF_GROUP) strFlags += 'G';
				if (shdr.sh_flags & SHF_TLS) strFlags += 'T';
				if (shdr.sh_flags & SHF_EXCLUDE) strFlags += 'E';
				if (shdr.sh_flags & SHF_COMPRESSED) strFlags += 'C';
				(*msgFile) << padString(strFlags, 3) << " ";
				(*msgFile) << padString(std::to_string(shdr.sh_link), 2, ' ' , true); (*msgFile) << " ";
				(*msgFile) << padString(std::to_string(shdr.sh_info), 3, ' ', true); (*msgFile) << " ";
				(*msgFile) << padString(std::to_string(shdr.sh_addralign), 2, ' ', true);

				(*msgFile) << "\n";
			}
		}

		if (msgFile) (*msgFile) << "\n";
	}


	// Read File Code/Data:
	// --------------------
	if (nReadMode == ERM_Data) {
		for (size_t ndxP = 0; ndxP < nPHdrNum; ++ndxP) {
			GElf_Phdr phdr;
			if (gelf_getphdr(pElf, ndxP, &phdr) != &phdr) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
													std::string("getphdr() failed on index ") + std::to_string(ndxP) +
																": " + std::string(elf_errmsg(-1)));

			// Read ones marked as "LOAD":
			if (phdr.p_type == PT_LOAD) {
				for (size_t ndxS = 0; ndxS < nSHdrNum; ++ndxS) {
					Elf_Scn *pScn = nullptr;
					GElf_Shdr shdr;
					pScn = elf_getscn(pElf, ndxS);
					if (pScn == nullptr) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
																std::string("getscn() failed: ") + std::string(elf_errmsg(-1)));
					if (gelf_getshdr(pScn, &shdr) != &shdr) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
																std::string("getshdr() failed: ") + std::string(elf_errmsg(-1)));
					if ((shdr.sh_offset == phdr.p_offset) &&
						(shdr.sh_addr == phdr.p_vaddr) &&
						(shdr.sh_type == SHT_PROGBITS)) {		// Check PROGBITS : .text, .data, .eeprom, etc
						Elf_Data *pData = nullptr;
						size_t nIndex = 0;
						while (nIndex < shdr.sh_size) {
							pData = elf_getdata(pScn, pData);
							if (pData == nullptr) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
														std::string("getdata() failed for vaddr ") + std::to_string(phdr.p_vaddr) +
														": " + std::string(elf_errmsg(-1)));
							if (pData->d_type == ELF_T_BYTE) {
								if (pData->d_buf && pData->d_size) {
									for (size_t nDataIndex = 0; nDataIndex < pData->d_size; ++nDataIndex, ++nIndex) {
										// Handle Read/Execute sections as CODE:
										if ((phdr.p_flags & PF_R) &&
											(phdr.p_flags & PF_X)) {
											TAddress nAddress = shdr.sh_addr + nNewBase + nIndex;

											if (pMemory) {
												// If only a single memory object is given, it's taken to be
												//	the MT_ROM range:
												pMemory->setElement(nAddress, ((unsigned char *)pData->d_buf)[nDataIndex]);
												if (pMemory->descriptor(nAddress) != 0) bRetVal = false;		// Signal overlap
												pMemory->setDescriptor(nAddress, nDesc);
											}
											if (pDisassembler) {
												pDisassembler->memory(CDisassembler::MT_ROM).setElement(nAddress, ((unsigned char *)pData->d_buf)[nDataIndex]);
												if (pDisassembler->memory(CDisassembler::MT_ROM).descriptor(nAddress) != 0) bRetVal = false;		// Signal overlap
												pDisassembler->memory(CDisassembler::MT_ROM).setDescriptor(nAddress, nDesc);
											}
										} else // Handle Read/Write sections as RAM/EE (DATA):
											if ((phdr.p_flags & PF_R) &&
												(phdr.p_flags & PF_W) &&
												((phdr.p_flags & PF_X) == 0)) {
											// Note: nNewBase is for ROM-area only
											bool bIsEE = false;
											TAddress nAddress = shdr.sh_addr + nIndex;
											if ((ehdr.e_machine == EM_AVR) && (nAddress >= E_AVR_EE_VirtualBase)) {		// Do EE first since it's >RAM
												nAddress -= E_AVR_EE_VirtualBaseAdj;
												bIsEE = true;
											} else if ((ehdr.e_machine == EM_AVR) && (nAddress >= E_AVR_RAM_VirtualBase)) {
												nAddress -= E_AVR_RAM_VirtualBaseAdj;
											}
											if (pDisassembler) {
												if (bIsEE) {
													pDisassembler->memory(CDisassembler::MT_EE).setElement(nAddress, ((unsigned char *)pData->d_buf)[nDataIndex]);
													if (pDisassembler->memory(CDisassembler::MT_EE).descriptor(nAddress) != 0) bRetVal = false;		// Signal overlap
													pDisassembler->memory(CDisassembler::MT_EE).setDescriptor(nAddress, nDesc);
												} else {
													// Put the actual data in the flash shadow:
													//	Note: nDataStartAddr already has nNewBase added,
													//		so there's no need to add it into nFlashShadowAddress
													TAddress nFlashShadowAddress = shdr.sh_addr + nIndex;
													if ((ehdr.e_machine == EM_AVR) && (nFlashShadowAddress >= E_AVR_RAM_VirtualBase)) {
														nFlashShadowAddress -= E_AVR_RAM_VirtualBase;	// Note: Remove ENTIRE virtual address here!
													}
													nFlashShadowAddress += nDataStartAddr;
													pDisassembler->memory(CDisassembler::MT_ROM).setElement(nFlashShadowAddress, ((unsigned char *)pData->d_buf)[nDataIndex]);
													if (pDisassembler->memory(CDisassembler::MT_ROM).descriptor(nFlashShadowAddress) != 0) bRetVal = false;		// Signal overlap
													pDisassembler->memory(CDisassembler::MT_ROM).setDescriptor(nFlashShadowAddress, nDesc);
													if (pMemory) {
														// If only a single memory object is given, it's taken to be
														//	the MT_ROM range:
														pMemory->setElement(nFlashShadowAddress, ((unsigned char *)pData->d_buf)[nDataIndex]);
														if (pMemory->descriptor(nFlashShadowAddress) != 0) bRetVal = false;		// Signal overlap
														pMemory->setDescriptor(nFlashShadowAddress, nDesc);
													}

													// Also tag the RAM area descriptors for allocation:
													pDisassembler->memory(CDisassembler::MT_RAM).setElement(nAddress, ((unsigned char *)pData->d_buf)[nDataIndex]);
													if (pDisassembler->memory(CDisassembler::MT_RAM).descriptor(nAddress) != 0) bRetVal = false;		// Signal overlap
													pDisassembler->memory(CDisassembler::MT_RAM).setDescriptor(nAddress, CDisassembler::DMEM_DATA);
												}
											}
										} else // Handle Read-only sections as ROM/EE (DATA):
											if ((phdr.p_flags & PF_R) &&
												((phdr.p_flags & PF_W) == 0) &&
												((phdr.p_flags & PF_X) == 0)) {
											bool bIsEE = false;
											TAddress nAddress = shdr.sh_addr + nNewBase + nIndex;
											if ((ehdr.e_machine == EM_AVR) && (nAddress >= E_AVR_EE_VirtualBase)) {		// Do EE first since it's >RAM
												nAddress -= E_AVR_EE_VirtualBaseAdj;
												bIsEE = true;
											} else if ((ehdr.e_machine == EM_AVR) && (nAddress >= E_AVR_RAM_VirtualBase)) {
												nAddress -= E_AVR_RAM_VirtualBaseAdj;
											}
											if (!bIsEE && pMemory) {
												// If only a single memory object is given, it's taken to be
												//	the MT_ROM range:
												pMemory->setElement(nAddress, ((unsigned char *)pData->d_buf)[nDataIndex]);
												if (pMemory->descriptor(nAddress) != 0) bRetVal = false;		// Signal overlap
												pMemory->setDescriptor(nAddress, nDesc);
											}
											if (pDisassembler) {
												if (bIsEE) {
													pDisassembler->memory(CDisassembler::MT_EE).setElement(nAddress, ((unsigned char *)pData->d_buf)[nDataIndex]);
													if (pDisassembler->memory(CDisassembler::MT_EE).descriptor(nAddress) != 0) bRetVal = false;		// Signal overlap
													pDisassembler->memory(CDisassembler::MT_EE).setDescriptor(nAddress, nDesc);
												} else {
													pDisassembler->memory(CDisassembler::MT_ROM).setElement(nAddress, ((unsigned char *)pData->d_buf)[nDataIndex]);
													if (pDisassembler->memory(CDisassembler::MT_ROM).descriptor(nAddress) != 0) bRetVal = false;		// Signal overlap
													pDisassembler->memory(CDisassembler::MT_ROM).setDescriptor(nAddress, nDesc);
												}
											}
										}
									}
								}
							} else {
								nIndex += pData->d_size;
							}
						}
						break;
					} else
						if ((shdr.sh_offset == phdr.p_offset) &&
							(shdr.sh_addr == phdr.p_vaddr) &&
							(shdr.sh_type == SHT_NOBITS)) {			// Check NOBITS : .bss, etc
							size_t nIndex = 0;
							while (nIndex < shdr.sh_size) {
								// Handle Read/Write sections as RAM (DATA):
								if ((phdr.p_flags & PF_R) &&
									(phdr.p_flags & PF_W) &&
									((phdr.p_flags & PF_X) == 0)) {
									// Note: nNewBase is for ROM-area only
									TAddress nAddress = shdr.sh_addr + nIndex;
									if ((ehdr.e_machine == EM_AVR) && (nAddress >= E_AVR_RAM_VirtualBase)) {
										nAddress -= E_AVR_RAM_VirtualBaseAdj;
									}
									if (pDisassembler) {
										if (pDisassembler->memory(CDisassembler::MT_RAM).descriptor(nAddress) != 0) bRetVal = false;		// Signal overlap
										pDisassembler->memory(CDisassembler::MT_RAM).setDescriptor(nAddress, nDesc);
									}
								}
								++nIndex;
							}
					}
				}
			}
		}
	}


	// Symbol Tables:
	// --------------
	if (nReadMode == ERM_Data) {
		size_t nStrTabNdx = getStrTabSectionIndex(pElf);		// String table section index
		if (nStrTabNdx == 0) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0, "Can't find .strtab");

		for (size_t ndx = 0; ndx < nSHdrNum; ++ndx) {
			Elf_Scn *pScn = nullptr;
			GElf_Shdr shdr;
			char *pName;
			pScn = elf_getscn(pElf, ndx);
			if (pScn == nullptr) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
														std::string("getscn() failed: ") + std::string(elf_errmsg(-1)));
			if (gelf_getshdr(pScn, &shdr) != &shdr) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
														std::string("getshdr() failed: ") + std::string(elf_errmsg(-1)));
			pName = elf_strptr(pElf, nSHdrStrNdx, shdr.sh_name);
			if (pName == nullptr) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
										std::string("strptr() failed for string index ") + std::to_string(shdr.sh_name) + ": " + std::string(elf_errmsg(-1)));

			if (shdr.sh_type != SHT_SYMTAB) continue;
			if (shdr.sh_size == 0) continue;

			size_t nNumSyms = (shdr.sh_entsize ? shdr.sh_size/shdr.sh_entsize : 0);

#if REPORT_SYMTABLE
			if (msgFile) {
				(*msgFile) << "Symbol table '" << pName << "' contains " << nNumSyms << " entries:\n";
				(*msgFile) << "   Num: Value      Size  Type    Bind    Vis       Ndx Name\n";
			}
#endif

			Elf_Data *pData = nullptr;
			size_t nIndex = 0;
			while (nIndex < shdr.sh_size) {
				pData = elf_getdata(pScn, pData);
				if (pData == nullptr) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
											std::string("getdata() failed for vaddr ") + std::to_string(shdr.sh_addr) +
											": " + std::string(elf_errmsg(-1)));

				for (size_t ndxSym = 0; ndxSym < nNumSyms; ++ndxSym) {
					char *pSymName;
					GElf_Sym sym;
					if (gelf_getsym(pData, ndxSym, &sym) == nullptr) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
																			std::string("getsym() failed for symbol ") + std::to_string(ndxSym) +
																			": " + std::string(elf_errmsg(-1)));

					pSymName = elf_strptr(pElf, nStrTabNdx, sym.st_name);
					if (pSymName == nullptr) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
												std::string("strptr() failed for string index ") + std::to_string(sym.st_name) + ": " + std::string(elf_errmsg(-1)));

#if REPORT_SYMTABLE
					if (msgFile) {
						(*msgFile) << padString(std::to_string(ndxSym), 6, ' ', true) << ": ";
						printField(*msgFile, TString(), 4, sym.st_value); (*msgFile) << " ";
						(*msgFile) << padString(std::to_string(sym.st_size), 5, ' ', true) << " ";
						(*msgFile) << padString(getSymType(sym.st_info), 7) << " ";
						(*msgFile) << padString(getSymBind(sym.st_info), 7) << " ";
						(*msgFile) << padString(getSymVis(sym.st_other), 9) << " ";
						if (sym.st_shndx == SHN_UNDEF) {
							(*msgFile) << "UND ";
						} else if (sym.st_shndx == SHN_ABS) {
							(*msgFile) << "ABS ";
						} else if (sym.st_shndx == SHN_COMMON) {
							(*msgFile) << "COM ";
						} else if (sym.st_shndx == SHN_XINDEX) {
							(*msgFile) << "XTD ";
						} else {
							(*msgFile) << padString(std::to_string(sym.st_shndx), 3, ' ', true) << " ";
						}
						(*msgFile) << pSymName;

						(*msgFile) << "\n";
					}
#endif

					// Add symbols to Disassembler:
					unsigned char nSymType = GELF_ST_TYPE(sym.st_info);
					TLabel strSymbol = pSymName;
					TLabel strLabel = pSymName;
					if (pDisassembler &&
						!strLabel.empty() &&				// Must have a label
						((nSymType == STT_NOTYPE) ||		// NOTYPE can be data, function, or something else
						(nSymType == STT_OBJECT) ||			// OBJECT is data (either RAM or ROM)
						(nSymType == STT_FUNC) ||			// FUNC is a function (Note: we cannot use the size to label memory as "code" because this area could contain indirect vectors and such that isn't "code")
						(nSymType == STT_TLS) ||			// TLS Thread Local Storage (RAM data)
						(nSymType == STT_GNU_IFUNC)) &&		// GNU_IFUNC is indirect function pointer
															// Ignore: SECTION, FILE, and COMMON types
						((sym.st_shndx != SHN_UNDEF) &&		// Ignore undefined sections
						 (sym.st_shndx < SHN_LORESERVE))) {	// Ignore reserved sections

						// Get the referenced section for the symbol:
						Elf_Scn *pScnRef = nullptr;
						GElf_Shdr shdrRef;
						pScnRef = elf_getscn(pElf, sym.st_shndx);
						if (pScnRef == nullptr) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
																	std::string("getscn() failed: ") + std::string(elf_errmsg(-1)));
						if (gelf_getshdr(pScnRef, &shdrRef) != &shdrRef) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
																	std::string("getshdr() failed: ") + std::string(elf_errmsg(-1)));

						bool bIsEE = false;
						TAddress nAddress = sym.st_value;
						if ((ehdr.e_machine == EM_AVR) && (nAddress >= E_AVR_EE_VirtualBase)) {		// Do EE first since it's >RAM
							nAddress -= E_AVR_EE_VirtualBaseAdj;
							bIsEE = true;
						} else if ((ehdr.e_machine == EM_AVR) && (nAddress >= E_AVR_RAM_VirtualBase)) {
							nAddress -= E_AVR_RAM_VirtualBaseAdj;
						}

						// Keep label valid:
						std::replace(strLabel.begin(), strLabel.end(), '.', '_');	// GCC uses "." in some labels, which isn't valid for most assemblers
						if (!pDisassembler->ValidateLabelName(strLabel)) {
							if (errFile) {
								(*errFile) << "    *** Symbol Name '" << strLabel << "' from ELF file is Invalid for Disassembler Labels.  Can't add it.\n";
							}
						} else {
							switch (nSymType) {
								case STT_NOTYPE:
									if (shdrRef.sh_flags & SHF_EXECINSTR) {
										pDisassembler->AddLabel(CDisassembler::MT_ROM, nAddress + nNewBase, false, 0, strLabel);
										pDisassembler->AddSymbol(CDisassembler::MT_ROM, nAddress + nNewBase, strSymbol);
										// TODO : See if there's a way to distinguish Code and Data here to possibly
										//		set an entry point?
									} else if (shdrRef.sh_flags & SHF_ALLOC) {		// necessary??
										pDisassembler->AddLabel(bIsEE ? CDisassembler::MT_EE : CDisassembler::MT_RAM, nAddress, false, 0, strLabel);
										pDisassembler->AddSymbol(bIsEE ? CDisassembler::MT_EE : CDisassembler::MT_RAM, nAddress, strSymbol);
									}
									break;
								case STT_OBJECT:
									if (shdrRef.sh_flags & SHF_EXECINSTR) {
										pDisassembler->AddLabel(CDisassembler::MT_ROM, nAddress + nNewBase, false, 0, strLabel);
										pDisassembler->AddSymbol(CDisassembler::MT_ROM, nAddress + nNewBase, strSymbol);
										pDisassembler->AddObjectMapping(CDisassembler::MT_ROM, nAddress + nNewBase, sym.st_size);
									} else if (shdrRef.sh_flags & SHF_ALLOC) {		// necessary??
										pDisassembler->AddLabel(bIsEE ? CDisassembler::MT_EE : CDisassembler::MT_RAM, nAddress, false, 0, strLabel);
										pDisassembler->AddSymbol(bIsEE ? CDisassembler::MT_EE : CDisassembler::MT_RAM, nAddress, strSymbol);
										pDisassembler->AddObjectMapping(bIsEE ? CDisassembler::MT_EE : CDisassembler::MT_RAM, nAddress, sym.st_size);
									}
									break;
								case STT_FUNC:
									if (shdrRef.sh_flags & SHF_EXECINSTR) {
										pDisassembler->AddEntry(nAddress + nNewBase);
										pDisassembler->AddLabel(CDisassembler::MT_ROM, nAddress + nNewBase, false, 0, strLabel);
										pDisassembler->AddSymbol(CDisassembler::MT_ROM, nAddress + nNewBase, strSymbol);
										pDisassembler->AddObjectMapping(CDisassembler::MT_ROM, nAddress + nNewBase, sym.st_size);
									}
									break;
								case STT_TLS:
									if (shdrRef.sh_flags & SHF_ALLOC) {
										if (shdrRef.sh_flags & SHF_WRITE) {
											pDisassembler->AddLabel((shdrRef.sh_flags & SHF_EXECINSTR) ?
																		CDisassembler::MT_ROM : (bIsEE ? CDisassembler::MT_EE : CDisassembler::MT_RAM),
																	(shdrRef.sh_flags & SHF_EXECINSTR) ?
																		nAddress + nNewBase : nAddress,
																	false, 0, strLabel);
											pDisassembler->AddSymbol((shdrRef.sh_flags & SHF_EXECINSTR) ?
																		CDisassembler::MT_ROM : (bIsEE ? CDisassembler::MT_EE : CDisassembler::MT_RAM),
																	(shdrRef.sh_flags & SHF_EXECINSTR) ?
																		nAddress + nNewBase : nAddress,
																	strSymbol);
										} else if (shdrRef.sh_flags & SHF_EXECINSTR) {
											pDisassembler->AddLabel(CDisassembler::MT_ROM, nAddress + nNewBase, false, 0, strLabel);
											pDisassembler->AddSymbol(CDisassembler::MT_ROM, nAddress + nNewBase, strSymbol);
										}
									}
									break;
								case STT_GNU_IFUNC:
									if ((shdrRef.sh_flags & SHF_EXECINSTR) &&
										!pDisassembler->HaveCodeIndirect(nAddress + nNewBase) &&
										!pDisassembler->HaveDataIndirect(nAddress + nNewBase)) {
										pDisassembler->AddCodeIndirect(nAddress + nNewBase, strLabel);
										pDisassembler->AddSymbol(CDisassembler::MT_ROM, nAddress + nNewBase, strSymbol);
									}
									break;
								default:
									assert(false);
									break;
							}
						}
					}
				}

				nIndex += pData->d_size;
			}

#if REPORT_SYMTABLE
			if (msgFile) (*msgFile) << "\n";
#endif
		}
	}

	return bRetVal;
}

// ============================================================================

bool CELFDataFileConverter::RetrieveFileMapping(CDisassembler &disassembler,
		const std::string &strFilePathName, TAddress nNewBase,
		std::ostream *msgFile, std::ostream *errFile) const
{
	if (elf_version(EV_CURRENT) == EV_NONE) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_LIBRARY_INIT_FAILED, 0,
												std::string("ELF Initialization Failed: ") + std::string(elf_errmsg(-1)));

	int fd = open(strFilePathName.c_str(), O_RDONLY);
	if (fd < 0) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_OPENREAD);

	bool bRetVal = false;
	Elf *pElf = elf_begin(fd, ELF_C_READ, nullptr);
	if (pElf == nullptr) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
												std::string("elf_begin() failed: ") + std::string(elf_errmsg(-1)));

	try {
		bRetVal = _ReadDataFile(ERM_Mapping, &disassembler, pElf,
								nNewBase, nullptr, nullptr, CDisassembler::DMEM_NOTLOADED, msgFile, errFile);
	} catch (...) {
		elf_end(pElf);
		close(fd);
		throw;
	}

	elf_end(pElf);
	close(fd);

	return bRetVal;
}

bool CELFDataFileConverter::ReadDataFile(CDisassembler &disassembler,
		const std::string &strFilePathName, TAddress nNewBase, TDescElement nDesc,
		std::ostream *msgFile, std::ostream *errFile) const
{
	if (elf_version(EV_CURRENT) == EV_NONE) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_LIBRARY_INIT_FAILED, 0,
												std::string("ELF Initialization Failed: ") + std::string(elf_errmsg(-1)));

	int fd = open(strFilePathName.c_str(), O_RDONLY);
	if (fd < 0) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_OPENREAD);

	bool bRetVal = false;
	Elf *pElf = elf_begin(fd, ELF_C_READ, nullptr);
	if (pElf == nullptr) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
												std::string("elf_begin() failed: ") + std::string(elf_errmsg(-1)));

	try {
		bRetVal = _ReadDataFile(ERM_Data, &disassembler, pElf,
								nNewBase, nullptr, nullptr, nDesc, msgFile, errFile);
	} catch (...) {
		elf_end(pElf);
		close(fd);
		throw;
	}

	elf_end(pElf);
	close(fd);

	return bRetVal;
}

bool CELFDataFileConverter::WriteDataFile(CDisassembler &disassembler,
		const std::string &strFilePathName, const CMemRanges &aRange, TAddress nNewBase,
		const CMemBlocks &aMemory, TDescElement nDesc, bool bUsePhysicalAddr,
		DFC_FILL_MODE_ENUM nFillMode, TMemoryElement nFillValue,
		std::ostream *msgFile, std::ostream *errFile) const
{
	UNUSED(disassembler);
	UNUSED(strFilePathName);
	UNUSED(aRange);
	UNUSED(nNewBase);
	UNUSED(aMemory);
	UNUSED(nDesc);
	UNUSED(bUsePhysicalAddr);
	UNUSED(nFillMode);
	UNUSED(nFillValue);
	UNUSED(msgFile);
	UNUSED(errFile);

	THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_NOT_IMPLEMENTED, 0, "ELF Writing not implemented");
	return false;
}

// ============================================================================

// RetrieveFileMapping:
//
//    This function reads in an already opened text BINARY file referenced by
//    'aFile' and fills in the CMemRanges object that encapsulates the file's
//    contents, offsetted by 'NewBase' (allowing loading of different files
//    to different base addresses).
bool CELFDataFileConverter::RetrieveFileMapping(std::istream &aFile, TAddress nNewBase, CMemRanges &aRange,
													std::ostream *msgFile, std::ostream *errFile) const
{
#ifdef __GLIBCXX__
	if (elf_version(EV_CURRENT) == EV_NONE) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_LIBRARY_INIT_FAILED, 0,
												std::string("ELF Initialization Failed: ") + std::string(elf_errmsg(-1)));

	aFile.seekg(0L, std::ios_base::beg);

	int fd = static_cast< __gnu_cxx::stdio_filebuf< char > * const >(aFile.rdbuf())->fd();

	bool bRetVal = false;
	Elf *pElf = elf_begin(fd, ELF_C_READ, nullptr);
	if (pElf == nullptr) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
												std::string("elf_begin() failed: ") + std::string(elf_errmsg(-1)));

	try {
		bRetVal = _ReadDataFile(ERM_Mapping, nullptr, pElf,
								nNewBase, nullptr, &aRange, CDisassembler::DMEM_NOTLOADED, msgFile, errFile);
	} catch (...) {
		elf_end(pElf);
		throw;
	}

	elf_end(pElf);

	return bRetVal;
#else
	THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_NOT_IMPLEMENTED, 0, "ELF I/O Stream Reading only implemented on GCC builds");
#endif
}


// ReadDataFile:
//
//    This function reads in an already opened text BINARY file referenced by
//    'aFile' into the CMemBlocks referenced by 'aMemory' offsetted by
//    'NewBase' (allowing loading of different files to different base
//    address) and setting the corresponding Memory Descriptors to 'nDesc'...
//
//    NOTE: The ranges in aMemory block must already be set, such as
//    from a call to RetrieveFileMapping() here and calling initFromRanges()
//    on the CMemBlocks to initialize it.  This function will only read
//    and populate the data on it
bool CELFDataFileConverter::ReadDataFile(std::istream &aFile, TAddress nNewBase, CMemBlocks &aMemory,
												TDescElement nDesc,
												std::ostream *msgFile, std::ostream *errFile) const
{
#ifdef __GLIBCXX__
	if (elf_version(EV_CURRENT) == EV_NONE) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_LIBRARY_INIT_FAILED, 0,
												std::string("ELF Initialization Failed: ") + std::string(elf_errmsg(-1)));

	aFile.seekg(0L, std::ios_base::beg);

	int fd = static_cast< __gnu_cxx::stdio_filebuf< char > * const >(aFile.rdbuf())->fd();

	bool bRetVal = true;
	Elf *pElf = elf_begin(fd, ELF_C_READ, nullptr);
	if (pElf == nullptr) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
												std::string("elf_begin() failed: ") + std::string(elf_errmsg(-1)));

	try {
		bRetVal = _ReadDataFile(ERM_Data, nullptr, pElf,
								nNewBase, &aMemory, nullptr, nDesc, msgFile, errFile);
	} catch (...) {
		elf_end(pElf);
		throw;
	}

	elf_end(pElf);

	return bRetVal;
#else
	THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_NOT_IMPLEMENTED, 0, "ELF I/O Stream Reading only implemented on GCC builds");
#endif
}


// WriteDataFile:
//
//    This function writes to an already opened text BINARY file referenced by
//    'aFile' from the CMemBlocks referenced by 'aMemory' and whose Descriptor
//    has a match specified by 'nDesc'.  Matching is done by 'anding' the
//    nDesc value with the descriptor in memory.  If the result is non-zero,
//    the location is written.  Unless, nDesc=0, in which case ALL locations
//    are written regardless of the actual descriptor.  If the nDesc validator
//    computes out as false, then the data is filled according to the FillMode.
bool CELFDataFileConverter::WriteDataFile(std::ostream &aFile, const CMemRanges &aRange, TAddress nNewBase,
												const CMemBlocks &aMemory, TDescElement nDesc, bool bUsePhysicalAddr,
												DFC_FILL_MODE_ENUM nFillMode, TMemoryElement nFillValue,
												std::ostream *msgFile, std::ostream *errFile) const
{
	UNUSED(aFile);
	UNUSED(aRange);
	UNUSED(nNewBase);
	UNUSED(aMemory);
	UNUSED(nDesc);
	UNUSED(bUsePhysicalAddr);
	UNUSED(nFillMode);
	UNUSED(nFillValue);
	UNUSED(msgFile);
	UNUSED(errFile);

	THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_NOT_IMPLEMENTED, 0, "ELF Writing not implemented");
	return false;
}

