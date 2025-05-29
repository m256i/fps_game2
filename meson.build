project('CHANGEME', ['c'],
  version : '0.1',
  default_options : ['warning_level=3', 'c_std=c11', 'buildtype=release', 'optimization=3', 'debug=false'])
# To build in release mode, run: meson setup build --buildtype=release

# DO NOT change variable names; AMCA relies on these.
output_name = 'a'
output_dir = '..\\compiled'  # Relative to meson.build location
build_dir_where = 'build'  # Relative to meson.build location

# set to disable some optimizations enable detailed stacktraces, debug logs etc...
DEBUGMODE = true


if DEBUGMODE == true
  # Compiler arguments
  c_compiler_args = [
    '-Wall',
    '-Wextra',
    '-g',
    '-static-libgcc', 
    '-static-libstdc++',
    '-O2',
    '-march=native',
    '-mtune=native',
    '-fomit-frame-pointer',
    '-ffast-math',
    '-DGAME_DEBUG'
  ]
  if host_machine.kernel() == 'nt'
    c_compiler_args += ('-fplugin=' + meson.project_source_root() + '/libftrace.dll')
  else
    c_compiler_args += ['-fsanitize=address', '-fsanitize=undefined']
  endif
else
  c_compiler_args = [
    '-Wall',
    '-Wextra',
    '-static-libgcc', 
    '-static-libstdc++',
    '-O3',
    '-march=native',
    '-mtune=native',
    '-fomit-frame-pointer',
    '-ffast-math',
  ]
endif

# Normalize paths
output_dir = output_dir.replace('/', '\\')
if not output_dir.startswith('\\')
  output_dir = '\\' + output_dir
endif
if not output_dir.endswith('\\') and output_dir != ''
  output_dir = ''.join([output_dir, '\\'])
endif
output_dir = meson.project_build_root() + output_dir

cmake = import('cmake')
fs    = import('fs')

# Dependencies (if any)
dependencies = []
dependencies_str = []
foreach item : dependencies_str
  dependencies += dependency(item)
endforeach

# Use a custom Python globber to list all source files.
source_files = run_command('python', 'globber.py', './', '*.c', check : true).stdout().strip().split('\n')

if host_machine.kernel() == 'nt'
  vulkan_lib = run_command('python', '-c', 'import os; print(os.environ["VULKAN_SDK"])').stdout().strip().replace('\\', '/')
else
  vulkan_lib = run_command('echo', '$VULKAN_SDK/lib').stdout().strip().replace('\\', '/')
endif

# Include directories
if fs.is_dir('./include/')
  headers = include_directories('./include/') 
elif fs.is_dir('./src/include/')
  headers = include_directories('./src/include/') 
else
  headers = []
endif

if host_machine.kernel() == 'nt'
  linker_args = [
    '-static', 
    '-static-libgcc', 
    '-static-libstdc++', 
    '-L' + vulkan_lib,  
    '-lvulkan-1', 
    '-lole32', 
    '-lopengl32', 
    '-ltdh', 
    '-ldxguid', 
    '-ldxgi', 
    '-ldbghelp'
  ]
else
  linker_args = [ 
    '-rdynamic',
    '-static-libgcc', 
    '-static-libstdc++',
    '-L/lib/x86_64-linux-gnu',
    '-lGL',
    '-lX11',
    '-lm',
    '-lXrandr',
    '-fsanitize=address',
    '-fsanitize=undefined',
    '-lasan',
    '-lubsan'
  ]
endif

executable(output_name,
  [source_files],
  dependencies : dependencies,
  include_directories : headers,
  c_args : c_compiler_args,
  link_args : linker_args,
  install : true,
  install_dir : output_dir
)

# Print build context
message('\033[2K\r\nsource files: \n   ', '   '.join(source_files), '\noutputs to:\n   ', output_dir + output_name, '\n')
