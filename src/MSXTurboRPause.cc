// $Id$

#include "MSXTurboRPause.hh"
#include "LedEvent.hh"
#include "LedStatus.hh"
#include "MSXMotherBoard.hh"
#include "BooleanSetting.hh"

namespace openmsx {

MSXTurboRPause::MSXTurboRPause(MSXMotherBoard& motherBoard,
                               const XMLElement& config)
	: MSXDevice(motherBoard, config)
	, pauseSetting(new BooleanSetting(motherBoard.getCommandController(),
	               "turborpause", "status of the TurboR pause", false))
	, status(255)
	, pauseLed(false)
	, turboLed(false)
	, hwPause(false)
{
	pauseSetting->attach(*this);
	reset(*static_cast<EmuTime*>(0));
}

MSXTurboRPause::~MSXTurboRPause()
{
	pauseSetting->detach(*this);
}

void MSXTurboRPause::reset(const EmuTime& dummy)
{
	pauseSetting->changeValue(false);
	writeIO(0, 0, dummy);
}

void MSXTurboRPause::powerDown(const EmuTime& dummy)
{
	writeIO(0, 0, dummy);
}

byte MSXTurboRPause::readIO(word port, const EmuTime& time)
{
	return peekIO(port, time);
}

byte MSXTurboRPause::peekIO(word /*port*/, const EmuTime& /*time*/) const
{
	return pauseSetting->getValue() ? 1 : 0;
}

void MSXTurboRPause::writeIO(word /*port*/, byte value, const EmuTime& /*time*/)
{
	status = value;
	bool newTurboLed = (status & 0x80);
	if (newTurboLed != turboLed) {
		turboLed = newTurboLed;
		getMotherBoard().getLedStatus().setLed(LedEvent::TURBO, turboLed);
	}
	updatePause();
}

void MSXTurboRPause::update(const Setting& /*setting*/)
{
	updatePause();
}

void MSXTurboRPause::updatePause()
{
	bool newHwPause = (status & 0x02) && pauseSetting->getValue();
	if (newHwPause != hwPause) {
		hwPause = newHwPause;
		if (hwPause) {
			getMotherBoard().pause();
		} else {
			getMotherBoard().unpause();
		}
	}

	bool newPauseLed = (status & 0x01) || hwPause;
	if (newPauseLed != pauseLed) {
		pauseLed = newPauseLed;
		getMotherBoard().getLedStatus().setLed(LedEvent::PAUSE, pauseLed);
	}
}

} // namespace openmsx
