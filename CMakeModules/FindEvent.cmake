# Try to find the event processing library
# Once done this will define
#
#  EVENT_FOUND - System has LibEvent
#  EVENT_INCLUDE_DIR - The LibEvent include directory
#  EVENT_LIBRARIES - The libraries needed to use LibEvent

FIND_PATH(EVENT_INCLUDE_DIR 
   NAMES event.h
   HINTS 
   PATHS /usr /usr/local
   PATH_SUFFIXES include
   )

FIND_LIBRARY(EVENT_LIBRARIES 
   NAMES event
   HINTS 
   PATHS /usr/local	/usr
   PATH_SUFFIXES lib lib64
   )

# handle the QUIETLY and REQUIRED arguments and set EVENT_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(Event DEFAULT_MSG EVENT_LIBRARIES EVENT_INCLUDE_DIR)

MARK_AS_ADVANCED(EVENT_INCLUDE_DIR EVENT_LIBRARIES)
