#pragma once
#include <map>
#include <vector>
#include "Transponder.h"
#include "Service.h"

class ChannelList:public std::map<Transponder*,std::vector<Service*>> {
public:
	/**
	 * Reads a channel list from a file
	 */
	ChannelList(std::string const &filename);
	Transponder *transponder(std::string const &name, std::string const &providerName="") const;
	Service *service(std::string const &name, std::string const &providerName="") const;
	std::pair<Transponder const *,Service const *> find(std::string const &name, std::string const &providerName="") const;
};
