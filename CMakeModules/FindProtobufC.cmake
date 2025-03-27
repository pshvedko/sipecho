# Locate and configure the Google Protocol Buffers library.
# Defines the following variables:
#
#   PROTOBUF_C_FOUND - Found the Google Protocol Buffers library
#   PROTOBUF_C_INCLUDE_DIRS - Include directories for Google Protocol Buffers
#   PROTOBUF_C_LIBRARIES - The protobuf library
#
# The following cache variables are also defined:
#   PROTOBUF_C_LIBRARY - The protobuf library
#   PROTOBUF_C_PROTOC_LIBRARY   - The protoc library
#   PROTOBUF_C_INCLUDE_DIR - The include directory for protocol buffers
#   PROTOBUF_C_PROTOC_EXECUTABLE - The protoc compiler
#
#  ====================================================================
#  Example:
#
#   find_package(Protobuf REQUIRED)
#   include_directories(${PROTOBUF_C_INCLUDE_DIRS})
#
#   include_directories(${CMAKE_CURRENT_BINARY_DIR})
#   PROTOBUF_C_GENERATE_C(PROTO_SRCS PROTO_HDRS foo.proto)
#   add_executable(bar bar.cc ${PROTO_SRCS} ${PROTO_HDRS})
#   target_link_libraries(bar ${PROTOBUF_C_LIBRARY})
#
# NOTE: You may need to link against pthreads, depending
# on the platform.
#  ====================================================================
#
# PROTOBUF_C_GENERATE_C (public function)
#   SRCS = Variable to define with autogenerated
#          source files
#   HDRS = Variable to define with autogenerated
#          header files
#   ARGN = proto files
#
#  ====================================================================

function(PROTOBUF_C_GENERATE_C SRCS HDRS)
  if(NOT ARGN)
    message(SEND_ERROR "Error: PROTOBUF_C_GENERATE_C() called without any proto files")
    return()
  endif(NOT ARGN)

  set(${SRCS})
  set(${HDRS})
  foreach(FIL ${ARGN})
    get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
    get_filename_component(FIL_WE ${FIL} NAME_WE)
    get_filename_component(FIL_PATH ${FIL} PATH)
    
    list(APPEND ${SRCS} "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb-c.c")
    list(APPEND ${HDRS} "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb-c.h")

    add_custom_command(
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb-c.c"
             "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb-c.h"
      COMMAND ${PROTOBUF_C_PROTOC_EXECUTABLE}
      ARGS --c_out=${CMAKE_CURRENT_BINARY_DIR} --proto_path=${CMAKE_CURRENT_SOURCE_DIR} ${ABS_FIL}
      DEPENDS ${ABS_FIL}
      COMMENT "Running C protocol buffer compiler on ${FIL}"
      VERBATIM )
  endforeach()

  set_source_files_properties(${${SRCS}} ${${HDRS}} PROPERTIES GENERATED TRUE)
  set(${SRCS} ${${SRCS}} PARENT_SCOPE)
  set(${HDRS} ${${HDRS}} PARENT_SCOPE)
endfunction()

find_path(PROTOBUF_C_INCLUDE_DIR google/protobuf-c/protobuf-c.h)

find_library(PROTOBUF_C_LIBRARY NAMES protobuf-c
             DOC "The Google Protocol Buffers Library"
)
find_library(PROTOBUF_C_PROTOC_LIBRARY NAMES protoc-c
             DOC "The Google Protocol Buffers Compiler Library"
)
find_program(PROTOBUF_C_PROTOC_EXECUTABLE NAMES protoc-c
             DOC "The Google Protocol Buffers Compiler"
)

mark_as_advanced(PROTOBUF_C_INCLUDE_DIR
                 PROTOBUF_C_LIBRARY
                 PROTOBUF_C_PROTOC_LIBRARY
                 PROTOBUF_C_PROTOC_EXECUTABLE)

# Restore original find library prefixes
if(WIN32)
    set(CMAKE_FIND_LIBRARY_PREFIXES "${PROTOBUF_C_ORIG_FIND_LIBRARY_PREFIXES}")
endif()

find_package(PackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ProtobufC DEFAULT_MSG
    PROTOBUF_C_LIBRARY PROTOBUF_C_INCLUDE_DIR)

if(ProtobufC_FOUND)
    SET(PROTOBUF_C_FOUND)
    set(PROTOBUF_C_INCLUDE_DIRS ${PROTOBUF_C_INCLUDE_DIR})
    set(PROTOBUF_C_LIBRARIES    ${PROTOBUF_C_LIBRARY})
endif()
