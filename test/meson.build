db_json_test_exe = executable('db_json_test', 'db_json_test.cc',
  dependencies: pwdb_lib_dep)
test('db_json_basic', db_json_test_exe, args: ['basic'])

db_test_exe = executable('db_test', 'db_test.cc',
  dependencies: pwdb_lib_dep)
test('db_add_remove', db_test_exe, args: ['add_remove'])
test('db_tags', db_test_exe, args: ['tags'])
