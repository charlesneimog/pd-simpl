set(SDIF_PARENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Libraries)

# Search for the SDIF directory matching the pattern
file(GLOB SDIF_DIRS "${SDIF_PARENT_DIR}/SDIF-*")

# Ensure exactly one directory matches the pattern
list(LENGTH SDIF_DIRS SDIF_COUNT)
if(SDIF_COUNT EQUAL 1)
    list(GET SDIF_DIRS 0 SDIF_DIR)
    message(STATUS "Adding subdirectory: ${SDIF_DIR}")
else()
    message(FATAL_ERROR "Expected exactly one SDIF directory matching the pattern, found ${SDIF_COUNT}")
endif()


set(SDIF_ROOT ${SDIF_DIR}/)
set(SDIF_DIR ${SDIF_DIR}/sdif/)

set(sdif_SOURCES ${SDIF_DIR}/SdifCheck.c  
  ${SDIF_DIR}/SdifConvToText.c  
  ${SDIF_DIR}/SdifErrMess.c 
  ${SDIF_DIR}/SdifFGet.c          
  ${SDIF_DIR}/SdifFile.c        
  ${SDIF_DIR}/SdifFPrint.c
  ${SDIF_DIR}/SdifFPut.c          
  ${SDIF_DIR}/SdifFrame.c       
  ${SDIF_DIR}/SdifFrameType.c
  ${SDIF_DIR}/SdifFRead.c         
  ${SDIF_DIR}/SdifFScan.c       
  ${SDIF_DIR}/SdifFWrite.c
  ${SDIF_DIR}/SdifGlobals.c       
  ${SDIF_DIR}/SdifHard_OS.c     
  ${SDIF_DIR}/SdifHash.c
  ${SDIF_DIR}/SdifHighLevel.c     
  ${SDIF_DIR}/SdifList.c        
  ${SDIF_DIR}/SdifMatrix.c
  ${SDIF_DIR}/SdifMatrixType.c    
  ${SDIF_DIR}/SdifNameValue.c   
  ${SDIF_DIR}/SdifPreTypes.c
  ${SDIF_DIR}/SdifPrint.c         
  ${SDIF_DIR}/SdifRWLowLevel.c  
  ${SDIF_DIR}/SdifSelect.c
  ${SDIF_DIR}/SdifSignatureTab.c  
  ${SDIF_DIR}/SdifStreamID.c    
  ${SDIF_DIR}/SdifString.c
  ${SDIF_DIR}/SdifTest.c          
  ${SDIF_DIR}/SdifTextConv.c    
  ${SDIF_DIR}/SdifTimePosition.c
)

message(STATUS "SDIF_DIR: ${SDIF_DIR}")
add_library(sdif_static STATIC ${sdif_SOURCES} )
set_target_properties(sdif_static PROPERTIES COMPILE_FLAGS "-DSDIF_IS_STATIC")
include_directories(${SDIF_ROOT}/include)
include_directories(${SDIF_DIR}/)

set_target_properties(sdif_static PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set_target_properties(sdif_static PROPERTIES POSITION_INDEPENDENT_CODE ON)

