# Dependencies
cc = meson.get_compiler('cpp')
stdfs_dep = cc.find_library('stdc++fs', required: cc.get_id() == 'gcc')
curses_dep = cc.find_library('curses')
boost_po_dep = dependency('boost', modules: ['program_options', 'system'])
protobuf_dep = dependency('protobuf')
# NOTE: protobuf files compile ok but are horribly broken without thread
#   support enabled!
thread_dep = dependency('threads')

# pwdb library - Any code needed by both unit tests and the pwdb application
#   must be in this library.
pwdb_lib_deps = [gpgh_dep, cmd_interp_dep, protobuf_dep, curses_dep, stdfs_dep,
  thread_dep]
pwdb_lib = library('pwdb',
  ['db.cc', 'pwdb_cmd_interp.cc', 'db_utils.cc', 'util.cc', pwdb_protoc_tgt],
  dependencies: pwdb_lib_deps,
  include_directories: pwdb_inc,
  install: true,
  install_rpath: get_option('install_rpath_set') ?
    get_option('install_rpath_lib') : '')
pwdb_lib_dep = declare_dependency(link_with: pwdb_lib,
  dependencies: pwdb_lib_deps,
  include_directories: pwdb_inc,
  sources: pwdb_pb_h)

# pwdb application
executable('pwdb', ['pwdb-main.cc', 'cl_options.cc'],
  dependencies: [pwdb_lib_dep, boost_po_dep],
  install: true,
  install_rpath: get_option('install_rpath_set') ?
    get_option('install_rpath_exe') : '')
