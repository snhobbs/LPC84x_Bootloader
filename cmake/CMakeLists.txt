cmake_minimum_required(VERSION 3.10.0)

#------------------------------------------------------------------------------------------
# Project Settings
#------------------------------------------------------------------------------------------
set(VERSION_MAJOR 0)
set(VERSION_MINOR 0)
set(VERSION_SUB 1)


project(Bootloader VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_SUB})
set(TargetName "${PROJECT_NAME}_V${VERSION_MAJOR}_${VERSION_MINOR}_${VERSION_SUB}_${CMAKE_BUILD_TYPE}")
message(STATUS "${PROJECT_NAME}")

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR})
set(ProjectDirectory ${CMAKE_SOURCE_DIR}/../bootloader)
set(LibraryDirectory ${ProjectDirectory})
set(ProjectSourceDirectories ${ProjectDirectory}/source ${ProjectDirectory}/firmware-shell/shell/src)
set(ProjectIncludeDirectories ${ProjectDirectory}/source ${ProjectDirectory}/firmware-shell/shell/include)
string(TIMESTAMP build_timestamp UTC)
configure_file(${CMAKE_SOURCE_DIR}/Config.h.in ${ProjectDirectory}/source/Config.h) # add header where version is passed to code

#------------------------------------------------------------------------------------------
# Project Settings
#------------------------------------------------------------------------------------------
add_executable(${TargetName})
set_property(TARGET ${TargetName} PROPERTY CXX_STANDARD 17)
set_property(TARGET ${TargetName} PROPERTY C_STANDARD 11)
set_property(TARGET ${TargetName} PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET ${TargetName} PROPERTY CMAKE_CXX_EXTENSIONS OFF)
set_property(TARGET ${TargetName} PROPERTY C_STANDARD_REQUIRED ON)
set_property(TARGET ${TargetName} PROPERTY CMAKE_C_EXTENSIONS OFF)

target_link_options(
  ${TargetName}
  PUBLIC
  $<$<CONFIG:RELEASE>:-s> # strip all symbols
  $<$<CONFIG:RELEASE>:-Os> #LTO
  -flto                          # Link time optimizations
  -ffat-lto-objects              # Link time optimization in object files
)

add_definitions(
  -DSHELL_PROMPT=""
  -DSHELL_RX_BUFFER_SIZE=2048
)

target_compile_options(
    ${TargetName}
    PUBLIC
    $<$<COMPILE_LANGUAGE:ASM,C,C++>:-fno-builtin>       # Uses libc instead of compiler routines for somethings
    #$<$<CONFIG:RELEASE>:-s> # strip all symbols
    $<$<CONFIG:Release>:-Os>
    $<$<CONFIG:Debug>:-Og>
    $<$<CONFIG:Debug>:-g3>

    -flto                          # Link time optimizations
    -ffat-lto-objects              # Link time optimization in object files

    -Wfatal-errors
    -Wall
    -Wextra
    -pedantic
    -Wpedantic
    #-pedantic-errors
    #-Werror=pedantic

    -Wconversion
    $<$<COMPILE_LANGUAGE:C>:-Wimplicit>
    $<$<COMPILE_LANGUAGE:C>:-Wunused>

    $<$<CXX_COMPILER_ID:GNU>:-Wsuggest-final-types>
    $<$<CXX_COMPILER_ID:GNU>:-Wmaybe-uninitialized>
    $<$<CXX_COMPILER_ID:GNU>:-Wdouble-promotion>
    $<$<CXX_COMPILER_ID:GNU>:-Wnull-dereference>
    -Winit-self

    $<$<CXX_COMPILER_ID:GNU>:-Wduplicated-cond>
    $<$<CXX_COMPILER_ID:GNU>:-Wduplicated-branches>
    $<$<CXX_COMPILER_ID:GNU>:-Wlogical-op>
    $<$<CXX_COMPILER_ID:GNU>:-Wrestrict>
    $<$<C_COMPILER_ID:GNU>:$<$<COMPILE_LANGUAGE:C>:-Wjump-misses-init>>
    $<$<COMPILE_LANGUAGE:CXX>:-Wold-style-cast>
    -Wshadow
)

