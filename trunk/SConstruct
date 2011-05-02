import os, glob

# maybe use libevent-2.0.x, set libevent_versioin=2
libevent_version = 1
install_dir = '/opt/memlink'
bin      = "memlink"
#defs     = ['DEBUG', 'RANGE_MASK_STR']
#defs     = ['RECV_LOG_BY_PACKAGE', 'DEBUG', "__USE_FILE_OFFSET64", "__USE_LARGEFILE64", "_LARGEFILE_SOURCE", "_LARGEFILE64_SOURCE", "_FILE_OFFSET_BITS=64", 'DEBUGMEM']
defs     = ['RECV_LOG_BY_PACKAGE','DEBUG', "__USE_FILE_OFFSET64", "__USE_LARGEFILE64", "_LARGEFILE_SOURCE", "_LARGEFILE64_SOURCE", "_FILE_OFFSET_BITS=64", "EVENTVER=1"]
includes = ['.']
libpath  = []
libs     = ['m']

#libs     = ['event', 'm', 'tcmalloc_minimal']
cflags   = "-ggdb -pthread -std=gnu99 -Wall -Werror"

env = Environment(CFLAGS=cflags, CPPDEFINES=defs, CPPPATH=includes, LIBPATH=libpath, LIBS=libs)
#files = Glob("*.c")
#files = glob.glob("*.c") + ['/usr/local/lib/libevent.a', '/usr/local/lib/libtcmalloc.a']
files = glob.glob("*.c") + ['/usr/local/lib/libevent.a']
memlink = env.Program(bin, files)

env.Install(os.path.join(install_dir, 'bin'), memlink)
env.Install(os.path.join(install_dir, 'etc'), 'etc/memlink.conf')
#env.StaticLibrary(bin, files)

#SConscript(['client/c/SConstruct', 'unittest/SConstruct', 'test/SConstruct', 'performance/SConstruct'])
