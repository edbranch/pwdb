project('pwdb', 'cpp',
  version: '0.4.1',
  meson_version: '>=0.49.0',
  license : ['GPL3+', 'LGPL3+'],
  default_options : [
    'cpp_std=c++17',
    'warning_level=1',
    'buildtype=debugoptimized'
])

pwdb_inc = include_directories('include')

subdir('lib')
subdir('include/pwdb')
subdir('src')
subdir('test')
