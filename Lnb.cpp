#include "Lnb.h"
#include "Transponder.h"

uint32_t Lnb::frequencyOffset(DVBSTransponder const &t) const {
	for(auto f: *this) {
		if(t.frequency() >= f.low && t.frequency() <= f.high && (f.polarizations&t.polarization()))
			return f.offset;
	}
	return 0;
}

const Lnb Lnb::Universal = std::vector<FrequencyRange>{
	{ 10800000, 11800000, 9750000, 11700000, Lnb::Any },
	{ 11600000, 12700000, 10600000, 0, Lnb::Any }
};
const Lnb Lnb::Expressvu = std::vector<FrequencyRange>{
	{ 12200000, 12700000, 11250000, 0, Lnb::Any }
};
const Lnb Lnb::Extended = std::vector<FrequencyRange>{
	{ 10700000, 11700000, 0, 0, Lnb::Any },
	{ 11700000, 12750000, 0, 0, Lnb::Any }
};
const Lnb Lnb::Standard = std::vector<FrequencyRange>{
	{ 10945000, 11450000, 10000000, 0, Lnb::Any }
};
const Lnb Lnb::L10700 = std::vector<FrequencyRange>{
	{ 11750000, 12750000, 10700000, 0, Lnb::Any }
};
const Lnb Lnb::L11300 = std::vector<FrequencyRange>{
	{ 12250000, 12750000, 11300000, 0, Lnb::Any }
};
const Lnb Lnb::Enhanced = std::vector<FrequencyRange>{
	{ 10700000, 11700000, 9750000, 0, Lnb::Any }
};
const Lnb Lnb::QPH031 = std::vector<FrequencyRange>{
	{ 11700000, 12200000, 10750000, 12200000, Lnb::Any },
	{ 12200000, 12700000, 11250000, 0, Lnb::Any }
};
const Lnb Lnb::CBand = std::vector<FrequencyRange>{
	{ 3700000, 4200000, 5150000, 0, Lnb::Any }
};
const Lnb Lnb::CBandMultipoint = std::vector<FrequencyRange>{
	{ 3700000, 4200000, 5150000, 0, Lnb::Right },
	{ 3700000, 4200000, 5750000, 0, Lnb::Left }
};
const Lnb Lnb::DishPro = std::vector<FrequencyRange>{
	{ 12200000, 12700000, 11250000, 0, Lnb::Vertical },
	{ 12200000, 12700000, 14350000, 0, Lnb::Horizontal },
};
const Lnb Lnb::L110BS = std::vector<FrequencyRange>{
	{ 11710000, 12751000, 10678000, 0, Lnb::Any }
};
const Lnb Lnb::BrasilSatStacked = std::vector<FrequencyRange>{
	{ 10700000, 11700000, 9710000, 0, Lnb::Horizontal },
	{ 10700000, 11700000, 9750000, 0, Lnb::Horizontal }
};
const Lnb Lnb::BrasilSatOi = std::vector<FrequencyRange>{
	{ 10950000, 11200000, 10000000, 11700000, Lnb::Any },
	{ 11800000, 12200000, 10445000, 0, Lnb::Any }
};
const Lnb Lnb::BrasilSatAmazonas3 = std::vector<FrequencyRange>{
	{ 11037000, 11450000, 9670000, 0, Lnb::Vertical },
	{ 11770000, 12070000, 9922000, 0, Lnb::Horizontal },
	{ 10950000, 11280000, 10000000, 0, Lnb::Horizontal }
};
const Lnb Lnb::BrasilSatAmazonas2 = std::vector<FrequencyRange>{
	{ 11037000, 11360000, 9670000, 0, Lnb::Vertical },
	{ 11780000, 12150000, 10000000, 0, Lnb::Horizontal },
	{ 10950000, 11280000, 10000000, 0, Lnb::Horizontal }
};
const Lnb Lnb::BrasilSatGVT = std::vector<FrequencyRange>{
	{ 11010500, 11067500, 12860000, 0, Lnb::Vertical },
	{ 11704000, 11941000, 13435000, 0, Lnb::Vertical },
	{ 10962500, 11199500, 13112000, 0, Lnb::Horizontal },
	{ 11704000, 12188000, 13138000, 0, Lnb::Horizontal }
};
