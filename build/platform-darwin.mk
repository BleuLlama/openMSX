# Configuration for creating a Darwin app folder.
# In practice, this is used for Mac OS X; I'm not sure all of it applies to
# other Darwin-based systems.

# Does platform support symlinks?
USE_SYMLINK:=true

# The app folder will set a hi-res icon, so the openMSX process should not
# replace this with its own low-res icon.
SET_WINDOW_ICON:=false

# Compile for the selected CPU.
ifeq ($(OPENMSX_TARGET_CPU),x86)
TARGET_FLAGS+=-arch i386
else
TARGET_FLAGS+=-arch $(OPENMSX_TARGET_CPU)
endif

# File name extension of executables.
EXEEXT:=
LIBRARYEXT:=.so

# Select the OS X version we want to be compatible with.
# In theory it is possible to compile against an OS X version number lower
# than the SDK version number, but in practice this doesn't seem to work
# since libraries such as libxml2 can change soname between OS X versions.
# Clang as shipped with Xcode requires OS X 10.7 or higher for compiling with
# libc++, when compiling Clang and libc++ from source 10.6 works as well.
OSX_VER:=10.7
TARGET_FLAGS+=-mmacosx-version-min=$(OSX_VER)

# Select Clang as the compiler and libc++ as the standard library.
CXX:=clang++
TARGET_FLAGS+=-stdlib=libc++

# Link against CoreMIDI.
LINK_FLAGS+=-framework CoreMIDI

# additional frameworks for 10.14 build.
LINK_FLAGS+= -framework CoreFoundation -framework ApplicationServices
