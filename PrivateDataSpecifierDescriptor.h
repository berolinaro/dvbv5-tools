#pragma once
#include "DVBDescriptor.h"
#include "ServiceType.h"
#include <map>
#include <cstdint>

class PrivateDataSpecifierDescriptor:public DVBDescriptor {
public:
	PrivateDataSpecifierDescriptor(DVBDescriptor *d):DVBDescriptor() {
		_tag = d->tag();
		_length = d->length();
		_data = d->data();
		delete d;
	}
	uint32_t privateDataType() const {
		return (_data[0]<<24)|(_data[1]<<16)|(_data[2]<<8)|(_data[3]);
	}
	void dump(std::ostream &where=std::cerr, std::string const &indent="") const override {
		where << indent << "Private data type (ETSI TS 101 162): " << privateDataType() << std::endl;
	}
};
