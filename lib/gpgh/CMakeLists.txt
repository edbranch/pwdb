get_filename_component(GPGH_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    DIRECTORY)

find_package("Gpgme" REQUIRED)

add_library(gpgh "gpg_helper.cc")
target_link_libraries(gpgh gpgme::gpgme)
target_include_directories(gpgh PUBLIC "${GPGH_INCLUDE_DIR}")

add_executable(gpg_test gpg_test.cc)
target_link_libraries(gpg_test gpgh stdc++fs)
add_test(NAME encrypt2file COMMAND "gpg_test" "encrypt2file")
add_test(NAME encrypt2string COMMAND "gpg_test" "encrypt2string")
