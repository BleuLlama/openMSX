# $Id$
#
# Configuration for Darwin on PPC machines.

# Does platform support symlinks?
USE_SYMLINK:=true

# Default compiler.
OPENMSX_CXX?=g++

# File name extension of executables.
EXEEXT:=

# Probe Overrides
# ===============

MMAP_PREHEADER:=<sys/types.h>
SYS_MMAN_PREHEADER:=<sys/types.h>
SYS_SOCKET_PREHEADER:=<sys/types.h>
# TODO:
# GL_HEADER:=<OpenGL/gl.h> iso GL_CFLAGS is cleaner,
# but we have to modify the build before we can use it.
GL_CFLAGS:=-I/System/Library/Frameworks/OpenGL.framework/Headers
GL_LDFLAGS:=-framework OpenGL -lGL \
	-L/System/Library/Frameworks/OpenGL.framework/Libraries
