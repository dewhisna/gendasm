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
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

#include "elfdfc.h"

#include <stringhelp.h>

#include <assert.h>

#include <ext/stdio_filebuf.h>

#include <libelf.h>
#include <gelf.h>
#include <elfutils/version.h>
#include <nlist.h>

#ifndef UNUSED
	#define UNUSED(x) ((void)(x))
#endif

#define DEBUG_ELF_FILE 0			// Set to '1' (or non-zero) to debug ELF header reading

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

std::string CELFDataFileConverter::GetShortDescription() const
{
	TString strVersion = std::to_string(_ELFUTILS_VERSION%1000);
	return "ELF Data File Converter (libelf " + std::to_string(_ELFUTILS_VERSION/1000) + "." + padString(strVersion, 3, '0', true) + ")";
}

// RetrieveFileMapping:
//
//    This function reads in an already opened text BINARY file referenced by
//    'aFile' and fills in the CMemRanges object that encapsulates the file's
//    contents, offsetted by 'NewBase' (allowing loading of different files
//    to different base addresses).
bool CELFDataFileConverter::RetrieveFileMapping(std::istream &aFile, TAddress nNewBase, CMemRanges &aRange,
													std::ostream *msgFile, std::ostream *errFile) const
{
	UNUSED(errFile);

	auto &&fnPrintField = [msgFile](const TString &strName, int nFieldSize, uint64_t nVal, bool bDecimal = false)->void {
		std::string strPrefix = (!strName.empty() ? ("    " + strName + " : ") : "");
		if (!bDecimal) {
			(*msgFile) << strPrefix << "0x" << std::uppercase << std::setfill('0') << std::setw(nFieldSize*2) << std::setbase(16)
						<< nVal << std::nouppercase << std::setbase(0);
		} else {
			(*msgFile) << strPrefix << nVal;
		}
		if (!strPrefix.empty()) (*msgFile) << "\n";
	};

	auto &&fnPHdrType = [](GElf_Word nValue)->TString {
		CELFInfoMap::const_iterator itrPT = g_mapPHdrType.find(nValue);
		if (itrPT == g_mapPHdrType.cend()) return "unknown";
		return itrPT->second;
	};

	auto &&fnSHdrType = [](GElf_Word nValue)->TString {
		CELFInfoMap::const_iterator itrST = g_mapSHdrType.find(nValue);
		if (itrST == g_mapSHdrType.cend()) return "unknown";
		return itrST->second;
	};

	// ------------------------------------------------------------------------

	Elf *pElf;

	aRange.clear();
	aFile.seekg(0L, std::ios_base::beg);

	if (elf_version(EV_CURRENT) == EV_NONE) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_LIBRARY_INIT_FAILED, 0,
												std::string("ELF Initialization Failed: ") + std::string(elf_errmsg(-1)));

	// Note: We can't use elf_memory() since it wants an actual uncompressed and padded
	//		memory image of the file

	int fd = static_cast< __gnu_cxx::stdio_filebuf< char > * const >(aFile.rdbuf())->fd();

	pElf = elf_begin(fd, ELF_C_READ, nullptr);
	if (pElf == nullptr) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
												std::string("elf_begin() failed: ") + std::string(elf_errmsg(-1)));

	try {
		// Basic ELF Header Verification:
		// ------------------------------
		if (elf_kind(pElf) != ELF_K_ELF) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_UNKNOWN_FILE_TYPE, 0, "Not an ELF object file");

		GElf_Ehdr ehdr;

		if (gelf_getehdr(pElf, &ehdr) == nullptr) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
													std::string("getehdr() failed: ") + std::string(elf_errmsg(-1)));

		int nClass = gelf_getclass(pElf);
		if (nClass == ELFCLASSNONE) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
													std::string("getclass() failed: ") + std::string(elf_errmsg(-1)));
		if (msgFile) (*msgFile) << ((nClass == ELFCLASS32) ? "32-bit" : "64-bit") << " ELF object\n";

