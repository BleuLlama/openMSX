# $Id$
#
# Some notes about static linking:
# There are two ways of linking to static library: using the -l command line
# option or specifying the full path to the library file as one of the inputs.
# When using the -l option, the library search paths will be searched for a
# dynamic version of the library, if that is not found, the search paths will
# be searched for a static version of the library. This means we cannot force
# static linking of a library this way. It is possible to force static linking
# of all libraries, but we want to control it per library.
# Conclusion: We have to specify the full path to each library that should be
#             linked statically.
#
# Legend of link modes:
# SYS_DYN: link dynamically against system libs
#          this is the default mode; useful for local binaries
# SYS_STA: link statically against system libs
#          this seems pointless to me; not implemented
# 3RD_DYN: link dynamically against libs from non-system dir
#          might be useful; not implemented
# 3RD_STA: link statically against libs from non-system dir
#          this is how we build our redistributable binaries

class SystemFunction(object):
	name = None

	@classmethod
	def getFunctionName(cls):
		return cls.name

	@classmethod
	def getMakeName(cls):
		return cls.name.upper()

	@classmethod
	def iterHeaders(cls, targetPlatform):
		raise NotImplementedError

class FTruncateFunction(SystemFunction):
	name = 'ftruncate'

	@classmethod
	def iterHeaders(cls, targetPlatform):
		yield '<unistd.h>'

class GetTimeOfDayFunction(SystemFunction):
	name = 'gettimeofday'

	@classmethod
	def iterHeaders(cls, targetPlatform):
		yield '<sys/time.h>'

class MMapFunction(SystemFunction):
	name = 'mmap'

	@classmethod
	def iterHeaders(cls, targetPlatform):
		if targetPlatform in ('darwin', 'openbsd'):
			yield '<sys/types.h>'
		yield '<sys/mman.h>'

class PosixMemAlignFunction(SystemFunction):
	name = 'posix_memalign'

	@classmethod
	def iterHeaders(cls, targetPlatform):
		yield '<stdlib.h>'

class USleepFunction(SystemFunction):
	name = 'usleep'

	@classmethod
	def iterHeaders(cls, targetPlatform):
		yield '<unistd.h>'

# Build a list of system functions using introspection.
def _discoverSystemFunctions(localObjects):
	for obj in localObjects:
		if isinstance(obj, type) and issubclass(obj, SystemFunction):
			if obj is not SystemFunction:
				yield obj
systemFunctions = list(_discoverSystemFunctions(locals().itervalues()))

class Library(object):
	libName = None
	makeName = None
	header = None
	configScriptName = None
	function = None
	dependsOn = ()

	@classmethod
	def getDynamicLibsOption(cls, platform): # pylint: disable-msg=W0613
		return '--libs'

	@classmethod
	def getStaticLibsOption(cls, platform): # pylint: disable-msg=W0613
		return None

	@classmethod
	def isSystemLibrary(cls, platform, linkMode): # pylint: disable-msg=W0613
		if linkMode == 'SYS_DYN':
			return True
		elif linkMode == '3RD_STA':
			return False
		else:
			raise ValueError(linkMode)

	@classmethod
	def getConfigScript( # pylint: disable-msg=W0613
		cls, platform, linkMode, distroRoot
		):
		scriptName = cls.configScriptName
		if scriptName is None:
			return None
		elif distroRoot is None:
			return scriptName
		else:
			return '%s/bin/%s' % (distroRoot, scriptName)

	@classmethod
	def getHeader(cls, platform): # pylint: disable-msg=W0613
		return cls.header

	@classmethod
	def getLibName(cls, platform): # pylint: disable-msg=W0613
		return cls.libName

	@classmethod
	def getCompileFlags(cls, platform, linkMode, distroRoot):
		configScript = cls.getConfigScript(platform, linkMode, distroRoot)
		if configScript is None:
			# TODO: We should allow multiple locations where libraries can be
			#       searched for. For example, MacPorts and Fink are neither
			#       systemwide nor our 3rdparty area.
			if cls.isSystemLibrary(platform, linkMode):
				return ''
			elif distroRoot is None:
				raise ValueError(
					'Library "%s" is not a system library and no alternative '
					'location is available.' % cls.makeName
					)
			else:
				return '-I%s/include' % distroRoot
		else:
			return '`%s --cflags`' % configScript

	@classmethod
	def getLinkFlags(cls, platform, linkMode, distroRoot):
		configScript = cls.getConfigScript(platform, linkMode, distroRoot)
		if configScript is not None:
			libsOption = (
				cls.getDynamicLibsOption(platform)
				if cls.isSystemLibrary(platform, linkMode)
				else cls.getStaticLibsOption(platform)
				)
			if libsOption is not None:
				return '`%s %s`' % (configScript, libsOption)
		if cls.isSystemLibrary(platform, linkMode):
			return '-l%s' % cls.getLibName(platform)
		elif distroRoot is None:
			raise ValueError(
				'Library "%s" is not a system library and no alternative '
				'location is available.' % cls.makeName
				)
		else:
			# TODO: "lib" vs "lib64".
			if linkMode == 'SYS_DYN':
				return '-L%s/lib -l%s' % (distroRoot, cls.getLibName(platform))
			elif linkMode == '3RD_STA':
				return ' '.join(
					[ '%s/lib/lib%s.a' % (
							distroRoot, cls.getLibName(platform)
							) ] +
					[ librariesByName[name].getLinkFlags(
							platform, linkMode, distroRoot
							)
					  for name in cls.dependsOn ]
					)
			else:
				raise ValueError('Invalid link mode "%s"' % linkMode)

	@classmethod
	def getResult(cls, platform, linkMode, distroRoot):
		configScript = cls.getConfigScript(platform, linkMode, distroRoot)
		if configScript is None:
			return 'yes'
		else:
			return '`%s --version`' % configScript

