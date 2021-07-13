cmake_minimum_required(VERSION 3.19)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

set(GRBL_STAGE_DIR "${CMAKE_CURRENT_BINARY_DIR}/grbl")

# Copy support binary files - Note: You MUST first run the 'get' scripts to retrieve them
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../../support/ASxxxx/asxv5p40/build-asxv5p40/as6811" "${CMAKE_CURRENT_BINARY_DIR}/as6811" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../../support/ASxxxx/asxv5p40/build-asxv5p40/asavr" "${CMAKE_CURRENT_BINARY_DIR}/asavr" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../../support/ASxxxx/asxv5p40/build-asxv5p40/aslink" "${CMAKE_CURRENT_BINARY_DIR}/aslink" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../../support/avra/build-avra/avra" "${CMAKE_CURRENT_BINARY_DIR}/avra" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../../support/hex2bin/build-hex2bin/hex2bin" "${CMAKE_CURRENT_BINARY_DIR}/hex2bin" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../../support/hex2bin/build-hex2bin/mot2bin" "${CMAKE_CURRENT_BINARY_DIR}/mot2bin" COPYONLY)

# -----------------------------------------------------------------------------

# Configure data for grbl-v-sainsmart3018:
configure_file(data/avr/grbl/grbl_v1.1f.20170801/grbl_v1.1f.20170801.hex ${GRBL_STAGE_DIR}/grbl-v-sainsmart3018/grbl_v1.1f.20170801.hex COPYONLY)
configure_file(data/avr/grbl/sainsmart-3018pro/SainSmart3018Pro_m328p_flash_original.hex ${GRBL_STAGE_DIR}/grbl-v-sainsmart3018/SainSmart3018Pro_m328p_flash_original.hex COPYONLY)
configure_file(data/avr/grbl/grbl-v-sainsmart3018/grbl_v1.1f.20170801.ctl ${GRBL_STAGE_DIR}/grbl-v-sainsmart3018/grbl_v1.1f.20170801.ctl COPYONLY)
configure_file(data/avr/grbl/grbl-v-sainsmart3018/SainSmart3018Pro_m328p_flash_original.ctl ${GRBL_STAGE_DIR}/grbl-v-sainsmart3018/SainSmart3018Pro_m328p_flash_original.ctl COPYONLY)

