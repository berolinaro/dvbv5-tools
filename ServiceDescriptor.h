#pragma once
#include "DVBDescriptor.h"
#include "ServiceType.h"
#include <string>
#include <cstring>
#include <bitset>
#include <iostream>
#include <cassert>
#include "Util.h"

class ServiceDescriptor:public DVBDescriptor {
public:
	ServiceDescriptor(DVBDescriptor * const d):DVBDescriptor(d) {
	}
	ServiceType serviceType() const {
		return static_cast<ServiceType>(_data[0]);
	}
	bool isTV() const {
		return (serviceType() == TV) ||
		       (serviceType() == MPEG2HDTV) ||
		       (serviceType() == H264TV) ||
		       (serviceType() == H264HDTV) ||
		       (serviceType() == H2643DHDTV) ||
		       (serviceType() == HEVCTV);
	}
	bool isRadio() const {
		return (serviceType() == Radio) ||
		       (serviceType() == ACRadio);
	}
	std::string provider() const {
		char provName[_data[1]+1];
		memcpy(provName, _data+2, _data[1]);
		provName[_data[1]]=0;
		return provName;
	}
	std::string name() const {
		assert(_data);
		unsigned char const *pos = _data + _data[1] + 2;
		uint8_t length=pos[0];
		char name[length+1];
		memcpy(name, pos+1, length);
		if(name[0] < 0x20) {
			// ETSI EN 300 468 V1.15.1 Annex A: First byte can,
			// but doesn't have to, be a character set indicator.
			// should do some conversion at some point.
			// 0x1f is a special value indicating there's another
			// encoding byte in the second byte.
			if(name[0] == 0x1f) {
				length -= 2;
				memmove(name, name+2, length);
			} else
				memmove(name, name+1, --length);
		}
		name[length]=0;
		return name;
	}
	void dump(std::ostream &where=std::cerr, std::string const &indent="") const override {
		where << indent << "Service type: ";
		switch(_data[0]) {
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
		where << indent << "Provider name: " << provider() << std::endl;
		where << indent << "Name: " << name() << std::endl;
	}
};
