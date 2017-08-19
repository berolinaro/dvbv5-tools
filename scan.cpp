#include "DVBInterface.h"
#include "Service.h"
#include <iostream>

int main(int argc, char **argv) {
//	DVBCTransponder initial(618000000, 6900000, QAM_256, FEC_NONE);
//	DVBTTransponder initial(530000000, 8000000, FEC_2_3, FEC_AUTO, TRANSMISSION_MODE_8K, GUARD_INTERVAL_1_4, HIERARCHY_NONE, QAM_16, INVERSION_AUTO);
//	DVBT2Transponder initial(506000000, 8000000);
	DVBSTransponder initial(12551500, 22000000, Lnb::Vertical, FEC_5_6, INVERSION_AUTO);
	DVBInterfaces cards = DVBInterfaces::all();
	if(cards.size() == 0) {
		std::cerr << "No DVB interfaces found" << std::endl;
		return 1;
	}

	for(DVBInterface &c: cards) {
		if(!c.tune(initial))
			std::cerr << "Can't tune to initial transponder " << initial.frequency() << std::endl;
		std::vector<Transponder*> tp=c.scanTransponders();
		// If we didn't get an NIT, the initial transponder is better than nothing...
		if(tp.size() == 0) {
			std::cerr << "No NIT on initial transponder" << std::endl;
			tp.insert(tp.end(), &initial);
		}
		for(auto const &t: tp) {
			std::cout << t->toString() << std::endl;
		}
		for(auto const &t: tp) {
			if(!c.tune(t)) {
				std::cerr << "Can't tune to transponder at frequency " << t->frequency() << " even though it's in the NIT" << std::endl;
				continue;
			}

			std::cerr << "Scanning transponder at frequency " << t->frequency() << std::endl;
			std::vector<Service> srv = c.scanTransponder();
			for(auto const &s: srv) {
				std::cout << t->frequency() << "\t" << s.name() << "\t" << s.providerName() << "\t" << s.serviceId() << "\t" << s.serviceType() << std::endl;
			}
		}
	}
	return 0;
}
