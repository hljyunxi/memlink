import os
install_dir = '/opt/memlink'
bin      = "memlink"
#defs     = ['DEBUG', 'RANGE_MASK_STR']
#defs     = ['RECV_LOG_BY_PACKAGE', 'DEBUG', "__USE_FILE_OFFSET64", "__USE_LARGEFILE64", "_LARGEFILE_SOURCE", "_LARGEFILE64_SOURCE", "_FILE_OFFSET_BITS=64", 'DEBUGMEM']
defs     = ['RECV_LOG_BY_PACKAGE','DEBUG', "__USE_FILE_OFFSET64", "__USE_LARGEFILE64", "_LARGEFILE_SOURCE", "_LARGEFILE64_SOURCE", "_FILE_OFFSET_BITS=64"]
includes = ['/Developer/usr/include', '.']
libpath  = ['/Developer/usr/lib']
libs     = ['event', 'm']
#libs     = ['event', 'm', 'tcmalloc_minimal']
cflags   = "-ggdb -pthread -std=gnu99 -Wall -Werror -O2"

env = Environment(CFLAGS=cflags, CPPDEFINES=defs, CPPPATH=includes, LIBPATH=libpath, LIBS=libs)
files = Glob("*.c")
memlink = env.Program(bin, files)

env.Install(os.path.join(install_dir, 'bin'), memlink)
env.Install(os.path.join(install_dir, 'etc'), 'etc/memlink.conf')
#env.StaticLibrary(bin, files)
