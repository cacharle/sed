test_sources = files(
  'test_parse.c',
  'test_utils.c',
  'test_exec.c',
)
cc = meson.get_compiler('c')
criterion_dep = cc.find_library('criterion', required : true)
gcov_dep = cc.find_library('gcov', required : true)
test_exec = executable(
  'test_criterion',
  sources + test_sources,
  # default_options : ['debug'],
  c_args : ['--coverage'],
  dependencies : [criterion_dep, gcov_dep],
  include_directories : include_dir
)
test('test', test_exec)
