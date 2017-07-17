#pragma once
#include "ServiceType.h"
#include <cstdint>
#include <string>

class Service {
public:
	/**
	 * Generate a service
	 * @param serviceId service ID
	 * @param providerName provider name
	 * @param name service name
	 * @param serviceType service type
	 */
	Service(uint16_t serviceId, std::string providerName=std::string(), std::string name=std::string(""), ServiceType serviceType=TV):_serviceId(serviceId),_providerName(providerName),_name(name),_serviceType(serviceType) {}
	/**
	 * Generate a service from a channel list entry
	 * @param line line from channel list describing the service
	 */
	Service(std::string const &line);
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
