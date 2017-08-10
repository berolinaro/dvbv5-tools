// Some T2 information can be found at
// https://www.dvb.org/resources/public/standards/a38_dvb-si_specification.pdf
#pragma once
#include "DVBDescriptor.h"
#include "Transponder.h"
#include "Util.h"
extern "C" {
#include <linux/dvb/frontend.h>
}

class TerrestrialDeliverySystemDescriptor:public DVBDescriptor {
public:
	TerrestrialDeliverySystemDescriptor(DVBDescriptor *d):DVBDescriptor() {
		_tag = d->tag();
		_length = d->length();
		_data = d->data();
		delete d;
	}
	uint32_t frequency() const {
		return ((_data[0]<<24) |
			(_data[1]<<16) |
			(_data[2]<<8) |
			(_data[3])) * 10;
	}
	uint32_t bandwidth() const {
		switch((_data[4]&0b11100000)>>5) {
		case 0b000:
			return 8000000;
		case 0b001:
			return 7000000;
		case 0b010:
			return 6000000;
		case 0b011:
			return 5000000;
		default:
			// 100 to 111 are "reserved for future use"
			return 0;
		}
	}
	fe_code_rate codeRateHp() const {
		switch(_data[5]&0b111) {
		case 0b000:
			return FEC_1_2;
		case 0b001:
			return FEC_2_3;
		case 0b010:
			return FEC_3_4;
		case 0b011:
			return FEC_5_6;
		case 0b100:
			return FEC_7_8;
		default:
			return FEC_AUTO; // If we're "reserved for future use", the driver might know better...
		}
	}
	fe_code_rate codeRateLp() const {
		switch((_data[6]&0b11100000)>>5) {
		case 0b000:
			return FEC_1_2;
		case 0b001:
			return FEC_2_3;
		case 0b010:
			return FEC_3_4;
		case 0b011:
			return FEC_5_6;
		case 0b100:
			return FEC_7_8;
		default:
			return FEC_AUTO; // If we're "reserved for future use", the driver might know better...
		}
	}
	fe_transmit_mode transmissionMode() const {
		switch((_data[6]&0b110)>>1) {
		case 0b00:
			return TRANSMISSION_MODE_2K;
		case 0b01:
			return TRANSMISSION_MODE_8K;
		case 0b10:
			return TRANSMISSION_MODE_4K;
		default:
			return TRANSMISSION_MODE_AUTO; // "reserved for future use" -- let's guess
		}
	}
	fe_guard_interval guardInterval() const {
		switch((_data[6]&0b11000)>>3) {
		case 0b00:
			return GUARD_INTERVAL_1_32;
		case 0b01:
			return GUARD_INTERVAL_1_16;
		case 0b10:
			return GUARD_INTERVAL_1_8;
		default:
			return GUARD_INTERVAL_1_4;
		}
	}
	fe_hierarchy hierarchy() const {
		// The next higher bit is part of the hierarchy coding as well
		// it specifies in-depth interleaver (set) vs native interleaver (unset)
		// which the current DVB kernel API doesn't care about
		// so we only check the lower 2 bits of hierarchy information
		switch((_data[5]&0b00011000)>>3) {
		case 0b00:
			return HIERARCHY_NONE;
		case 0b01:
			return HIERARCHY_1;
		case 0b10:
			return HIERARCHY_2;
		case 0b11:
			return HIERARCHY_4;
		}
		return HIERARCHY_AUTO; // Can't happen, but let's silence warnings
	}
	fe_modulation modulation() const {
		switch((_data[5]&0b11000000)>>6) {
		case 0b00:
			return QPSK;
		case 0b01:
			return QAM_16;
		case 0b10:
			return QAM_64;
		default:
			return QAM_AUTO; // reserved for future use -- guess
		}
	}
	DVBTTransponder *transponder() const {
		return new DVBTTransponder(frequency(), bandwidth(), codeRateHp(), codeRateLp(), transmissionMode(), guardInterval(), hierarchy(), modulation(), INVERSION_AUTO);
	}
	void dump(std::ostream &where=std::cerr, std::string const &indent="") const override {
		where << indent << "Terrestrial transponder:" << std::endl
			<< indent << "\t" << "Frequency: " << frequency() << std::endl
			<< indent << "\t" << "Bandwidth: " << bandwidth() << std::endl
			<< indent << "\t" << "Modulation: " << static_cast<int>(modulation()) << std::endl;
	}
};
