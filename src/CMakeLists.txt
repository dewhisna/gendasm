#
#	Generic Code-Seeking Disassembler
#	Copyright(c)2021 by Donna Whisnant
#

cmake_minimum_required(VERSION 3.19)

project(gendasm LANGUAGES CXX VERSION 1.0 DESCRIPTION "Generic Code-Seeking Disassembler")

# -----------------------------------------------------------------------------

option(ENABLE_TESTING "Enable Testing (Requires first running the 'get' scripts in /support to bootstrap 3rd party tools)" OFF)

# -----------------------------------------------------------------------------

add_subdirectory(gendasm)
add_subdirectory(funcanal)

if(ENABLE_TESTING)
	enable_testing()
	add_subdirectory(test)
endif()