#if DEBUG_ELF_FILE
		if (msgFile) {
			(*msgFile) << "Elf Header:\n";

			(*msgFile) << "    e_ident: ";
			for (auto const & byte : std::vector<unsigned char>(ehdr.e_ident, &ehdr.e_ident[std::min(static_cast<size_t>(EI_ABIVERSION), sizeof(ehdr.e_ident))])) {
				(*msgFile) << " " << std::uppercase << std::setfill('0') << std::setw(2) << std::setbase(16)
							<< static_cast<unsigned int>(byte) << std::nouppercase << std::setbase(0);
			}
			(*msgFile) << "\n";

			fnPrintField("Object File Type", sizeof(ehdr.e_type), ehdr.e_type);
			fnPrintField("Machine", sizeof(ehdr.e_machine), ehdr.e_machine);
			fnPrintField("Object File Version:", sizeof(ehdr.e_version), ehdr.e_version);
			fnPrintField("Entry Point Virtual Address", std::min(4ul, sizeof(ehdr.e_entry)), ehdr.e_entry);
			fnPrintField("Program Header Table File Offset", std::min(4ul, sizeof(ehdr.e_phoff)), ehdr.e_phoff);
			fnPrintField("Section Header Table File Offset", std::min(4ul, sizeof(ehdr.e_shoff)), ehdr.e_shoff);
			fnPrintField("Processor-Specific Flags", sizeof(ehdr.e_flags), ehdr.e_flags);
			fnPrintField("ELF Header Size in Bytes", sizeof(ehdr.e_ehsize), ehdr.e_ehsize, true);
			fnPrintField("Program Header Table Entry Size", sizeof(ehdr.e_phentsize), ehdr.e_phentsize, true);
			fnPrintField("Program Header Table Entry Count", sizeof(ehdr.e_phnum), ehdr.e_phnum, true);
			fnPrintField("Section Header Table Entry Size", sizeof(ehdr.e_shentsize), ehdr.e_shentsize, true);
			fnPrintField("Section Header Table Entry Count", sizeof(ehdr.e_shnum), ehdr.e_shnum, true);
			fnPrintField("Section Header String Table Index", sizeof(ehdr.e_shstrndx), ehdr.e_shstrndx, true);
		}