class FreeType(Library):
	libName = 'freetype'
	makeName = 'FREETYPE'
	dependsOn = ('ZLIB', )

class GL(Library):
	libName = 'GL'
	makeName = 'GL'
	function = 'glGenTextures'

	@classmethod
	def isSystemLibrary(cls, platform, linkMode):
		return True

	@classmethod
	def getHeader(cls, platform):
		if platform == 'darwin':
			return '<OpenGL/gl.h>'
		else:
			return '<GL/gl.h>'

	@classmethod
	def getCompileFlags(cls, platform, linkMode, distroRoot):
		if platform in ('netbsd', 'openbsd'):
			return '-I/usr/X11R6/include -I/usr/X11R7/include'
		else:
			return super(GL, cls).getCompileFlags(
				platform, linkMode, distroRoot
				)

	@classmethod
	def getLinkFlags(cls, platform, linkMode, distroRoot):
		if platform == 'darwin':
			return '-framework OpenGL'
		elif platform == 'mingw32':
			return '-lopengl32'
		elif platform in ('netbsd', 'openbsd'):
			return '-L/usr/X11R6/lib -L/usr/X11R7/lib -lGL'
		else:
			return super(GL, cls).getLinkFlags(platform, linkMode, distroRoot)

class GLEW(Library):
	makeName = 'GLEW'
	header = '<GL/glew.h>'
	function = 'glewInit'
	dependsOn = ('GL', )

	@classmethod
	def getLibName(cls, platform):
		if platform == 'mingw32':
			return 'glew32'
		else:
			return 'GLEW'

	@classmethod
	def getCompileFlags(cls, platform, linkMode, distroRoot):
		flags = super(GLEW, cls).getCompileFlags(platform, linkMode, distroRoot)
		if platform == 'mingw32':
			if cls.isSystemLibrary(platform, linkMode):
				return flags
			else:
				return '%s -DGLEW_STATIC' % flags
		else:
			return flags

class JACK(Library):
	libName = 'jack'
	makeName = 'JACK'
	header = '<jack/jack.h>'
	function = 'jack_client_new'

class LibPNG(Library):
	libName = 'png12'
	makeName = 'PNG'
	header = '<png.h>'
	configScriptName = 'libpng-config'
	function = 'png_write_image'
	dependsOn = ('ZLIB', )

	@classmethod
	def getDynamicLibsOption(cls, platform):
		return '--ldflags'

	@classmethod
	def getCompileFlags(cls, platform, linkMode, distroRoot):
		flags = super(LibPNG, cls).getCompileFlags(
			platform, linkMode, distroRoot
			)
		if cls.isSystemLibrary(platform, linkMode):
			return flags
		else:
			# Note: The additional -I is to pick up the zlib headers when zlib
			#       is not installed systemwide.
			return flags + ' -I%s/include' % distroRoot

class SDL(Library):
	libName = 'SDL'
	makeName = 'SDL'
	header = '<SDL.h>'
	configScriptName = 'sdl-config'
	function = 'SDL_Init'

	@classmethod
	def getStaticLibsOption(cls, platform):
		return '--static-libs'

	@classmethod
	def getLinkFlags(cls, platform, linkMode, distroRoot):
		flags = super(SDL, cls).getLinkFlags(platform, linkMode, distroRoot)
		if platform in ('linux', 'gnu'):
			if cls.isSystemLibrary(platform, linkMode):
				return flags
			else:
				# TODO: Fix sdl-config instead.
				return '%s -ldl' % flags
		elif platform == 'mingw32':
			if cls.isSystemLibrary(platform, linkMode):
				return flags.replace('-mwindows', '-mconsole')
			else:
				return ' '.join((
					'%s/lib/libSDL.a' % distroRoot,
					'/mingw/lib/libmingw32.a',
					'%s/lib/libSDLmain.a' % distroRoot,
					'-lwinmm',
					'-lgdi32'
					))
		else:
			return flags

