#pragma once

#include "DVBTable.h"
#include <map>

extern "C" {
#include <stdint.h>
}

class ProgramAssociationTable:public DVBTable {
public:
	ProgramAssociationTable(DVBTable *t):DVBTable(t) { }
	std::map<uint16_t,uint16_t> pids() const;
	uint16_t pid(uint16_t programNumber) const;
	void dump(std::ostream &where=std::cerr, std::string const &indent="") const override;
	uint16_t networkId() const { return _number; }
	static constexpr uint8_t tablePid = PAT;
	static constexpr uint8_t tableFilter = ProgramAssociation;
	static constexpr uint8_t tableMask = 0xff;
protected:
	mutable std::map<uint16_t,uint16_t> _pids;
};

class ProgramAssociationTables:public DVBTables<ProgramAssociationTable> {
public:
	ProgramAssociationTables():DVBTables<ProgramAssociationTable>() { }
	ProgramAssociationTables(ProgramAssociationTable *t):DVBTables<ProgramAssociationTable>(t) { }
	std::map<uint16_t,uint16_t> pids() const;
	void dump(std::ostream &where=std::cerr, std::string const &indent="") const override;
};

