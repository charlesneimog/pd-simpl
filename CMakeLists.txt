cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(partialtrack)

set(PDCMAKE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/pd.cmake
    CACHE PATH "Path to pd.cmake")
include(${PDCMAKE_DIR}/pd.cmake)

# ╭──────────────────────────────────────╮
# │              Libraries               │
# ╰──────────────────────────────────────╯

set(GSL_DISABLE_TESTS ON)
add_subdirectory(Libraries/agsl)
set_target_properties(gsl PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                     ${CMAKE_SOURCE_DIR})
set_target_properties(gsl PROPERTIES POSITION_INDEPENDENT_CODE ON)

include(
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/CMakeAudioHelpers/CMake/fftw3.cmake)
include(
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/CMakeAudioHelpers/CMake/sdif.cmake)

# ╭──────────────────────────────────────╮
# │             Static Simpl             │
# ╰──────────────────────────────────────╯
add_subdirectory(Libraries/simpl)

set_target_properties(simpl PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                       ${CMAKE_SOURCE_DIR})
set_target_properties(simpl PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(WIN32)
  target_compile_definitions(simpl PRIVATE M_PI=3.14159265358979323846)
  target_compile_definitions(sms PRIVATE random=rand)
endif()

# ╭──────────────────────────────────────╮
# │             Pd External              │
# ╰──────────────────────────────────────╯
# Get all cpp files inside ${CMAKE_SOURCE_DIR}/Sources/ and add them to the
# project
file(GLOB EXTERNAL_SOURCES "${CMAKE_SOURCE_DIR}/Sources/*.cpp")
pd_add_external(partialtrack ${EXTERNAL_SOURCES})

file(GLOB HELP_PATCHES "${CMAKE_SOURCE_DIR}/Resources/LibFiles/*.pd")
file(GLOB SOUNDS "${CMAKE_SOURCE_DIR}/Resources/LibFiles/*.wav")

pd_add_datafile(partialtrack ${HELP_PATCHES})
pd_add_datafile(partialtrack ${SOUNDS})
pd_add_datafile(partialtrack "${CMAKE_SOURCE_DIR}/README.md")

# if (WIN32) target_link_options(partialtrack PUBLIC
# "-Wl,-allow-multiple-definition") endif()

target_link_libraries(partialtrack PUBLIC simpl sdif_static)

target_include_directories(
  partialtrack PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/fftw-3.3.10/api")
target_include_directories(
  partialtrack PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/simpl/src/simpl")
target_include_directories(
  partialtrack PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/simpl/src/mq")
target_include_directories(
  partialtrack PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/simpl/src/sms")
target_include_directories(
  partialtrack PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/simpl/src/loris")
target_include_directories(
  partialtrack PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/simpl/src/sndobj")

set_target_properties(partialtrack PROPERTIES CXX_STANDARD 17)
