import os
install_dir = '/opt/memlink'
bin      = "memlink"
#defs     = ['DEBUG', 'RANGE_MASK_STR']
defs     = ['DEBUG', "_FILE_OFFSET_BITS=64", "_LARGE_FILE"]
includes = ['/Developer/usr/include', '.']
libpath  = ['/Developer/usr/lib']
libs     = ['event', 'm']
cflags   = "-ggdb -pthread -std=gnu99 -Wall -Werror"

env = Environment(CFLAGS=cflags, CPPDEFINES=defs, CPPPATH=includes, LIBPATH=libpath, LIBS=libs)
files = Glob("*.c")
memlink = env.Program(bin, files)

env.Install(os.path.join(install_dir, 'bin'), memlink)
env.Install(os.path.join(install_dir, 'etc'), 'etc/memlink.conf')
#env.StaticLibrary(bin, files)
