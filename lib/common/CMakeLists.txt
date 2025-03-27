#
#
#

CMAKE_COMPILE(ARCHIVE common
        LIBRARIES
        DEFINES HAVE_CONFIG_H _DEFAULT_SOURCE
        SOURCES base64.c
                map.c
                xml.c
                g711.c
                json.c
                md5.c
                strncasestr.c
                memdup.c
)
