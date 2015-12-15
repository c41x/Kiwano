
find_package(PkgConfig)
pkg_check_modules(PC_TAGLIB QUIET taglib)

find_path(TAGLIB_INCLUDE_DIR taglib/taglib.h
  $ENV{PROGRAMFILES}/taglib/include)

find_library(TAGLIB_LIBRARY NAMES libtag PATHS
  $ENV{PROGRAMFILES}/taglib/lib)

set(TAGLIB_LIBRARIES ${TAGLIB_LIBRARY})
set(TAGLIB_INCLUDE_DIRS ${TAGLIB_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(taglib DEFAULT_MSG TAGLIB_LIBRARY TAGLIB_INCLUDE_DIR)
mark_as_advanced(TAGLIB_INCLUDE_DIR TAGLIB_LIBRARY)

if(TAGLIB_FOUND)
  if(NOT Taglib_FIND_QUIETLY AND TAGLIBCONFIG_EXECUTABLE)
    message(STATUS "Found TagLib: ${TAGLIB_LIBRARIES}")
    message(STATUS "Found TagLib: ${TAGLIB_INCLUDE_DIR}")
  endif(NOT Taglib_FIND_QUIETLY AND TAGLIBCONFIG_EXECUTABLE)
else(TAGLIB_FOUND)
  if(Taglib_FIND_REQUIRED)
    message(FATAL_ERROR "Could not find Taglib")
  endif(Taglib_FIND_REQUIRED)
endif(TAGLIB_FOUND)