class SDL_image(Library):
	libName = 'SDL_image'
	makeName = 'SDL_IMAGE'
	header = '<SDL_image.h>'
	function = 'IMG_LoadPNG_RW'
	dependsOn = ('SDL', 'PNG')

	@classmethod
	def getCompileFlags(cls, platform, linkMode, distroRoot):
		return SDL.getCompileFlags(platform, linkMode, distroRoot)

class SDL_ttf(Library):
	libName = 'SDL_ttf'
	makeName = 'SDL_TTF'
	header = '<SDL_ttf.h>'
	function = 'TTF_OpenFont'
	dependsOn = ('SDL', 'FREETYPE')

	@classmethod
	def getCompileFlags(cls, platform, linkMode, distroRoot):
		return SDL.getCompileFlags(platform, linkMode, distroRoot)

class TCL(Library):
	libName = 'tcl'
	makeName = 'TCL'
	header = '<tcl.h>'
	function = 'Tcl_CreateInterp'

	@classmethod
	def getConfigScript(cls, platform, linkMode, distroRoot):
		# TODO: Convert (part of) tcl-search.sh to Python as well?
		if cls.isSystemLibrary(platform, linkMode):
			return 'build/tcl-search.sh'
		else:
			return 'TCL_CONFIG_DIR=%s/lib build/tcl-search.sh' % distroRoot

	@classmethod
	def getCompileFlags(cls, platform, linkMode, distroRoot):
		configScript = cls.getConfigScript(platform, linkMode, distroRoot)
		return '`%s --cflags`' % configScript

	@classmethod
	def getLinkFlags(cls, platform, linkMode, distroRoot):
		# Note: Tcl can be compiled with a static or a dynamic library, not
		#       both. So whether this returns static or dynamic link flags
		#       depends on how this copy of Tcl was built.
		# TODO: If we are trying to link statically against a a dynamic
		#       build of Tcl (or vice versa), consider it an error.
		return '`%s %s`' % (
			cls.getConfigScript(platform, linkMode, distroRoot),
			'--ldflags' if cls.isSystemLibrary(platform, linkMode)
				else '--static-libs'
			)

class LibXML2(Library):
	libName = 'xml2'
	makeName = 'XML'
	header = '<libxml/parser.h>'
	configScriptName = 'xml2-config'
	function = 'xmlSAXUserParseFile'
	dependsOn = ('ZLIB', )

	@classmethod
	def getConfigScript(cls, platform, linkMode, distroRoot):
		if platform == 'darwin':
			# Use xml2-config from /usr: ideally we would use xml2-config from
			# the SDK, but the SDK doesn't contain that file. The -isysroot
			# compiler argument makes sure the headers are taken from the SDK
			# though.
			return '/usr/bin/%s' % cls.configScriptName
		else:
			return super(LibXML2, cls).getConfigScript(
				platform, linkMode, distroRoot
				)

	@classmethod
	def getCompileFlags(cls, platform, linkMode, distroRoot):
		flags = super(LibXML2, cls).getCompileFlags(
			platform, linkMode, distroRoot
			)
		if cls.isSystemLibrary(platform, linkMode):
			return flags
		else:
			return flags + ' -DLIBXML_STATIC'

class ZLib(Library):
	libName = 'z'
	makeName = 'ZLIB'
	header = '<zlib.h>'
	function = 'inflate'

# Build a dictionary of libraries using introspection.
def _discoverLibraries(localObjects):
	for obj in localObjects:
		if isinstance(obj, type) and issubclass(obj, Library):
			if not (obj is Library):
				yield obj.makeName, obj
librariesByName = dict(_discoverLibraries(locals().itervalues()))

def resolveDependencies(makeNames):
	'''Returns a list containing the given libraries and all their direct and
	indirect dependencies, in the proper order for static linking.
	For static linking, if lib A depends on B, A must be in the list before B.
	'''
	# Build dependency graph.
	newNames = set(makeNames)
	dependencies = {}
	while newNames:
		name = newNames.pop()
		deps = set(librariesByName[name].dependsOn)
		dependencies[name] = deps
		newNames |= set(dep for dep in deps if dep not in dependencies)

	# Flatten dependency graph.
	# The algorithm is quadratic, but that doesn't matter for the small data
	# sizes we're feeding it.
	orderedDependencies = []
	while dependencies:
		# Find a package that does not depend on anything is not in
		# orderedDependencies yet.
		freePackages = set(
			name
			for name, dependsOn in dependencies.iteritems()
			if not dependsOn
			)
		if not freePackages:
			raise ValueError('Circular dependency between packages')
		orderedDependencies += reversed(sorted(freePackages))
		# Remove dependency just moved to orderedDependencies.
		for name in freePackages:
			del dependencies[name]
		for dependsOn in dependencies.itervalues():
			dependsOn -= freePackages

	# Reverse order and output Make names.
	return reversed(orderedDependencies)
