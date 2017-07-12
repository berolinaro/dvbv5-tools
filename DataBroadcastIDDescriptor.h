#pragma once
#include "DVBDescriptor.h"
#include <string>
#include <cstring>
#include <bitset>

class DataBroadcastIdDescriptor:public DVBDescriptor {
public:
	DataBroadcastIdDescriptor(DVBDescriptor *d):DVBDescriptor() {
		_tag = d->tag();
		_length = d->length();
		_data = d->data();
		delete d;
	}
	uint16_t broadcastId() const {
		return (_data[0]<<8)|_data[1];
	}
	uint8_t *selectorBytes() const {
		if(_length == 2)
			return nullptr;
		uint8_t *ret = new uint8_t[_length-2];
		memcpy(ret, _data+2, _length-2);
		return ret;
	}
	void dump(std::ostream &where=std::cerr, std::string const &indent="") const override {
		where << indent << "Data broadcast ID: " << broadcastId() << std::endl;
		uint8_t *sb = selectorBytes();
		if(sb) {
			where << indent << "\t" << "Selector bytes: " << sb << std::endl;
			delete sb;
		}
	}
};
