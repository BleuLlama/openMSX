#include "LocalFileReference.hh"
#include "File.hh"
#include "Filename.hh"
#include "FileOperations.hh"
#include "FileException.hh"
#include "build-info.hh"
#include <cstdio>
#include <cassert>

using std::string;

namespace openmsx {

LocalFileReference::LocalFileReference(File& file)
{
	init(file);
}

LocalFileReference::LocalFileReference(const string& filename)
{
	File file(filename);
	init(file);
}

LocalFileReference::LocalFileReference(const Filename& filename)
	: LocalFileReference(filename.getResolved())
{
}

void LocalFileReference::init(File& file)
{
	tmpFile = file.getLocalReference();
	if (!tmpFile.empty()) {
		// file is backed on the (local) filesystem,
		// we can simply use the path to that file
		assert(tmpDir.empty()); // no need to delete file/dir later
		return;
	}

	// create temp dir
#if defined(_WIN32) || PLATFORM_ANDROID
	tmpDir = FileOperations::getTempDir() + FileOperations::nativePathSeparator + "openmsx";
#else
	// TODO - why not just use getTempDir()?
	tmpDir = strCat("/tmp/openmsx.", int(getpid()));
#endif
	// it's possible this directory already exists, in that case the
	// following function does nothing
	FileOperations::mkdirp(tmpDir);

	// create temp file
	auto fp = FileOperations::openUniqueFile(tmpDir, tmpFile);
	if (!fp) {
		throw FileException("Couldn't create temp file");
	}

	// write temp file
	auto mmap = file.mmap();
	if (fwrite(mmap.data(), 1, mmap.size(), fp.get()) != mmap.size()) {
		throw FileException("Couldn't write temp file");
	}
}

LocalFileReference::~LocalFileReference()
{
	if (!tmpDir.empty()) {
		FileOperations::unlink(tmpFile);
		// it's possible the directory is not empty, in that case
		// the following function will fail, we ignore that error
		FileOperations::rmdir(tmpDir);
	}
}

const string& LocalFileReference::getFilename() const
{
	assert(!tmpFile.empty());
	return tmpFile;
}

} // namespace openmsx
