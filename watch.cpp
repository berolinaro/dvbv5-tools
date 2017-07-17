#include "DVBInterface.h"
#include "ChannelList.h"
#include "Service.h"
#include <iostream>
#include <fstream>

extern "C" {
#include <sys/poll.h>
}

int main(int argc, char **argv) {
	if(argc<2) {
		std::cerr << "Usage: " << argv[0] << " \"Channel name\"" << std::endl;
		return 1;
	}

	DVBInterfaces cards = DVBInterfaces::all();
	if(cards.size() == 0) {
		std::cerr << "No DVB interfaces found" << std::endl;
		return 1;
	}

	ChannelList channels("channels.dvb");
	auto channel=channels.find(argv[1]);
	if(!channel.first) {
		std::cerr << "No such channel " << argv[1] << std::endl;
		return 1;
	}

	cards[0].tune(channel.first);
	cards[0].setup(*channel.second);

	std::cerr << "You can now grab data from /dev/dvb/adapter0/dvr0. Press any key when done." << std::endl;

	pollfd p;
	p.fd = 0;
	p.events = POLLIN;
	while(!poll(&p, 1, 100000)) {}

	return 0;
}
