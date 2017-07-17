#include "Service.h"
#include <string>
#include <sstream>

Service::Service(std::string const &line) {
	// FIXME do some validity checking here
	std::string part;
	std::istringstream iss(line);
	std::getline(iss, part, '\t'); // First entry is the transponder -- skip
	std::getline(iss, part, '\t');
	_name = part;
	std::getline(iss, part, '\t');
	_providerName = part;
	std::getline(iss, part, '\t');
	_serviceId = std::stoi(part);
	std::getline(iss, part, '\t');
	_serviceType = static_cast<ServiceType>(std::stoi(part));
}
