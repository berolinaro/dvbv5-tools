#pragma once

#include "DVBTable.h"
#include <map>

extern "C" {
#include <stdint.h>
}

class NetworkInformationTable:public DVBTable {
public:
	NetworkInformationTable(DVBTable *t):DVBTable(t) { }
	void dump(std::ostream &where=std::cerr, std::string const &indent="") const override;
	uint16_t networkId() const { return _number; }
	std::vector<Transponder*> transponders() const;
	static constexpr uint8_t tablePid = NIT;
	static constexpr uint8_t tableFilter = NetworkInformation;
	static constexpr uint8_t tableMask = 0xff;
protected:
	mutable std::map<uint16_t,uint16_t> _pids;
};

class NetworkInformationTables:public DVBTables<NetworkInformationTable> {
public:
	NetworkInformationTables():DVBTables<NetworkInformationTable>() { }
	NetworkInformationTables(NetworkInformationTable *t):DVBTables<NetworkInformationTable>(t) { }
	void dump(std::ostream &where=std::cerr, std::string const &indent="") const override;
	std::vector<Transponder*> transponders() const;
};

