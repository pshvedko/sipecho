#
#
#

include_directories(${OSIP2_INCLUDE_DIR})

protobuf_c_generate_c(PROTO_SIP_SRC PROTO_SIP_HDR
                        message.proto
                        service.proto
                        type.proto )

cmake_compile(ARCHIVE protosip
            LIBRARIES 
              DEFINES HAVE_CONFIG_H _DEFAULT_SOURCE
              SOURCES ${PROTO_SIP_SRC}
                      sip.c )
