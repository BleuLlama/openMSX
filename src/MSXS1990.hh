#ifndef MSXS1990_HH
#define MSXS1990_HH

#include "MSXDevice.hh"
#include "SimpleDebuggable.hh"
#include <memory>

namespace openmsx {

class FirmwareSwitch;

/**
 * This class implements the MSX-engine found in a MSX Turbo-R (S1990)
 *
 * TODO explanation
 */
class MSXS1990 final : public MSXDevice
{
public:
	explicit MSXS1990(const DeviceConfig& config);
	~MSXS1990();

	void reset(EmuTime::param time) override;
	byte readIO(word port, EmuTime::param time) override;
	byte peekIO(word port, EmuTime::param time) const override;
	void writeIO(word port, byte value, EmuTime::param time) override;

	template<typename Archive>
	void serialize(Archive& ar, unsigned version);

private:
	byte readRegister(byte reg) const;
	void writeRegister(byte reg, byte value);
	void setCPUStatus(byte value);

	const std::unique_ptr<FirmwareSwitch> firmwareSwitch;

	class Debuggable final : public SimpleDebuggable {
	public:
		Debuggable(MSXMotherBoard& motherBoard, MSXS1990& s1990);
		byte read(unsigned address) override;
		void write(unsigned address, byte value) override;
	private:
		MSXS1990& s1990;
	} debuggable;

	byte registerSelect;
	byte cpuStatus;
};

} // namespace openmsx

#endif
