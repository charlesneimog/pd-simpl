# Define your standard CMake header (for example):
cmake_minimum_required(VERSION 3.5)
include(${CMAKE_CURRENT_SOURCE_DIR}/Resources/pd.build/pd.cmake)

project(pd-partialtrack)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG_MODE")

set_pd_external_path(${CMAKE_CURRENT_SOURCE_DIR}/pd-partialtrack)

set(PD_FLOATSIZE64 "OFF")

#╭──────────────────────────────────────╮
#│              Static GSL              │
#╰──────────────────────────────────────╯

add_subdirectory(Libraries/agsl)
set_target_properties(gsl PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set_target_properties(gsl PROPERTIES POSITION_INDEPENDENT_CODE ON)

#╭──────────────────────────────────────╮
#│             Static FFTW3             │
#╰──────────────────────────────────────╯

cmake_policy(SET CMP0135 NEW)
option(BUILD_SHARED_LIBS OFF)
option(BUILD_TESTS OFF)
option(DISABLE_FORTRAN ON)

# check if fftw-3.3.10.tar.gz has been downloaded
set(FFTW3_FILE ${CMAKE_BINARY_DIR}/fftw-3.3.10.tar.gz)
if (NOT EXISTS ${FFTW3_FILE})
    message(STATUS "Downloading FFTW3")
    file(DOWNLOAD https://www.fftw.org/fftw-3.3.10.tar.gz ${FFTW3_FILE})
endif()

file(ARCHIVE_EXTRACT
    INPUT ${CMAKE_BINARY_DIR}/fftw-3.3.10.tar.gz
    DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/
)

add_subdirectory(Libraries/fftw-3.3.10)
set_target_properties(fftw3 PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set_target_properties(fftw3 PROPERTIES POSITION_INDEPENDENT_CODE ON)

#╭──────────────────────────────────────╮
#│             Static Simpl             │
#╰──────────────────────────────────────╯
add_subdirectory(Libraries/simpl)

if (WIN32)
    target_compile_definitions(simpl PRIVATE random=rand srandom=srand)
    target_link_options(simpl PRIVATE
        "-static-libgcc"
        "-static-libstdc++"  
        "-static"
        "-Wl,-allow-multiple-definition"
    )
endif()

set_target_properties(simpl PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set_target_properties(simpl PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(simpl PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/fftw-3.3.10/api")


#╭──────────────────────────────────────╮
#│             Pd External              │
#╰──────────────────────────────────────╯
# Get all cpp files inside ${CMAKE_SOURCE_DIR}/Sources/ and add them to the project
file(GLOB EXTERNAL_SOURCES "${CMAKE_SOURCE_DIR}/Sources/*.cpp")
add_pd_external(pd-partialtrack partialtrack ${EXTERNAL_SOURCES})

if (WIN32)
    target_link_options(pd-partialtrack PRIVATE
			"-Wl,-allow-multiple-definition"
		)
endif()



target_link_libraries(pd-partialtrack PRIVATE simpl)

target_include_directories(pd-partialtrack PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/fftw-3.3.10/api")
target_include_directories(pd-partialtrack PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/simpl/src/simpl")
target_include_directories(pd-partialtrack PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/simpl/src/mq")
target_include_directories(pd-partialtrack PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/simpl/src/sms")
target_include_directories(pd-partialtrack PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/simpl/src/loris")
target_include_directories(pd-partialtrack PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/simpl/src/sndobj")

set_target_properties(pd-partialtrack PROPERTIES CXX_STANDARD 11)
target_compile_options(pd-partialtrack PRIVATE 
                "-O3" 
                "-ffast-math" 
                "-funroll-loops" 
                "-fomit-frame-pointer" 
                "-mfpmath=sse" 
                "-msse" 
                "-msse2" 
                "-msse3" 
                "-DPD"
)
