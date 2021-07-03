//
//	Memory Management Class
//
//
//	Generic Code-Seeking Disassembler
//	Copyright(c)2021 by Donna Whisnant
//
//
//	Description:
//
//	Each memory object represents the entire memory being acted upon,
//	and typically, only one memory object needs to be created for an
//	application.
//
//	The memory is divided into pages (for paged memory applications)
//	and each page has a descriptor byte that the application can use
//	for processing -- useful for things such as "is_loaded", "is_data",
//	"is_code", etc.
//
//	Note:  All Addresses and All Block Numbers are ZERO ORIGINATED!
//		Size numbers (such as NBlocks and BlockSize) are ONE ORIGINATED!
//

#include "memclass.h"
#include "errmsgs.h"

#include <algorithm>
#include <iterator>
#include <assert.h>

// ============================================================================

// addressInRange
//		The following determines if a specified address exists within the
//		ranges specified by this list of ranges.
//		This is useful for programs needing to identify addresses that are
//		in a range without necessarily needing to use descriptor bytes or
//		other methods of address referal.
bool CMemRanges::addressInRange(TAddress nAddr) const
{
	for (auto const & itr : *this) {
		if (itr.addressInRange(nAddr)) return true;
	}
	return false;
}


// isNullRange
//		The following function will test the ranges in this list to see if
//		all are null.  A null range is defined as one whose size records are
//		all zero.
bool CMemRanges::isNullRange() const
{
	for (auto const & itr : *this) {
		if (!itr.isNullRange()) return false;
	}
	return true;
}


//	consolidate
//		The following function spans through the CMemRange and finds the
//		lowest and highest addresses.  It then purges the CMemRanges to
//		a single range and sets its size and length to the corresponding values.
//		The passed TUserData is used for the newly consolidated range.
void CMemRanges::consolidate(const TUserData &aUserData)
{
	TAddress nMinAddr;
	TAddress nMaxAddr;

	if (empty()) push_back(CMemRange());

	nMinAddr = front().startAddr();
	nMaxAddr = front().startAddr() + front().size();

	for (auto & itr : *this) {
		if (itr.startAddr() < nMinAddr) nMinAddr = itr.startAddr();
		if ((itr.startAddr() + itr.size()) > nMaxAddr) nMaxAddr = (itr.startAddr() + itr.size());
	}
	clear();
	push_back(CMemRange(nMinAddr, nMaxAddr-nMinAddr, aUserData));
}


//	Invert
//		The following is the converse of Consolidate.  It transforms
//		the range list to its inverse within the bounds of the
//		given range.
//		Note: The UserData is lost from the original Range... (obviously)
//		but will be set to the UserData of the range given
void CMemRanges::invert(const CMemRange &aRange)
{
	if (aRange.isNullRange()) {
		clear();
		return;
	}

	compact();
	removeOverlaps(true);

	// Remove any range that doesn't intersect the new range:
	iterator itr = begin();
	while (itr != end()) {
		if (!CMemRange::rangesOverlap(*itr, aRange)) {
			itr = erase(itr);		// erase returns next position
		} else {
			++itr;
		}
	}

	sort();

	// At this point, we have a collection of ranges inside the given range
	//	which we need to replace with the inverse:
	CMemRanges newRanges;
	TAddress nStartAddress = aRange.startAddr();
	TSize nRemainingSize = aRange.size();
	itr = begin();
	while (itr != end()) {
		if (itr->startAddr() >= nStartAddress) {
			if (itr->startAddr() != nStartAddress) {
				// Add range prior to current range (if it's not empty):
				newRanges.push_back(CMemRange(nStartAddress, itr->startAddr()-nStartAddress, aRange.userData()));
			}
			TSize nSize = itr->size() + (itr->startAddr()-nStartAddress);	// We just consumed this range and what was added
			nStartAddress += nSize;
			nRemainingSize -= nSize;
			assert(nRemainingSize >= 0);
		} else {
			// We can't have an address ahead of our current start address,
			//	since we already removed overlaps above, which would have
			//	consolidated anything that intersected
			assert(false);
		}
		++itr;
	}
	if (nRemainingSize) {
		// Add a range for anything remaining:
		newRanges.push_back(CMemRange(nStartAddress, nRemainingSize, aRange.userData()));
	}

	// Replace our ranges with the inverse:
	std::swap(*this, newRanges);
}


