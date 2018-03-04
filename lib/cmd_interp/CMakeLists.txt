get_filename_component(CMD_INTERP_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    DIRECTORY)

find_package("Readline" REQUIRED)
find_package("Boost" "1.3.2" REQUIRED COMPONENTS "program_options")

add_library(cmd_interp "cmd_interp.cc")
target_include_directories(cmd_interp PRIVATE ${Boost_INCLUDE_DIRECTORIES}
    PUBLIC "${CMD_INTERP_INCLUDE_DIR}")
target_link_libraries(cmd_interp readline::readline
    ${Boost_PROGRAM_OPTIONS_LIBRARY})

add_executable(cmd_interp_test cmd_interp_test.cc)
target_link_libraries(cmd_interp_test cmd_interp)

add_test(NAME cmd_interp_utils COMMAND "cmd_interp_test" "utils_test")
add_test(NAME cmd_interp_interp COMMAND "cmd_interp_test" "interp_test")
