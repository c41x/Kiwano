#  GRANITE_FOUND - System has Granite
#  GRANITE_INCLUDE_DIRS - The Granite include directories
#  GRANITE_LIBRARIES - The libraries needed to use Granite

find_package(PkgConfig)
pkg_check_modules(PC_GRANITE QUIET Granite)

find_path(GRANITE_INCLUDE_DIR base/base.hpp
  $ENV{PROGRAMFILES}/granite/include)

find_library(GRANITE_LIBRARY NAMES libbase libsystem base system PATHS
  $ENV{PROGRAMFILES}/granite/lib)

set(GRANITE_LIBRARIES ${GRANITE_LIBRARY})
set(GRANITE_INCLUDE_DIRS ${GRANITE_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Granite DEFAULT_MSG GRANITE_LIBRARY GRANITE_INCLUDE_DIR)
mark_as_advanced(GRANITE_INCLUDE_DIR GRANITE_LIBRARY)