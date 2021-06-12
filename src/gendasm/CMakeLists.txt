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

set(gendasm_Sources
	dfc.cpp				# Data File Converter
	errmsgs.cpp			# Error Message Handler
	gendasm.cpp			# main
	gdc.cpp				# Generic Disassembly Class
	memclass.cpp		# Memory Management Class
)

set(gendasm_Headers
	dfc.h				# Data File Converter
	errmsgs.h			# Error Message Handler
	gdc.h				# Generic Disassembly Class
	memclass.h			# Memory Management Class
	stringhelp.h		# String Helper Functions
)

# -----------------------------------------------------------------------------

add_executable(gendasm
	${gendasm_Sources}
)

