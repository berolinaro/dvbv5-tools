#pragma once
#include "DVBDescriptor.h"
#include <string>
#include <cstring>
#include <bitset>

class AC3Descriptor:public DVBDescriptor {
public:
	AC3Descriptor(DVBDescriptor *d):DVBDescriptor() {
		_tag = d->tag();
		_length = d->length();
		_data = d->data();
		delete d;
	}
	void dump(std::ostream &where=std::cerr, std::string const &indent="") const override {
		where << indent << "AC-3 Audio:" << std::endl;
		std::bitset<8> AC3Flags = _data[0];
		unsigned char *pos = _data+1;
		if(AC3Flags.test(7))
			where << indent << "\t" << "Component Type " << static_cast<int>(*(pos++)) << std::endl;
		if(AC3Flags.test(6))
			where << indent << "\t" << "BSID Coding version " << static_cast<int>(*(pos++)) << std::endl;
		if(AC3Flags.test(5))
			where << indent << "\t" << "Main audio service: " << static_cast<int>(*(pos++)) << std::endl;
		if(AC3Flags.test(4))
			where << indent << "\t" << "ASVC: " << static_cast<int>(*(pos++)) << std::endl;
		if(pos < _data+_length)
			where << indent << "\t" << "Additional info byte: " << static_cast<int>(*pos) << std::endl;
	}
};
