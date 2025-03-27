#
#
#

CMAKE_COMPILE(ARCHIVE mqtt
        LIBRARIES
        DEFINES HAVE_CONFIG_H _DEFAULT_SOURCE
        SOURCES mqtt_pal.c
                mqtt.c
)
