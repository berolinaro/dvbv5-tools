#pragma once
#include "DVBDescriptor.h"
#include "ServiceType.h"
#include <map>
#include <cstdint>

class ServiceListDescriptor:public DVBDescriptor {
public:
	ServiceListDescriptor(DVBDescriptor *d):DVBDescriptor() {
		_tag = d->tag();
		_length = d->length();
		_data = d->data();
		delete d;
	}
	std::map<uint16_t,ServiceType> services() const {
		std::map<uint16_t,ServiceType> ret;
		uint8_t *pos=_data, *end=_data+_length;
		while(pos<end) {
			ret.insert(std::make_pair<uint16_t,ServiceType>((pos[0]<<8)|pos[1], static_cast<ServiceType>(pos[2])));
			pos += 3;
		}
		return ret;
	}
	void dump(std::ostream &where=std::cerr, std::string const &indent="") const override {
		where << indent << "Service List Descriptor:" << std::endl;
		for(auto a: services()) {
			where << indent << "\t" << "Service ID " << a.first << " is a ";
			switch(a.second) {
			case 0x1:
				where << "TV";
				break;
			case 0x2:
				where << "Radio";
				break;
			case 0x7:
				where << "FM Radio";
				break;
			case 0x11:
				where << "MPEG-2 HD TV";
				break;
			case 0x16:
				where << "H.264 SD TV";
				break;
			case 0x19:
				where << "H.264 HD TV";
				break;
			case 0x1f:
				where << "HEVC TV";
				break;
			default:
				where << static_cast<int>(_data[0]);
			}
			where << std::endl;
		}
	}
};
