# Define your standard CMake header (for example):
cmake_minimum_required(VERSION 3.5)
set(CMAKE_SUPPRESS_REGENERATION true)
include(${CMAKE_CURRENT_SOURCE_DIR}/pd.build/pd.cmake)
project(pd-simpl)

set_pd_external_path(${CMAKE_CURRENT_SOURCE_DIR}/${project_name})
set(PD_FLOATSIZE64 "OFF")

# Required packages
add_subdirectory(Libraries/simpl)

# Add one or several externals (5):   
add_pd_external(pd-simpl simpl 
    ${CMAKE_CURRENT_SOURCE_DIR}/Sources/pd-simpl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Sources/s-peaks~.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Sources/s-partials.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Sources/s-synth~.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Sources/s-get.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Sources/s-create.cpp
)


# Add include directories for pd-simpl project
target_include_directories(pd-simpl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/simpl/src/simpl)
target_include_directories(pd-simpl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/simpl/src/mq)
target_include_directories(pd-simpl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/simpl/src/sms)
target_include_directories(pd-simpl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/simpl/src/loris)
target_include_directories(pd-simpl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/simpl/src/sndobj)

target_link_libraries(pd-simpl PRIVATE simpl)

