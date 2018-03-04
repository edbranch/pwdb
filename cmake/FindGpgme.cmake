# Finds the gpgme library
#
# This will define the following variables:
#
#   Gpgme_FOUND         - True if the system has the gpgme library
#   Gpgme_LIBRARIES     - Link libraries usage requirement
#   Gpgme_INCLUDE_DIRS  - Include directories usage requirement
#
# and the following imported targets:
#
#   gpgme::gpgme   - The gpgme library

find_path(Gpgme_INCLUDE_DIRS NAMES "gpgme.h")
find_library(Gpgme_LIBRARIES "gpgme")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args("Gpgme" DEFAULT_MSG
    "Gpgme_LIBRARIES" "Gpgme_INCLUDE_DIRS")

if(Gpgme_FOUND AND NOT TARGET gpgme::gpgme)
    add_library(gpgme::gpgme UNKNOWN IMPORTED)
    set_target_properties(gpgme::gpgme PROPERTIES
        IMPORTED_LOCATION ${Gpgme_LIBRARIES}
        INTERFACE_INCLUDE_DIRECTORIES ${Gpgme_INCLUDE_DIRS})
endif()
