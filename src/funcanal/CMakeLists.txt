#
#	Fuzzy Function Analyzer
#	for the Generic Code-Seeking Disassembler
#	Copyright(c)2021 by Donna Whisnant
#

cmake_minimum_required(VERSION 3.19)

project(funcanal LANGUAGES CXX VERSION 1.0 DESCRIPTION "Fuzzy Function Analyzer")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -----------------------------------------------------------------------------

set(funcanal_Sources
	funcanal.cpp		# Main Function Analyzer
	funccomp.cpp		# Function Comparison
	funcdesc.cpp		# Function Descriptors
)

set(funcanal_Headers
	funcanal.h			# Main Function Analyzer
	funccomp.h			# Function Comparison
	funcdesc.h			# Function Descriptors
)

# -----------------------------------------------------------------------------

# Code Borrowed from main gendasm:

set(gendasm_Sources
	../gendasm/errmsgs.cpp		# Error Message Handler
	../gendasm/memclass.cpp		# Memory Management Class
)

set(gendasm_Headers
	../gendasm/enumhelp.h		# Enum Helper Functions/Defines
	../gendasm/errmsgs.h		# Error Message Handler
	../gendasm/gdc.h			# Generic Disassembly Class
	../gendasm/memclass.h		# Memory Management Class
	../gendasm/stringhelp.h		# String Helper Functions
)

# -----------------------------------------------------------------------------

add_executable(funcanal
	${funcanal_Sources}
	${funcanal_Headers}
	${gendasm_Sources}
	${gendasm_Headers}
)

target_include_directories(funcanal
	PRIVATE ../gendasm/
)