#endif

		// Identify Machine Type:
		// ----------------------
		CELFInfoMap::const_iterator itrMachine = g_mapMachineType.find(ehdr.e_machine);
		if (msgFile && (itrMachine == g_mapMachineType.cend())) {
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
		if (msgFile) {
			(*msgFile) << "    Program Header Table Entry Count: " << nPHdrNum << "\n";
			(*msgFile) << "    Section Header Table Entry Count: " << nSHdrNum << "\n";
			(*msgFile) << "    Section Header String Table Index: " << nSHdrStrNdx << "\n";
		}
#endif

		// Get Program Headers:
		// --------------------
		if (msgFile) {
			(*msgFile) << "Program Headers:\n";
			(*msgFile) << "    Type           Offset   VirtAddr   PhysAddr   FileSize MemSize  Flg Align  Section\n";
		}

		for (size_t ndxP = 0; ndxP < nPHdrNum; ++ndxP) {
			GElf_Phdr phdr;
			if (gelf_getphdr(pElf, ndxP, &phdr) != &phdr) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
													std::string("getphdr() failed on index ") + std::to_string(ndxP) +
																": " + std::string(elf_errmsg(-1)));

			// Allocate ranges for "LOAD" and Read/Execute sections:
			if ((phdr.p_type == PT_LOAD) &&
				(phdr.p_flags & PF_R) &&
				(phdr.p_flags & PF_X)) {
				aRange.push_back(CMemRange(phdr.p_vaddr + nNewBase, phdr.p_memsz));
			}

			// Print Header Detail:
			if (msgFile) {
				(*msgFile) << "    ";
				(*msgFile) << padString(fnPHdrType(phdr.p_type), 15);
				fnPrintField(TString(), 3, phdr.p_offset); (*msgFile) << " ";
				fnPrintField(TString(), 4, phdr.p_vaddr); (*msgFile) << " ";
				fnPrintField(TString(), 4, phdr.p_paddr); (*msgFile) << " ";
				fnPrintField(TString(), 3, phdr.p_filesz); (*msgFile) << " ";
				fnPrintField(TString(), 3, phdr.p_memsz); (*msgFile) << " ";
				(*msgFile) << ((phdr.p_flags & PF_R) ? "R" : " ");
				(*msgFile) << ((phdr.p_flags & PF_W) ? "W" : " ");
				(*msgFile) << ((phdr.p_flags & PF_X) ? "X" : " ");
				(*msgFile) << " ";
				fnPrintField(TString(), 1, phdr.p_align);(*msgFile) << " ";
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
												std::string("elf_strptr() failed for string index ") + std::to_string(shdr.sh_name) + ": " + std::string(elf_errmsg(-1)));
					if (msgFile) {
						(*msgFile) << " " << pName;
					}
					break;
				}
			}

			if (msgFile) {
				(*msgFile) << "\n";
			}
		}

		(*msgFile) << "\n";


		// Section Headers:
		// ----------------
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
										std::string("elf_strptr() failed for string index ") + std::to_string(shdr.sh_name) + ": " + std::string(elf_errmsg(-1)));

			if (msgFile) {
				(*msgFile) << "  ";
				TString strTemp = "[" + padString(std::to_string(elf_ndxscn(pScn)), 2, ' ', true) + "]";
				(*msgFile) << padString(strTemp, 5) << " ";
				(*msgFile) << padString(pName, 24).substr(0, 24) << " ";
				(*msgFile) << padString(fnSHdrType(shdr.sh_type), 15) << " ";
				fnPrintField(TString(), 4, shdr.sh_addr); (*msgFile) << " ";
				fnPrintField(TString(), 3, shdr.sh_offset); (*msgFile) << " ";
				fnPrintField(TString(), 3, shdr.sh_size); (*msgFile) << " ";
				fnPrintField(TString(), 1, shdr.sh_entsize); (*msgFile) << " ";
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

		(*msgFile) << "\n";

		elf_end(pElf);
	} catch (...) {
		elf_end(pElf);
		throw;
	}

	return true;
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
	UNUSED(msgFile);
	UNUSED(errFile);

	bool bRetVal = true;
	Elf *pElf;

	aFile.seekg(0L, std::ios_base::beg);

	if (elf_version(EV_CURRENT) == EV_NONE) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_LIBRARY_INIT_FAILED, 0,
												std::string("ELF Initialization Failed: ") + std::string(elf_errmsg(-1)));

	// Note: We can't use elf_memory() since it wants an actual uncompressed and padded
	//		memory image of the file

	int fd = static_cast< __gnu_cxx::stdio_filebuf< char > * const >(aFile.rdbuf())->fd();

	pElf = elf_begin(fd, ELF_C_READ, nullptr);
	if (pElf == nullptr) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
												std::string("elf_begin() failed: ") + std::string(elf_errmsg(-1)));

	try {
		// Basic ELF Header Verification:
		// ------------------------------
		if (elf_kind(pElf) != ELF_K_ELF) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_UNKNOWN_FILE_TYPE, 0, "Not an ELF object file");
		if (gelf_getclass(pElf) == ELFCLASSNONE) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
													std::string("getclass() failed: ") + std::string(elf_errmsg(-1)));

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

		// Read File Code:
		// ---------------
		for (size_t ndxP = 0; ndxP < nPHdrNum; ++ndxP) {
			GElf_Phdr phdr;
			if (gelf_getphdr(pElf, ndxP, &phdr) != &phdr) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
													std::string("getphdr() failed on index ") + std::to_string(ndxP) +
																": " + std::string(elf_errmsg(-1)));

			// Allocate ranges for "LOAD" and Read/Execute sections:
			if ((phdr.p_type == PT_LOAD) &&
				(phdr.p_flags & PF_R) &&
				(phdr.p_flags & PF_X)) {

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
						(shdr.sh_type == SHT_PROGBITS)) {
						Elf_Data *pData = nullptr;
						size_t nIndex = 0;
						while (nIndex < shdr.sh_size) {
							pData = elf_getdata(pScn, pData);
							if (pData == nullptr) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED, 0,
														std::string("getdata() failed for vaddr ") + std::to_string(phdr.p_vaddr) +
														": " + std::string(elf_errmsg(-1)));
							if (pData->d_buf && (pData->d_type == ELF_T_BYTE) && pData->d_size) {
								for (size_t nDataIndex = 0; nDataIndex < pData->d_size; ++nDataIndex, ++nIndex) {
									TAddress nAddress = shdr.sh_addr + nNewBase + nIndex;
									aMemory.setElement(nAddress, ((unsigned char *)pData->d_buf)[nDataIndex]);
									if (aMemory.descriptor(nAddress) != 0) bRetVal = false;		// Signal overlap
									aMemory.setDescriptor(nAddress, nDesc);
								}
							}
						}
						break;
					}
				}

			}
		}


		elf_end(pElf);
	} catch (...) {
		elf_end(pElf);
		throw;
	}

	return bRetVal;
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

