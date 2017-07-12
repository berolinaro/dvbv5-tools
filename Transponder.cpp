#include "Transponder.h"

Transponder::Transponder() {
	_props.num = 0;
}

bool Transponder::operator ==(Transponder const &other) const {
	return getParameter(DTV_DELIVERY_SYSTEM) == other.getParameter(DTV_DELIVERY_SYSTEM) &&
		frequency() == other.frequency();
}

uint32_t Transponder::getParameter(uint32_t p) const {
	for(int i=0; i<_props.num; i++) {
		if(_props.props[i].cmd == p)
			return _props.props[i].u.data;
	}
	return 0;
}

DVBCTransponder::DVBCTransponder(uint32_t frequency, uint32_t srate, fe_modulation modulation, fe_code_rate fec, fe_spectral_inversion inversion):Transponder() {
	dtv_property *prop=new dtv_property[7];
	prop[0].cmd = DTV_FREQUENCY;
	prop[0].u.data = frequency;
	prop[1].cmd = DTV_MODULATION;
	prop[1].u.data = modulation;
	prop[2].cmd = DTV_INVERSION;
	prop[2].u.data = inversion;
	prop[3].cmd = DTV_SYMBOL_RATE;
	prop[3].u.data = srate;
	prop[4].cmd = DTV_INNER_FEC;
	prop[4].u.data = fec;
	prop[5].cmd = DTV_DELIVERY_SYSTEM;
	prop[5].u.data = SYS_DVBC_ANNEX_A;
	prop[6].cmd = DTV_TUNE;
	prop[6].u.data = 0;
	_props.num=7;
	_props.props=&prop[0];
}

DVBCTransponder::~DVBCTransponder() {
	delete[] _props.props;
}

DVBCTransponder::operator dtv_properties const *() const {
	return &_props;
}
