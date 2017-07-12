#pragma once
#include "DVBDescriptor.h"
#include "Util.h"
#include <cstring>

class CopyrightDescriptor:public DVBDescriptor {
public:
	CopyrightDescriptor(DVBDescriptor *d):DVBDescriptor() {
		_tag = d->tag();
		_length = d->length();
		_data = d->data();
		delete d;
	}
	uint32_t identifier() const {
		return (_data[0]<<24)|(_data[1]<<16)|(_data[2]<<8)|_data[3];
	}
	uint8_t *additionalInfo() const {
		if(_length == 4)
			return nullptr;
		uint8_t *info = new uint8_t[_length-4];
		memcpy(info, _data+4, _length-4);
		return info;
	}
	void dump(std::ostream &where=std::cerr, std::string const &indent="") const override {
		Util::SaveIOState s(where);
		where << indent << "Copyright: " << std::endl;
		where << indent << "\t" << "Owner ID: " << std::hex << identifier() << std::endl;
		if(uint8_t *info = additionalInfo()) {
			where << indent << "\t" << "Additional info: " << info << std::endl;
			delete info;
		}
	}
};
