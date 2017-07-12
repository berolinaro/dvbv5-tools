#pragma once
#include "DVBDescriptor.h"
#include <string>
#include <cstring>

struct TeletextInfo {
	char const languageCode[3];
	uint8_t type;
	uint8_t magazineType;
	uint8_t pageNumber;
};

class TeletextDescriptor:public DVBDescriptor {
public:
	TeletextDescriptor(DVBDescriptor *d):DVBDescriptor() {
		_tag = d->tag();
		_length = d->length();
		_data = d->data();
		delete d;
	}
	uint8_t streamIdentifier() const { return _data[0]; }
	void dump(std::ostream &where=std::cerr, std::string const &indent="") const override {
		where << indent << "Teletext: " << static_cast<int>(streamIdentifier()) << std::endl;
	}
};
