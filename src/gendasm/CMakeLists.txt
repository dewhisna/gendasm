#
#	Generic Code-Seeking Disassembler
#	Copyright(c)2021 by Donna Whisnant
#

cmake_minimum_required(VERSION 3.19)

project(gendasm LANGUAGES CXX VERSION 1.0 DESCRIPTION "Generic Code-Seeking Disassembler")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -----------------------------------------------------------------------------

# Compile Options

option(FUNC_FLAG_COMMENT "Add Function Flags to Comment Field" OFF)

# -----------------------------------------------------------------------------

set(gendasm_Sources
	dfc.cpp				# Data File Converter
	errmsgs.cpp			# Error Message Handler
	gendasm.cpp			# main
	gdc.cpp				# Generic Disassembly Class
	memclass.cpp		# Memory Management Class
)

set(gendasm_Headers
	dfc.h				# Data File Converter
	enumhelp.h			# Enum Helper Functions/Defines
	errmsgs.h			# Error Message Handler
	gdc.h				# Generic Disassembly Class
	memclass.h			# Memory Management Class
	stringhelp.h		# String Helper Functions
)

# -----------------------------------------------------------------------------

#
# Binary Data File Converter
#

set(dfc_binary_Sources
	dfc/binary/binarydfc.cpp
)

set(dfc_binary_Headers
	dfc/binary/binarydfc.h
)

# -----------------------------------------------------------------------------

#
# Intel (Hex) Data File Converter
#

set(dfc_intel_Sources
	dfc/intel/inteldfc.cpp
)

set(dfc_intel_Headers
	dfc/intel/inteldfc.h
)

# -----------------------------------------------------------------------------

#
# Motorola (Srec) Data File Converter
#

set(dfc_srec_Sources
	dfc/srec/srecdfc.cpp
)

set(dfc_srec_Headers
	dfc/srec/srecdfc.h
)

# -----------------------------------------------------------------------------

#
# M6811 Disassembler
#

set(cpu_m6811_Sources
	cpu/m6811/m6811gdc.cpp
)

set(cpu_m6811_Headers
	cpu/m6811/m6811gdc.h
)

# -----------------------------------------------------------------------------

#
# AVR Disassembler
#

set(cpu_avr_Sources
	cpu/avr/avrgdc.cpp
)

set(cpu_avr_Headers
	cpu/avr/avrgdc.h
)

# -----------------------------------------------------------------------------

add_executable(gendasm
	${gendasm_Sources}
	${gendasm_Headers}
	# ----
	${dfc_binary_Sources}
	${dfc_binary_Headers}
	# ----
	${dfc_intel_Sources}
	${dfc_intel_Headers}
	# ----
	${dfc_srec_Sources}
	${dfc_srec_Headers}
	# ----
	${cpu_m6811_Sources}
	${cpu_m6811_Headers}
	# ----
	${cpu_avr_Sources}
	${cpu_avr_Headers}
)

if(FUNC_FLAG_COMMENT)
	target_compile_definitions(gendasm PRIVATE
		FUNC_FLAG_COMMENT
	)
endif()
