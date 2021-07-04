//
//	Data File Converter Class
//
//
//	Generic Code-Seeking Disassembler
//	Copyright(c)2021 by Donna Whisnant
//

#ifndef DFC_H
#define DFC_H

// ============================================================================

#include "memclass.h"

#include <iostream>
#include <vector>

// ============================================================================


enum DFC_FILL_MODE_ENUM {
	DFC_NO_FILL,
	DFC_ALWAYS_FILL_WITH,
	DFC_ALWAYS_FILL_WITH_RANDOM,
	DFC_CONDITIONAL_FILL_WITH,
	DFC_CONDITIONAL_FILL_WITH_RANDOM
};


/////////////////////////////////////////////////////////////////////////////


class CDataFileConverter
{
public:
	// GetLibraryName: Returns the basic name of the DFC Library, like "Intel", "Binary", etc.
	virtual std::string GetLibraryName() const = 0;
	virtual std::vector<std::string> GetLibraryNameAliases() const { return std::vector<std::string>(); }

	// GetShortDescription: Returns the short description of the DFC Library, like "Intel Hex Data File Converter":
	virtual std::string GetShortDescription() const = 0;

	// GetDescription: Returns a long description of the DFC Library, with more details than short description:
	virtual std::string GetDescription() const = 0;

	// DefaultExtension:
	//    This function returns a constant character string that represents
	//    the default filename extension for the defined data file type.  This
	//    is used by calling programs in open-file dialogs.
	virtual const char * DefaultExtension() const
	{
		static const char * strDefaultExtension = "*";
		return strDefaultExtension;
	}

	// TODO : Change these function signatures to enable a better way to handle posix file descriptors
	//		compatible with libelf, etc., without requiring __gnu_cxx shenanigans...

	// RetrieveFileMapping
	//    This is a pure virtual function that must be overriden in child Data File Converter
	//    classes.  Overriding functions are to read the open BINARY file specified
	//    by 'aFile' and determine the mapping (or range) criteria for the particular
	//    file that is necessary to completely read the file, but including all
	//    breaks that exist in the file.  For example, a binary file will set only
	//    the base range mapping and set it to the NewBase with the size equal to the
	//    file length, since the entire file is read at that base.  An Intel hex file,
	//    for example, will be searched for each skip in offset and that will start
	//    new range entries.  So if an Intel file, for example, has a group at 0000-7FFF,
	//    a group from C000-DFFF and a group from FFD6-FFFF and the NewBase is 10000,
	//    then 3 range entries will be created as: 1) Start: 10000, Size: 8000,
	//    2) Start 1C000, Size: 2000, and 3) Start: 1FFD6, Size: 2A.
	//    Such a routine is needed when reading an unknown file in, as in a generic
	//    file converter routine.  By reading the necessary sizing structure of the
	//    data file, memory can dynamically be allocated to accommodate such a file.
	//    This function returns 'true' if everything was read without problems.
	//    'false' is returned if a problem was found in determining the range (but a
	//    problem that is non-detrimental).
	//    The following throw backs are performed AND loading is halted as follows:
	//       ERR_CHECKSUM if there is a checksum error encountered during loading.
	//       ERR_UNEXPECTED_EOF if end_of_file is encountered before it was expected.
	//       ERR_OUT_OF_MEMORY if an allocation of a new MemRange entry fails.
	//
	virtual bool RetrieveFileMapping(std::istream &aFile, TAddress nNewBase, CMemRanges &aRange, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr) const = 0;

