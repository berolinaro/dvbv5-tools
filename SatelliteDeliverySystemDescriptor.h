#pragma once
#include "DVBDescriptor.h"
#include "Transponder.h"
#include "Util.h"
extern "C" {
#include <linux/dvb/frontend.h>
}

class SatelliteDeliverySystemDescriptor:public DVBDescriptor {
public:
	SatelliteDeliverySystemDescriptor(DVBDescriptor *d):DVBDescriptor() {
		_tag = d->tag();
		_length = d->length();
		_data = d->data();
		delete d;
	}
	uint32_t frequency() const {
		return	(((_data[0]&0b11110000)>>4)*10000000 +
			 ((_data[0]&0b1111))       *1000000  +
			 ((_data[1]&0b11110000)>>4)*100000   +
			 ((_data[1]&0b1111))       *10000    +
			 ((_data[2]&0b11110000)>>4)*1000     +
			 ((_data[2]&0b1111))       *100      +
			 ((_data[3]&0b11110000)>>4)*10       +
			 ((_data[3]&0b1111)));
	}
	uint16_t orbitalPosition() const {
		return  (((_data[4]&0b11110000)>>4)*1000 +
			 ((_data[4]&0b1111))       *100  +
			 ((_data[5]&0b11110000)>>4)*10   +
			 ((_data[5]&0b1111)));
	}
	uint32_t symbolRate() const {
		return	(((_data[7]& 0b11110000)>>4) *1000000 +
			 ((_data[7]& 0b1111))        *100000  +
			 ((_data[8]& 0b11110000)>>4) *10000   +
			 ((_data[8]& 0b1111))        *1000    +
			 ((_data[9]& 0b11110000)>>4) *100     +
			 ((_data[9]& 0b1111))        *10      +
			 ((_data[10]&0b11110000)>>4)) * 100;
	}
	fe_code_rate fec() const {
		switch(_data[10]&0xf) {
		case 1:
			return FEC_1_2;
		case 2:
			return FEC_2_3;
		case 3:
			return FEC_3_4;
		case 4:
			return FEC_5_6;
		case 5:
			return FEC_7_8;
		case 6:
			return FEC_8_9;
		case 7:
			return FEC_3_5;
		case 8:
			return FEC_4_5;
		case 9:
			return FEC_9_10;
		case 15:
			return FEC_NONE;
		default:
			return FEC_AUTO; // If we're "reserved for future use", the driver might know better...
		}
	}
	Lnb::Polarization polarization() const {
		switch((_data[6]&0b01100000)>>5) {
		case 0b00:
			return Lnb::Horizontal;
		case 0b01:
			return Lnb::Vertical;
		case 0b10:
			return Lnb::Left;
		case 0b11:
			return Lnb::Right;
		}
		return Lnb::Horizontal; // not reachable
	}
	fe_rolloff rolloff() const {
		switch((_data[6]&0b00011000)>>3) {
		case 0b00:
			return ROLLOFF_35;
		case 0b01:
			return ROLLOFF_25;
		case 0b10:
			return ROLLOFF_20;
		case 0b11:
			return ROLLOFF_AUTO;
		}
		return ROLLOFF_AUTO;
	}
	fe_modulation modulation() const {
		switch(_data[6]&0b00000011) {
		case 0b00:
			return QAM_AUTO;
		case 0b01:
			return QPSK;
		case 0b10:
			return PSK_8;
		case 0b11:
			return QAM_16;
		}
		return QPSK; // not reachable
	}
	bool isS2() const {
		return _data[6]&0b00000100;
	}
	DVBSTransponder *transponder() const {
		if(isS2())
			return new DVBS2Transponder(frequency(), symbolRate(), polarization(), fec());
		else
			return new DVBSTransponder(frequency(), symbolRate(), polarization(), fec());
	}
	void dump(std::ostream &where=std::cerr, std::string const &indent="") const override {
		if(isS2())
			where << indent << "DVB-S2 transponder:" << std::endl;
		else
			where << indent << "DVB-S transponder:" << std::endl;
		where	<< indent << "\t" << "Frequency: " << frequency() << std::endl
			<< indent << "\t" << "FEC: " << static_cast<int>(fec()) << " was " << static_cast<int>(_data[10]&0xf) << std::endl
			<< indent << "\t" << "Modulation: " << static_cast<int>(modulation()) << std::endl
			<< indent << "\t" << "Symbol rate: " << symbolRate() << std::endl;
	}
};
