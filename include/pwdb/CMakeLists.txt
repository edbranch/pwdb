get_filename_component(PWDB_INCLUDE_BIN_DIR "${CMAKE_CURRENT_BINARY_DIR}"
    DIRECTORY)

find_package("Protobuf" REQUIRED)

configure_file("config.h.in" "config.h" @ONLY)
add_library(pwdb_config INTERFACE)
target_include_directories(pwdb_config INTERFACE "${PWDB_INCLUDE_BIN_DIR}")

protobuf_generate_cpp(protobuf_srcs protobuf_hdrs pwdb.proto)
add_library(pwdb_proto ${protobuf_srcs})
target_include_directories(pwdb_proto PUBLIC "${PWDB_INCLUDE_BIN_DIR}")
target_link_libraries(pwdb_proto protobuf::libprotobuf)
