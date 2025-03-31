#
#
#

include_directories(${LDNS_INCLUDE_DIR})
include_directories(${OSIP2_INCLUDE_DIR})
include_directories(${EVENT_INCLUDE_DIR})
include_directories(${CONFUSE_INCLUDE_DIR})
include_directories(${PROTOBUF_C_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIRS})

cmake_compile(
        PROGRAM sipechod
        LIBRARIES protosip
        proto
        mqtt
        ${LDNS_LIBRARIES}
        ${EVENT_LIBRARIES}
        ${OSIP2_LIBRARIES}
        ${CONFUSE_LIBRARIES}
        ${PROTOBUF_C_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        common
        DEFINES HAVE_CONFIG_H _DEFAULT_SOURCE
        SOURCES app.c aor.c cmd.c dns.c
        ini.c
        log.c
        mem.c
        net.c notify.c
        sip.c
        tcp.c # tls.c
        udp.c
        main.c)
