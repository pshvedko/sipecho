# Try to find the event processing library
# Once done this will define
#
#  LDNS_FOUND - System has LibEvent
#  LDNS_INCLUDE_DIR - The LibEvent include directory
#  LDNS_LIBRARIES - The libraries needed to use LibEvent

FIND_PATH(LDNS_INCLUDE_DIR 
   NAMES ldns/ldns.h
   HINTS 
   PATHS /usr /usr/local
   PATH_SUFFIXES include
   )

FIND_LIBRARY(LDNS_LIBRARIES 
   NAMES ldns
   HINTS 
   PATHS /usr/local	/usr
   PATH_SUFFIXES lib lib64
   )

# handle the QUIETLY and REQUIRED arguments and set LDNS_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(Ldns DEFAULT_MSG LDNS_LIBRARIES LDNS_INCLUDE_DIR)

MARK_AS_ADVANCED(LDNS_INCLUDE_DIR LDNS_LIBRARIES)
