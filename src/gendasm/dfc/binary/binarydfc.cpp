//
//	BINARY Data File Converter Class
//
//
//	Generic Code-Seeking Disassembler
//	Copyright(c)2021 by Donna Whisnant
//

#include <iostream>
#include <string>
#include <time.h>
#include "binarydfc.h"

#include <assert.h>

// RetrieveFileMapping:
//
//    This function reads in an already opened BINARY file referenced by
//    'aFile' and fills in the CMemRanges object that encapsulates the file's
//    contents, offsetted by 'NewBase' (allowing loading of different files
//    to different base addresses).
bool CBinaryDataFileConverter::RetrieveFileMapping(std::istream &aFile, TAddress nNewBase, CMemRanges &aRange) const
{
	aRange.clear();
	aFile.seekg(0L, std::ios_base::end);
	std::istream::pos_type nSize = aFile.tellg();
	aRange.push_back(CMemRange(nNewBase, (nSize == std::istream::pos_type(-1)) ? TSize(0) : TSize(nSize)));
	aFile.seekg(0L, std::ios_base::beg);
	return true;
}


// ReadDataFile:
//
//    This function reads in an already opened BINARY file referenced by
//    'aFile' into the CMemBlocks referenced by 'aMemory' offsetted by
//    'NewBase' (allowing loading of different files to different base
//    address and setting the corresponding Memory Descriptors to 'nDesc'...
//
//    NOTE: The ranges in aMemory block must already be set, such as
//    from a call to RetrieveFileMapping() here and calling initFromRanges()
//    on the CMemBlocks to initialize it.  This function will only read
//    and populate the data on it
bool CBinaryDataFileConverter::ReadDataFile(std::istream &aFile, TAddress nNewBase, CMemBlocks &aMemory,
												TDescElement nDesc) const
{
	bool bRetVal = true;
	TAddress nCurrAddr = nNewBase;
	TMemoryElement nByte;

	aFile.seekg(0L, std::ios_base::beg);
	while ((!aFile.eof()) && (aFile.peek() != std::istream::traits_type::eof()) && (aFile.good())) {	// Note: peek needed because eof() doesn't set until following read
		nByte = aFile.get();
		if (!aFile.good()) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED);
		if (!aMemory.setElement(nCurrAddr, nByte)) {
			THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_OVERFLOW);
		}
		if (aMemory.descriptor(nCurrAddr) != 0) bRetVal = false;
		aMemory.setDescriptor(nCurrAddr, nDesc);
		++nCurrAddr;
	}

	return bRetVal;
}


// WriteDataFile:
//
//    This function writes to an already opened BINARY file referenced by
//    'aFile' from the CMemBlocks referenced by 'aMemory' and whose Descriptor
//    has a match specified by 'nDesc'.  Matching is done by 'anding' the
//    nDesc value with the descriptor in memory.  If the result is non-zero,
//    the location is written.  Unless, nDesc==0, in which case ALL locations
//    are written regardless of the actual descriptor.  If the nDesc validator
//    computes out as false, then the data is filled according to the FillMode.
bool CBinaryDataFileConverter::WriteDataFile(std::ostream &aFile, const CMemRanges &aRange, TAddress nNewBase,
												const CMemBlocks &aMemory, TDescElement nDesc, bool bUsePhysicalAddr,
												DFC_FILL_MODE_ENUM nFillMode, TMemoryElement nFillValue) const
{
	bool bRetVal = true;
	TSize nBytesLeft;				// Remaining number of bytes to check/write
	TAddress nCurrAddr;				// Current Logical Memory Address
	TAddress nRealAddr = 0;			// Current Written Address equivalent
	bool bNeedNewOffset = true;

	CMemRanges zRanges = aRange;	// Copy the ranges so we can compact, removeOverlaps, and sort it
	zRanges.compact();
	zRanges.removeOverlaps();
	zRanges.sort();

	srand(static_cast<unsigned>(time(nullptr)));

	for (auto const & itrRange : zRanges) {
		nCurrAddr = itrRange.startAddr();
		nBytesLeft = itrRange.size();

		while (nBytesLeft) {
			// TODO : While this does filling within a given range, it doesn't
			//		do filling across ranges in the area between them.  Should
			//		we just call consolidate on the range before we start
			//		since binary files can't convey and address detail??
			while (nBytesLeft && bNeedNewOffset) {
				if ((nDesc == 0) || (nDesc & aMemory.descriptor(nCurrAddr)) || (nFillMode != DFC_NO_FILL)) {
					if (bUsePhysicalAddr) {
						nRealAddr = aMemory.physicalAddr(nCurrAddr) + nNewBase;
					} else {
						nRealAddr = nCurrAddr + nNewBase;
					}
					bNeedNewOffset = false;
				} else {
					--nBytesLeft;
					++nCurrAddr;
				}
			}

			// TODO : Figure out how to handle non-byte TMemoryElement types to
			//		deal with Endianness, address incrementation, etc.  This only
			//		works with bytes right now...
			while (nBytesLeft && !bNeedNewOffset) {
				if ((nDesc == 0) || (nDesc & aMemory.descriptor(nCurrAddr))) {
					aFile.put(static_cast<unsigned char>(aMemory.element(nCurrAddr)));
					if (!aFile.good()) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_WRITEFAILED);
				} else {
					switch (nFillMode) {
						case DFC_ALWAYS_FILL_WITH:
						case DFC_CONDITIONAL_FILL_WITH:
							aFile.put(static_cast<unsigned char>(nFillValue));
							if (!aFile.good()) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_WRITEFAILED);
							break;
						case DFC_ALWAYS_FILL_WITH_RANDOM:
						case DFC_CONDITIONAL_FILL_WITH_RANDOM:
							aFile.put(static_cast<unsigned char>(static_cast<TMemoryElement>(rand() % 256)));
							if (!aFile.good()) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_WRITEFAILED);
							break;
						case DFC_NO_FILL:		// Binary files always have to be filled with something
							aFile.put(static_cast<unsigned char>(0x00));
							if (!aFile.good()) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_WRITEFAILED);
							break;
						default:
							assert(false);
							break;
					}
				}

				--nBytesLeft;
				++nCurrAddr;
				++nRealAddr;
				if ((nDesc == 0) || (nDesc & aMemory.descriptor(nCurrAddr)) || (nFillMode != DFC_NO_FILL)) {
					if (bUsePhysicalAddr) {
						if (nRealAddr != (aMemory.physicalAddr(nCurrAddr) + nNewBase)) {
							bNeedNewOffset = true;
						}
					} else {
						if (nRealAddr != (nCurrAddr + nNewBase)) {
							bNeedNewOffset = true;
						}
					}
				} else {
					bNeedNewOffset = true;
				}
			}
		}
	}

	return bRetVal;
}

