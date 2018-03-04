# Finds the GNU readline library
#
# This will define the following variables:
#
#   Readline_FOUND         - True if the system has the readline library
#   Readline_LIBRARIES     - Link libraries usage requirement
#   Readline_INCLUDE_DIRS  - Include directories usage requirement
#
# and the following imported targets:
#
#   readline::readline   - The readline library

find_path(Readline_INCLUDE_DIRS NAMES "readline.h" PATH_SUFFIXES "readline")
find_library(Readline_LIBRARIES "readline")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args("Readline" DEFAULT_MSG
    "Readline_LIBRARIES" "Readline_INCLUDE_DIRS")

if(Readline_FOUND AND NOT TARGET readline::readline)
    add_library(readline::readline UNKNOWN IMPORTED)
    set_target_properties(readline::readline PROPERTIES
        IMPORTED_LOCATION ${Readline_LIBRARIES}
        INTERFACE_INCLUDE_DIRECTORIES ${Readline_INCLUDE_DIRS})
endif()
