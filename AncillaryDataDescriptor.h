#pragma once
#include "DVBDescriptor.h"
#include <string>
#include <cstring>
#include <bitset>

class AncillaryDataDescriptor:public DVBDescriptor {
public:
	enum AncillaryData {
		DVDVideo = 0, // ETSI TS 101 154 [9]
		Extended = 1, // ETSI TS 101 154 [9]
		AnnouncementSwitching = 2, // ETSI TS 101 154 [9]
		DAB = 3, // ETSI EN 300 401 [2]
		ScaleFactorErrorCheck = 4, // ETSI TS 101 154 [9]
		MPEG4 = 5, // ETSI TS 101 154 [9]
		RDS = 6, // ETSI TS 101 154 [9]
		ReservedFutureUse = 7
	};

	AncillaryDataDescriptor(DVBDescriptor * const d):DVBDescriptor(d) {
	}
	std::bitset<8> ancillaryData() const { return _data[0]; }
	void dump(std::ostream &where=std::cerr, std::string const &indent="") const override {
		where << indent << "AncillaryData:";
		if(ancillaryData().test(DVDVideo))
			where << " [DVD Video]";
		if(ancillaryData().test(Extended))
			where << " [Extended]";
		if(ancillaryData().test(AnnouncementSwitching))
			where << " [Announcement Switching]";
		if(ancillaryData().test(ScaleFactorErrorCheck))
			where << " [Scale Factor Error Check]";
		if(ancillaryData().test(MPEG4))
			where << " [MPEG-4]";
		if(ancillaryData().test(RDS))
			where << " [RDS]";
		if(ancillaryData().test(ReservedFutureUse))
			where << " [Reserved]";
		if(!_data[0])
			where << " none";
		where << std::endl;
	}
};
