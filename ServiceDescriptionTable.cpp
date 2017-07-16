#include "ServiceDescriptionTable.h"
#include "DVBDescriptor.h"
#include "ServiceDescriptor.h"
#include "Util.h"
#include <iostream>

ServiceDescriptionTable::ServiceDescriptionTable(DVBTable *t):DVBTable(t) {
}

void ServiceDescriptionTable::dump(std::ostream &where, std::string const &indent) {
	Util::SaveIOState s(where);
	DVBTable::dump(where, indent);
	uint16_t originalNetworkId = (_data[0]<<8)+_data[1];
	where << indent << "Original network ID" << std::hex << std::setfill('0') << std::setw(4) << originalNetworkId << std::endl;
	unsigned char *pos = _data+3;
	while(pos < _data+_dataLength) {
		uint16_t serviceId = (pos[0]<<8)+pos[1];
		bool inSchedule = pos[2]&0b00000010;
		bool inPresentFollowing = pos[2]&0b00000001;
		uint8_t runningStatus = (pos[3]&0b11100000)>>5;
		bool needsCA = pos[3]&0b00010000;
		uint16_t descriptorLength = (pos[3]&0b00001111<<8)|pos[4];
		pos += 5;
		unsigned char *end=pos + descriptorLength;
		where << indent << "Service ID " << serviceId << std::endl;
		if(inSchedule)
			where << indent << "\t" << "In EIT Schedule" << std::endl;
		if(inPresentFollowing)
			where << indent << "\t" << "In EIT Present/Following" << std::endl;
		switch(runningStatus) {
		case 1:
			where << indent << "\t" << "Not running" << std::endl;
			break;
		case 2:
			where << indent << "\t" << "About to start" << std::endl;
			break;
		case 3:
			where << indent << "\t" << "Pausing" << std::endl;
			break;
		case 4:
			where << indent << "\t" << "Running" << std::endl;
			break;
		case 5:
			where << indent << "\t" << "Service off-air" << std::endl;
			break;
		default:
			where << indent << "\t" << "Undefined running status: " << static_cast<int>(runningStatus) << std::endl;
		}
		while(pos < end) {
			DVBDescriptor *d=DVBDescriptor::get(pos);
			d->dump(where, indent + "\t");
		}
	}
}

std::vector<Service> ServiceDescriptionTable::services() const {
	std::vector<Service> ret;
	if(!_data)
		return ret;
	uint16_t originalNetworkId = (_data[0]<<8)+_data[1];
	unsigned char *pos = _data+3;
	while(pos < _data+_dataLength) {
		Service s(pos[0]<<8|pos[1]);
		bool isValid = false;
		bool inSchedule = pos[2]&0b00000010;
		bool inPresentFollowing = pos[2]&0b00000001;
		uint8_t runningStatus = (pos[3]&0b11100000)>>5;
		bool needsCA = pos[3]&0b00010000;
		uint16_t descriptorLength = (pos[3]&0b00001111<<8)|pos[4];
		pos += 5;
		unsigned char *end=pos + descriptorLength;
		while(pos < end) {
			DVBDescriptor *d=DVBDescriptor::get(pos);
			if(ServiceDescriptor *sd=dynamic_cast<ServiceDescriptor*>(d)) {
				s.setName(sd->name());
				s.setProviderName(sd->provider());
				s.setServiceType(sd->serviceType());
				isValid = true;
			}
		}
		if(isValid)
			ret.push_back(s);
	}
	return ret;
}

std::vector<Service> ServiceDescriptionTables::services() const {
	std::vector<Service> ret;
	for(auto const &sdt: *this) {
		std::vector<Service> const s=sdt->services();
		ret.insert(ret.end(), s.begin(), s.end());
	}
	return ret;
}
