#
#
#

include_directories(${PROTOBUF_C_INCLUDE_DIRS})

add_subdirectory(sip)

cmake_compile(ARCHIVE proto
        LIBRARIES
        DEFINES HAVE_CONFIG_H _DEFAULT_SOURCE
        SOURCES message_xml.c
        message_clone.c
        message_print.c
        transport.c
)