set(source_excludes mtb.c semihost_hardfault.c uart.cpp)
set(static_analysis_excludes ${source_excludes})


#------------------------------------------------------------
# Include Libraries
#------------------------------------------------------------
find_package(CppUtilities 1.0.1 REQUIRED PATHS ${LibraryDirectory})
target_include_directories(${TargetName} PUBLIC "${CppUtilities_DIR}/include")
list(APPEND static_analysis_excludes ${CppUtilities_DIR})

set(ChipName LPC845)
find_package(LPC84x 1.0.1 REQUIRED PATHS ${LibraryDirectory})
include(${LPC84x_DIR}/cmake/Chip.cmake)
set(DeviceLibrarySource "${LPC84x_DIR}/Chip")
set(VendorCodeBaseDirectory ${DeviceLibrarySource}/VendorSdks/SDK_2.7.0_LPC845)
GetVendorDirectories("${VendorCodeBaseDirectory}")# Sets VendorDirectories
GetVendorSources("${VendorCodeBaseDirectory}") # Sets VendorSources

list(APPEND VendorSources "../LPC845_Bootloader/src/cpp_config.cpp")
#list(APPEND VendorSources "../LPC845_Bootloader/src/startup_lpc845.cpp")
option(ROMDIVIDE "Use ROM Integer divide" ON)
if(ROMDIVIDE)
  add_definitions(
    -D__USE_ROMDIVIDE
  )
  list(APPEND VendorSources "${DeviceLibrarySource}/aeabi_romdiv_patch.s")
  list(APPEND VendorSources "${DeviceLibrarySource}/ROMDivide.cpp")
  message("\n\nUsing ROM Divide\n==================================================\n\n")
endif()


target_include_directories(${TargetName} PUBLIC "${LPC84x_DIR}/Chip")
target_sources(${TargetName} PUBLIC ${VendorSources})
list(APPEND static_analysis_excludes ${LPC84x_DIR})

find_package(HardwareInterfaces 1.0.1 REQUIRED PATHS ${LibraryDirectory})
target_include_directories(${TargetName} PUBLIC "${HardwareInterfaces_DIR}/include")
list(APPEND static_analysis_excludes ${HardwareInterfaces_DIR})

find_package(EmbeddedCMake 1.0.1 REQUIRED PATHS ${LibraryDirectory})
include(${EmbeddedCMake_DIR}/source/ProjectBase.cmake)

function(PinGenerator directory)
  set(files "IOPins.h;IOPins.cpp")
  set(generated_files ${files})
  list(TRANSFORM generated_files PREPEND ${directory}/)
  message(STATUS ${generated_files})
  target_sources(${TargetName} PUBLIC ${generated_files})
  set_source_files_properties(${generated_files} GENERATED)

  set(script "${CMAKE_CURRENT_SOURCE_DIR}/PinGenerator.py")
  add_custom_command(
    COMMENT "Generating Pins"
    WORKING_DIRECTORY ${directory}
    DEPENDS ${script}
    OUTPUT ${generated_files}
    COMMAND python
    ARGS ${script}
  )
endfunction()

function(TaskGenerator directory args)
  set(files "main.cpp")
  set(generated_files ${files})
  list(TRANSFORM generated_files PREPEND "${directory}/")
  message("Generated Files appended" ${generated_files})
  target_sources(${TargetName} PUBLIC ${generated_files})

  set(script "${CMAKE_CURRENT_SOURCE_DIR}/GenerateTaskScheduling.py")
  add_custom_command(
    COMMENT "Generating tasks"
    WORKING_DIRECTORY ${directory}
    DEPENDS ${script}
    OUTPUT ${generated_files}
    COMMAND python
    ARGS ${script}
    ARGS ${args}
  )
endfunction()

#TaskGenerator(${ProjectDirectory}/source "")
PinGenerator(${ProjectDirectory}/source)
