cc = meson.get_compiler('cpp')
gpgme_dep = cc.find_library('gpgme')

gpgh_inc = include_directories('..')
gpgh_lib = library('gpgh++', 'gpg_helper.cc',
  include_directories: gpgh_inc,
  dependencies: gpgme_dep,
  install: true)
gpgh_dep = declare_dependency(link_with: gpgh_lib,
  include_directories: gpgh_inc,
  dependencies: gpgme_dep)

stdfs_dep = cc.find_library('stdc++fs', required: cc.get_id() == 'gcc')
gpg_test_exe = executable('gpg_test', 'gpg_test.cc',
  dependencies: [gpgh_dep, stdfs_dep])
test('encrypt2file', gpg_test_exe, args: ['encrypt2file'])
test('encrypt2string', gpg_test_exe, args: ['encrypt2string'])
