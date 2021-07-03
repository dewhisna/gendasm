cmake_minimum_required(VERSION 3.19)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

set(GRBL_STAGE_DIR "${CMAKE_CURRENT_BINARY_DIR}/grbl")

configure_file(data/avr/grbl/grbl_v1.1f.20170801/grbl_v1.1f.20170801.hex ${GRBL_STAGE_DIR}/grbl-v-sainsmart3018/grbl_v1.1f.20170801.hex COPYONLY)
configure_file(data/avr/grbl/grbl_v1.1f.20170801/grbl_v1.1f.20170801.ctl ${GRBL_STAGE_DIR}/grbl-v-sainsmart3018/grbl_v1.1f.20170801.ctl COPYONLY)

configure_file(data/avr/grbl/sainsmart-3018pro/SainSmart3018Pro_m328p_flash_original.hex ${GRBL_STAGE_DIR}/grbl-v-sainsmart3018/SainSmart3018Pro_m328p_flash_original.hex COPYONLY)
configure_file(data/avr/grbl/sainsmart-3018pro/SainSmart3018Pro_m328p_flash_original.ctl ${GRBL_STAGE_DIR}/grbl-v-sainsmart3018/SainSmart3018Pro_m328p_flash_original.ctl COPYONLY)

# Disassemble grbl_v1.1f.20170801 firmware:
add_test(NAME "grbl-v-sainsmart3018,dasm_grbl" COMMAND bash -c "$<TARGET_FILE:gendasm> --deterministic avr ${GRBL_STAGE_DIR}/grbl-v-sainsmart3018/grbl_v1.1f.20170801.ctl > grbl_v1.1f.20170801.log 2>&1" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-v-sainsmart3018")
set_property(TEST "grbl-v-sainsmart3018,dasm_grbl" APPEND PROPERTY FIXTURES_REQUIRED grbl_v1.1f.20170801.ctl)
set_property(TEST "grbl-v-sainsmart3018,dasm_grbl" APPEND PROPERTY FIXTURES_REQUIRED grbl_v1.1f.20170801.hex)

# Disassemble SainSmart3018Pro_m328p_flash_original firmware:
add_test(NAME "grbl-v-sainsmart3018,dasm_sainsmart3018" COMMAND bash -c "$<TARGET_FILE:gendasm> --deterministic avr ${GRBL_STAGE_DIR}/grbl-v-sainsmart3018/SainSmart3018Pro_m328p_flash_original.ctl > SainSmart3018Pro_m328p_flash_original.log 2>&1" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-v-sainsmart3018")
set_property(TEST "grbl-v-sainsmart3018,dasm_sainsmart3018" APPEND PROPERTY FIXTURES_REQUIRED SainSmart3018Pro_m328p_flash_original.ctl)
set_property(TEST "grbl-v-sainsmart3018,dasm_sainsmart3018" APPEND PROPERTY FIXTURES_REQUIRED SainSmart3018Pro_m328p_flash_original.hex)

# Check grbl Disassembler Output (must pass the Disassemble test to run these):
add_test(NAME "grbl-v-sainsmart3018,dasm_grbl,log" COMMAND ${CMAKE_COMMAND} -E compare_files "grbl_v1.1f.20170801.log" "${CMAKE_CURRENT_SOURCE_DIR}/ref/avr/grbl/grbl-v-sainsmart3018/grbl_v1.1f.20170801.log" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-v-sainsmart3018")
set_property(TEST "grbl-v-sainsmart3018,dasm_grbl,log" APPEND PROPERTY DEPENDS "grbl-v-sainsmart3018,dasm_grbl")
add_test(NAME "grbl-v-sainsmart3018,dasm_grbl,dis" COMMAND ${CMAKE_COMMAND} -E compare_files "grbl_v1.1f.20170801.dis" "${CMAKE_CURRENT_SOURCE_DIR}/ref/avr/grbl/grbl-v-sainsmart3018/grbl_v1.1f.20170801.dis" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-v-sainsmart3018")
set_property(TEST "grbl-v-sainsmart3018,dasm_grbl,dis" APPEND PROPERTY DEPENDS "grbl-v-sainsmart3018,dasm_grbl")
add_test(NAME "grbl-v-sainsmart3018,dasm_grbl,fnc" COMMAND ${CMAKE_COMMAND} -E compare_files "grbl_v1.1f.20170801.fnc" "${CMAKE_CURRENT_SOURCE_DIR}/ref/avr/grbl/grbl-v-sainsmart3018/grbl_v1.1f.20170801.fnc" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-v-sainsmart3018")
set_property(TEST "grbl-v-sainsmart3018,dasm_grbl,fnc" APPEND PROPERTY DEPENDS "grbl-v-sainsmart3018,dasm_grbl")

# Check SainSmart Disassembler Output (must pass the Disassemble test to run these):
add_test(NAME "grbl-v-sainsmart3018,dasm_sainsmart3018,log" COMMAND ${CMAKE_COMMAND} -E compare_files "SainSmart3018Pro_m328p_flash_original.log" "${CMAKE_CURRENT_SOURCE_DIR}/ref/avr/grbl/grbl-v-sainsmart3018/SainSmart3018Pro_m328p_flash_original.log" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-v-sainsmart3018")
set_property(TEST "grbl-v-sainsmart3018,dasm_sainsmart3018,log" APPEND PROPERTY DEPENDS "grbl-v-sainsmart3018,dasm_sainsmart3018")
add_test(NAME "grbl-v-sainsmart3018,dasm_sainsmart3018,dis" COMMAND ${CMAKE_COMMAND} -E compare_files "SainSmart3018Pro_m328p_flash_original.dis" "${CMAKE_CURRENT_SOURCE_DIR}/ref/avr/grbl/grbl-v-sainsmart3018/SainSmart3018Pro_m328p_flash_original.dis" WORKING_DIRECTORY "${GRBL_STAGE_DIR}/grbl-v-sainsmart3018")
set_property(TEST "grbl-v-sainsmart3018,dasm_sainsmart3018,dis" APPEND PROPERTY DEPENDS "grbl-v-sainsmart3018,dasm_sainsmart3018")
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
