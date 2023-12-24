project('pwdb', 'cpp',
  version: '0.5.0',
  meson_version: '>=0.49.0',
  license : ['GPL3+', 'LGPL3+'],
  default_options : [
    'cpp_std=c++23',
    'warning_level=1',
    'buildtype=debugoptimized'
])

pwdb_inc = include_directories('include')

subdir('lib')
subdir('include/pwdb')
subdir('src')
subdir('test')
