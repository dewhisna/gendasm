//
//	Memory Management Class
//
//
//	Generic Code-Seeking Disassembler
//	Copyright(c)2021 by Donna Whisnant
//

#ifndef MEMCLASS_H
#define MEMCLASS_H

// ============================================================================

#include <stdint.h>
#include <cstddef>
#include <vector>

// ============================================================================

typedef uint32_t TAddress;									// Basic Address type (should be numeric)
typedef int32_t TAddressOffset;								// Basic Address Offset type (signed)
typedef std::size_t TSize;									// Basic Range Size type (should be numeric)
typedef unsigned int TUserData;								// UserData type (arbitrary type)

typedef uint8_t TMemoryElement;								// Basic Memory Element type (should be numeric)
typedef std::vector<TMemoryElement> CMemoryArray;
typedef uint32_t TDescElement;								// Memory Descriptor Element type (must be mappable to MEM_DESC enum in the CDisassembler object)
typedef std::vector<TDescElement> CDescArray;

// ============================================================================

class CMemRange
{
protected:
	TAddress m_nStartAddr = 0;
	TSize m_nSize = 0;
	TUserData m_aUserData = {};

public:
	CMemRange()
	{ }

	CMemRange(TAddress nStartAddr, TSize nSize, const TUserData & aUserData = { })
		:	m_nStartAddr(nStartAddr),
			m_nSize(nSize),
			m_aUserData(aUserData)
	{ }

	inline TAddress startAddr() const { return m_nStartAddr; }
	inline void setStartAddr(TAddress nStartAddr) { m_nStartAddr = nStartAddr; }

	inline TSize size() const { return m_nSize; }
	inline void setSize(TSize nSize) { m_nSize = nSize; }

	inline TUserData userData() const { return m_aUserData; }
	inline void setUserData(const TUserData &aUserData) { m_aUserData = aUserData; }

	static bool rangesOverlap(const CMemRange &range1, const CMemRange &range2);

	// ----

	bool addressInRange(TAddress nAddr) const
	{
		return ((nAddr >= m_nStartAddr) && (nAddr < (m_nStartAddr + m_nSize)));
	}

	bool isNullRange() const
	{
		return (m_nSize == 0);
	}
};

// ----------------------------------------------------------------------------

class CMemRanges : public std::vector<CMemRange>
{
public:
	bool addressInRange(TAddress nAddr) const;
	bool isNullRange() const;

	void consolidate(const TUserData & aUserData = { });
	void invert(const CMemRange &aRange);
	void sort();
	void removeOverlaps(bool bIgnoreUserData = false);
	void compact();

	inline TUserData userData() const { return m_aUserData; }
	inline void setUserData(const TUserData &aUserData) { m_aUserData = aUserData; }

protected:
	TUserData m_aUserData = {};
};

// ============================================================================

class CMemBlock {
protected:
	TAddress m_nLogicalAddr;
	TAddress m_nPhysicalAddr;
	// ----
	bool m_bUseDescriptors;
	CMemoryArray m_arrMemoryData;
	CDescArray m_arrMemoryDescriptors;

public:
	CMemBlock(TAddress nLogicalAddr, TAddress nPhysicalAddr, bool bUseDescriptors,
				TSize nSize, TMemoryElement nFillValue, TDescElement nDescValue = 0)
		:	m_nLogicalAddr(nLogicalAddr),
			m_nPhysicalAddr(nPhysicalAddr),
			m_bUseDescriptors(bUseDescriptors)
	{
		m_arrMemoryData.resize(nSize, nFillValue);
		m_arrMemoryDescriptors.resize(nSize, nDescValue);
	}

