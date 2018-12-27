cc = meson.get_compiler('cpp')
thread_dep = dependency('threads')
gpgme_dep = cc.find_library('gpgme')

gpgh_inc = include_directories('..')
gpgh_lib_deps = [gpgme_dep, thread_dep]
gpgh_lib = library('gpgh++', ['gpg_helper.cc', 'gen_test_key.cc'],
  include_directories: gpgh_inc,
  dependencies: gpgh_lib_deps,
  install: true)
gpgh_dep = declare_dependency(link_with: gpgh_lib,
  include_directories: gpgh_inc,
  dependencies: gpgh_lib_deps)

stdfs_dep = cc.find_library('stdc++fs', required: cc.get_id() == 'gcc')
gpg_test_exe = executable('gpg_test', 'gpg_test.cc',
  dependencies: [gpgh_dep, stdfs_dep])
test('encrypt2file', gpg_test_exe, args: ['encrypt2file'])
test('encrypt2string', gpg_test_exe, args: ['encrypt2string'])
