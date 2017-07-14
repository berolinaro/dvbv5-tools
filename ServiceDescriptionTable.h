#pragma once
#include "DVBTable.h"
#include "ServiceType.h"
#include <vector>
#include <string>

class Service {
public:
	Service(uint16_t serviceId, std::string providerName=std::string(), std::string name=std::string(""), ServiceType serviceType=TV):_serviceId(serviceId),_providerName(providerName),_name(name),_serviceType(serviceType) {}
	uint16_t serviceId() const { return _serviceId; }
	std::string providerName() const { return _providerName; }
	std::string name() const { return _name; }
	ServiceType serviceType() const { return _serviceType; }
	void setProviderName(std::string providerName) { _providerName = providerName; }
	void setName(std::string name) { _name = name; }
	void setServiceType(ServiceType st) { _serviceType = st; }
protected:
	uint16_t	_serviceId;
	std::string	_providerName;
	std::string	_name;
	ServiceType	_serviceType;
};

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
