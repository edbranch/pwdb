set(PWDB_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include")

find_package("Boost" "1.3.2" REQUIRED COMPONENTS "program_options")
find_package("Curses" REQUIRED)

add_library(pwdb_comp "db.cc" "pwdb_cmd_interp.cc" "util.cc" "db_utils.cc")
target_include_directories(pwdb_comp
    PRIVATE ${Boost_INCLUDE_DIRECTORIES} ${CURSES_INCLUDE_DIRS}
    PUBLIC "${PWDB_INCLUDE_DIR}")
target_link_libraries(pwdb_comp pwdb_proto gpgh cmd_interp
    ${Boost_PROGRAM_OPTIONS_LIBRARY} ${CURSES_LIBRARIES} "stdc++fs")

add_executable(pwdb "pwdb-main.cc" "cl_options.cc")
target_link_libraries(pwdb pwdb_config pwdb_comp)

install(TARGETS pwdb RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
