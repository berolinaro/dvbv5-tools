#pragma once

#include <string>
#include <vector>
#include <bitset>
#include <cmath>
#include "FD.h"
#include "Transponder.h"
#include "Service.h"

extern "C" {
#include <linux/dvb/frontend.h>
#include <linux/dvb/dmx.h>
#include <fcntl.h>
}

/**
 * PIDs (Program Identifiers) of standard tables that are parts
 * of the DVB specs
 */
enum PIDs {
	/// Program Association Table, ETSI EN 300 468 v1.15.1
	PAT = 0x0000,
	/// Conditional Access Table (optional), ETSI EN 300 468 v1.15.1
	CAT = 0x0001,
	/// Transport Streams Description Table, ETSI EN 300 468 v1.15.1
	TSDT = 0x0002,
	/// Network Information Table, ETSI EN 300 468 v1.15.1
	NIT = 0x0010,
	/// Time and Date Table, ETSI EN 300 468 v1.15.1
	TDT = 0x0014,
	/// Service Description Table, ETSI EN 300 468 v1.15.1
	SDT = 0x0011,
	/// Event Information Table (optional), ETSI EN 300 468 v1.15.1
	EIT = 0x0012,
	/// Running Status Table (optional), ETSI EN 300 468 v1.15.1
	RST = 0x0013
};

class DVBInterface {
public:
	DVBInterface(int num=0, std::string const devPath="/dev/dvb/");
	bool exists() const { return *_feInfo.name; }
	std::string name() const { return _feInfo.name; }
	uint32_t minFrequency() const { return _feInfo.frequency_min; }
	uint32_t maxFrequency() const { return _feInfo.frequency_max; }
	uint32_t freqStepSize() const { return _feInfo.frequency_stepsize; }
	uint32_t freqTolerance() const { return _feInfo.frequency_tolerance; }
	uint32_t srateMin() const { return _feInfo.symbol_rate_min; }
	uint32_t srateMax() const { return _feInfo.symbol_rate_max; }
	uint32_t srateTolerance() const { return _feInfo.symbol_rate_tolerance; }
	std::bitset<30> caps() const { return _feInfo.caps; }
	enum Caps { // Must be kept in sync with fe_caps -- for each entry in fe_caps, Caps has the log2 value
		AutoInversion = 0,
		FEC1_2 = 1,
		FEC2_3 = 2,
		FEC3_4 = 3,
		FEC4_5 = 4,
		FEC5_6 = 5,
		FEC6_7 = 6,
		FEC7_8 = 7,
		FEC8_9 = 8,
		AutoFEC = 9,
		QPSK = 10,
		QAM_16 = 11,
		QAM_32 = 12,
		QAM_64 = 13,
		QAM_128 = 14,
		QAM_256 = 15,
		AutoQAM = 16,
		AutoTransmissionMode=17,
		AutoBandwidth=18,
		AutoGuardInterval=19,
		AutoHierarchy=20,
		VSB8=21,
		VSB16=22,
		//ExtendedCaps=23, (Not relevant to us - we can just extend the bitset on overflow)
		Multistream=24,
		TurboFEC=25,
		Modulation2G=26,
		//NeedsBending=27,
		Recover=28,
		MuteTS=29
	};
	bool canAutoInversion() const { return caps().test(AutoInversion); }
	bool canFEC1_2() const { return caps().test(FEC1_2); }
	bool canFEC2_3() const { return caps().test(FEC2_3); }
	bool canFEC3_4() const { return caps().test(FEC3_4); }
	bool canFEC4_5() const { return caps().test(FEC4_5); }
	bool canFEC5_6() const { return caps().test(FEC5_6); }
	bool canFEC6_7() const { return caps().test(FEC6_7); }
	bool canFEC7_8() const { return caps().test(FEC7_8); }
	bool canFEC8_9() const { return caps().test(FEC8_9); }
	bool canFECAuto() const { return caps().test(AutoFEC); }
	bool canTurboFEC() const { return caps().test(TurboFEC); }
	std::string FEC() const;
	bool canQPSK() const { return caps().test(QPSK); }
	bool canQAM16() const { return caps().test(QAM_16); }
	bool canQAM32() const { return caps().test(QAM_32); }
	bool canQAM64() const { return caps().test(QAM_64); }
	bool canQAM128() const { return caps().test(QAM_128); }
	bool canQAM256() const { return caps().test(QAM_256); }
	bool canQAMAuto() const { return caps().test(AutoQAM); }
	std::string QAM() const;
	bool canTransmissionModeAuto() const { return caps().test(AutoTransmissionMode); }
	bool canBandwidthAuto() const { return caps().test(AutoBandwidth); }
	bool canGuardIntervalAuto() const { return caps().test(AutoGuardInterval); }
	bool canHierarchyAuto() const { return caps().test(AutoHierarchy); }
	bool can8VSB() const { return caps().test(VSB8); }
	bool can16VSB() const { return caps().test(VSB16); }
	bool canMultistream() const { return caps().test(Multistream); }
	bool can2GModulation() const { return caps().test(Modulation2G); }
	bool canRecover() const { return caps().test(Recover); }
	bool canMuteTs() const { return caps().test(MuteTS); }
	enum Status { // Must be kept in sync with fe_status -- for each entry in fe_status, Status has the log2 value
		Signal = 0,
		Carrier = 1,
		Viterbi = 2,
		Sync = 3,
		Lock = 4,
		TimedOut = 5,
		ReInit = 6
	};
	std::bitset<6> status() const;
	bool hasSignal() const { return status().test(Signal); }
	bool hasCarrier() const { return status().test(Carrier); }
	bool hasViterbi() const { return status().test(Viterbi); }
	bool hasSync() const { return status().test(Sync); }
	bool hasLock() const { return status().test(Lock); }
	bool timedOut() const { return status().test(TimedOut); }
	bool needsReInit() const { return status().test(ReInit); }
	bool resetDiseqcOverload() const;
	bool tune(Transponder const &t, uint32_t timeout = 0);
	bool tune(Transponder const * const t, uint32_t timeout = 0) { return tune(*t, timeout); }
	void close();
	uint32_t currentFrequency() const { return _currentTransponder ? _currentTransponder->frequency() : 0; }
	/**
	 * Set up the DVR device for retrieving the specified channel
	 */
	bool setup(Service const &s);

	std::vector<Transponder*> scanTransponders();

	void scan();
	std::vector<Service> scanTransponder();
protected:
	int open(std::string const &dev, int mode = O_RDONLY) const;
	int openPES(dmx_pes_type_t pes);
protected:
	std::string _devPath;
	int _dmxFd;
	int _frontendFd;
	int _pesFd[DMX_PES_OTHER+1];
	dvb_frontend_info _feInfo;
	Transponder const *_currentTransponder;
};

class DVBInterfaces:public std::vector<DVBInterface> {
public:
	static DVBInterfaces all();
};
