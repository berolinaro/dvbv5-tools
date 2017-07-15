#pragma once
#include "DVBDescriptor.h"
#include "ISO639_2.h"
#include <string>
#include <cstring>

class LanguageDescriptor:public DVBDescriptor {
public:
	enum StreamType {
		CleanEffects = 0x01,
		HearingImpaired = 0x02,
		VisualImpaired = 0x03
	};
	LanguageDescriptor(DVBDescriptor * const d):DVBDescriptor(d) {
	}
	std::string language() const {
		return ISO639_2::language(langCode());
	}
	std::string langCode() const {
		char lc[4];
		memcpy(lc, _data, 3);
		lc[3]=0;
		return lc;
	}
	StreamType streamType() const { return static_cast<StreamType>(_data[3]); }
	void dump(std::ostream &where=std::cerr, std::string const &indent="") const override {
		where << indent << "Language: " << ISO639_2::language(langCode()) << " (";
		switch(streamType()) {
		case CleanEffects:
			where << "normal";
			break;
		case HearingImpaired:
			where << "for hearing impaired";
			break;
		case VisualImpaired:
			where << "for visual impaired";
			break;
		default:
			where << "unknown subtype " << static_cast<int>(streamType());
		}
		where << ")" << std::endl;
	}
};
