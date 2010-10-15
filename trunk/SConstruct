bin      = "memlink"
defs     = ['DEBUG']
includes = ['/Developer/usr/include', '.']
libpath  = ['/Developer/usr/lib']
libs     = ['event', 'm']
cflags   = "-ggdb -pthread -std=gnu99 -Wall -Werror -O2"

env = Environment(CFLAGS=cflags, CPPDEFINES=defs, CPPPATH=includes, LIBPATH=libpath, LIBS=libs)
files = Glob("*.c")
env.Program(bin, files)
#env.StaticLibrary(bin, files)


