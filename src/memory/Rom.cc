// $Id$

#include <string>
#include <sstream>
#include "XMLElement.hh"
#include "Rom.hh"
#include "RomInfo.hh"
#include "File.hh"
#include "FileContext.hh"
#include "FileException.hh"
#include "PanasonicMemory.hh"
#include "StringOp.hh"
#include "Debugger.hh"
#include "sha1.hh"
#include "CliComm.hh"
#include "FilePool.hh"
#include "ConfigException.hh"
#include "HardwareConfig.hh"
#include "EmptyPatch.hh"
#include "IPSPatch.hh"

using std::string;
using std::auto_ptr;

namespace openmsx {

Rom::Rom(const string& name_, const string& description_,
         const XMLElement& config)
	: name(name_), description(description_)
{
	init(config.getChild("rom"));
}

Rom::Rom(const string& name_, const string& description_,
         const XMLElement& config, const string& id)
	: name(name_), description(description_)
{
	XMLElement::Children romConfigs;
	config.getChildren("rom", romConfigs);
	for (XMLElement::Children::const_iterator it = romConfigs.begin();
	     it != romConfigs.end(); ++it) {
		if ((*it)->getId() == id) {
			init(**it);
			return;
		}
	}
	throw ConfigException("ROM tag \"" + id + "\" missing.");
}

void Rom::init(const XMLElement& config)
{
	extendedRom = NULL;
	XMLElement::Children sums;
	config.getChildren("sha1", sums);
	const XMLElement* filenameElem = config.findChild("filename");
	if (!sums.empty() || filenameElem) {
		// file specified with SHA1 or filename
		string filename;
		if (filenameElem) {
			filename = filenameElem->getData();
		}
		for (XMLElement::Children::const_iterator it = sums.begin();
		     it != sums.end(); ++it) {
			const string& sha1 = (*it)->getData();
			string sha1File = FilePool::instance().getFile(sha1);
			if (!sha1File.empty()) {
				filename = sha1File;
				sha1sum = sha1;
				break;
			}
		}
		if (filename.empty()) {
			throw FatalError("Couldn't find ROM file for \"" +
			                 config.getId() + "\".");
		}
		read(config, filename);

	} else if (config.findChild("firstblock")) {
		// part of the TurboR main ROM
		int first = config.getChildDataAsInt("firstblock");
		int last  = config.getChildDataAsInt("lastblock");
		size = (last - first + 1) * 0x2000;
		rom = PanasonicMemory::instance().getRomRange(first, last);
		assert(rom);

	} else {
		// Assumption: this only happens for an empty SCC
		size = 0;
	}

	if (size != 0 ) {
		const XMLElement* patchesElem = config.findChild("patches");
		if (patchesElem) {
			auto_ptr<const PatchInterface> patch(
				new EmptyPatch(rom, size));

			FileContext& context = config.getFileContext();
			XMLElement::Children patches;
			patchesElem->getChildren("ips", patches);
			for (XMLElement::Children::const_iterator it
			       = patches.begin(); it != patches.end(); ++it) {
				const string& filename = (*it)->getData();
				patch.reset(new IPSPatch(
					context.resolve(filename), patch));
			}
			unsigned patchSize = patch->getSize();
			if (patchSize <= size) {
				patch->copyBlock(0, const_cast<byte*>(rom), size);
			} else {
				size = patchSize;
				extendedRom = new byte[size];
				patch->copyBlock(0, extendedRom, size);
				rom = extendedRom;
			}
		}
	}
	info = RomInfo::fetchRomInfo(*this);

	// TODO fix this, this is a hack that depends heavily on MSXRomCLI.cc
	if (!info->getTitle().empty() &&
	    ((name.size() >= 6) && (name.substr(0, 6) == "MSXRom"))) {
		name = HardwareConfig::instance().makeUnique(info->getTitle());
	}

	if (size) {
		Debugger::instance().registerDebuggable(name, *this);
	}
}



void Rom::read(const XMLElement& config, const string& filename)
{
	// open file
	try {
		file.reset(new File(config.getFileContext().resolve(filename)));
	} catch (FileException& e) {
		throw FatalError("Error reading ROM: " + filename);
	}
	
	// get filesize
	int fileSize;
	string fileSizeStr = config.getChildData("filesize", "auto");
	if (fileSizeStr == "auto") {
		fileSize = file->getSize();
	} else {
		fileSize = StringOp::stringToInt(fileSizeStr);
	}
	
	// get offset
	int offset = config.getChildDataAsInt("skip_headerbytes", 0);
	if (fileSize <= offset) {
		throw FatalError("Offset greater than filesize");
	}
	size = fileSize - offset;
	
	// read file
	byte* tmp = 0;	// avoid warning
	try {
		tmp = file->mmap() + offset;
		rom = tmp;
	} catch (FileException &e) {
		throw FatalError("Error reading ROM image: " + filename);
	}

	// verify SHA1
	if (!checkSHA1(config)) {
		CliComm::instance().printWarning(
			"SHA1 sum for '" + config.getId() +
			"' does not match with sum of '" + filename +
			"'.");
	}
}

bool Rom::checkSHA1(const XMLElement& config)
{
	const string& sha1sum = getSHA1Sum();
	XMLElement::Children sums;
	config.getChildren("sha1", sums);
	if (sums.empty()) {
		return true;
	}
	for (XMLElement::Children::const_iterator it = sums.begin();
	     it != sums.end(); ++it) {
		if ((*it)->getData() == sha1sum) {
			return true;
		}
	}
	return false;
}

Rom::~Rom()
{
	if (size) {
		Debugger::instance().unregisterDebuggable(name, *this);
	}
	if (file.get()) {
		file->munmap();
	}
	delete[] extendedRom;
}


unsigned Rom::getSize() const
{
	return size;
}

const string& Rom::getDescription() const
{
	return description;
}

byte Rom::read(unsigned address)
{
	assert(address < size);
	return rom[address];
}

void Rom::write(unsigned /*address*/, byte /*value*/)
{
	// ignore
}

const string& Rom::getName() const
{
	return name;
}

const string& Rom::getSHA1Sum() const
{
	if (sha1sum.empty()) {
		SHA1 sha1;
		sha1.update(rom, size);
		sha1sum = sha1.hex_digest();
	}
	return sha1sum;
}

} // namespace openmsx
