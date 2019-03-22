#include "RS232Console.hh"
#include "RS232Connector.hh"
#include "PlugException.hh"
#include "EventDistributor.hh"
#include "Scheduler.hh"
#include "FileOperations.hh"
#include "serialize.hh"

namespace openmsx {

RS232Console::RS232Console(EventDistributor& eventDistributor_,
                         Scheduler& scheduler_,
                         CommandController& commandController)
	: eventDistributor(eventDistributor_), scheduler(scheduler_)
	, rs232InputFilenameSetting(
	        commandController, "rs232-inputfilename",
	        "filename of the file where the RS232 input is read from",
	        "rs232-input")
	, rs232OutputFilenameSetting(
	        commandController, "rs232-outputfilename",
	        "filename of the file where the RS232 output is written to",
	        "rs232-output")
{
	eventDistributor.registerEventListener(OPENMSX_RS232_TESTER_EVENT, *this);
	printf( "RS232 Console starting...\n" );
}

RS232Console::~RS232Console()
{
	eventDistributor.unregisterEventListener(OPENMSX_RS232_TESTER_EVENT, *this);
}

// Pluggable
void RS232Console::plugHelper(Connector& connector_, EmuTime::param /*time*/)
{
	// output
	string_view outName = rs232OutputFilenameSetting.getString();
	FileOperations::openofstream(outFile, outName.str());
	if (outFile.fail()) {
		outFile.clear();
		throw PlugException("Error opening output file: ", outName);
	}

	// input
	string_view inName = rs232InputFilenameSetting.getString();
	inFile = FileOperations::openFile(inName.str(), "rb");
	if (!inFile) {
		outFile.close();
		throw PlugException("Error opening input file: ", inName);
	}

	auto& rs232Connector = static_cast<RS232Connector&>(connector_);
	rs232Connector.setDataBits(SerialDataInterface::DATA_8);	// 8 data bits
	rs232Connector.setStopBits(SerialDataInterface::STOP_1);	// 1 stop bit
	rs232Connector.setParityBit(false, SerialDataInterface::EVEN); // no parity

	setConnector(&connector_); // base class will do this in a moment,
	                           // but thread already needs it
	thread = std::thread([this]() { run(); });
}

void RS232Console::unplugHelper(EmuTime::param /*time*/)
{
	// output
	outFile.close();

	// input
	poller.abort();
	thread.join();
	inFile.reset();
}

const std::string& RS232Console::getName() const
{
	static const std::string name("rs232-console");
	return name;
}

string_view RS232Console::getDescription() const
{
	return	"RS232 console pluggable. Reads data from stdin and outoputs "
		"data to stdout.  For serial console type interactions."
/*
		"with the 'rs-232-inputfilename' setting. Writes all data "
		"to the file specified with the 'rs232-outputfilename' "
		"setting." */
	;
}

void RS232Console::run()
{
	byte buf;
	if (!inFile) return;
	while (!feof(inFile.get())) {
#ifndef _WIN32
		if (poller.poll(fileno(inFile.get()))) {
			break;
		}
#endif
		size_t num = fread(&buf, 1, 1, inFile.get());
		if (poller.aborted()) {
			break;
		}
		if (num != 1) {
			continue;
		}
		assert(isPluggedIn());
		std::lock_guard<std::mutex> lock(mutex);
		queue.push_back(buf);
		eventDistributor.distributeEvent(
			std::make_shared<SimpleEvent>(OPENMSX_RS232_TESTER_EVENT));
	}
}

// input
void RS232Console::signal(EmuTime::param time)
{
	auto* conn = static_cast<RS232Connector*>(getConnector());
	if (!conn->acceptsData()) {
		std::lock_guard<std::mutex> lock(mutex);
		queue.clear();
		return;
	}
	if (!conn->ready()) return;

	std::lock_guard<std::mutex> lock(mutex);
	if (queue.empty()) return;
	conn->recvByte(queue.pop_front(), time);
}

// EventListener
int RS232Console::signalEvent(const std::shared_ptr<const Event>& /*event*/)
{
	if (isPluggedIn()) {
		signal(scheduler.getCurrentTime());
	} else {
		std::lock_guard<std::mutex> lock(mutex);
		queue.clear();
	}
	return 0;
}


// output
void RS232Console::recvByte(byte value, EmuTime::param /*time*/)
{
	if (outFile.is_open()) {
		outFile.put(value);
		outFile.flush();
	}
}


template<typename Archive>
void RS232Console::serialize(Archive& /*ar*/, unsigned /*version*/)
{
	// don't try to resume a previous logfile (see PrinterPortLogger)
}
INSTANTIATE_SERIALIZE_METHODS(RS232Console);
REGISTER_POLYMORPHIC_INITIALIZER(Pluggable, RS232Console, "RS232Console");

} // namespace openmsx
