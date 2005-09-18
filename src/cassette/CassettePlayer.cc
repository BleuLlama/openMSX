// $Id$

#include "CassettePlayer.hh"
#include "CommandController.hh"
#include "GlobalSettings.hh"
#include "XMLElement.hh"
#include "FileContext.hh"
#include "WavImage.hh"
#include "CasImage.hh"
#include "DummyCassetteImage.hh"
#include "CliComm.hh"
#include "CommandException.hh"
#include "MSXMotherBoard.hh"
#include <cstdlib>

using std::auto_ptr;
using std::list;
using std::string;
using std::vector;
using std::set;

namespace openmsx {

MSXCassettePlayerCLI::MSXCassettePlayerCLI(CommandLineParser& commandLineParser_)
	: commandLineParser(commandLineParser_)
{
	commandLineParser.registerOption("-cassetteplayer", this);
	commandLineParser.registerFileClass("cassetteimage", this);
}

bool MSXCassettePlayerCLI::parseOption(const string& option,
                                       list<string>& cmdLine)
{
	parseFileType(getArgument(option, cmdLine), cmdLine);
	return true;
}
const string& MSXCassettePlayerCLI::optionHelp() const
{
	static const string text(
	  "Put cassette image specified in argument in virtual cassetteplayer");
	return text;
}

void MSXCassettePlayerCLI::parseFileType(const string& filename,
                                         list<string>& /*cmdLine*/)
{
	XMLElement& config = commandLineParser.getMotherBoard().
	                getCommandController().getGlobalSettings().getMediaConfig();
	XMLElement& playerElem = config.getCreateChild("cassetteplayer");
	playerElem.setData(filename);
	playerElem.setFileContext(auto_ptr<FileContext>(
		new UserFileContext(commandLineParser.getMotherBoard().
							getCommandController())));
}
const string& MSXCassettePlayerCLI::fileTypeHelp() const
{
	static const string text("Cassette image, raw recording or fMSX CAS image");
	return text;
}


CassettePlayer::CassettePlayer(
	CliComm& cliComm_, CommandController& commandController,
	Mixer& mixer)
	: SoundDevice(mixer, getName(), getDescription())
	, SimpleCommand(commandController, "cassetteplayer")
	, motor(false), forcePlay(false), mode(PLAY)
	, cliComm(cliComm_)
{
	XMLElement& config = commandController.getGlobalSettings().getMediaConfig();
	playerElem = &config.getCreateChild("cassetteplayer");
	const string filename = playerElem->getData();
	if (!filename.empty()) {
		try {
			insertTape(playerElem->getFileContext().resolve(filename));
		} catch (MSXException& e) {
			throw FatalError("Couldn't load tape image: " + filename);
		}
	} else {
		removeTape();
	}

	static XMLElement cassettePlayerConfig("cassetteplayer");
	static bool init = false;
	if (!init) {
		init = true;
		auto_ptr<XMLElement> sound(new XMLElement("sound"));
		sound->addChild(auto_ptr<XMLElement>(new XMLElement("volume", "5000")));
		cassettePlayerConfig.addChild(sound);
	}
	registerSound(cassettePlayerConfig);
}

CassettePlayer::~CassettePlayer()
{
	unregisterSound();
}

void CassettePlayer::insertTape(const string& filename)
{
	try {
		// first try WAV
		cassette.reset(new WavImage(filename));
	} catch (MSXException &e) {
		// if that fails use CAS
		cassette.reset(new CasImage(filename));
	}
	
	rewind();
	playerElem->setData(filename);
}

void CassettePlayer::removeTape()
{
	cassette.reset(new DummyCassetteImage());
	playerElem->setData("");
}

void CassettePlayer::rewind()
{
	tapeTime = EmuTime::zero;
	playTapeTime = EmuTime::zero;
}

void CassettePlayer::updatePosition(const EmuTime& time)
{
	if (motor || forcePlay) {
		tapeTime += (time - prevTime);
		playTapeTime = tapeTime;
	}
	prevTime = time;
}

void CassettePlayer::setMotor(bool status, const EmuTime& time)
{
	updatePosition(time);
	motor = status;
	setMute(!motor && !forcePlay);
}

short CassettePlayer::getSample(const EmuTime& time)
{
	return (motor || forcePlay) ? cassette->getSampleAt(time) : 0;
}

short CassettePlayer::readSample(const EmuTime& time)
{
	updatePosition(time);
	return getSample(tapeTime);
}

void CassettePlayer::setSignal(bool /*output*/, const EmuTime& /*time*/)
{
	// recording not implemented for cassetteplayer
}

const string& CassettePlayer::getName() const
{
	static const string name("cassetteplayer");
	return name;
}

const string& CassettePlayer::getDescription() const
{
	static const string desc("Cassetteplayer, use to read .cas or .wav files.\n");
	return desc;
}

void 
CassettePlayer::plugHelper(Connector* /*connector*/, const EmuTime& /*time*/)
{
}

void CassettePlayer::unplugHelper(const EmuTime& /*time*/)
{
}


string CassettePlayer::execute(const vector<string>& tokens)
{
	string result;
	if (tokens[1] == "-mode") {
		if (tokens[2] == "record") {
			// TODO: add filename parameter and maybe also -prefix option
			result += "Record mode set\n";
			mode = RECORD;
		} else if (tokens[2] == "play") {
			result += "Play mode set\n";
			mode = PLAY;
		} else if (tokens.size() == 2) {
			result += "Mode is: ";
			if (mode == PLAY) {
				result+="play";
			} else if (mode == RECORD) {
				result+= "record";
			} else assert(false); // Illegal mode!? impossible...
			result += "\n";
		} else throw SyntaxError();
	} else if (tokens.size() != 2) {
		throw SyntaxError();
	} else 	if (tokens[1] == "-eject") {
		result += "Tape ejected\n";
		removeTape();
		cliComm.update(CliComm::MEDIA,
		                           "cassetteplayer", "");
	} else if (tokens[1] == "eject") {
		result += "Tape ejected\nWarning: use of 'eject' is deprecated, instead use '-eject'\n";
		removeTape();
		cliComm.update(CliComm::MEDIA,
		                           "cassetteplayer", "");
	} else if (tokens[1] == "-rewind") {
		result += "Tape rewinded\n";
		rewind();
	} else if (tokens[1] == "rewind") {
		result += "Tape rewinded\nWarning: use of 'rewind' is deprecated, instead use '-rewind'\n";
		rewind();
	} else if (tokens[1] == "-force_play") {
		forcePlay = true;
		setMute(false);
	} else if (tokens[1] == "force_play") {
		forcePlay = true;
		setMute(false);
		result += "Warning: use of 'force_play' is deprecated, instead use '-force_play'\n";
	} else if (tokens[1] == "-no_force_play") {
		forcePlay = false;
		setMute(!motor);
	} else if (tokens[1] == "no_force_play") {
		forcePlay = false;
		setMute(!motor);
		result += "Warning: use of 'no_force_play' is deprecated, instead use '-no_force_play'\n";
	} else {
		try {
			result += "Changing tape\n";
			UserFileContext context(getCommandController());
			insertTape(context.resolve(tokens[1]));
			cliComm.update(CliComm::MEDIA,
			               "cassetteplayer", tokens[1]);
		} catch (MSXException &e) {
			throw CommandException(e.getMessage());
		}
	}
	return result;
}

string CassettePlayer::help(const vector<string>& /*tokens*/) const
{
	return "cassetteplayer -eject         : remove tape from virtual player\n"
	       "cassetteplayer -rewind        : rewind tape in virtual player\n"
	       "cassetteplayer -force_play    : force playing of tape (no remote)\n"
	       "cassetteplayer -no_force_play : don't force playing of tape\n"
	       "cassetteplayer -mode          : change mode (record or play)\n"
	       "cassetteplayer <filename>     : change the tape file\n";
}

void CassettePlayer::tabCompletion(vector<string>& tokens) const
{
	if (tokens.size() == 2) {
		set<string> extra;
		extra.insert("-eject");
		extra.insert("-rewind");
		extra.insert("-force_play");
		extra.insert("-no_force_play");
		extra.insert("-mode");
		UserFileContext context(getCommandController());
		completeFileName(tokens, context, extra);
	}
}

void CassettePlayer::setVolume(int newVolume)
{
	volume = newVolume;
}

void CassettePlayer::setSampleRate(int sampleRate)
{
	delta = EmuDuration(1.0 / sampleRate);
}

void CassettePlayer::updateBuffer(unsigned length, int* buffer,
     const EmuTime& /*time*/, const EmuDuration& /*sampDur*/)
{
	while (length--) {
		*(buffer++) = (((int)getSample(playTapeTime)) * volume) >> 15;
		playTapeTime += delta;
	}
}

} // namespace openmsx
