//
//	Motorola (Srec) Data File Converter Class
//
//
//	Generic Code-Seeking Disassembler
//	Copyright(c)2021 by Donna Whisnant
//

#include <iostream>
#include <string>
#include <time.h>
#include <cstdio>
#include "srecdfc.h"

#include <stringhelp.h>

#include <assert.h>

#ifndef UNUSED
	#define UNUSED(x) ((void)(x))
#endif

// Motorola format:
//		Sxccaa..aadd....kk
//		where:	S is the record character "S"
//				x=record mode or type (See below)
//				cc=byte count after cc -- unlike Intel where the byte count is ONLY
//							the number of data bytes, this is the count of the
//							number of bytes in the address field, plus the number of
//							bytes in the data field, plus the 1 byte checksum
//				aa..aa=offset address -- Can be 2, 3, or 4 bytes depending on the 'x' type
//				dd....=data bytes
//				kk=checksum = 1's complement of the sum of all bytes in the record
//							starting with 'cc' through the last 'dd'...
//							Total sum (including checkbyte) should equal 0xFF
//
//			Record mode/types:
//				0 = Header character -- this is just a file signon that can contain
//						any desired data...  GM PTP's, for example, use "HDR" in the
//						data field...  The Address field is 2 bytes (4 characters) and
//						is usually 0000 except for Motorola DSP's where it indicates
//						the DSP Memory Space for the s-records that follow, where
//						0001=X, 0002=Y, 0003=L, 0004=P, and 0005=E (DSP only!)
//						This field is optional
//				1 = Data Record with a 2-byte address (aaaa = start address for data in this record)
//				2 = Data Record with a 3-byte address (aaaaaa = start address for data in this record)
//				3 = Data Record with a 4-byte address (aaaaaaaa = start address for data in this record)
//				4 = Reserved
//				5 = 16-bit Count of S1/S2/S3 records if count is <=65535 (else S6 is used) [Optional]
//				6 = 24-bit Count of S1/S2/S3 records if count is <=16777215 [Optional] (Note: Latest addition, note 100% ratified?)
//				7 = EOF Record with a 4-byte address (aaaaaaaa = Executable start address for this s-file)
//				8 = EOF Record with a 3-byte address (aaaaaa = Executable start address for this s-file)
//				9 = EOF Record with a 2-byte address (aaaa = Executable start address for this s-file)
//
//		Extensions:
//			S19 = 16-Bit addressing files (ones using S1 and S9 records)
//			S28 = 24-Bit addressing files (ones using S2 and S8 records)
//			S37 = 32-Bit addressing files (ones using S3 and S7 records)
//



