#pragma once
#include <iostream>
#include "Lnb.h"

extern "C" {
#include <linux/dvb/frontend.h>
#include <stdint.h>
}

class DVBInterface;

class Transponder {
public:
	static Transponder *fromString(std::string const &t);
	uint32_t frequency() const { return getParameter(DTV_FREQUENCY); }
	virtual operator dtv_properties const *() const { return &_props; }
	virtual bool operator ==(Transponder const &other) const;
	virtual std::string toString() const;
	virtual bool tune(DVBInterface * const i, uint32_t timeout = 0) const;
protected:
	Transponder();
public:
	~Transponder();
protected:
	uint32_t getParameter(uint32_t p) const;
protected:
	dtv_properties	_props;
};

std::ostream &operator <<(std::ostream &os, Transponder const &t);

class DVBCTransponder:public Transponder {
public:
	DVBCTransponder(uint32_t frequency, uint32_t srate, fe_modulation modulation=QAM_AUTO, fe_code_rate fec=FEC_AUTO, fe_spectral_inversion inversion=INVERSION_AUTO);
	std::string toString() const override;

	uint32_t symbolRate() const { return getParameter(DTV_SYMBOL_RATE); }
	fe_modulation modulation() const { return static_cast<fe_modulation>(getParameter(DTV_MODULATION)); }
	fe_code_rate fec() const { return static_cast<fe_code_rate>(getParameter(DTV_INNER_FEC)); }
	fe_spectral_inversion inversion() const { return static_cast<fe_spectral_inversion>(getParameter(DTV_INVERSION)); }
};

std::ostream &operator <<(std::ostream &os, DVBCTransponder const &t);

class DVBTTransponder:public Transponder {
public:
	DVBTTransponder(uint32_t frequency, uint32_t bandwidth=8000000, fe_code_rate codeRateHp=FEC_AUTO, int codeRateLp=FEC_AUTO, fe_transmit_mode mode=TRANSMISSION_MODE_AUTO, fe_guard_interval guardInterval=GUARD_INTERVAL_AUTO, fe_hierarchy hierarchy=HIERARCHY_AUTO, fe_modulation modulation=QAM_AUTO, fe_spectral_inversion inversion=INVERSION_AUTO);
	std::string toString() const override;

	uint32_t bandwidth() const { return getParameter(DTV_BANDWIDTH_HZ); }
	fe_code_rate codeRateHp() const { return static_cast<fe_code_rate>(getParameter(DTV_CODE_RATE_HP)); }
	fe_code_rate codeRateLp() const { return static_cast<fe_code_rate>(getParameter(DTV_CODE_RATE_LP)); }
	fe_transmit_mode transmissionMode() const { return static_cast<fe_transmit_mode>(getParameter(DTV_TRANSMISSION_MODE)); }
	fe_guard_interval guardInterval() const { return static_cast<fe_guard_interval>(getParameter(DTV_GUARD_INTERVAL)); }
	fe_hierarchy hierarchy() const { return static_cast<fe_hierarchy>(getParameter(DTV_HIERARCHY)); }
	fe_modulation modulation() const { return static_cast<fe_modulation>(getParameter(DTV_MODULATION)); }
	fe_spectral_inversion inversion() const { return static_cast<fe_spectral_inversion>(getParameter(DTV_INVERSION)); }
};

std::ostream &operator <<(std::ostream &os, DVBTTransponder const &t);

class DVBT2Transponder:public Transponder {
public:
	DVBT2Transponder(uint32_t frequency, uint32_t bandwidth=8000000, fe_code_rate codeRateHp=FEC_AUTO, int codeRateLp=FEC_AUTO, fe_transmit_mode mode=TRANSMISSION_MODE_AUTO, fe_guard_interval guardInterval=GUARD_INTERVAL_AUTO, fe_hierarchy hierarchy=HIERARCHY_AUTO, fe_modulation modulation=QAM_AUTO, fe_spectral_inversion inversion=INVERSION_AUTO);
	std::string toString() const override;

	uint32_t bandwidth() const { return getParameter(DTV_BANDWIDTH_HZ); }
	fe_code_rate codeRateHp() const { return static_cast<fe_code_rate>(getParameter(DTV_CODE_RATE_HP)); }
	fe_code_rate codeRateLp() const { return static_cast<fe_code_rate>(getParameter(DTV_CODE_RATE_LP)); }
	fe_transmit_mode transmissionMode() const { return static_cast<fe_transmit_mode>(getParameter(DTV_TRANSMISSION_MODE)); }
	fe_guard_interval guardInterval() const { return static_cast<fe_guard_interval>(getParameter(DTV_GUARD_INTERVAL)); }
	fe_hierarchy hierarchy() const { return static_cast<fe_hierarchy>(getParameter(DTV_HIERARCHY)); }
	fe_modulation modulation() const { return static_cast<fe_modulation>(getParameter(DTV_MODULATION)); }
	fe_spectral_inversion inversion() const { return static_cast<fe_spectral_inversion>(getParameter(DTV_INVERSION)); }
};

std::ostream &operator <<(std::ostream &os, DVBT2Transponder const &t);

class DVBSTransponder:public Transponder {
public:
	DVBSTransponder(uint32_t frequency, uint32_t srate, Lnb::Polarization polarization = Lnb::Horizontal, fe_code_rate fec=FEC_AUTO, fe_spectral_inversion inversion = INVERSION_AUTO);
	uint32_t symbolRate() const { return getParameter(DTV_SYMBOL_RATE); }
	fe_code_rate fec() const { return static_cast<fe_code_rate>(getParameter(DTV_INNER_FEC)); }
	Lnb::Polarization polarization() const { return _polarization; }
	fe_spectral_inversion inversion() const { return static_cast<fe_spectral_inversion>(getParameter(DTV_INVERSION)); }

	std::string toString() const override;
	bool tune(DVBInterface * const i, uint32_t timeout = 0) const override;
protected:
	Lnb::Polarization	_polarization;
};

std::ostream &operator <<(std::ostream &os, DVBSTransponder const &t);

class DVBS2Transponder:public DVBSTransponder {
public:
	DVBS2Transponder(uint32_t frequency, uint32_t srate, Lnb::Polarization polarization = Lnb::Horizontal, fe_code_rate fec=FEC_AUTO, fe_spectral_inversion inversion = INVERSION_AUTO);
	std::string toString() const override;
};

std::ostream &operator <<(std::ostream &os, DVBS2Transponder const &t);
