project('ftrace_plugin', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'buildtype=release', 'optimization=3'])

GCC_VER = run_command('gcc', '-dumpfullversion').stdout().strip()
MINGW_CRT = 'ucrt64'

gcc_loc = run_command('where', 'gcc').stdout().strip()
parts = gcc_loc.split(MINGW_CRT) 
if parts.length() > 0
  mingw_root = parts[0] + '\\' + MINGW_CRT + '\\'
else
  error('Could not locate version ' + GCC_VER + ' ' + MINGW_CRT + 'in gcc path: ' + gcc_loc)
endif

shared_library('ftrace',
           'strace_entry.cpp',
           include_directories: [
            include_directories(mingw_root + 'lib\\gcc\\x86_64-w64-mingw32\\' + GCC_VER + '\\plugin\\include')],
           cpp_args: ['-shared','-fno-rtti', '-fPIC', '-static-libgcc','-static-libstdc++', '-fpermissive'],
           link_args: [
            '-shared',
            '-lgcc',
            '-lstdc++',
            '-O2',
            '-fomit-frame-pointer',
            '-L'+ mingw_root + 'lib\\gcc\\x86_64-w64-mingw32\\' + GCC_VER + '\\plugin',
            '-DSTRACE_C',
            '-l:cc1.exe.a'],
           install : true)

shared_library('ftracepp',
           'strace_entry.cpp',
           include_directories: [
            include_directories(mingw_root + 'lib\\gcc\\x86_64-w64-mingw32\\' + GCC_VER + '\\plugin\\include')],
           cpp_args: ['-shared','-fno-rtti', '-fPIC', '-static-libgcc','-static-libstdc++', '-fpermissive'],
           link_args: [
            '-shared',
            '-lgcc',
            '-lstdc++',
            '-O2',
            '-fomit-frame-pointer',
            '-L'+ mingw_root + 'lib\\gcc\\x86_64-w64-mingw32\\' + GCC_VER + '\\plugin',
            '-DSTRACE_CPP',
            '-l:cc1plus.exe.a'],
           install : true)