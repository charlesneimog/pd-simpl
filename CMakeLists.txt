# Define your standard CMake header (for example):
cmake_minimum_required(VERSION 3.5)
set(CMAKE_SUPPRESS_REGENERATION true)
include(${CMAKE_CURRENT_SOURCE_DIR}/Resources/pd.build/pd.cmake)
project(pd-simpl)

set_pd_external_path(${CMAKE_CURRENT_SOURCE_DIR}/${project_name})


set(PD_FLOATSIZE64 "OFF")

# Required packages
add_subdirectory(Libraries/simpl)
set_target_properties(simpl PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

# Suppress warnings for the simpl library
if(MSVC)
    target_compile_options(simpl PRIVATE /W0)
else()
    target_compile_options(simpl PRIVATE -w)
endif()

# Get all cpp files inside ${CMAKE_SOURCE_DIR}/Sources/ and add them to the project
set(SOURCES_DIR ${CMAKE_SOURCE_DIR}/Sources)
file(GLOB SOURCES ${SOURCES_DIR}/*.cpp)

# Add one or several externals (5):   
add_pd_external(pd-simpl simpl ${SOURCES})

# Copy all dynamic libraries for local dir
local_deps(simpl)

# Add include directories for pd-simpl project
target_include_directories(pd-simpl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/simpl/src/simpl)
target_include_directories(pd-simpl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/simpl/src/mq)
target_include_directories(pd-simpl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/simpl/src/sms)
target_include_directories(pd-simpl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/simpl/src/loris)
target_include_directories(pd-simpl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/simpl/src/sndobj)

if(APPLE)
    set_target_properties(pd-simpl PROPERTIES CXX_STANDARD 11)
endif()


target_link_libraries(pd-simpl PRIVATE simpl)
