
if (LIBCUE_INCLUDE_DIR AND LIBCUE_LIBRARIES)
    set(libcue_FIND_QUIETLY TRUE)
endif (LIBCUE_INCLUDE_DIR AND LIBCUE_LIBRARIES)

if (NOT WIN32)
    find_package(PkgConfig)
    pkg_check_modules(PC_LIBCUE libcue)
endif (NOT WIN32)

find_path(LIBCUE_INCLUDE_DIR NAMES libcue/libcue.h
    PATHS
    ${PC_LIBCUE_INCLUDEDIR}
    ${PC_LIBCUE_INCLUDE_DIRS}
)

message(STATUS "libcue/includes: ${LIBCUE_INCLUDE_DIR}")

find_library(LIBCUE_LIBRARIES NAMES cue
    PATHS
    ${PC_LIBCUE_LIBDIR}
    ${PC_LIBCUE_LIBRARY_DIRS}
)

message(STATUS "libcue/libraries: ${LIBCUE_LIBRARY}")

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(libcue DEFAULT_MSG LIBCUE_INCLUDE_DIR LIBCUE_LIBRARIES)
mark_as_advanced(LIBCUE_INCLUDE_DIR LIBCUE_LIBRARIES)