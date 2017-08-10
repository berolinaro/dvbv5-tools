// Format of the Program Map Table (PMT) is defined in
// ISO/IEC 13818-1, available here: http://www.ece.cmu.edu/~ece796/documents/MPEG-2_Systems_IS.doc
#include "ProgramMapTable.h"
#include "Util.h"
#include <cxxabi.h>
#include <cassert>

void ProgramMapTable::dump(std::ostream &where, std::string const &indent) const {
	Util::SaveIOState s(where);
	DVBTable::dump(where, indent);
	uint16_t pcrPid = ((_data[0]&0b00011111)<<8)|_data[1];
	uint16_t programInfoLength = ((_data[2]&0b00001111)<<8)|_data[3];
	where << indent << "PCR Pid " << pcrPid << std::endl;
	where << indent << "Program info length" << programInfoLength << std::endl;
	if(programInfoLength)
		Util::hexdump(_data+4, programInfoLength, where);
	unsigned char *pos = _data+4+programInfoLength;
	while(pos < _data+_dataLength) {
		uint8_t streamType = pos[0];
		uint16_t pid = ((pos[1]&0b00011111)<<8)|pos[2];
		uint16_t esInfoLength = ((pos[3]&0b00001111)<<8)|pos[4];
		where << indent << "	Stream type: ";
		if(streamType == 0x00)
			where << "Reserved";
		else if(streamType == 0x01)
			where << "ISO/IEC 11172-2 Video";
		else if(streamType == 0x02)
			where << "H.262 Video";
		else if(streamType == 0x03)
			where << "ISO/IEC 11172-3 Audio";
		else if(streamType == 0x04)
			where << "ISO/IEC 13818-3 Audio";
		else if(streamType == 0x05)
			where << "ISO/IEC 13818-1 reserved";
		else if(streamType == 0x06)
			where << "Private data";
		else if(streamType == 0x07)
			where << "ISO/IEC 13522 MHEG";
		else if(streamType == 0x08)
			where << "IEC 13818-1 Annex A DSM CC";
		else if(streamType == 0x09)
			where << "ITU-T Rec. H.222.1";
		else if(streamType == 0x0a)
			where << "ISO/IEC 13818-6 type A";
		else if(streamType == 0x0a)
			where << "ISO/IEC 13818-6 type B";
		else if(streamType == 0x0a)
			where << "ISO/IEC 13818-6 type C";
		else if(streamType == 0x0a)
			where << "ISO/IEC 13818-6 type D";
		else if(streamType == 0x0a)
			where << "ISO/IEC 13818-1 auxiliary";
		else if(streamType >= 0x0f && streamType <= 0x7f)
			where << "ISO/IEC 13818-1 reserved";
		else
			where << "User Private";
		where << " (" << std::hex << static_cast<int>(streamType) << ")" << std::endl;
		where << indent << "	PID " << pid << std::endl;
		if(esInfoLength)
			where << indent << "	ES Info:" << std::endl;
		pos += 5;
		unsigned char *end=pos+esInfoLength;
		while(pos<end) {
			DVBDescriptor *d=DVBDescriptor::get(pos);
			d->dump(std::cerr, indent + "\t\t");
			delete d;
		}
	}
}

Program::Program(ProgramMapTables const &pmts) {
	_programNumber = (*pmts.begin())->programNumber();
	_pcrPid = (((*pmts.begin())->_data[0]&0b00011111)<<8)|(*pmts.begin())->_data[1];
	int i=0;
	for(auto const &pmt: pmts) {
		if(_programNumber != pmt->programNumber()) {
			std::cerr << "Program number" << ++i << ": " << pmt->programNumber() << std::endl;
			std::cerr << "Found a program map table with inconsistent program numbers! Ignoring..." << std::endl;
			assert(false);
			continue;
		}
		if(_pcrPid != (((pmt->_data[0]&0b00011111)<<8)|pmt->_data[1])) {
			std::cerr << "Found a program map table with inconsistent program numbers! Ignoring..." << std::endl;
			assert(false);
			continue;
		}
		uint16_t programInfoLength = ((pmt->_data[2]&0b00001111)<<8)|pmt->_data[3];
		unsigned char *pos = pmt->_data+4;
		while(pos < pmt->_data+4+programInfoLength) {
			_descriptors.push_back(DVBDescriptor::get(pos));
		}
		pos = pmt->_data+4+programInfoLength;
		while(pos < pmt->_data+pmt->_dataLength) {
			uint8_t streamType = pos[0];
			uint16_t pid = ((pos[1]&0b00011111)<<8)|pos[2];
			uint16_t esInfoLength = ((pos[3]&0b00001111)<<8)|pos[4];
			pos += 5;
			unsigned char *end=pos+esInfoLength;
			std::vector<DVBDescriptor*> descriptors;
			while(pos<end) {
				DVBDescriptor *d=DVBDescriptor::get(pos);
				descriptors.push_back(d);
			}
			_streams.push_back(Stream(streamType, pid, descriptors));
		}
	}
}

