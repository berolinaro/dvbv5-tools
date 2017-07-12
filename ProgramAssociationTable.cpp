// Format of the Program Association Table (PAT) is defined in
// ISO/IEC 13818-1, available here:
// http://www.ece.cmu.edu/~ece796/documents/MPEG-2_Systems_IS.doc

#include "ProgramAssociationTable.h"
#include "Util.h"

std::map<uint16_t,uint16_t> ProgramAssociationTable::pids() const {
	if(_pids.size())
		return _pids;

	for(uint16_t i=0; i<_dataLength; i+=4) {
		_pids.insert(std::make_pair((_data[i]<<8)+_data[i+1], ((_data[i+2]&0b00011111)<<8)+_data[i+3]));
	}
	return _pids;
}

uint16_t ProgramAssociationTable::pid(uint16_t programNumber) const {
	if(pids().find(programNumber) != _pids.end())
		return _pids[programNumber];
	return 0;
}

void ProgramAssociationTable::dump(std::ostream &where, std::string const &indent) const {
	Util::SaveIOState isr(where);
	DVBTable::dump(where, indent);
	std::map<uint16_t,uint16_t> PIDs=pids();
	where << std::hex << std::setfill('0');
	for(std::pair<uint16_t,uint16_t> p: PIDs) {
		where << indent << "Program number " << std::setw(4) << p.first << " PID " << std::setw(4) << p.second << std::endl;
	}
}

std::map<uint16_t,uint16_t> ProgramAssociationTables::pids() const {
	std::map<uint16_t,uint16_t> result;
	for(auto p: *this) {
		std::map<uint16_t,uint16_t> inSection = p->pids();
		result.insert(inSection.begin(), inSection.end());
	}
	return result;
}

void ProgramAssociationTables::dump(std::ostream &where, std::string const &indent) const {
	Util::SaveIOState isr(where);
	std::map<uint16_t,uint16_t> PIDs=pids();
	where << std::hex << std::setfill('0');
	for(std::pair<uint16_t,uint16_t> p: PIDs) {
		where << indent << "Program number " << std::setw(4) << p.first << " PID " << std::setw(4) << p.second << std::endl;
	}
}
