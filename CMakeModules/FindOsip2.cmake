# Try to find the osip2 processing library
# Once done this will define
#
#  OSIP2_FOUND - System has libosip2
#  OSIP2_INCLUDE_DIR - The libosip2 include directory
#  OSIP2_LIBRARIES - The libraries needed to use libosip2

FIND_PATH(OSIP2_INCLUDE_DIR 
   NAMES osip.h
   HINTS 
   PATHS /usr /usr/local
   PATH_SUFFIXES include/osip2
   )

FIND_LIBRARY(OSIP2_LIBRARY
   NAMES osip2 osipparser2
   HINTS 
   PATHS /usr/local	/usr
   PATH_SUFFIXES lib lib64
   )

FIND_LIBRARY(OSIPPARSER2_LIBRARY
   NAMES osipparser2
   HINTS 
   PATHS /usr/local	/usr
   PATH_SUFFIXES lib lib64
   )

LIST(APPEND OSIP2_LIBRARIES ${OSIP2_LIBRARY} ${OSIPPARSER2_LIBRARY})

# handle the QUIETLY and REQUIRED arguments and set OSIP2_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(Osip2 DEFAULT_MSG OSIP2_LIBRARIES OSIP2_INCLUDE_DIR)

MARK_AS_ADVANCED(OSIP2_INCLUDE_DIR OSIP2_LIBRARIES)