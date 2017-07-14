#pragma once
#include <iostream>

extern "C" {
#include <linux/dvb/frontend.h>
#include <stdint.h>
}

class Transponder {
public:
	Transponder();
	uint32_t frequency() const { return getParameter(DTV_FREQUENCY); }
	virtual operator dtv_properties const *() const = 0;
	virtual bool operator ==(Transponder const &other) const;
	virtual std::string toString() const;
protected:
	uint32_t getParameter(uint32_t p) const;
protected:
	dtv_properties	_props;
};

std::ostream &operator <<(std::ostream &os, Transponder const &t);

class DVBCTransponder:public Transponder {
public:
	DVBCTransponder(uint32_t frequency, uint32_t srate, fe_modulation modulation=QAM_AUTO, fe_code_rate fec=FEC_AUTO, fe_spectral_inversion inversion=INVERSION_AUTO);
	~DVBCTransponder();
	operator dtv_properties const *() const override;
	std::string toString() const override;

	uint32_t symbolRate() const { return getParameter(DTV_SYMBOL_RATE); }
	fe_modulation modulation() const { return static_cast<fe_modulation>(getParameter(DTV_MODULATION)); }
	fe_code_rate fec() const { return static_cast<fe_code_rate>(getParameter(DTV_INNER_FEC)); }
	fe_spectral_inversion inversion() const { return static_cast<fe_spectral_inversion>(getParameter(DTV_INVERSION)); }
};

std::ostream &operator <<(std::ostream &os, DVBCTransponder const &t);