# Disassemble grbl_v1.1f.20170801 firmware for grbl-v-sainsmart3018:
add_test(NAME "grbl-v-sainsmart3018,dasm_grbl" COMMAND bash -c "$<TARGET_FILE:gendasm> --deterministic avr ${GRBL_STAGE_DIR}/grbl-v-sainsmart3018/grbl_v1.1f.20170801.ctl > grbl_v1.1f.20170801.log 2>&1" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-v-sainsmart3018")
set_property(TEST "grbl-v-sainsmart3018,dasm_grbl" APPEND PROPERTY FIXTURES_REQUIRED grbl_v1.1f.20170801.ctl)
set_property(TEST "grbl-v-sainsmart3018,dasm_grbl" APPEND PROPERTY FIXTURES_REQUIRED grbl_v1.1f.20170801.hex)

# Disassemble SainSmart3018Pro_m328p_flash_original firmware for grbl-v-sainsmart3018:
add_test(NAME "grbl-v-sainsmart3018,dasm_sainsmart3018" COMMAND bash -c "$<TARGET_FILE:gendasm> --deterministic avr ${GRBL_STAGE_DIR}/grbl-v-sainsmart3018/SainSmart3018Pro_m328p_flash_original.ctl > SainSmart3018Pro_m328p_flash_original.log 2>&1" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-v-sainsmart3018")
set_property(TEST "grbl-v-sainsmart3018,dasm_sainsmart3018" APPEND PROPERTY FIXTURES_REQUIRED SainSmart3018Pro_m328p_flash_original.ctl)
set_property(TEST "grbl-v-sainsmart3018,dasm_sainsmart3018" APPEND PROPERTY FIXTURES_REQUIRED SainSmart3018Pro_m328p_flash_original.hex)

# Check grbl Disassembler Output (must pass the Disassemble test to run these):
add_test(NAME "grbl-v-sainsmart3018,dasm_grbl,log" COMMAND ${CMAKE_COMMAND} -E compare_files "grbl_v1.1f.20170801.log" "${CMAKE_CURRENT_SOURCE_DIR}/ref/avr/grbl/grbl-v-sainsmart3018/grbl_v1.1f.20170801.log" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-v-sainsmart3018")
set_property(TEST "grbl-v-sainsmart3018,dasm_grbl,log" APPEND PROPERTY DEPENDS "grbl-v-sainsmart3018,dasm_grbl")
if(FUNC_FLAG_COMMENT)
add_test(NAME "grbl-v-sainsmart3018,dasm_grbl,dis" COMMAND ${CMAKE_COMMAND} -E compare_files "grbl_v1.1f.20170801.dis" "${CMAKE_CURRENT_SOURCE_DIR}/ref/avr/grbl/grbl-v-sainsmart3018/grbl_v1.1f.20170801.dis" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-v-sainsmart3018")
set_property(TEST "grbl-v-sainsmart3018,dasm_grbl,dis" APPEND PROPERTY DEPENDS "grbl-v-sainsmart3018,dasm_grbl")
endif()
add_test(NAME "grbl-v-sainsmart3018,dasm_grbl,fnc" COMMAND ${CMAKE_COMMAND} -E compare_files "grbl_v1.1f.20170801.fnc" "${CMAKE_CURRENT_SOURCE_DIR}/ref/avr/grbl/grbl-v-sainsmart3018/grbl_v1.1f.20170801.fnc" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-v-sainsmart3018")
set_property(TEST "grbl-v-sainsmart3018,dasm_grbl,fnc" APPEND PROPERTY DEPENDS "grbl-v-sainsmart3018,dasm_grbl")

# Check SainSmart Disassembler Output (must pass the Disassemble test to run these):
add_test(NAME "grbl-v-sainsmart3018,dasm_sainsmart3018,log" COMMAND ${CMAKE_COMMAND} -E compare_files "SainSmart3018Pro_m328p_flash_original.log" "${CMAKE_CURRENT_SOURCE_DIR}/ref/avr/grbl/grbl-v-sainsmart3018/SainSmart3018Pro_m328p_flash_original.log" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-v-sainsmart3018")
set_property(TEST "grbl-v-sainsmart3018,dasm_sainsmart3018,log" APPEND PROPERTY DEPENDS "grbl-v-sainsmart3018,dasm_sainsmart3018")
if(FUNC_FLAG_COMMENT)
add_test(NAME "grbl-v-sainsmart3018,dasm_sainsmart3018,dis" COMMAND ${CMAKE_COMMAND} -E compare_files "SainSmart3018Pro_m328p_flash_original.dis" "${CMAKE_CURRENT_SOURCE_DIR}/ref/avr/grbl/grbl-v-sainsmart3018/SainSmart3018Pro_m328p_flash_original.dis" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-v-sainsmart3018")
set_property(TEST "grbl-v-sainsmart3018,dasm_sainsmart3018,dis" APPEND PROPERTY DEPENDS "grbl-v-sainsmart3018,dasm_sainsmart3018")
endif()
add_test(NAME "grbl-v-sainsmart3018,dasm_sainsmart3018,fnc" COMMAND ${CMAKE_COMMAND} -E compare_files "SainSmart3018Pro_m328p_flash_original.fnc" "${CMAKE_CURRENT_SOURCE_DIR}/ref/avr/grbl/grbl-v-sainsmart3018/SainSmart3018Pro_m328p_flash_original.fnc" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-v-sainsmart3018")
set_property(TEST "grbl-v-sainsmart3018,dasm_sainsmart3018,fnc" APPEND PROPERTY DEPENDS "grbl-v-sainsmart3018,dasm_sainsmart3018")

# Run Fuzzy-Function Analysis (must pass the function output test to run these):
add_test(NAME "grbl-v-sainsmart3018,funcanal" COMMAND bash -c "$<TARGET_FILE:funcanal> --deterministic -f -ooa -cn grbl-v-sainsmart3018.cmp -s grbl-v-sainsmart3018.sym -e grbl-v-sainsmart3018.oes -dl 2 -dc grbl-v-sainsmart3018.dro -mo grbl-v-sainsmart3018.mtx grbl_v1.1f.20170801.fnc SainSmart3018Pro_m328p_flash_original.fnc  > grbl-v-sainsmart3018.log 2>&1" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-v-sainsmart3018")
set_property(TEST "grbl-v-sainsmart3018,funcanal" APPEND PROPERTY DEPENDS "grbl-v-sainsmart3018,dasm_grbl,fnc;grbl-v-sainsmart3018,dasm_sainsmart3018,fnc")

# Check Fuzzy-Function Analysis Output (must pass the Fuzzy-Function Analysis test to run these):
add_test(NAME "grbl-v-sainsmart3018,funcanal,log" COMMAND ${CMAKE_COMMAND} -E compare_files "grbl-v-sainsmart3018.log" "${CMAKE_CURRENT_SOURCE_DIR}/ref/avr/grbl/grbl-v-sainsmart3018/grbl-v-sainsmart3018.log" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-v-sainsmart3018")
set_property(TEST "grbl-v-sainsmart3018,funcanal,log" APPEND PROPERTY DEPENDS "grbl-v-sainsmart3018,funcanal")
add_test(NAME "grbl-v-sainsmart3018,funcanal,cmp" COMMAND ${CMAKE_COMMAND} -E compare_files "grbl-v-sainsmart3018.cmp" "${CMAKE_CURRENT_SOURCE_DIR}/ref/avr/grbl/grbl-v-sainsmart3018/grbl-v-sainsmart3018.cmp" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-v-sainsmart3018")
set_property(TEST "grbl-v-sainsmart3018,funcanal,cmp" APPEND PROPERTY DEPENDS "grbl-v-sainsmart3018,funcanal")
add_test(NAME "grbl-v-sainsmart3018,funcanal,dro" COMMAND ${CMAKE_COMMAND} -E compare_files "grbl-v-sainsmart3018.dro" "${CMAKE_CURRENT_SOURCE_DIR}/ref/avr/grbl/grbl-v-sainsmart3018/grbl-v-sainsmart3018.dro" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-v-sainsmart3018")
set_property(TEST "grbl-v-sainsmart3018,funcanal,dro" APPEND PROPERTY DEPENDS "grbl-v-sainsmart3018,funcanal")
add_test(NAME "grbl-v-sainsmart3018,funcanal,mtx" COMMAND ${CMAKE_COMMAND} -E compare_files "grbl-v-sainsmart3018.mtx" "${CMAKE_CURRENT_SOURCE_DIR}/ref/avr/grbl/grbl-v-sainsmart3018/grbl-v-sainsmart3018.mtx" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-v-sainsmart3018")
set_property(TEST "grbl-v-sainsmart3018,funcanal,mtx" APPEND PROPERTY DEPENDS "grbl-v-sainsmart3018,funcanal")
add_test(NAME "grbl-v-sainsmart3018,funcanal,oes" COMMAND ${CMAKE_COMMAND} -E compare_files "grbl-v-sainsmart3018.oes" "${CMAKE_CURRENT_SOURCE_DIR}/ref/avr/grbl/grbl-v-sainsmart3018/grbl-v-sainsmart3018.oes" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-v-sainsmart3018")
set_property(TEST "grbl-v-sainsmart3018,funcanal,oes" APPEND PROPERTY DEPENDS "grbl-v-sainsmart3018,funcanal")
add_test(NAME "grbl-v-sainsmart3018,funcanal,sym" COMMAND ${CMAKE_COMMAND} -E compare_files "grbl-v-sainsmart3018.sym" "${CMAKE_CURRENT_SOURCE_DIR}/ref/avr/grbl/grbl-v-sainsmart3018/grbl-v-sainsmart3018.sym" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-v-sainsmart3018")
set_property(TEST "grbl-v-sainsmart3018,funcanal,sym" APPEND PROPERTY DEPENDS "grbl-v-sainsmart3018,funcanal")

# -----------------------------------------------------------------------------

# Configure data for grbl-roundtrip:
configure_file(data/avr/grbl/grbl_v1.1f.20170801/grbl_v1.1f.20170801.hex ${GRBL_STAGE_DIR}/grbl-roundtrip/grbl_v1.1f.20170801.hex COPYONLY)
configure_file(data/avr/grbl/sainsmart-3018pro/SainSmart3018Pro_m328p_flash_original.hex ${GRBL_STAGE_DIR}/grbl-roundtrip/SainSmart3018Pro_m328p_flash_original.hex COPYONLY)
configure_file(data/avr/grbl/roundtrip/grbl_v1.1f.20170801.ctl ${GRBL_STAGE_DIR}/grbl-roundtrip/grbl_v1.1f.20170801.ctl COPYONLY)
configure_file(data/avr/grbl/roundtrip/SainSmart3018Pro_m328p_flash_original.ctl ${GRBL_STAGE_DIR}/grbl-roundtrip/SainSmart3018Pro_m328p_flash_original.ctl COPYONLY)

# Disassemble grbl_v1.1f.20170801 firmware for grbl-roundtrip:
add_test(NAME "grbl-roundtrip,dasm_grbl" COMMAND bash -c "$<TARGET_FILE:gendasm> --deterministic avr ${GRBL_STAGE_DIR}/grbl-roundtrip/grbl_v1.1f.20170801.ctl > grbl_v1.1f.20170801.log 2>&1" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-roundtrip")
set_property(TEST "grbl-roundtrip,dasm_grbl" APPEND PROPERTY FIXTURES_REQUIRED grbl_v1.1f.20170801.ctl)
set_property(TEST "grbl-roundtrip,dasm_grbl" APPEND PROPERTY FIXTURES_REQUIRED grbl_v1.1f.20170801.hex)

# Disassemble SainSmart3018Pro_m328p_flash_original firmware for grbl-roundtrip:
add_test(NAME "grbl-roundtrip,dasm_sainsmart3018" COMMAND bash -c "$<TARGET_FILE:gendasm> --deterministic avr ${GRBL_STAGE_DIR}/grbl-roundtrip/SainSmart3018Pro_m328p_flash_original.ctl > SainSmart3018Pro_m328p_flash_original.log 2>&1" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-roundtrip")
set_property(TEST "grbl-roundtrip,dasm_sainsmart3018" APPEND PROPERTY FIXTURES_REQUIRED SainSmart3018Pro_m328p_flash_original.ctl)
set_property(TEST "grbl-roundtrip,dasm_sainsmart3018" APPEND PROPERTY FIXTURES_REQUIRED SainSmart3018Pro_m328p_flash_original.hex)

# Reassemble grbl_v1.1f.20170801 firmware for grbl-roundtrip:
add_test(NAME "grbl-roundtrip,asm_grbl" COMMAND bash -c "${CMAKE_CURRENT_BINARY_DIR}/avra grbl_v1.1f.20170801.dis" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-roundtrip")
set_property(TEST "grbl-roundtrip,asm_grbl" APPEND PROPERTY DEPENDS "grbl-roundtrip,dasm_grbl")

# Reassemble SainSmart3018Pro_m328p_flash_original firmware for grbl-roundtrip:
add_test(NAME "grbl-roundtrip,asm_sainsmart3018" COMMAND bash -c "${CMAKE_CURRENT_BINARY_DIR}/avra SainSmart3018Pro_m328p_flash_original.dis" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-roundtrip")
set_property(TEST "grbl-roundtrip,asm_sainsmart3018" APPEND PROPERTY DEPENDS "grbl-roundtrip,dasm_sainsmart3018")

# Convert grbl_v1.1f.20170801 files from hex to bin:
add_test(NAME "grbl-roundtrip,hex2bin_grbl" COMMAND bash -c "${CMAKE_CURRENT_BINARY_DIR}/hex2bin -b grbl_v1.1f.20170801.hex;${CMAKE_CURRENT_BINARY_DIR}/hex2bin -b grbl_v1.1f.20170801.dis.hex" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-roundtrip")
set_property(TEST "grbl-roundtrip,hex2bin_grbl" APPEND PROPERTY DEPENDS "grbl-roundtrip,asm_grbl")

# Convert SainSmart3018Pro_m328p_flash_original files from hex to bin:
add_test(NAME "grbl-roundtrip,hex2bin_sainsmart3018" COMMAND bash -c "${CMAKE_CURRENT_BINARY_DIR}/hex2bin -b SainSmart3018Pro_m328p_flash_original.hex;${CMAKE_CURRENT_BINARY_DIR}/hex2bin -b SainSmart3018Pro_m328p_flash_original.dis.hex" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-roundtrip")
set_property(TEST "grbl-roundtrip,hex2bin_sainsmart3018" APPEND PROPERTY DEPENDS "grbl-roundtrip,asm_sainsmart3018")

# Compare grbl_v1.1f.20170801 files:
add_test(NAME "grbl-roundtrip,diff_grbl" COMMAND ${CMAKE_COMMAND} -E compare_files "grbl_v1.1f.20170801.bin" "grbl_v1.1f.20170801.dis.bin" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-roundtrip")
set_property(TEST "grbl-roundtrip,diff_grbl" APPEND PROPERTY DEPENDS "grbl-roundtrip,hex2bin_grbl")

# Compare SainSmart3018Pro_m328p_flash_original files:
add_test(NAME "grbl-roundtrip,diff_sainsmart3018" COMMAND ${CMAKE_COMMAND} -E compare_files "SainSmart3018Pro_m328p_flash_original.bin" "SainSmart3018Pro_m328p_flash_original.dis.bin" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-roundtrip")
set_property(TEST "grbl-roundtrip,diff_sainsmart3018" APPEND PROPERTY DEPENDS "grbl-roundtrip,hex2bin_sainsmart3018")

# -----------------------------------------------------------------------------

# Configure data for grbl-roundtrip-asavr:
configure_file(data/avr/grbl/grbl_v1.1f.20170801/grbl_v1.1f.20170801.hex ${GRBL_STAGE_DIR}/grbl-roundtrip-asavr/grbl_v1.1f.20170801.hex COPYONLY)
configure_file(data/avr/grbl/sainsmart-3018pro/SainSmart3018Pro_m328p_flash_original.hex ${GRBL_STAGE_DIR}/grbl-roundtrip-asavr/SainSmart3018Pro_m328p_flash_original.hex COPYONLY)
configure_file(data/avr/grbl/roundtrip-asavr/grbl_v1.1f.20170801.ctl ${GRBL_STAGE_DIR}/grbl-roundtrip-asavr/grbl_v1.1f.20170801.ctl COPYONLY)
configure_file(data/avr/grbl/roundtrip-asavr/SainSmart3018Pro_m328p_flash_original.ctl ${GRBL_STAGE_DIR}/grbl-roundtrip-asavr/SainSmart3018Pro_m328p_flash_original.ctl COPYONLY)

# Disassemble grbl_v1.1f.20170801 firmware for grbl-roundtrip-asavr:
add_test(NAME "grbl-roundtrip-asavr,dasm_grbl" COMMAND bash -c "$<TARGET_FILE:gendasm> --deterministic avr ${GRBL_STAGE_DIR}/grbl-roundtrip-asavr/grbl_v1.1f.20170801.ctl > grbl_v1.1f.20170801.log 2>&1" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-roundtrip-asavr")
set_property(TEST "grbl-roundtrip-asavr,dasm_grbl" APPEND PROPERTY FIXTURES_REQUIRED grbl_v1.1f.20170801.ctl)
set_property(TEST "grbl-roundtrip-asavr,dasm_grbl" APPEND PROPERTY FIXTURES_REQUIRED grbl_v1.1f.20170801.hex)

# Disassemble SainSmart3018Pro_m328p_flash_original firmware for grbl-roundtrip-asavr:
add_test(NAME "grbl-roundtrip-asavr,dasm_sainsmart3018" COMMAND bash -c "$<TARGET_FILE:gendasm> --deterministic avr ${GRBL_STAGE_DIR}/grbl-roundtrip-asavr/SainSmart3018Pro_m328p_flash_original.ctl > SainSmart3018Pro_m328p_flash_original.log 2>&1" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-roundtrip-asavr")
set_property(TEST "grbl-roundtrip-asavr,dasm_sainsmart3018" APPEND PROPERTY FIXTURES_REQUIRED SainSmart3018Pro_m328p_flash_original.ctl)
set_property(TEST "grbl-roundtrip-asavr,dasm_sainsmart3018" APPEND PROPERTY FIXTURES_REQUIRED SainSmart3018Pro_m328p_flash_original.hex)

# Compare grbl_v1.1f.20170801.dis for grbl-roundtrip-asavr:
if(FUNC_FLAG_COMMENT)
add_test(NAME "grbl-roundtrip-asavr,diff_grbl,dis" COMMAND ${CMAKE_COMMAND} -E compare_files "grbl_v1.1f.20170801.dis" "${CMAKE_CURRENT_SOURCE_DIR}/ref/avr/grbl/grbl-roundtrip-asavr/grbl_v1.1f.20170801.dis" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-roundtrip-asavr")
set_property(TEST "grbl-roundtrip-asavr,diff_grbl,dis" APPEND PROPERTY DEPENDS "grbl-roundtrip-asavr,dasm_grbl")
endif()

# Compare SainSmart3018Pro_m328p_flash_original.dis for grbl-roundtrip-asavr:
if(FUNC_FLAG_COMMENT)
add_test(NAME "grbl-roundtrip-asavr,diff_sainsmart3018,dis" COMMAND ${CMAKE_COMMAND} -E compare_files "SainSmart3018Pro_m328p_flash_original.dis" "${CMAKE_CURRENT_SOURCE_DIR}/ref/avr/grbl/grbl-roundtrip-asavr/SainSmart3018Pro_m328p_flash_original.dis" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-roundtrip-asavr")
set_property(TEST "grbl-roundtrip-asavr,diff_sainsmart3018,dis" APPEND PROPERTY DEPENDS "grbl-roundtrip-asavr,dasm_sainsmart3018")
endif()

# Reassemble grbl_v1.1f.20170801 firmware for grbl-roundtrip-asavr:
add_test(NAME "grbl-roundtrip-asavr,asm_grbl" COMMAND bash -c "${CMAKE_CURRENT_BINARY_DIR}/asavr -o grbl_v1.1f.20170801.dis;${CMAKE_CURRENT_BINARY_DIR}/aslink -i grbl_v1.1f.20170801.rel" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-roundtrip-asavr")
set_property(TEST "grbl-roundtrip-asavr,asm_grbl" APPEND PROPERTY DEPENDS "grbl-roundtrip-asavr,dasm_grbl")

# Reassemble SainSmart3018Pro_m328p_flash_original firmware for grbl-roundtrip-asavr:
add_test(NAME "grbl-roundtrip-asavr,asm_sainsmart3018" COMMAND bash -c "${CMAKE_CURRENT_BINARY_DIR}/asavr -o SainSmart3018Pro_m328p_flash_original_dis.rel SainSmart3018Pro_m328p_flash_original.dis;${CMAKE_CURRENT_BINARY_DIR}/aslink -i SainSmart3018Pro_m328p_flash_original_dis.rel" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-roundtrip-asavr")
set_property(TEST "grbl-roundtrip-asavr,asm_sainsmart3018" APPEND PROPERTY DEPENDS "grbl-roundtrip-asavr,dasm_sainsmart3018")

# Convert grbl_v1.1f.20170801 files from hex to bin:
#	Note: aslink has a bug that drops two '.ext' off the name ... so grbl_v1.1f.20170801.rel becomes grbl_v1.1f.i86 in the link step above
add_test(NAME "grbl-roundtrip-asavr,hex2bin_grbl" COMMAND bash -c "${CMAKE_CURRENT_BINARY_DIR}/hex2bin -b grbl_v1.1f.20170801.hex;${CMAKE_CURRENT_BINARY_DIR}/hex2bin -b grbl_v1.1f.i86" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-roundtrip-asavr")
set_property(TEST "grbl-roundtrip-asavr,hex2bin_grbl" APPEND PROPERTY DEPENDS "grbl-roundtrip-asavr,asm_grbl")

# Convert SainSmart3018Pro_m328p_flash_original files from hex to bin:
add_test(NAME "grbl-roundtrip-asavr,hex2bin_sainsmart3018" COMMAND bash -c "${CMAKE_CURRENT_BINARY_DIR}/hex2bin -b SainSmart3018Pro_m328p_flash_original.hex;${CMAKE_CURRENT_BINARY_DIR}/hex2bin -b SainSmart3018Pro_m328p_flash_original_dis.i86" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-roundtrip-asavr")
set_property(TEST "grbl-roundtrip-asavr,hex2bin_sainsmart3018" APPEND PROPERTY DEPENDS "grbl-roundtrip-asavr,asm_sainsmart3018")

# Compare grbl_v1.1f.20170801 files:
add_test(NAME "grbl-roundtrip-asavr,diff_grbl,bin" COMMAND ${CMAKE_COMMAND} -E compare_files "grbl_v1.1f.20170801.bin" "grbl_v1.1f.bin" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-roundtrip-asavr")
set_property(TEST "grbl-roundtrip-asavr,diff_grbl,bin" APPEND PROPERTY DEPENDS "grbl-roundtrip-asavr,hex2bin_grbl")

# Compare SainSmart3018Pro_m328p_flash_original files:
add_test(NAME "grbl-roundtrip-asavr,diff_sainsmart3018,bin" COMMAND ${CMAKE_COMMAND} -E compare_files "SainSmart3018Pro_m328p_flash_original.bin" "SainSmart3018Pro_m328p_flash_original_dis.bin" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-roundtrip-asavr")
set_property(TEST "grbl-roundtrip-asavr,diff_sainsmart3018,bin" APPEND PROPERTY DEPENDS "grbl-roundtrip-asavr,hex2bin_sainsmart3018")

# -----------------------------------------------------------------------------

# Configure data for grbl-elf:
configure_file(data/avr/grbl/grbl_v1.1f.20170801_arduino_v1.8.8/grbl_v1.1f.20170801_arduino_v1.8.8.elf ${GRBL_STAGE_DIR}/grbl-elf/grbl_v1.1f.20170801_arduino_v1.8.8.elf COPYONLY)
configure_file(data/avr/grbl/elf/grbl_v1.1f.20170801_arduino_v1.8.8.ctl ${GRBL_STAGE_DIR}/grbl-elf/grbl_v1.1f.20170801_arduino_v1.8.8.ctl COPYONLY)

# Disassemble grbl_v1.1f.20170801_arduino_v1.8.8.elf
add_test(NAME "grbl-elf,dasm_v1.1f.20170801_arduino_v1.8.8" COMMAND bash -c "$<TARGET_FILE:gendasm> --deterministic avr ${GRBL_STAGE_DIR}/grbl-elf/grbl_v1.1f.20170801_arduino_v1.8.8.ctl > grbl_v1.1f.20170801_arduino_v1.8.8.log 2>&1" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-elf")
set_property(TEST "grbl-elf,dasm_v1.1f.20170801_arduino_v1.8.8" APPEND PROPERTY FIXTURES_REQUIRED grbl_v1.1f.20170801_arduino_v1.8.8.ctl)
set_property(TEST "grbl-elf,dasm_v1.1f.20170801_arduino_v1.8.8" APPEND PROPERTY FIXTURES_REQUIRED grbl_v1.1f.20170801_arduino_v1.8.8.elf)

# Reassemble grbl_v1.1f.20170801_arduino_v1.8.8 firmware for grbl-elf:
add_test(NAME "grbl-elf,asm_v1.1f.20170801_arduino_v1.8.8" COMMAND bash -c "${CMAKE_CURRENT_BINARY_DIR}/avra grbl_v1.1f.20170801_arduino_v1.8.8.dis" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-elf")
set_property(TEST "grbl-elf,asm_v1.1f.20170801_arduino_v1.8.8" APPEND PROPERTY DEPENDS "grbl-elf,dasm_v1.1f.20170801_arduino_v1.8.8")

# Convert grbl_v1.1f.20170801_arduino_v1.8.8 reassembly files from hex to bin:
add_test(NAME "grbl-elf,hex2bin_v1.1f.20170801_arduino_v1.8.8" COMMAND bash -c "${CMAKE_CURRENT_BINARY_DIR}/hex2bin -b grbl_v1.1f.20170801_arduino_v1.8.8.dis.hex" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-elf")
set_property(TEST "grbl-elf,hex2bin_v1.1f.20170801_arduino_v1.8.8" APPEND PROPERTY DEPENDS "grbl-elf,asm_v1.1f.20170801_arduino_v1.8.8")

# Compare grbl_v1.1f.20170801_arduino_v1.8.8 firmware files:
add_test(NAME "grbl-elf,diff_v1.1f.20170801_arduino_v1.8.8,bin" COMMAND ${CMAKE_COMMAND} -E compare_files "grbl_v1.1f.20170801_arduino_v1.8.8.dis.bin" "${CMAKE_CURRENT_SOURCE_DIR}/ref/avr/grbl/grbl-elf/grbl_v1.1f.20170801_arduino_v1.8.8.bin" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-elf")
set_property(TEST "grbl-elf,diff_v1.1f.20170801_arduino_v1.8.8,bin" APPEND PROPERTY DEPENDS "grbl-elf,hex2bin_v1.1f.20170801_arduino_v1.8.8")
add_test(NAME "grbl-elf,diff_v1.1f.20170801_arduino_v1.8.8,log" COMMAND ${CMAKE_COMMAND} -E compare_files "grbl_v1.1f.20170801_arduino_v1.8.8.log" "${CMAKE_CURRENT_SOURCE_DIR}/ref/avr/grbl/grbl-elf/grbl_v1.1f.20170801_arduino_v1.8.8.log" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-elf")
set_property(TEST "grbl-elf,diff_v1.1f.20170801_arduino_v1.8.8,log" APPEND PROPERTY DEPENDS "grbl-elf,dasm_v1.1f.20170801_arduino_v1.8.8")
if(FUNC_FLAG_COMMENT)
add_test(NAME "grbl-elf,diff_v1.1f.20170801_arduino_v1.8.8,dis" COMMAND ${CMAKE_COMMAND} -E compare_files "grbl_v1.1f.20170801_arduino_v1.8.8.dis" "${CMAKE_CURRENT_SOURCE_DIR}/ref/avr/grbl/grbl-elf/grbl_v1.1f.20170801_arduino_v1.8.8.dis" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-elf")
set_property(TEST "grbl-elf,diff_v1.1f.20170801_arduino_v1.8.8,dis" APPEND PROPERTY DEPENDS "grbl-elf,dasm_v1.1f.20170801_arduino_v1.8.8")
endif()

# -----------------------------------------------------------------------------

# Configure data for sainsmart3018-v-sainsmartcnc3:
configure_file(data/avr/grbl/sainsmart-3018pro/SainSmart3018Pro_m328p_flash_original.hex ${GRBL_STAGE_DIR}/sainsmart3018-v-sainsmartcnc3/SainSmart3018Pro_m328p_flash_original.hex COPYONLY)
configure_file(data/avr/grbl/sainsmart-grbl1.1f-cnc3/grbl1.1f-cnc3.hex ${GRBL_STAGE_DIR}/sainsmart3018-v-sainsmartcnc3/grbl1.1f-cnc3.hex COPYONLY)
configure_file(data/avr/grbl/sainsmart3018-v-sainsmartcnc3/SainSmart3018Pro_m328p_flash_original.ctl ${GRBL_STAGE_DIR}/sainsmart3018-v-sainsmartcnc3/SainSmart3018Pro_m328p_flash_original.ctl COPYONLY)
configure_file(data/avr/grbl/sainsmart3018-v-sainsmartcnc3/grbl1.1f-cnc3.ctl ${GRBL_STAGE_DIR}/sainsmart3018-v-sainsmartcnc3/grbl1.1f-cnc3.ctl COPYONLY)

# Disassemble SainSmart3018Pro_m328p_flash_original firmware for sainsmart3018-v-sainsmartcnc3:
add_test(NAME "sainsmart3018-v-sainsmartcnc3,dasm_sainsmart3018" COMMAND bash -c "$<TARGET_FILE:gendasm> --deterministic avr ${GRBL_STAGE_DIR}/sainsmart3018-v-sainsmartcnc3/SainSmart3018Pro_m328p_flash_original.ctl > SainSmart3018Pro_m328p_flash_original.log 2>&1" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/sainsmart3018-v-sainsmartcnc3")
set_property(TEST "sainsmart3018-v-sainsmartcnc3,dasm_sainsmart3018" APPEND PROPERTY FIXTURES_REQUIRED SainSmart3018Pro_m328p_flash_original.ctl)
set_property(TEST "sainsmart3018-v-sainsmartcnc3,dasm_sainsmart3018" APPEND PROPERTY FIXTURES_REQUIRED SainSmart3018Pro_m328p_flash_original.hex)

# Disassemble grbl1.1f-cnc3 firmware for sainsmart3018-v-sainsmartcnc3:
add_test(NAME "sainsmart3018-v-sainsmartcnc3,dasm_sainsmartcnc3" COMMAND bash -c "$<TARGET_FILE:gendasm> --deterministic avr ${GRBL_STAGE_DIR}/sainsmart3018-v-sainsmartcnc3/grbl1.1f-cnc3.ctl > grbl1.1f-cnc3.log 2>&1" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/sainsmart3018-v-sainsmartcnc3")
set_property(TEST "sainsmart3018-v-sainsmartcnc3,dasm_sainsmartcnc3" APPEND PROPERTY FIXTURES_REQUIRED grbl1.1f-cnc3.ctl)
set_property(TEST "sainsmart3018-v-sainsmartcnc3,dasm_sainsmartcnc3" APPEND PROPERTY FIXTURES_REQUIRED grbl1.1f-cnc3.hex)

# Check SainSmart Disassembler Output (must pass the Disassemble test to run these):
add_test(NAME "sainsmart3018-v-sainsmartcnc3,dasm_sainsmart3018,log" COMMAND ${CMAKE_COMMAND} -E compare_files "SainSmart3018Pro_m328p_flash_original.log" "${CMAKE_CURRENT_SOURCE_DIR}/ref/avr/grbl/sainsmart3018-v-sainsmartcnc3/SainSmart3018Pro_m328p_flash_original.log" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/sainsmart3018-v-sainsmartcnc3")
set_property(TEST "sainsmart3018-v-sainsmartcnc3,dasm_sainsmart3018,log" APPEND PROPERTY DEPENDS "sainsmart3018-v-sainsmartcnc3,dasm_sainsmart3018")
if(FUNC_FLAG_COMMENT)
add_test(NAME "sainsmart3018-v-sainsmartcnc3,dasm_sainsmart3018,dis" COMMAND ${CMAKE_COMMAND} -E compare_files "SainSmart3018Pro_m328p_flash_original.dis" "${CMAKE_CURRENT_SOURCE_DIR}/ref/avr/grbl/sainsmart3018-v-sainsmartcnc3/SainSmart3018Pro_m328p_flash_original.dis" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/sainsmart3018-v-sainsmartcnc3")
set_property(TEST "sainsmart3018-v-sainsmartcnc3,dasm_sainsmart3018,dis" APPEND PROPERTY DEPENDS "sainsmart3018-v-sainsmartcnc3,dasm_sainsmart3018")
endif()
add_test(NAME "sainsmart3018-v-sainsmartcnc3,dasm_sainsmart3018,fnc" COMMAND ${CMAKE_COMMAND} -E compare_files "SainSmart3018Pro_m328p_flash_original.fnc" "${CMAKE_CURRENT_SOURCE_DIR}/ref/avr/grbl/sainsmart3018-v-sainsmartcnc3/SainSmart3018Pro_m328p_flash_original.fnc" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/sainsmart3018-v-sainsmartcnc3")
set_property(TEST "sainsmart3018-v-sainsmartcnc3,dasm_sainsmart3018,fnc" APPEND PROPERTY DEPENDS "sainsmart3018-v-sainsmartcnc3,dasm_sainsmart3018")

# Check grbl1.1f-cnc3 Disassembler Output (must pass the Disassemble test to run these):
add_test(NAME "sainsmart3018-v-sainsmartcnc3,dasm_sainsmartcnc3,log" COMMAND ${CMAKE_COMMAND} -E compare_files "grbl1.1f-cnc3.log" "${CMAKE_CURRENT_SOURCE_DIR}/ref/avr/grbl/sainsmart3018-v-sainsmartcnc3/grbl1.1f-cnc3.log" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/sainsmart3018-v-sainsmartcnc3")
set_property(TEST "sainsmart3018-v-sainsmartcnc3,dasm_sainsmartcnc3,log" APPEND PROPERTY DEPENDS "sainsmart3018-v-sainsmartcnc3,dasm_sainsmartcnc3")
if(FUNC_FLAG_COMMENT)
add_test(NAME "sainsmart3018-v-sainsmartcnc3,dasm_sainsmartcnc3,dis" COMMAND ${CMAKE_COMMAND} -E compare_files "grbl1.1f-cnc3.dis" "${CMAKE_CURRENT_SOURCE_DIR}/ref/avr/grbl/sainsmart3018-v-sainsmartcnc3/grbl1.1f-cnc3.dis" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/sainsmart3018-v-sainsmartcnc3")
set_property(TEST "sainsmart3018-v-sainsmartcnc3,dasm_sainsmartcnc3,dis" APPEND PROPERTY DEPENDS "sainsmart3018-v-sainsmartcnc3,dasm_sainsmartcnc3")
endif()
add_test(NAME "sainsmart3018-v-sainsmartcnc3,dasm_sainsmartcnc3,fnc" COMMAND ${CMAKE_COMMAND} -E compare_files "grbl1.1f-cnc3.fnc" "${CMAKE_CURRENT_SOURCE_DIR}/ref/avr/grbl/sainsmart3018-v-sainsmartcnc3/grbl1.1f-cnc3.fnc" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/sainsmart3018-v-sainsmartcnc3")
set_property(TEST "sainsmart3018-v-sainsmartcnc3,dasm_sainsmartcnc3,fnc" APPEND PROPERTY DEPENDS "sainsmart3018-v-sainsmartcnc3,dasm_sainsmartcnc3")

# Run Fuzzy-Function Analysis (must pass the function output test to run these):
add_test(NAME "sainsmart3018-v-sainsmartcnc3,funcanal" COMMAND bash -c "$<TARGET_FILE:funcanal> --deterministic -f -ooa -cn sainsmart3018-v-sainsmartcnc3.cmp -s sainsmart3018-v-sainsmartcnc3.sym -e sainsmart3018-v-sainsmartcnc3.oes -dl 1 -dc sainsmart3018-v-sainsmartcnc3.dro -mo sainsmart3018-v-sainsmartcnc3.mtx grbl1.1f-cnc3.fnc SainSmart3018Pro_m328p_flash_original.fnc  > sainsmart3018-v-sainsmartcnc3.log 2>&1" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/sainsmart3018-v-sainsmartcnc3")
set_property(TEST "sainsmart3018-v-sainsmartcnc3,funcanal" APPEND PROPERTY DEPENDS "sainsmart3018-v-sainsmartcnc3,dasm_sainsmartcnc3,fnc;sainsmart3018-v-sainsmartcnc3,dasm_sainsmart3018,fnc")

# Check Fuzzy-Function Analysis Output (must pass the Fuzzy-Function Analysis test to run these):
add_test(NAME "sainsmart3018-v-sainsmartcnc3,funcanal,log" COMMAND ${CMAKE_COMMAND} -E compare_files "sainsmart3018-v-sainsmartcnc3.log" "${CMAKE_CURRENT_SOURCE_DIR}/ref/avr/grbl/sainsmart3018-v-sainsmartcnc3/sainsmart3018-v-sainsmartcnc3.log" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/sainsmart3018-v-sainsmartcnc3")
set_property(TEST "sainsmart3018-v-sainsmartcnc3,funcanal,log" APPEND PROPERTY DEPENDS "sainsmart3018-v-sainsmartcnc3,funcanal")
add_test(NAME "sainsmart3018-v-sainsmartcnc3,funcanal,cmp" COMMAND ${CMAKE_COMMAND} -E compare_files "sainsmart3018-v-sainsmartcnc3.cmp" "${CMAKE_CURRENT_SOURCE_DIR}/ref/avr/grbl/sainsmart3018-v-sainsmartcnc3/sainsmart3018-v-sainsmartcnc3.cmp" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/sainsmart3018-v-sainsmartcnc3")
set_property(TEST "sainsmart3018-v-sainsmartcnc3,funcanal,cmp" APPEND PROPERTY DEPENDS "sainsmart3018-v-sainsmartcnc3,funcanal")
add_test(NAME "sainsmart3018-v-sainsmartcnc3,funcanal,dro" COMMAND ${CMAKE_COMMAND} -E compare_files "sainsmart3018-v-sainsmartcnc3.dro" "${CMAKE_CURRENT_SOURCE_DIR}/ref/avr/grbl/sainsmart3018-v-sainsmartcnc3/sainsmart3018-v-sainsmartcnc3.dro" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/sainsmart3018-v-sainsmartcnc3")
set_property(TEST "sainsmart3018-v-sainsmartcnc3,funcanal,dro" APPEND PROPERTY DEPENDS "sainsmart3018-v-sainsmartcnc3,funcanal")
add_test(NAME "sainsmart3018-v-sainsmartcnc3,funcanal,mtx" COMMAND ${CMAKE_COMMAND} -E compare_files "sainsmart3018-v-sainsmartcnc3.mtx" "${CMAKE_CURRENT_SOURCE_DIR}/ref/avr/grbl/sainsmart3018-v-sainsmartcnc3/sainsmart3018-v-sainsmartcnc3.mtx" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/sainsmart3018-v-sainsmartcnc3")
set_property(TEST "sainsmart3018-v-sainsmartcnc3,funcanal,mtx" APPEND PROPERTY DEPENDS "sainsmart3018-v-sainsmartcnc3,funcanal")
add_test(NAME "sainsmart3018-v-sainsmartcnc3,funcanal,oes" COMMAND ${CMAKE_COMMAND} -E compare_files "sainsmart3018-v-sainsmartcnc3.oes" "${CMAKE_CURRENT_SOURCE_DIR}/ref/avr/grbl/sainsmart3018-v-sainsmartcnc3/sainsmart3018-v-sainsmartcnc3.oes" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/sainsmart3018-v-sainsmartcnc3")
set_property(TEST "sainsmart3018-v-sainsmartcnc3,funcanal,oes" APPEND PROPERTY DEPENDS "sainsmart3018-v-sainsmartcnc3,funcanal")
add_test(NAME "sainsmart3018-v-sainsmartcnc3,funcanal,sym" COMMAND ${CMAKE_COMMAND} -E compare_files "sainsmart3018-v-sainsmartcnc3.sym" "${CMAKE_CURRENT_SOURCE_DIR}/ref/avr/grbl/sainsmart3018-v-sainsmartcnc3/sainsmart3018-v-sainsmartcnc3.sym" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/sainsmart3018-v-sainsmartcnc3")
set_property(TEST "sainsmart3018-v-sainsmartcnc3,funcanal,sym" APPEND PROPERTY DEPENDS "sainsmart3018-v-sainsmartcnc3,funcanal")

# -----------------------------------------------------------------------------

if(NOT FUNC_FLAG_COMMENT)
	message(WARNING "Without FUNC_FLAG_COMMENT support enabled, tests comparing actual disassembler text output can't be performed and will be skipped...")
endif()