	inline TAddress logicalAddr() const { return m_nLogicalAddr; }
	inline TAddress physicalAddr() const { return m_nPhysicalAddr; }
	inline TAddress physicalAddr(TAddress nLogicalAddr) const
	{
		if (nLogicalAddr < m_nLogicalAddr) return 0;
		CMemoryArray::size_type nIndex = (nLogicalAddr - m_nLogicalAddr);
		if (nIndex >= m_arrMemoryData.size()) return 0;
		return m_nPhysicalAddr + nIndex;
	}
	inline bool containsAddress(TAddress nLogicalAddr) const
	{
		if (nLogicalAddr < m_nLogicalAddr) return false;
		if ((nLogicalAddr - m_nLogicalAddr) >= m_arrMemoryData.size()) return false;
		return true;
	}
	// ----
	inline bool useDescriptors() const { return m_bUseDescriptors; }
	// ----
	inline TSize size() const { return m_arrMemoryData.size(); }
	// ----
	void clearMemory(TMemoryElement nFillByte)
	{
		m_arrMemoryData.resize(m_arrMemoryData.size(), nFillByte);
	}
	void clearDescriptors(TDescElement nDescValue)
	{
		if (!m_bUseDescriptors) return;
		m_arrMemoryDescriptors.resize(m_arrMemoryDescriptors.size(), nDescValue);
	}
	// ----
	TMemoryElement element(TAddress nLogicalAddr) const
	{
		if (nLogicalAddr < m_nLogicalAddr) return 0;
		CMemoryArray::size_type nIndex = (nLogicalAddr - m_nLogicalAddr);
		if (nIndex >= m_arrMemoryData.size()) return 0;
		return m_arrMemoryData.at(nIndex);
	}
	bool setElement(TAddress nLogicalAddr, TMemoryElement nValue)
	{
		if (nLogicalAddr < m_nLogicalAddr) return false;
		CMemoryArray::size_type nIndex = (nLogicalAddr - m_nLogicalAddr);
		if (nIndex >= m_arrMemoryData.size()) return false;
		m_arrMemoryData[nIndex] = nValue;
		return true;
	}
	// ----
	TDescElement descriptor(TAddress nLogicalAddr) const
	{
		if (!m_bUseDescriptors) return 0;
		if (nLogicalAddr < m_nLogicalAddr) return 0;
		CDescArray::size_type nIndex = (nLogicalAddr - m_nLogicalAddr);
		if (nIndex >= m_arrMemoryDescriptors.size()) return 0;
		return m_arrMemoryDescriptors.at(nIndex);
	}
	bool setDescriptor(TAddress nLogicalAddr, TDescElement nValue)
	{
		if (!m_bUseDescriptors) return false;
		if (nLogicalAddr < m_nLogicalAddr) return false;
		CDescArray::size_type nIndex = (nLogicalAddr - m_nLogicalAddr);
		if (nIndex >= m_arrMemoryDescriptors.size()) return false;
		m_arrMemoryDescriptors[nIndex] = nValue;
		return true;
	}
};

// ----------------------------------------------------------------------------

class CMemBlocks : public std::vector<CMemBlock>
{
public:
	void initFromRanges(const CMemRanges &ranges, TAddressOffset nPhysicalAddrOffset,		// Note: ranges will be the logical addresses
						bool bUseDescriptors, TMemoryElement nFillValue, TDescElement nDescValue = 0);

	TMemoryElement element(TAddress nLogicalAddr) const;
	bool setElement(TAddress nLogicalAddr, TMemoryElement nValue);
	// ----
	TDescElement descriptor(TAddress nLogicalAddr) const;
	bool setDescriptor(TAddress nLogicalAddr, TDescElement nValue);
	// ----
	TSize totalMemorySize() const;
	TAddress lowestLogicalAddress() const;		// Returns 0 if there's no memory defined
	TAddress highestLogicalAddress() const;		// Returns 0 if there's no memory defined
	bool containsAddress(TAddress nLogicalAddr) const;
	// ----
	void clearMemory(TMemoryElement nFillByte);
	void clearDescriptors(TDescElement nDescValue);
	// ----
	inline TAddress physicalAddr(TAddress nLogicalAddr) const;
};

// ============================================================================

#endif	// MEMCLASS_H

