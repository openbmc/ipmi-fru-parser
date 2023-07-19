project(
  'ipmi-fru-parser',
  'cpp',
  version: '1.0',
  default_options: [
    'buildtype=debugoptimized',
    'cpp_std=c++23',
    'warning_level=3',
    'werror=true',
  ],
  meson_version: '>=1.1.1',
)

cxx = meson.get_compiler('cpp')

phosphor_logging_dep = dependency('phosphor-logging')
sdbusplus_dep = dependency('sdbusplus')
ipmid_dep = dependency('libipmid')

if cxx.has_header('CLI/CLI.hpp')
    CLI11_dep = declare_dependency()
else
    CLI11_dep = dependency('CLI11')
endif

python_prog = find_program('python3', native: true)

fru_gen = custom_target(
  'fru-gen.cpp'.underscorify(),
  input: [
    'scripts/fru_gen.py',
    get_option('fru_yaml'),
  ],
  output: 'fru-gen.cpp',
  command: [
    python_prog, '@INPUT0@',
    '-i', '@INPUT1@',
    '-o', meson.current_build_dir(),
    'generate-cpp',
  ],
)

properties_gen = custom_target(
  'extra-properties-gen.cpp'.underscorify(),
  input: [
    'scripts/extra-properties.py',
    get_option('properties_yaml'),
  ],
  output: 'extra-properties-gen.cpp',
  command: [
    python_prog, '@INPUT0@',
    '-e', '@INPUT1@',
  ],
)

writefrudata_lib = library(
  'writefrudata',
  fru_gen,
  properties_gen,
  'fru_area.cpp',
  'frup.cpp',
  'writefrudata.cpp',
  dependencies: [
    sdbusplus_dep,
    phosphor_logging_dep,
    ipmid_dep,
  ],
  version: meson.project_version(),
  install: true,
)

writefrudata_dep = declare_dependency(
  link_with: writefrudata_lib,
)

strgfnhandler_lib = library(
  'strgfnhandler',
  'strgfnhandler.cpp',
  dependencies: [
    writefrudata_dep,
    phosphor_logging_dep,
    ipmid_dep,
  ],
  override_options: [ 'b_lundef=false' ],
  version: meson.project_version(),
  install: true,
)

executable(
  'phosphor-read-eeprom',
  'readeeprom.cpp',
  dependencies: [
    CLI11_dep,
    phosphor_logging_dep,
    sdbusplus_dep,
    writefrudata_dep,
  ],
  install: true,
)