void Stream::dump(std::ostream &where, std::string const &indent) const {
	Util::SaveIOState s(where);
	where << std::hex << std::setfill('0');
	where << indent << "Stream Type: " << std::setw(2) << static_cast<int>(_streamType) << " ";
	if(_streamType == 0x00)
		where << "Reserved";
	else if(_streamType == 0x01)
		where << "ISO/IEC 11172-2 Video";
	else if(_streamType == 0x02)
		where << "H.262 Video";
	else if(_streamType == 0x03)
		where << "ISO/IEC 11172-3 Audio";
	else if(_streamType == 0x04)
		where << "ISO/IEC 13818-3 Audio";
	else if(_streamType == 0x05)
		where << "ISO/IEC 13818-1 reserved";
	else if(_streamType == 0x06)
		where << "Private data";
	else if(_streamType == 0x07)
		where << "ISO/IEC 13522 MHEG";
	else if(_streamType == 0x08)
		where << "IEC 13818-1 Annex A DSM CC";
	else if(_streamType == 0x09)
		where << "ITU-T Rec. H.222.1";
	else if(_streamType == 0x0a)
		where << "ISO/IEC 13818-6 type A";
	else if(_streamType == 0x0a)
		where << "ISO/IEC 13818-6 type B";
	else if(_streamType == 0x0a)
		where << "ISO/IEC 13818-6 type C";
	else if(_streamType == 0x0a)
		where << "ISO/IEC 13818-6 type D";
	else if(_streamType == 0x0a)
		where << "ISO/IEC 13818-1 auxiliary";
	else if(_streamType == 0x11) // This is a guess -- not documented in ISO/IEC 13818-1, seen on "Das Erste HD" in Berlin DVB-T2
		where << "AAC-LATM audio";
	else if(_streamType == 0x1b)
		where << "HD Video"; // This is a guess -- not documented in ISO/IEC 13818-1, seen on "SRF info HD" in EWGoms DVB-C
	else if(_streamType == 0x24)
		where << "UHD Video"; // This is a guess -- not documented in ISO/IEC 13818-1, seen on "UHD Demo Channel" in EWGoms DVB-C
	else if(_streamType >= 0x0f && _streamType <= 0x7f)
		where << "ISO/IEC 13818-1 reserved";
	else
		where << "User Private";
	where << std::endl;
	where << indent << "PID: " << std::setw(4) << _pid << std::endl;
	if(!_descriptors.empty()) {
		where << indent << "Descriptors:" << std::endl;
		for(auto const &d: _descriptors)
			d->dump(where, indent+"\t");
	}
}

bool Stream::isAudio() const {
	return _streamType == 0x03 || _streamType == 0x04 || _streamType == 0x11;
}

bool Stream::isVideo() const {
	return _streamType == 0x01 || _streamType == 0x02 || _streamType == 0x1b || _streamType == 0x24;
}

bool Stream::isTeletext() const {
	// FIXME identify the streamType for Teletext
	return false;
}

bool Stream::isSubtitle() const {
	// FIXME identify the streamType for subtitles
	return false;
}

bool Stream::isPcr() const {
	// FIXME do we ever get a PCR in the stream list?
	return false;
}

void Program::dump(std::ostream &where, std::string const &indent) const {
	Util::SaveIOState s(where);
	where << std::hex << std::setfill('0');
	where << indent << "Program number: " << std::setw(4) << _programNumber << std::endl;
	where << indent << "\t" << "PCR Pid: " << std::setw(4) << _pcrPid << std::endl;
	where << indent << "\t" << "Number of descriptors: " << _descriptors.size() << std::endl;
	for(auto const &d: _descriptors) {
		std::cerr << "Dumping descriptor" << std::endl;
		std::cerr << "Type " << static_cast<int>(d->tag()) << std::endl;
		std::cerr << abi::__cxa_demangle(typeid(d).name(), 0, 0, 0) << std::endl;
		d->dump(where, indent+"\t");
		std::cerr << "Done dumping" << std::endl;
	}
	where << indent << "\t" << "Streams:" << std::endl;
	for(auto const &s: _streams)
		s.dump(where, indent+"\t\t");
}

Program ProgramMapTables::program() const {
	return Program(*this);
}
