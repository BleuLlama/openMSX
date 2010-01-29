// $Id$

#ifndef OGGREADER_HH
#define OGGREADER_HH

#include "openmsx.hh"
#include "File.hh"

#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <theora/theoradec.h>

#include <list>
#include <deque>
#include <map>

namespace openmsx {

class CliComm;
class RawFrame;

struct AudioFragment
{
	static const unsigned UNKNOWN_POS = 0xffffffff;
	static const unsigned MAX_SAMPLES = 2048;
	unsigned position;
	unsigned length;
	float pcm[2][MAX_SAMPLES];
};

struct Frame
{
	int no;
	th_ycbcr_buffer buffer;
};

class OggReader
{
public:
	OggReader(const Filename& filename, CliComm& cli_);
	~OggReader();

	bool seek(int frame, int sample);
	unsigned getSampleRate() const { return vi.rate; }
	void getFrame(RawFrame& frame, int frameno);
	AudioFragment* getAudio(unsigned sample);

	// metadata
	bool stopFrame(int frame) { return stopFrames[frame] != 0; }
	int chapter(int chapterNo) { return chapters[chapterNo]; }

private:
	void cleanup();
	void readTheora(ogg_packet* packet);
	void theoraHeaderPage(ogg_page* page, th_info& video_info,
	        th_comment& video_comment, th_setup_info*& video_setup_info);
	void readMetadata(th_comment& tc);
	void readVorbis(ogg_packet* packet);
	void vorbisHeaderPage(ogg_page* page);
	bool nextPage(ogg_page* page);
	bool nextPacket();
	void returnAudio(AudioFragment* audio);
	void vorbisFoundPosition();
	int frameNo(ogg_packet* packet);

	unsigned guessSeek(int frame, unsigned sample);
	unsigned binarySearch(int frame, unsigned sample,
		unsigned maxOffset, unsigned maxSamples, unsigned maxFrames);

	CliComm& cli;
	File file;

	enum State { 
		PLAYING,
		FIND_LAST,
		FIND_FIRST,
		FIND_KEYFRAME
	} state;

	// ogg state
	ogg_sync_state sync;
	ogg_stream_state vorbisStream, theoraStream;
	int audioSerial;
	int videoSerial;
	int skeletonSerial;
	unsigned currentOffset;
	unsigned totalBytes;

	// video
	int videoHeaders;
	th_dec_ctx* video_state;
	int keyFrame;
	int currentFrame;
	int granuleShift;

	typedef std::deque<Frame*> Frames;

	Frames frameList;
	Frames recycleFrameList;

	// audio
	int audioHeaders;
	vorbis_info vi;
	vorbis_comment vc;
	vorbis_dsp_state vd;
	vorbis_block vb;
	unsigned currentSample;
	unsigned vorbisPos;

	typedef std::list<AudioFragment*> AudioFragments;

	AudioFragments audioList;
	AudioFragments recycleAudioList;

	// Metadata
	std::map<int, int> stopFrames;
	std::map<int, int> chapters;
};

} // namespace openmsx

#endif
