#pragma once
#include "DVBDescriptor.h"
#include <string>
#include <cstring>
#include <bitset>
#include <iostream>
#include "Util.h"

class ServiceDescriptor:public DVBDescriptor {
public:
	enum ServiceType {
		TV = 0x01,
		Radio = 0x02,
		Teletext = 0x03,
		NVODReference = 0x04,
		NVODTimeShifted = 0x05,
		Mosaic = 0x06,
		FMRadio = 0x07,
		SRM = 0x08,
		ACRadio = 0x0a,
		AVCMosaic = 0x0b,
		DataBroadcast = 0x0c,
		CI = 0x0d,
		RCSMap = 0x0e,
		RCSFLS = 0x0f,
		MHP = 0x10,
		MPEG2HDTV = 0x11,
		H264TV = 0x16,
		H264NVODTimeShifted = 0x17,
		H264NVODReference = 0x18,
		H264HDTV = 0x19,
		H264HDNVODTimeShifted = 0x1a,
		H264HDNVODReference = 0x1b,
		H2643DHDTV = 0x1c,
		H2643DHDNVODTimeShifted = 0x1d,
		H2643DHDNVODReference = 0x1e,
		HEVCTV = 0x1f
	};
	ServiceDescriptor(DVBDescriptor *d):DVBDescriptor() {
		_tag = d->tag();
		_length = d->length();
		_data = d->data();
		delete d;
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
