// $Id$

#ifndef __MSXIODEVICE_HH__
#define __MSXIODEVICE_HH__

#include "MSXDevice.hh"

class MSXIODevice : virtual public MSXDevice
{
	public:
		/**
		 * Constructor
		 */
		MSXIODevice(MSXConfig::Device *config, const EmuTime &time);
		
		/**
		 * Read a byte from an IO port at a certain time from this device.
		 * The default implementation returns 255.
		 */
		virtual byte readIO(byte port, const EmuTime &time);

		/**
		 * Write a byte to a given IO port at a certain time to this
		 * device.
		 * The default implementation ignores the write (does nothing)
		 */
		virtual void writeIO(byte port, byte value, const EmuTime &time);
};

#endif

