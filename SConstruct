import os, glob

# maybe use libevent-2.0.x, set libevent_versioin=2
libevent_version = 1
install_dir = '/opt/memlink'
defs     = ['RECV_LOG_BY_PACKAGE', 'DEBUG', "__USE_FILE_OFFSET64", "__USE_LARGEFILE64", "_LARGEFILE_SOURCE", "_LARGEFILE64_SOURCE", "_FILE_OFFSET_BITS=64", '_REENTRANT']
includes = ['.']
libpath  = []

# use gnu malloc
libs     = ['m', 'pthread']

cflags   = "-ggdb -pthread -std=gnu99 -Wall -Werror"
libevent = '/usr/lib/libevent.a'
libtcmalloc = '/usr/local/lib/libtcmalloc_minimal.a'
files = glob.glob("*.c")

if ARGUMENTS.get('static','0') == '1':
	files.append(libevent)
	libs.append('rt')
else:
	libs.insert(0,'event')
if ARGUMENTS.get('tcmalloc','0') == '1':
	if os.path.isfile(libtcmalloc):
		print '====== use google tcmalloc! ======'
		files.append(libtcmalloc)
		libs.append('stdc++')
		defs.append('TCMALLOC')
	else:
		print '====== not find tcmalloc,use gnu malloc! ======'

if 'debug' in  BUILD_TARGETS or 'memlink-debug' in BUILD_TARGETS:
	bin = 'memlink-debug'
	if 'debug' in BUILD_TARGETS:
		BUILD_TARGETS[0] = bin
	defs.append('DEBUGMEM')

	env = Environment(CFLAGS=cflags, CPPDEFINES=defs, CPPPATH=includes, LIBPATH=libpath, LIBS=libs)
	memlink = env.Program(bin, files)
	#env.StaticLibrary(bin, files)
	Default(memlink)
else:
	bin    = "memlink"
	cflags += " -O2"
	env = Environment(CFLAGS=cflags, CPPDEFINES=defs, CPPPATH=includes, LIBPATH=libpath, LIBS=libs)
	memlink = env.Program(bin, files)

	env.Install(os.path.join(install_dir, 'bin'), memlink)
	env.Install(os.path.join(install_dir, 'etc'), 'etc/memlink.conf')
	#env.StaticLibrary(bin, files)

SConscript(['client/c/SConstruct', 'unittest/SConstruct', 'test/SConstruct', 'performance/SConstruct'])


