//
//	Intel (Hex) Data File Converter Class
//
//
//	Generic Code-Seeking Disassembler
//	Copyright(c)2021 by Donna Whisnant
//

#include <iostream>
#include <string>
#include <time.h>
#include <cstdio>
#include "inteldfc.h"

#include <stringhelp.h>

#include <assert.h>

// Intel format:
//    ccaaaammdd....kk
//    where: cc=byte count
//           aaaa=offset address
//           mm=mode byte
//           dd=date bytes (there are 'cc' many of 'dd' entries)
//           kk=checksum = sum of ALL hex pairs as bytes (including cc, aaaa, mm, and dd)
//                            Sum (including checkbyte) should equal zero.
//       mm=00 -> normal line
//       mm=01 -> end of file
//       mm=02 -> SBA entry (Extended Segment Offset)
//       mm=03 -> Start Segment Address
//       mm=04 -> Extended Linear Address
//       mm=05 -> Start Linear Addrss


// RetrieveFileMapping:
//
//    This function reads in an already opened text BINARY file referenced by
//    'aFile' and fills in the CMemRanges object that encapsulates the file's
//    contents, offsetted by 'NewBase' (allowing loading of different files
//    to different base addresses).
bool CIntelDataFileConverter::RetrieveFileMapping(std::istream &aFile, TAddress nNewBase, CMemRanges &aRange) const
{
	return _ReadDataFile(aFile, nNewBase, nullptr, &aRange, 0);
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
bool CIntelDataFileConverter::ReadDataFile(std::istream &aFile, TAddress nNewBase, CMemBlocks &aMemory,
												TDescElement nDesc) const
{
	return _ReadDataFile(aFile, nNewBase, &aMemory, nullptr, nDesc);
}


// _ReadDataFile:
//
//    Internal helper for ReadDataFile that's a composite function to handle
//    both the reading of the file into memory and simply reading the Mapping.
//    This is a private function called by the regular ReadDataFile and
//    RetrieveFileMapping.  If pMemory is nullptr then memory is not filled.  If
//    pRange is nullptr, the mapping is not generated.  If either or both is
//    not nullptr, then the corresponding operation is performed.
bool CIntelDataFileConverter::_ReadDataFile(std::istream &aFile, TAddress nNewBase, CMemBlocks *pMemory, CMemRanges *pRange, TDescElement nDesc) const
{
	bool bRetVal = true;
	int nLineCount = 0;				// Line Counter
	std::string strBuffer;			// Line Buffer
	TAddress nExtendedAddr = 0;		// Will be either the SBA (Segment Base Address) times 16 or the Extended Linear Address times 65536
	unsigned int nBytes;			// Number of bytes supposed to be on line
	TAddress nOffsetAddr;			// Offset Address read on line
	unsigned int nChecksum;			// Running Checksum Calculated
	unsigned int nMode;				// Mode byte from file
	bool bEndReached = false;		// End reached flag
	bool bNeedRangeUpdate = true;	// Flag for needing to update the range capture (force the first update)
	bool bNeedToWriteRange = false;	// Flag for needing to write the range to the memory image
	TAddress nStartAddr = 0;		// StartAddr of the Current Range
	TAddress nCurrAddr = 0;			// Current Memory Address
	TSize nCurrSize = 0;			// Size of the Current Range

	if (pRange) {
		pRange->clear();
	}

	aFile.seekg(0L, std::ios_base::beg);
	while ((!aFile.eof()) && (aFile.peek() != std::istream::traits_type::eof()) && aFile.good() && !bEndReached) {	// Note: peek needed because eof() doesn't set until following read
		std::getline(aFile, strBuffer);
		if (!aFile.good()) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_READFAILED);
		++nLineCount;

		trim(strBuffer);
		if (starts_with(strBuffer, ":") && (strBuffer.size() >= 11)) {
			sscanf(&strBuffer.c_str()[1], "%2X%4X%2X", &nBytes, &nOffsetAddr, &nMode);
			nChecksum = nBytes + (nOffsetAddr/256) + (nOffsetAddr%256) + nMode;

			if (strBuffer.size() < (11 + (nBytes*2)))
				THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_INVALID_RECORD, nLineCount, "Line too short");

			if ((nMode == 0) && (nCurrAddr != (nExtendedAddr + nOffsetAddr + nNewBase))) bNeedRangeUpdate = true;

			if (bNeedRangeUpdate) {
				// If we need to update the Range do so, else only update the StartAddr
				if (bNeedToWriteRange) {
					if (pRange) {
						pRange->push_back(CMemRange(nStartAddr, nCurrSize));
					}
					bNeedToWriteRange = false;
				}

				// Set start of next range and init variables:
				nStartAddr = (nExtendedAddr + nOffsetAddr + nNewBase);
				nCurrAddr = nStartAddr;
				nCurrSize = 0;
				bNeedRangeUpdate = false;
			}

			switch (nMode) {
				case 0:			// Data Entry
					for (unsigned int i = 0; i < nBytes; ++i) {
						unsigned int nTempByte;
						sscanf(&strBuffer.c_str()[9+i*2], "%2X", &nTempByte);
						nChecksum += nTempByte;
						if (pMemory) {
							if (!pMemory->setElement(nCurrAddr, nTempByte)) {
								THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_OVERFLOW, nLineCount);
							}
							if (pMemory->descriptor(nCurrAddr) != 0) bRetVal = false;	// Signal overlap
							pMemory->setDescriptor(nCurrAddr, nDesc);
						}
						++nCurrAddr;
						++nCurrSize;
						// Any time we add a byte, make sure the range will get updated
						bNeedToWriteRange = true;
					}
					break;
				case 1:			// End of File marker
					if (nBytes != 0) {
						THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_INVALID_RECORD, nLineCount, "Invalid Length");
					}
					bEndReached = true;
					break;
				case 2:			// SBA - Extended Segment Base Address found
					if (nBytes != 2) {
						THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_INVALID_RECORD, nLineCount, "Invalid Length");
					}
					sscanf(&strBuffer.c_str()[9], "%4X", &nExtendedAddr);
					nChecksum = (nExtendedAddr/256) + (nExtendedAddr%256);
					nExtendedAddr *= 16;		// Convert segment offset to linear
					break;
				case 3:			// Start Segment Address
					if (nBytes != 4) {
						THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_INVALID_RECORD, nLineCount, "Invalid Length");
					}
					for (unsigned int i = 0; i < nBytes; ++i) {
						unsigned int nTempByte;
						sscanf(&strBuffer.c_str()[9+i*2], "%2X", &nTempByte);
						nChecksum += nTempByte;
					}
					// Ignore results -- TODO : Find a way to report this back to the CDisassembler class as an entry point!
					break;
				case 4:			// Extended Linear Address
					if (nBytes != 2) {
						THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_INVALID_RECORD, nLineCount, "Invalid Length");
					}
					sscanf(&strBuffer.c_str()[9], "%4X", &nExtendedAddr);
					nChecksum = (nExtendedAddr/256) + (nExtendedAddr%256);
					nExtendedAddr = nExtendedAddr << 16;		// Convert to upper word
					break;
				case 5:			// Start Linear Address
					if (nBytes != 4) {
						THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_INVALID_RECORD, nLineCount, "Invalid Length");
					}
					for (unsigned int i = 0; i < nBytes; ++i) {
						unsigned int nTempByte;
						sscanf(&strBuffer.c_str()[9+i*2], "%2X", &nTempByte);
						nChecksum += nTempByte;
					}
					// Ignore results -- TODO : Find a way to report this back to the CDisassembler class as an entry point!
					break;
				default:
					THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_INVALID_RECORD, nLineCount, "Unknown record type");
					break;
			}

			unsigned int nTempByte;
			sscanf(&strBuffer.c_str()[9+nBytes*2], "%2X", &nTempByte);
			nChecksum += nTempByte;
			if ((nChecksum % 256) != 0) {
				THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_CHECKSUM, nLineCount);
			}
		} else if (starts_with(strBuffer, ":")) {
			THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_INVALID_RECORD, nLineCount, "Line too short");
		}	// Ignore other lines not starting with ':' to support comments, etc.
	}

	// Make a final test to see if the range needs to be saved,
	//    This is necessary to save the last range operated upon
	//    since we can't write the range until we actually know
	//    its size.
	if (bNeedToWriteRange) {
		if (pRange) {
			pRange->push_back(CMemRange(nStartAddr, nCurrSize));
		}
	}

	// Check EOF:
	if (((aFile.eof()) || (aFile.peek() != std::istream::traits_type::eof()) || aFile.good()) &&
		!bEndReached) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_UNEXPECTED_EOF);

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
bool CIntelDataFileConverter::WriteDataFile(std::ostream &aFile, const CMemRanges &aRange, TAddress nNewBase,
												const CMemBlocks &aMemory, TDescElement nDesc, bool bUsePhysicalAddr,
												DFC_FILL_MODE_ENUM nFillMode, TMemoryElement nFillValue) const
{
	bool bRetVal = true;
	int nLineCount = 0;				// Line Counter
	std::string strBuffer;			// Line Buffer
	TAddress nExtendedAddr = 0;		// Extended Linear address for addresses >16-bit (just upper 16-bits right-justified)
	unsigned int nBytes;			// Number of bytes supposed to be on line
	TAddress nOffsetAddr;			// Offset Address for line
	unsigned int nChecksum;			// Running Checksum Calculated
	TSize nBytesLeft;				// Remaining number of bytes to check/write
	TAddress nCurrAddr;				// Current Logical Memory Address
	TAddress nRealAddr = 0;			// Current Written Address equivalent
	bool bNeedNewOffset = true;
	char arrTemp[30];				// Temp buffer for formatting data

	CMemRanges zRanges = aRange;	// Copy the ranges so we can compact, removeOverlaps, and sort it
	zRanges.compact();
	zRanges.removeOverlaps();
	zRanges.sort();

	srand(static_cast<unsigned>(time(nullptr)));

	for (auto const & itrRange : zRanges) {
		nCurrAddr = itrRange.startAddr();
		nBytesLeft = itrRange.size();

		while (nBytesLeft) {
			strBuffer.clear();
			nBytes = 0;
			nChecksum = 0;
			bNeedNewOffset = true;

			while ((nBytesLeft) && (bNeedNewOffset)) {		// Find first location to write next
				if ((nDesc == 0) || (nDesc & aMemory.descriptor(nCurrAddr)) ||
					((nFillMode != DFC_NO_FILL) &&
					 (nFillMode != DFC_CONDITIONAL_FILL_WITH) &&
					 (nFillMode != DFC_CONDITIONAL_FILL_WITH_RANDOM))) {
					if (bUsePhysicalAddr) {
						nRealAddr = aMemory.physicalAddr(nCurrAddr) + nNewBase;
					} else {
						nRealAddr = nCurrAddr + nNewBase;
					}
					nOffsetAddr = (nRealAddr & 0xFFFF);
					if ((nRealAddr / 65536ul) != nExtendedAddr) {
						nExtendedAddr = (nRealAddr / 65536ul);
						if (nExtendedAddr > 65535ul) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_OVERFLOW, nLineCount);
						unsigned int nCheckByte = (256-(((nExtendedAddr/256)+(nExtendedAddr%256)+2+4)%256))%256;
						std::sprintf(arrTemp, ":02000004%04X%02X\r\n", nExtendedAddr, nCheckByte);
						aFile << arrTemp;
						if (!aFile.good()) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_WRITEFAILED);
						++nLineCount;
					}
					bNeedNewOffset = false;
				} else {
					--nBytesLeft;
					++nCurrAddr;
				}
			}

			for (int i = 0; ((i < 16) && !bNeedNewOffset && (nBytesLeft)); ++i) {
				bool bWriteByte = true;		// Default to write a byte
				unsigned int nTempByte = 0;
				if ((nDesc == 0) || (nDesc & aMemory.descriptor(nCurrAddr))) {
					nTempByte = aMemory.element(nCurrAddr);
				} else {
					switch (nFillMode) {
						case DFC_ALWAYS_FILL_WITH:
							nTempByte = nFillValue;
							break;
						case DFC_ALWAYS_FILL_WITH_RANDOM:
							nTempByte = static_cast<unsigned char>(static_cast<TMemoryElement>(rand() % 256));
							break;
						default:
							bWriteByte = false;		// No write on No-Fill
							break;
					}
				}
				if (bWriteByte) {
					std::sprintf(arrTemp, "%02X", nTempByte);
					strBuffer += arrTemp;
					nChecksum += nTempByte;
					++nBytes;
				}
				--nBytesLeft;
				++nCurrAddr;
				++nRealAddr;

				if ((nDesc == 0) || (nDesc & aMemory.descriptor(nCurrAddr)) ||
					((nFillMode != DFC_NO_FILL) &&
					 (nFillMode != DFC_CONDITIONAL_FILL_WITH) &&
					 (nFillMode != DFC_CONDITIONAL_FILL_WITH_RANDOM))) {
					if (bUsePhysicalAddr) {
						if (nRealAddr != (aMemory.physicalAddr(nCurrAddr)+nNewBase)) {
							bNeedNewOffset = true;
						}
					}
					if ((nRealAddr/65536ul) != nExtendedAddr) {
						bNeedNewOffset = true;
					} else {
						if ((nRealAddr & 0xFFFF) == 0) bNeedNewOffset = true;	// TODO : Not sure this is needed with linear extended addresses, was it for SBA? (now deprecated)
					}
				} else {
					// If moving into unavailable data and not filling, we
					//	need a new offset:
					bNeedNewOffset = true;
				}
			}

			if (nBytes) {
				nChecksum += nBytes;
				nChecksum += (nOffsetAddr/256)+(nOffsetAddr%256);
				nChecksum = (256-(nChecksum%256))%256;
				std::sprintf(arrTemp, ":%02X%04X00", nBytes, nOffsetAddr);
				aFile << arrTemp;
				aFile << strBuffer;
				std::sprintf(arrTemp, "%02X\r\n", nChecksum);
				aFile << arrTemp;
				if (!aFile.good()) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_WRITEFAILED);
				++nLineCount;
			}
		}
	}

	aFile << ":00000001FF\r\n";		// EOF record
	if (!aFile.good()) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_WRITEFAILED);
	return bRetVal;
}

