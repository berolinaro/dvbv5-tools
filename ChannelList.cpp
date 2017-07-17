#include "ChannelList.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

ChannelList::ChannelList(std::string const &filename):std::map<Transponder*,std::vector<Service*>>() {
	std::ifstream f(filename, std::ifstream::in);
	if(!f.good())
		return;
	while(f.good()) {
		std::string line;
		if(!std::getline(f, line, '\n'))
			break;
		if(line[0] == '#')
			continue;
		if((line[0] >= 'A' && line[0] <= 'Z') || (line[0] >= 'a' && line[0] <= 'z')) {
			Transponder *t = Transponder::fromString(line);
			if(t)
				insert(end(), make_pair(t, std::vector<Service*>()));
		} else {
			Service *s=new Service(line);
			std::istringstream iss(line);
			std::string tp;
			std::getline(iss, tp, '\t');
			uint32_t freq = std::stoi(tp);

			for(auto &tp: *this) {
				if(tp.first->frequency() == freq) {
					at(tp.first).push_back(s);
					break;
				}
			}
		}
	}
	f.close();
}

Transponder *ChannelList::transponder(std::string const &name, std::string const &providerName) const {
	for(auto const &tp: *this) {
		for(auto const &s: tp.second) {
			if(s->name() == name && (providerName.size()==0 || providerName == s->providerName()))
				return tp.first;
		}
	}
	return nullptr;
}

Service *ChannelList::service(std::string const &name, std::string const &providerName) const {
	for(auto const &tp: *this) {
		for(auto const &s: tp.second) {
			if(s->name() == name && (providerName.size()==0 || providerName == s->providerName()))
				return s;
		}
	}
	return nullptr;
}

std::pair<Transponder const *,Service const *> ChannelList::find(std::string const &name, std::string const &providerName) const {
	for(auto const &tp: *this) {
		for(auto const &s: tp.second) {
			if(s->name() == name && (providerName.size()==0 || providerName == s->providerName()))
				return std::make_pair<Transponder const *,Service const *>(tp.first,s);
		}
	}
	return std::make_pair<Transponder const *,Service const *>(nullptr, nullptr);
}
