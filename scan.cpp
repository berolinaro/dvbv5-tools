#include "DVBInterface.h"
#include <iostream>

int main(int argc, char **argv) {
	DVBCTransponder initial(618000000, 6900000, QAM_256, FEC_NONE);
	DVBInterfaces cards = DVBInterfaces::all();
	if(cards.size() == 0) {
		std::cerr << "No DVB interfaces found" << std::endl;
		return 1;
	}

	for(DVBInterface &c: cards) {
		c.tune(initial);
		std::vector<Transponder*> tp=c.scanTransponders();
		for(auto const &t: tp) {
			if(!c.tune(t)) {
				std::cerr << "Can't tune to transponder at frequency " << t->frequency() << " even though it's in the NIT" << std::endl;
				continue;
			}
			std::cerr << "Scanning transponder at frequency " << t->frequency() << std::endl;
			c.scanTransponder();
		}
	}
}