// RetrieveFileMapping:
//
//    This function reads in an already opened text BINARY file pointed to by
//    'aFile' and generates a TMemRange object that encapsulates the file's
//    contents, offsetted by 'NewBase' (allowing loading of different files
//    to different base addresses).
bool CSrecDataFileConverter::RetrieveFileMapping(std::istream &aFile, TAddress nNewBase, CMemRanges &aRange,
													std::ostream *msgFile, std::ostream *errFile) const
{
	UNUSED(msgFile);
	UNUSED(errFile);

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
bool CSrecDataFileConverter::ReadDataFile(std::istream &aFile, TAddress nNewBase, CMemBlocks &aMemory,
												TDescElement nDesc,
												std::ostream *msgFile, std::ostream *errFile) const
{
	UNUSED(msgFile);
	UNUSED(errFile);

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
bool CSrecDataFileConverter::_ReadDataFile(std::istream &aFile, TAddress nNewBase, CMemBlocks *pMemory, CMemRanges *pRange, TDescElement nDesc) const
{
	bool bRetVal = true;
	int nLineCount = 0;				// Line Counter
	std::string strBuffer;			// Line Buffer
	unsigned int nBytes;			// Number of bytes supposed to be on line
	std::string::size_type nStrPos;	// Position in the string for processing
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
		if (starts_with(strBuffer, "S") && (strBuffer.size() >= 6)) {	// Must have a minimum of Sxnnkk ('S', mode, byte count, checksum)
			sscanf(&strBuffer.c_str()[1], "%1X%2X", &nMode, &nBytes);
			nChecksum = nBytes;
			nStrPos = 4;		// Start at the address field
			if (strBuffer.size() < (4 + (nBytes*2)))
				THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_INVALID_RECORD, nLineCount, "Line too short");
			// NOTE: nBytes includes addresss, data, and checksum fields in the count!!  (unlike Intel which is data only)
			nBytes -= 1;		// Don't count the checksum, since we aren't reading it with the data
			switch (nMode) {
				case 0:		// S0,S1,S5,S9 use 16-bit address field
				case 1:
				case 5:
				case 9:
					sscanf(&strBuffer.c_str()[nStrPos], "%4X", &nOffsetAddr);
					nChecksum += (nOffsetAddr/256) + (nOffsetAddr%256);
					nStrPos += 4;
					nBytes -= 2;
					break;
				case 2:		// S2,S6,S8 use 24-bit address field
				case 6:
				case 8:
					sscanf(&strBuffer.c_str()[nStrPos], "%6X", &nOffsetAddr);
					nChecksum += ((nOffsetAddr/65536ul)%256) + ((nOffsetAddr/256ul)%256) + (nOffsetAddr%256);
					nStrPos += 6;
					nBytes -= 3;
					break;
				case 3:		// S3,S7 use 32-bit address field
				case 7:
					sscanf(&strBuffer.c_str()[nStrPos], "%8X", &nOffsetAddr);
					nChecksum += ((nOffsetAddr/16777216ul)%256) + ((nOffsetAddr/65536ul)%256) + ((nOffsetAddr/256ul)%256) + (nOffsetAddr%256);
					nStrPos += 8;
					nBytes -= 4;
					break;
				default:
					THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_INVALID_RECORD, nLineCount, "Unknown record type");
					break;
			}

			if (((nMode == 1) || (nMode == 2) || (nMode == 3)) &&
				(nCurrAddr != (nOffsetAddr + nNewBase))) bNeedRangeUpdate = true;

			if (bNeedRangeUpdate) {
				// If we need to write the Range do so, else only update the StartAddr
				if (bNeedToWriteRange) {
					if (pRange) {
						pRange->push_back(CMemRange(nStartAddr, nCurrSize));
					}
					bNeedToWriteRange = false;
				}

				// Set start of next range and init variables:
				nStartAddr = (nOffsetAddr + nNewBase);
				nCurrAddr = nStartAddr;
				nCurrSize = 0;
				bNeedRangeUpdate = false;
			}

			switch (nMode) {
				case 0:			// ASCII Text Comment, encoded as data
					for (unsigned int i = 0; i < nBytes; ++i) {
						unsigned int nTempByte;
						sscanf(&strBuffer.c_str()[nStrPos+i*2], "%2X", &nTempByte);
						nChecksum += nTempByte;
					}
					// Ignore the comment data -- TODO : Find something to do with it?  like print it?
					break;
				case 1:			// Data Entry
				case 2:
				case 3:
					for (unsigned int i = 0; i < nBytes; ++i) {
						unsigned int nTempByte;
						sscanf(&strBuffer.c_str()[nStrPos+i*2], "%2X", &nTempByte);
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
				case 7:			// End of File marker (and start address)
				case 8:
				case 9:
					if (nBytes != 0) {
						THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_INVALID_RECORD, nLineCount, "Invalid Length");
					}
					bEndReached = true;
					// Ignore start address results -- TODO : Find a way to report this back to the CDisassembler class as an entry point!
					break;
				case 5:			// Record count (ignore)
				case 6:
					break;
				default:
					THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_INVALID_RECORD, nLineCount, "Unknown record type");
					break;
			}

			unsigned int nTempByte;
			sscanf(&strBuffer.c_str()[nStrPos+nBytes*2], "%2X", &nTempByte);
			nChecksum += nTempByte;
			if ((nChecksum % 256) != 0xFF) {		// Checksum is one's complement, so sum including it is 0xFF
				THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_CHECKSUM, nLineCount);
			}
		} else if (starts_with(strBuffer, "S")) {
			THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_INVALID_RECORD, nLineCount, "Line too short");
		}	// Ignore other lines not starting with 'S' to support random comments, etc.
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
bool CSrecDataFileConverter::WriteDataFile(std::ostream &aFile, const CMemRanges &aRange, TAddress nNewBase,
												const CMemBlocks &aMemory, TDescElement nDesc, bool bUsePhysicalAddr,
												DFC_FILL_MODE_ENUM nFillMode, TMemoryElement nFillValue,
												std::ostream *msgFile, std::ostream *errFile) const
{
	UNUSED(msgFile);
	UNUSED(errFile);

	bool bRetVal = true;
	int nLineCount = 0;				// Line Counter
	std::string strBuffer;			// Line Buffer
	unsigned int nMode = 1;			// Will be 1, 2, or 3 for S19, S28, or S37, respectively
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

	if (aRange.highestAddress() > 16777215ul) {
		nMode = 3;
	} else if (aRange.highestAddress() > 65535ul) {
		nMode = 2;
	}

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
					nOffsetAddr = nRealAddr;
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
					} else {
						if (nRealAddr != (nCurrAddr + nNewBase)) {
							bNeedNewOffset = true;
						}
					}
				} else {
					// If moving into unavailable data and not filling, we
					//	need a new offset:
					bNeedNewOffset = true;
				}
			}

			if (nBytes) {
				nBytes += 2+nMode;		// Byte count includes the checksum and address bytes
				nChecksum += nBytes;
				switch (nMode) {
					case 1:
						nChecksum += (nOffsetAddr/256)+(nOffsetAddr%256);
						std::sprintf(arrTemp, "S1%02X%04X", nBytes, nOffsetAddr);
						break;
					case 2:
						nChecksum += ((nOffsetAddr/65536ul)%256) + ((nOffsetAddr/256ul)%256) + (nOffsetAddr%256);
						std::sprintf(arrTemp, "S2%02X%06X", nBytes, nOffsetAddr);
						break;
					case 3:
						nChecksum += ((nOffsetAddr/16777216ul)%256) + ((nOffsetAddr/65536ul)%256) + ((nOffsetAddr/256ul)%256) + (nOffsetAddr%256);
						std::sprintf(arrTemp, "S3%02X%08X", nBytes, nOffsetAddr);
						break;
				}
				nChecksum = (~nChecksum) & 0xFF;		// 1's complement of sum
				aFile << arrTemp;
				aFile << strBuffer;
				std::sprintf(arrTemp, "%02X\r\n", nChecksum);
				aFile << arrTemp;
				if (!aFile.good()) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_WRITEFAILED);
				++nLineCount;
			}
		}
	}

	// This is optional, but might as well do it and write the number of S1/S2/S3 record:
	bool bS6 = (nLineCount > 65535);
	nChecksum = (2+(bS6 ? 2 : 1));		// number of bytes
	if (bS6) {
		nChecksum += ((nLineCount/65536ul)%256) + ((nLineCount/256ul)%256) + (nLineCount%256);
	} else {
		nChecksum += (nLineCount/256)+(nLineCount%256);
	}
	nChecksum = (~nChecksum) & 0xFF;		// 1's complement of sum
	if (bS6) {
		std::sprintf(arrTemp, "S604%06X%02X\r\n", nLineCount, nChecksum);
	} else {
		std::sprintf(arrTemp, "S503%04X%02X\r\n", nLineCount, nChecksum);
	}
	aFile << arrTemp;

	switch (nMode) {		// EOF record
		case 1:
			aFile << "S9030000FC\r\n";
			break;
		case 2:
			aFile << "S804000000FB\r\n";
			break;
		case 3:
			aFile << "S70500000000FA\r\n";
			break;
	}
	if (!aFile.good()) THROW_EXCEPTION_ERROR(EXCEPTION_ERROR::ERR_WRITEFAILED);
	return bRetVal;
}