	// ReadDataFile:
	//    This is a pure virtual function that must be overriden in child Data File Converter
	//    routines.  Overriding functions are to read the open BINARY file specified
	//    by 'aFile' into the CMemBlocks object 'aMemory' with an offset of
	//    'NewBase'.  Files that have addresses specified internally, the internal
	//    addresses are treaded as 'Logical Addresses' in the memory structure.
	//    NewBase too is treaded as a Logical Address, but this can be used to
	//    load a file that really has physical addresses into a special logical
	//    area.    For example, a file with internal address for 0x0000-0x7FFF
	//    that needs to be read into logical 0x10000.  Just use NewBase=0x10000.
	//    The Memory locations that receive bytes from the loaded file will have
	//    their Descriptor bytes changed to 'aDesc'.  This function returns
	//    'true' if everything was read without problems.  'false' is returned if
	//    a memory location overwrote an existing already loaded location.
	//
	//    NOTE: The ranges in aMemory block must already be set, such as
	//    from a call to RetrieveFileMapping() here and calling initFromRanges()
	//    on the CMemBlocks to initialize it.  This function will only read
	//    and populate the data on it
	//
	//    The following throw backs are performed AND loading is halted as follows:
	//       ERR_CHECKSUM if there is a checksum error encountered during loading.
	//       ERR_UNEXPECTED_EOF if end_of_file is encountered before it was expected.
	//       ERR_OVERFLOW if an attempt is made to load an address outside the memory bounds.
	//
	virtual bool ReadDataFile(std::istream &aFile, TAddress nNewBase, CMemBlocks &aMemory,
													TDescElement nDesc, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr) const = 0;

	// WriteDataFile:
	//    This is a pure virutal function that must be overriden in child Data File Converter
	//    routines.  Overriding functions are to write the open BINARY file specified
	//    by 'aFile' from the Memory Class object 'aMemory' from the LOGICAL memory
	//    range specified by 'aRange'.  If 'UsePhysicalAddr' is true, calls are
	//    made to the Memory blocks to convert the logical addresses to physical
	//    addresses, and the physical addresses are used in the output file.  The
	//    offset 'NewBase' is added to the final address going in the file.  Note
	//    that this doesn't apply to children writing an image file that has no
	//    address information (like a binary file).  Data written is based on the
	//    validator 'aDesc'.  If 'aDesc' is ZERO, ALL data in the specified range
	//    is written.  Otherwise, the 'aDesc' is ANDED with EACH descriptor byte from
	//    memory and if the result is non-zero, the data is written.  This allows
	//    code to be written separately from data, etc...
	//    The the validator computes as False, then data is actually written or
	//    not written based on the FillMode value, as follows:
	//          00 = No filling done on unused bytes -- on Hex files, this
	//                   results in a smaller, more compact file.
	//          01 = Fill unused elements with the value specified by nFillValue.
	//                   This is useful for binary files.  (Regardless of type)
	//          02 = Fill unused elements with random values.  This is useful for
	//                   data fodder.
	//          03 = Fill unused elements with the byte values specified by nFillValue
	//                   if and only if datafile type requires padding (such as binary).
	//          04 = Fill unused elements with random values if and only
	//                   if datafile type requires padding (such as binary).  This
	//                   is useful for data fodder.
	//
	//       Note types 3&4 are equivalent to 1&2 except that the particular DFC
	//          makes the decision as to whether padding is necessary or not.  This
	//          allows the calling program to be totally generic in the call,
	//          without having to know (for example) if a file is a binary type
	//          and needs padding or a hex type and doesn't.  Hex file types should
	//          not pad on such instances, but binary file types should but only
	//          within the specified range.
	//
	//    True is returned if everything was successful in the write operation.
	//    The following throw backs are performed AND writing is halted as follows:
	//       ERR_WRITEFAILED if there was an error in while writing to the output file.
	//       ERR_OVERFLOW if a write to a address included file yielded an oversized address.
	virtual bool WriteDataFile(std::ostream &aFile, const CMemRanges &aRange, TAddress nNewBase,
													const CMemBlocks &aMemory, TDescElement nDesc, bool bUsePhysicalAddr,
													DFC_FILL_MODE_ENUM nFillMode, TMemoryElement nFillValue, std::ostream *msgFile = nullptr, std::ostream *errFile = nullptr) const = 0;
};


/////////////////////////////////////////////////////////////////////////////

class CDataFileConverters : public std::vector<const CDataFileConverter *>
{
private:
	CDataFileConverters();
public:
	~CDataFileConverters();

	const CDataFileConverter *locateDFC(const std::string &strLibraryName) const;
	static CDataFileConverters *instance();

	static void registerDataFileConverter(const CDataFileConverter *pConverter);
};

/////////////////////////////////////////////////////////////////////////////

#endif	// DFC_H

