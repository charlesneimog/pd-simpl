cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(partialtrack)

message(STATUS "${CMAKE_CURRENT_BINARY_DIR}/gsl-2.8")

set(PDCMAKE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/pd.cmake
    CACHE PATH "Path to pd.cmake")
include(${PDCMAKE_DIR}/pd.cmake)

# ╭──────────────────────────────────────╮
# │                FFTW3                 │
# ╰──────────────────────────────────────╯
set(FFTW3_VERSION "3.3.10")
cmake_policy(SET CMP0135 NEW)
option(BUILD_SHARED_LIBS OFF)
option(BUILD_TESTS OFF)
option(DISABLE_FORTRAN ON)

set(FFTW3_FILE ${CMAKE_BINARY_DIR}/fftw-${FFTW3_VERSION}.tar.gz)

if(NOT EXISTS ${FFTW3_FILE})
    message(STATUS "Downloading FFTW3")
    file(DOWNLOAD https://www.fftw.org/fftw-${FFTW3_VERSION}.tar.gz ${FFTW3_FILE} SHOW_PROGRESS)
endif()

set(FFTW3_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/fftw-3.3.10-build)
file(ARCHIVE_EXTRACT INPUT ${FFTW3_FILE} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)

# Create the binary directory if it doesn't exist

add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/fftw-3.3.10 ${FFTW3_BINARY_DIR} EXCLUDE_FROM_ALL)
set_target_properties(fftw3 PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(fftw3 PROPERTIES POSITION_INDEPENDENT_CODE ON)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/fftw-3.3.10/api)

# ╭──────────────────────────────────────╮
# │                 GSL                  │
# ╰──────────────────────────────────────╯
cmake_policy(SET CMP0075 NEW)
if(EMSCRIPTEN)
    include(ExternalProject)
    set(LIBGSL_PATH ${CMAKE_CURRENT_BINARY_DIR}/gsl-2.8/.libs/libgsl.a)
    if(EXISTS "${LIBGSL_PATH}")
        add_library(gsl STATIC IMPORTED)
        set_target_properties(gsl PROPERTIES IMPORTED_LOCATION ${LIBGSL_PATH})
        include_directories(${CMAKE_CURRENT_BINARY_DIR}/gsl-2.8)
    else()
        set(GSL_FILE ${CMAKE_BINARY_DIR}/gsl.tar.gz)
        if(NOT EXISTS ${GSL_FILE})
            message(STATUS "Downloading GSL")
            file(DOWNLOAD https://mirror.ibcp.fr/pub/gnu/gsl/gsl-latest.tar.gz ${GSL_FILE}
                 SHOW_PROGRESS)
        endif()
        file(ARCHIVE_EXTRACT INPUT ${GSL_FILE} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)
        message(STATUS "${CMAKE_CURRENT_BINARY_DIR}/gsl-2.8")

        message(STATUS "Configuring GSL")
        execute_process(
            COMMAND ${EMCONFIGURE} ./configure --disable-shared --enable-static
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/gsl-2.8"
            RESULT_VARIABLE result_configure
            OUTPUT_VARIABLE output_configure
            ERROR_VARIABLE error_configure
            OUTPUT_STRIP_TRAILING_WHITESPACE
            COMMAND tee output.log)

        if(result_configure)
            message(FATAL_ERROR "Error configuring GSL: ${error_configure}")
        endif()

        message(STATUS "Building GSL")
        execute_process(
            COMMAND ${EMMAKE} make -j4
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/gsl-2.8"
            RESULT_VARIABLE result_configure
            OUTPUT_VARIABLE output_configure
            ERROR_VARIABLE error_configure
            OUTPUT_STRIP_TRAILING_WHITESPACE
            COMMAND tee output.log)

        if(result_configure)
            message(FATAL_ERROR "Error building GSL: ${error_configure}")
        endif()

        message(STATUS "Importing GSL")
        add_library(gsl IMPORTED STATIC GLOBAL)
        set_target_properties(gsl PROPERTIES IMPORTED_LOCATION
                                             ${CMAKE_CURRENT_BINARY_DIR}/gsl-2.8/.libs/libgsl.a)

        include_directories(${CMAKE_CURRENT_BINARY_DIR}/gsl-2.8)
        message(STATUS "${CMAKE_CURRENT_BINARY_DIR}/gsl-2.8")
        add_dependencies(gsl gsl_build)
    endif()

else()
    set(GSL_DISABLE_TESTS ON)
    add_subdirectory(Libraries/agsl EXCLUDE_FROM_ALL)
    set_target_properties(gsl PROPERTIES POSITION_INDEPENDENT_CODE ON)
    set_target_properties(gsl PROPERTIES BUILD_SHARED_LIBS OFF)
    set_target_properties(gsl PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/")
endif()

# ╭──────────────────────────────────────╮
# │             Static Simpl             │
# ╰──────────────────────────────────────╯
add_subdirectory(Libraries/simpl EXCLUDE_FROM_ALL)

set_target_properties(simpl PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(simpl PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(simpl PROPERTIES CXX_STANDARD 11)

if(EMSCRIPTEN)
    include_directories("${CMAKE_CURRENT_BINARY_DIR}/gsl-2.8")
endif()

if(WIN32)
    target_compile_definitions(simpl PRIVATE M_PI=3.14159265358979323846)
    target_compile_definitions(sms PRIVATE random=rand)
endif()

# ╭──────────────────────────────────────╮
# │             Pd External              │
# ╰──────────────────────────────────────╯
# Get all cpp files inside ${CMAKE_SOURCE_DIR}/Sources/ and add them to the project
list(APPEND LIB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Sources/partialtrack.cpp")
list(APPEND LIB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Sources/ptobj.cpp")

pd_add_external(peaks~ "${CMAKE_CURRENT_SOURCE_DIR}/Sources/peaks~.cpp;${LIB_SOURCES}")
pd_add_external(synth~ "${CMAKE_CURRENT_SOURCE_DIR}/Sources/synth~.cpp;${LIB_SOURCES}")
pd_add_external(trans "${CMAKE_CURRENT_SOURCE_DIR}/Sources/trans.cpp;${LIB_SOURCES}")

set_target_properties(peaks_tilde PROPERTIES CXX_STANDARD 11)
set_target_properties(synth_tilde PROPERTIES CXX_STANDARD 11)
set_target_properties(trans PROPERTIES CXX_STANDARD 11)

# ╭──────────────────────────────────────╮
# │         Final Configuration          │
# ╰──────────────────────────────────────╯
file(GLOB HELP_PATCHES "${CMAKE_CURRENT_SOURCE_DIR}/Resources/LibFiles/*.pd")
file(GLOB SOUNDS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/LibFiles/*.wav")

target_link_libraries(peaks_tilde PUBLIC simpl gsl)
target_link_libraries(synth_tilde PUBLIC simpl gsl)
target_link_libraries(trans PUBLIC simpl gsl)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/Libraries/fftw-3.3.10/api")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/Libraries/simpl/src/simpl")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/Libraries/simpl/src/mq")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/Libraries/simpl/src/sms")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/Libraries/simpl/src/loris")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/Libraries/simpl/src/sndobj")