static bool ascendingLessThanRanges(const CMemRange &range1, const CMemRange &range2)
{
	if (range1.startAddr() < range2.startAddr()) return true;
	if ((range1.startAddr() == range2.startAddr()) &&
		(range1.size() < range2.size())) return true;
	return false;
}


//	Sort
//		The following sorts the range in ascending format.  Useful
//		for traversing, etc.  If two ranges have the same starting
//		address, then the shorter is placed ahead of the longer.
void CMemRanges::sort()
{
	std::sort(begin(), end(), ascendingLessThanRanges);
}

bool CMemRange::rangesOverlap(const CMemRange &range) const
{
	return rangesOverlap(*this, range);
}

bool CMemRange::rangesOverlap(const CMemRange &range1, const CMemRange &range2)
{
	if (range1.isNullRange() || range2.isNullRange()) return false;		// Empty ranges can't overlap

	TAddress nR1Lo = range1.startAddr();
	TAddress nR1Hi = range1.startAddr() + range1.size() - 1;
	TAddress nR2Lo = range2.startAddr();
	TAddress nR2Hi = range2.startAddr() + range2.size() - 1;
	return (((nR1Lo >= nR2Lo) && (nR1Lo <= nR2Hi)) ||			// Front end of #1 starts somewhere in #2
			((nR1Hi >= nR2Lo) && (nR1Hi <= nR2Hi)) ||			// Tail end of #1 ends somewhere in #2
			((nR2Lo >= nR1Lo) && (nR2Lo <= nR1Hi)) ||			// Front end of #2 starts somewhere in #1
			((nR2Hi >= nR1Lo) && (nR2Hi <= nR1Hi)));			// Tail end of #2 ends somewhere in #1
}

//	RemoveOverlaps
//		The following function traverses the range and removes
//		overlaps.  If overlaps exist, the offending range
//		entries are replaced by a single range spanning the same
//		total range.  Note that the order of the ranges will
//		change if overlaps exist and are not deterministic.
//		Also, the UserData value must be IDENTICAL on the
//		offending ranges, otherwise, they are not changed.
void CMemRanges::removeOverlaps(bool bIgnoreUserData)
{
	sort();

	for (iterator itrA = begin(); itrA != end(); ++itrA) {
		for (iterator itrB = itrA + 1; itrB != end(); ++itrB) {
			if (!bIgnoreUserData && (itrA->userData() != itrB->userData())) continue;
			if (CMemRange::rangesOverlap(*itrA, *itrB)) {
				TAddress nNewStartAddr = std::min(itrA->startAddr(), itrB->startAddr());
				TSize nNewSize = std::max(itrA->startAddr() + itrA->size(), itrB->startAddr() + itrB->size());
				itrA->setStartAddr(nNewStartAddr);
				itrA->setSize(nNewSize);
				itrB = erase(itrB) - 1;		// Note: erase returns the iterator AFTER the removed element, which we need to still check
			}
		}
	}
}


//	Compact
//		This function removes any size-zero entries
void CMemRanges::compact()
{
	iterator itr = begin();
	while (itr != end()) {
		if (itr->isNullRange()) {
			itr = erase(itr);		// erase returns next position
		} else {
			++itr;
		}
	}
}


bool CMemRanges::rangesOverlap(const CMemRange &range) const
{
	for (auto const & itr : *this) {
		if (itr.rangesOverlap(range)) return true;
	}
	return false;
}


TAddress CMemRanges::lowestAddress() const
{
	TAddress nLowest = (empty() ? 0 : at(0).startAddr());
	for (auto const & itr : *this) {
		if (itr.startAddr() < nLowest) nLowest = itr.startAddr();
	}
	return nLowest;
}


TAddress CMemRanges::highestAddress() const
{
	TAddress nHighest = (empty() ? 0 : at(0).startAddr());	// Just set it to the first address here and update in the loop to minimize calculations/code
	for (auto const & itr : *this) {
		TAddress nLastAddr = (itr.size() ? (itr.startAddr()+itr.size()-1) : itr.startAddr());
		if (nLastAddr > nHighest) nHighest = nLastAddr;
	}
	return nHighest;
}


