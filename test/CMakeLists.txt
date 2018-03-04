add_executable(db_json_test db_json_test.cc)
target_link_libraries(db_json_test pwdb_comp)
add_test(NAME db_json_basic COMMAND "db_json_test" "basic")

add_executable(db_test db_test.cc)
target_link_libraries(db_test pwdb_comp)
add_test(NAME db_add_remove COMMAND "db_test" "add_remove")
add_test(NAME db_tags COMMAND "db_test" "tags")
