cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(partialtrack)

set(PDCMAKE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/pd.cmake
    CACHE PATH "Path to pd.cmake")
include(${PDCMAKE_DIR}/pd.cmake)

# ╭──────────────────────────────────────╮
# │              Libraries               │
# ╰──────────────────────────────────────╯
cmake_policy(SET CMP0075 NEW)
if(EMSCRIPTEN)
    include(ExternalProject)
    ExternalProject_Add(
        gsl_build
        URL https://mirror.ibcp.fr/pub/gnu/gsl/gsl-latest.tar.gz
        CONFIGURE_COMMAND emconfigure ./configure --disable-shared --enable-static
        BUILD_COMMAND emmake make -j8
        INSTALL_COMMAND "" # Skip the installation step
        BUILD_IN_SOURCE true)

    add_library(gsl STATIC IMPORTED IMPORTED_LOCATION
                ${CMAKE_CURRENT_BINARY_DIR}/gsl_build-prefix/src/gsl_build/.libs/libgsl.a)
    add_dependencies(gsl gsl_build)
    include_directories(${CMAKE_CURRENT_BINARY_DIR}/gsl_build-prefix/src/gsl_build)
else()
    set(GSL_DISABLE_TESTS ON)
    add_subdirectory(Libraries/agsl EXCLUDE_FROM_ALL)
    set_target_properties(gsl PROPERTIES POSITION_INDEPENDENT_CODE ON)
    set_target_properties(gsl PROPERTIES BUILD_SHARED_LIBS OFF)
    set_target_properties(gsl PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/")
endif()

# ╭──────────────────────────────────────╮
# │                FFTW3                 │
# ╰──────────────────────────────────────╯
option(BUILD_SHARED_LIBS OFF)
option(BUILD_TESTS OFF)
option(DISABLE_FORTRAN ON)
set(FFTW3_FILE ${CMAKE_CURRENT_BINARY_DIR}/fftw-${FFTW3_VERSION}.tar.gz)

if(NOT EXISTS ${FFTW3_FILE})
    message(STATUS "Downloading FFTW3")
    file(DOWNLOAD https://www.fftw.org/fftw-${FFTW3_VERSION}.tar.gz ${FFTW3_FILE})
endif()

file(ARCHIVE_EXTRACT INPUT ${CMAKE_CURRENT_BINARY_DIR}/fftw-${FFTW3_VERSION}.tar.gz DESTINATION
     ${CMAKE_CURRENT_BINARY_DIR}/)

add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/fftw-3.3.10 EXCLUDE_FROM_ALL)
set_target_properties(fftw3 PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(fftw3 PROPERTIES POSITION_INDEPENDENT_CODE ON)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/fftw-3.3.10/api)

# ╭──────────────────────────────────────╮
# │             Static Simpl             │
# ╰──────────────────────────────────────╯
add_subdirectory(Libraries/simpl EXCLUDE_FROM_ALL)

set_target_properties(simpl PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(simpl PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(WIN32)
    target_compile_definitions(simpl PRIVATE M_PI=3.14159265358979323846)
    target_compile_definitions(sms PRIVATE random=rand)
endif()

# ╭──────────────────────────────────────╮
# │             Pd External              │
# ╰──────────────────────────────────────╯
# Get all cpp files inside ${CMAKE_SOURCE_DIR}/Sources/ and add them to the project
list(APPEND EXTERNAL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Sources/partialtrack.cpp")
list(APPEND EXTERNAL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Sources/peaks~.cpp")
list(APPEND EXTERNAL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Sources/ptobj.cpp")
list(APPEND EXTERNAL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Sources/synth~.cpp")
list(APPEND EXTERNAL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Sources/trans.cpp")

pd_add_external(partialtrack "${EXTERNAL_SOURCES}")

file(GLOB HELP_PATCHES "${CMAKE_CURRENT_SOURCE_DIR}/Resources/LibFiles/*.pd")
file(GLOB SOUNDS "${CMAKE_CURRENT_SOURCE_DIR}/Resources/LibFiles/*.wav")

pd_add_datafile(partialtrack ${HELP_PATCHES})
pd_add_datafile(partialtrack ${SOUNDS})
pd_add_datafile(partialtrack "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

target_link_libraries(partialtrack PUBLIC simpl)

target_include_directories(partialtrack
                           PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/fftw-3.3.10/api")
target_include_directories(partialtrack
                           PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/simpl/src/simpl")
target_include_directories(partialtrack PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/simpl/src/mq")
target_include_directories(partialtrack
                           PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/simpl/src/sms")
target_include_directories(partialtrack
                           PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/simpl/src/loris")
target_include_directories(partialtrack
                           PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/simpl/src/sndobj")

set_target_properties(partialtrack PROPERTIES CXX_STANDARD 11)