// ============================================================================


void CMemBlocks::initFromRanges(const CMemRanges &ranges, TAddressOffset nPhysicalAddrOffset,
					bool bUseDescriptors, TMemoryElement nFillValue, TDescElement nDescValue)
{
	CMemRanges tempRanges = ranges;		// Copy so we can remove overlaps and sort it for efficiency
	tempRanges.removeOverlaps(true);
	tempRanges.sort();

	clear();
	for (CMemRanges::const_iterator itr = tempRanges.cbegin(); itr != tempRanges.cend(); ++itr) {
		push_back(CMemBlock(itr->startAddr(), itr->startAddr() + nPhysicalAddrOffset, bUseDescriptors,
							itr->size(), nFillValue, nDescValue));
	}
}

CMemRanges CMemBlocks::ranges() const
{
	CMemRanges tempRanges;
	for (auto const & itr : * this) {
		tempRanges.push_back(CMemRange(itr.logicalAddr(), itr.size()));
	}
	tempRanges.removeOverlaps(true);
	tempRanges.sort();
	return tempRanges;
}

TMemoryElement CMemBlocks::element(TAddress nLogicalAddr) const
{
	for (auto const & itr : *this) {
		if (itr.containsAddress(nLogicalAddr)) return itr.element(nLogicalAddr);
	}
	return 0;
}

bool CMemBlocks::setElement(TAddress nLogicalAddr, TMemoryElement nValue)
{
	for (auto & itr : *this) {
		if (itr.containsAddress(nLogicalAddr)) return itr.setElement(nLogicalAddr, nValue);
	}
	return false;
}

TDescElement CMemBlocks::descriptor(TAddress nLogicalAddr) const
{
	for (auto const & itr : *this) {
		if (itr.containsAddress(nLogicalAddr)) return itr.descriptor(nLogicalAddr);
	}
	return 0;
}

bool CMemBlocks::setDescriptor(TAddress nLogicalAddr, TDescElement nValue)
{
	for (auto & itr : *this) {
		if (itr.containsAddress(nLogicalAddr)) return itr.setDescriptor(nLogicalAddr, nValue);
	}
	return false;
}

TSize CMemBlocks::totalMemorySize() const
{
	TSize nTotalSize = 0;
	for (auto const & itr : *this) {
		nTotalSize += itr.size();
	}
	return nTotalSize;
}

TAddress CMemBlocks::lowestLogicalAddress() const
{
	TAddress nLowest = (empty() ? 0 : at(0).logicalAddr());
	for (auto const & itr : *this) {
		if (itr.logicalAddr() < nLowest) nLowest = itr.logicalAddr();
	}
	return nLowest;
}

TAddress CMemBlocks::highestLogicalAddress() const
{
	TAddress nHighest = (empty() ? 0 : at(0).logicalAddr());	// Just set it to the first address here and update in the loop to minimize calculations/code
	for (auto const & itr : *this) {
		TAddress nLastAddr = (itr.size() ? (itr.logicalAddr()+itr.size()-1) : itr.logicalAddr());
		if (nLastAddr > nHighest) nHighest = nLastAddr;
	}
	return nHighest;
}

bool CMemBlocks::containsAddress(TAddress nLogicalAddr) const
{
	for (auto const & itr : *this) {
		if (itr.containsAddress(nLogicalAddr)) return true;
	}
	return false;
}

void CMemBlocks::clearMemory(TMemoryElement nFillByte)
{
	for (auto & itr : *this) {
		itr.clearMemory(nFillByte);
	}
}

void CMemBlocks::clearDescriptors(TDescElement nDescValue)
{
	for (auto & itr : *this) {
		itr.clearDescriptors(nDescValue);
	}
}

TAddress CMemBlocks::physicalAddr(TAddress nLogicalAddr) const
{
	for (auto const & itr : *this) {
		if (itr.containsAddress(nLogicalAddr)) return itr.physicalAddr(nLogicalAddr);
	}
	return 0;
}


// ============================================================================

