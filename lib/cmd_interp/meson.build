cc = meson.get_compiler('cpp')
readline_dep = cc.find_library('readline')
boost_po_dep = dependency('boost', modules: ['program_options', 'system'])

cmd_interp_lib_deps = [readline_dep, boost_po_dep]
cmd_interp_inc = include_directories('..')
cmd_interp_lib = library('cmd_interp', 'cmd_interp.cc',
  include_directories: cmd_interp_inc,
  dependencies: cmd_interp_lib_deps,
  install: true)
cmd_interp_dep = declare_dependency(link_with: cmd_interp_lib,
  include_directories: cmd_interp_inc,
  dependencies: cmd_interp_lib_deps)

cmd_interp_test_exe = executable('cmd_interp_test', 'cmd_interp_test.cc',
  dependencies: cmd_interp_dep)
test('cmd_interp_utils', cmd_interp_test_exe, args: ['utils_test'])
test('cmd_interp_interp', cmd_interp_test_exe, args: ['interp_test'])
