// $Id$

/*
 * Based on code from NLMSX written by Frits Hilderink
 */

#include "TurboRFDC.hh"
#include "TC8566AF.hh"
#include "Rom.hh"
#include "CacheLine.hh"

namespace openmsx {

TurboRFDC::TurboRFDC(MSXMotherBoard& motherBoard, const XMLElement& config)
	: MSXFDC(motherBoard, config)
	, controller(new TC8566AF(reinterpret_cast<DiskDrive**>(drives),
	                          getCurrentTime()))
{
	blockMask = (rom->getSize() / 0x4000) - 1;
	reset(getCurrentTime());
}

TurboRFDC::~TurboRFDC()
{
}

void TurboRFDC::reset(const EmuTime& time)
{
	setBank(0);
	controller->reset(time);
}

byte TurboRFDC::readMem(word address, const EmuTime& time)
{
	byte result;
	if (0x3FF0 <= (address & 0x3FFF)) {
		switch (address & 0x3FFF) {
		case 0x3FF1:
			result = 0x33;
			if (controller->diskChanged(0)) result &= ~0x10;
			if (controller->diskChanged(1)) result &= ~0x20;
			break;
		case 0x3FF4:
		case 0x3FFA:
			result = controller->readReg(4, time);
			break;
		case 0x3FF5:
		case 0x3FFB:
			result = controller->readReg(5, time);
			break;
		default:
			result = peekMem(address, time);
			break;
		}
		//std::cout << "TurboRFDC read 0x" << std::hex << (int)address <<
		//                           " 0x" << (int)result << std::dec << std::endl;
	} else {
		result = peekMem(address, time);
	}
	return result;
}

byte TurboRFDC::peekMem(word address, const EmuTime& time) const
{
	byte result;
	if (0x3FF0 <= (address & 0x3FFF)) {
		switch (address & 0x3FFF) {
		case 0x3FF1:
			// bit 0	FD2HD1	High Density detect drive 1
			// bit 1	FD2HD2	High Density detect drive 2
			// bit 4	FDCHG1	Disk Change detect on drive 1
			// bit 5	FDCHG2	Disk Change detect on drive 2
			// active low
			result = 0x33;
			if (controller->peekDiskChanged(0)) result &= ~0x10;
			if (controller->peekDiskChanged(1)) result &= ~0x20;
			break;
		case 0x3FF4:
		case 0x3FFA:
			result = controller->peekReg(4, time);
			break;
		case 0x3FF5:
		case 0x3FFB:
			result = controller->peekReg(5, time);
			break;
		default:
			result = 0xFF;
			break;
		}
	} else if ((0x4000 <= address) && (address < 0x8000)) {
		result = memory[address & 0x3FFF];
	} else {
		result = 0xFF;
	}
	return result;
}

const byte* TurboRFDC::getReadCacheLine(word start) const
{
	if ((start & 0x3FF0) == (0x3FF0 & CacheLine::HIGH)) {
		return NULL;
	} else if ((0x4000 <= start) && (start < 0x8000)) {
		return &memory[start & 0x3FFF];
	} else {
		return unmappedRead;
	}
}

void TurboRFDC::writeMem(word address, byte value, const EmuTime& time)
{
	if ((address == 0x6000) || (address == 0x7FF0) || (address == 0x7FFE)) {
		setBank(value);
	} else {
		//std::cout << "TurboRFDC write 0x" << std::hex << (int)address <<
		//                            " 0x" << (int)value << std::dec << std::endl;
		switch (address & 0x3FFF) {
		case 0x3FF2:
		case 0x3FF8:
			// bit 0	Drive select bit 0
			// bit 1	Drive select bit 1
			// bit 2	0 = Reset FDC, 1 = Enable FDC
			// bit 3	1 = Enable DMA and interrupt, 0 = always '0' on Turbo R
			// bit 4	Motor Select Drive A
			// bit 5	Motor Select Drive B
			// Bit 6	Motor Select Drive C
			// Bit 7	Motor Select Drive D
			controller->writeReg(2, value, time);
			break;
		case 0x3FF3:
		case 0x3FF9:
			controller->writeReg(3, value, time);
			break;
		case 0x3FF4:
		case 0x3FFA:
			controller->writeReg(4, value, time);
			break;
		case 0x3FF5:
		case 0x3FFB:	// FDC data port
			controller->writeReg(5, value, time);
			break;
		}
	}
}

void TurboRFDC::setBank(byte value)
{
	invalidateMemCache(0x4000, 0x4000);
	memory = &(*rom)[0x4000 * (value & blockMask)];
}

byte* TurboRFDC::getWriteCacheLine(word address) const
{
	if ((address == (0x6000 & CacheLine::HIGH)) ||
	    (address == (0x7FF0 & CacheLine::HIGH)) ||
	    (address == (0x7FFE & CacheLine::HIGH)) ||
	    ((address & 0x3FF0) == (0x3FF0 & CacheLine::HIGH))) {
		return NULL;
	} else {
		return unmappedWrite;
	}
}

} // namespace openmsx
