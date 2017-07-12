#pragma once
#include "DVBDescriptor.h"
#include <string>
#include <cstring>
#include <bitset>

// Format defined in ETSI TS 102 809,
// http://www.etsi.org/deliver/etsi_ts/102800_102899/102809/01.03.01_60/ts_102809v010301p.pdf
class ApplicationSignalingDescriptor:public DVBDescriptor {
public:
	ApplicationSignalingDescriptor(DVBDescriptor *d):DVBDescriptor() {
		_tag = d->tag();
		_length = d->length();
		_data = d->data();
		delete d;
	}
	void dump(std::ostream &where=std::cerr, std::string const &indent="") const override {
		where << indent << "Application Signaling: " << std::endl;
		unsigned char *pos = _data;
		while(pos < _data+_length) {
			where << indent << "\t" << "Application type: " << static_cast<int>((pos[0]&0b01111111)|(pos[1])) << " AIT version: " << static_cast<int>(pos[2]&0b00011111) << std::endl;
			pos += 3;
		}
	}
};
