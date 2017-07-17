#pragma once
#include "DVBTable.h"
#include "ServiceType.h"
#include "Service.h"
#include <vector>
#include <string>

class ServiceDescriptionTable:public DVBTable {
public:
	ServiceDescriptionTable(DVBTable *t);
	void dump(std::ostream &where=std::cerr, std::string const &indent="");
	std::vector<Service> services() const;
	static constexpr uint8_t tablePid = SDT;
	static constexpr uint8_t tableFilter = ServiceDescription;
	static constexpr uint8_t tableMask = 0xff;
};

class ServiceDescriptionTables:public DVBTables<ServiceDescriptionTable> {
public:
	ServiceDescriptionTables():DVBTables<ServiceDescriptionTable>() { }
	ServiceDescriptionTables(ServiceDescriptionTable *t):DVBTables<ServiceDescriptionTable>(t) { }
	std::vector<Service> services() const;
};
