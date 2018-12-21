# config.h
config_h_data = {'pwdb_VERSION': meson.project_version()}
configure_file(configuration: config_h_data, input: 'config.h.in',
  output: '@BASENAME@')

# protobuf
protoc_prog = find_program('protoc')
pwdb_protoc_tgt = custom_target('pwdb_protoc',
  input: ['pwdb.proto'],
  output: ['pwdb.pb.h', 'pwdb.pb.cc'],
  command: [protoc_prog, '--proto_path=' + meson.current_source_dir(),
    '--cpp_out=@OUTDIR@', '@PLAINNAME@'])
pwdb_pb_h = pwdb_protoc_tgt[0]
