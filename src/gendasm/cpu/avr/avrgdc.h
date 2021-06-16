//
//	Atmel(Microchip) AVR Disassembler GDC Module
//	Copyright(c)2021 by Donna Whisnant
//
//	Generic Code-Seeking Disassembler
//	Copyright(c)2021 by Donna Whisnant
//

#ifndef _AVRGDC_H
#define _AVRGDC_H

// ============================================================================

#include <gdc.h>

// ============================================================================

typedef TDisassemblerTypes<
		uint16_t,		// TOpcodeSymbol
		uint32_t,		// TControlFlags
		uint32_t		// TGroupFlags
> TAVRDisassembler;

// ----------------------------------------------------------------------------

class CAVRDisassembler : public CDisassembler, protected CDisassemblerData<CAVRDisassembler, TAVRDisassembler>
{
public:
	CAVRDisassembler();

	// --------------------------------

protected:
	static constexpr TAVRDisassembler::TControlFlags OCTL_MASK = 0xFFFF;		// Mask for our control byte

	static inline TAVRDisassembler::TControlFlags MAKEOCTL(TAVRDisassembler::TControlFlags d, TAVRDisassembler::TControlFlags s)
	{
		return (((d & 0xFF) << 8) | (s & 0xFF));
	}

	inline TAVRDisassembler::TControlFlags OCTL_SRC() const
	{
		return (m_CurrentOpcode.control() & 0xFF);
	}

	inline TAVRDisassembler::TControlFlags OCTL_DST() const
	{
		return ((m_CurrentOpcode.control() >> 8) & 0xFF);
	}

	static inline constexpr TAVRDisassembler::TGroupFlags OGRP_MASK = 0xFFFF;	// Mask for our group byte

	static inline TAVRDisassembler::TGroupFlags MAKEOGRP(TAVRDisassembler::TGroupFlags d, TAVRDisassembler::TGroupFlags s)
	{
		return (((d & 0xFF) << 8) | (s & 0xFF));
	}

	inline TAVRDisassembler::TGroupFlags OGRP_SRC() const
	{
		return (m_CurrentOpcode.group() & 0xFF);
	}

	inline TAVRDisassembler::TGroupFlags OGRP_DST() const
	{
		return ((m_CurrentOpcode.group() >> 8) & 0xFF);
	}

	// --------------------------------

private:

};

// ============================================================================

#endif	// _AVRGDC_H
