# Try to find the confuse processing library
# Once done this will define
#
#  Confuse_FOUND - System has LibEvent
#  Confuse_INCLUDE_DIR - The LibEvent include directory
#  Confuse_LIBRARIES - The libraries needed to use LibEvent

FIND_PATH(Confuse_INCLUDE_DIR 
   NAMES confuse.h
   HINTS 
   PATHS /usr /usr/local
   PATH_SUFFIXES include
   )

FIND_LIBRARY(Confuse_LIBRARIES 
   NAMES confuse
   HINTS 
   PATHS /usr/local	/usr
   PATH_SUFFIXES lib lib64
   )

# handle the QUIETLY and REQUIRED arguments and set CONFUSE_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(Confuse DEFAULT_MSG Confuse_LIBRARIES Confuse_INCLUDE_DIR)

MARK_AS_ADVANCED(Confuse_INCLUDE_DIR Confuse_LIBRARIES)
