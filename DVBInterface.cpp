#define DEBUG_TUNING 1

#include "DVBInterface.h"
#include "DVBTable.h"
#include "ProgramAssociationTable.h"
#include "ProgramMapTable.h"
#include "ServiceDescriptionTable.h"
#include "NetworkInformationTable.h"
#include "Util.h"
#include <cstring>

extern "C" {
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <unistd.h>
#include <linux/dvb/dmx.h>
}

DVBInterface::DVBInterface(int num, std::string const devPath):_frontendFd(-1),_currentTransponder(nullptr),_lnb(nullptr) {
	_devPath = devPath;
	if(_devPath[_devPath.length()-1] != '/')
		_devPath += '/';
	_devPath += "adapter" + std::to_string(num) + "/";

	for(int i=0; i<DMX_PES_OTHER+1; i++)
		_pesFd[i] = -1;

	memset(&_feInfo, 0, sizeof(dvb_frontend_info));

	int fd = open("frontend0", O_RDWR);
	if(fd >= 0) {
		ioctl(fd, FE_GET_INFO, &_feInfo);
		::close(fd);
	}

	if(canQPSK())
		_lnb = &Lnb::Universal;
}

std::string DVBInterface::FEC() const {
	std::string ret;
	if(canFEC1_2())
		ret += " 1/2";
	if(canFEC2_3())
		ret += " 2/3";
	if(canFEC3_4())
		ret += " 3/4";
	if(canFEC4_5())
		ret += " 4/5";
	if(canFEC5_6())
		ret += " 5/6";
	if(canFEC6_7())
		ret += " 6/7";
	if(canFEC7_8())
		ret += " 7/8";
	if(canFEC8_9())
		ret += " 8/9";
	if(canFECAuto())
		ret += " Auto";
	if(canTurboFEC())
		ret += " TurboFEC";
	return ret.size() ? ret.substr(1) : ret;
}

std::string DVBInterface::QAM() const {
	std::string ret;
	if(canQAM16())
		ret += " 16-QAM";
	if(canQAM32())
		ret += " 32-QAM";
	if(canQAM64())
		ret += " 64-QAM";
	if(canQAM128())
		ret += " 128-QAM";
	if(canQAM256())
		ret += " 256-QAM";
	if(canQAMAuto())
		ret += " Auto";
	return ret.size() ? ret.substr(1) : ret;
}

std::bitset<6> DVBInterface::status() const {
	fe_status s;

	int fd = (_frontendFd >= 0) ? _frontendFd : open("frontend0", O_RDONLY);
	if(fd >= 0)
		ioctl(fd, FE_READ_STATUS, &s);

	return s;
}

int DVBInterface::open(std::string const &dev, int mode) const {
	return ::open((_devPath + dev).c_str(), mode);
}

bool DVBInterface::resetDiseqcOverload() const {
	return !ioctl(frontendFd(), FE_DISEQC_RESET_OVERLOAD);
}

bool DVBInterface::tune(Transponder const &t, uint32_t timeout) {
	dtv_properties const *p = t;

	if(t.tune(this, timeout)) {
		_currentTransponder = &t;
		return true;
	}
	return false;
}

void DVBInterface::close() {
	if(_frontendFd >= 0) {
		::close(_frontendFd);
		_frontendFd = -1;
	}
	for(int i=0; i<DMX_PES_OTHER+1; i++) {
		if(_pesFd[i] >= 0) {
			::close(_pesFd[i]);
			_pesFd[i] = -1;
		}
	}
}

#include <iostream>
using namespace std;
#include <errno.h>
#include <cassert>

std::vector<Service> DVBInterface::scanTransponder() {
	int dmx = open("demux0", O_RDWR|O_NONBLOCK);
	if(dmx < 0)
		return std::vector<Service>();
#if 0
	// Code kept here for reference for now
	// We don't need to scan the PAT and PMT before
	// knowing what channel we want.
	// May want to fall back to PAT/PMT scanning if there's
	// no proper SDT or if we find any service IDs not mentioned
	// in the SDT.
	ProgramAssociationTables *pats = DVBTables<ProgramAssociationTable>::read<ProgramAssociationTables>(dmx);
	if(!pats) // Bogus transponder didn't even send a PAT on time
		return std::vector<Service>();

	cerr << "Transport stream ID " << (*pats->begin())->number() << std::endl;

	std::map<uint16_t,uint16_t> PMTPids = pats->pids();
	delete pats;
	std::vector<Program> programs;
	for(auto const &p: PMTPids) {
		if(p.first == 0) continue;
		ProgramMapTables *pmts = DVBTables<ProgramMapTable>::read<ProgramMapTables>(dmx, p.second);
		if(!pmts) // Dropped w/ timeout or other error
			continue;
		programs.push_back(Program(*pmts));
		delete pmts;
	}

	for(auto const &p: programs)
		p.dump(std::cerr);
#endif

	ServiceDescriptionTables *sdts = DVBTables<ServiceDescriptionTable>::read<ServiceDescriptionTables>(dmx);
	::close(dmx);
	if(!sdts)
		return std::vector<Service>();
	std::vector<Service> services=sdts->services();
	delete sdts;
	return services;
}

void DVBInterface::scan() {
	int dmx = open("demux0", O_RDWR|O_NONBLOCK);
	if(dmx < 0)
		return;
	NetworkInformationTables *nits = DVBTables<NetworkInformationTable>::read<NetworkInformationTables>(dmx, nitPID());
	::close(dmx);
	if(!nits)
		return;
	nits->dump();
	std::vector<Transponder*> t=nits->transponders();
	for(auto const &tp: t) {
		if(tune(tp, 5000000))
			std::cerr << "Found transponder at " << tp->frequency() << std::endl;
		else
			std::cerr << "Transponder at " << tp->frequency() << " is listed in NIT, but doesn't seem to exist" << std::endl;
	}
}

bool DVBInterface::setup(Service const &s) {
	int dmx = open("demux0", O_RDWR|O_NONBLOCK);
	ProgramAssociationTables *pats = DVBTables<ProgramAssociationTable>::read<ProgramAssociationTables>(dmx);
	::close(dmx);
	if(!pats)
		return false;
	std::map<uint16_t,uint16_t> PMTPids = pats->pids();
	auto pmtpid = PMTPids.find(s.serviceId());
	if(pmtpid == PMTPids.end()) {
		std::cerr << "Service ID not found in PAT" << std::endl;
		return false;
	}
	dmx = open("demux0", O_RDWR|O_NONBLOCK);
	ProgramMapTables *pmts = DVBTables<ProgramMapTable>::read<ProgramMapTables>(dmx, (*pmtpid).second);
	::close(dmx);
	if(!pmts) {
		delete pats;
		std::cerr << "No PMT" << std::endl;
		return false;
	}

	Program p(*pmts);
	p.dump();

	addPES(Stream::PCR, p.pcrPid());
	for(auto s: p.streams())
		addPES(s.type(), s.pid());
	return true;
}

bool DVBInterface::addPES(Stream::StreamType t, uint16_t pid, bool fallbackToAny) {
	// The code below assumes that dmx_pes_type_t is a list of
	// audio, video, teletext, subtitle, pcr, audio, video, ...
	// If the layout of dmx_pes_type_t ever changes in the kernel,
	// we need to make some adjustments here.
	// In the mean time, this calculation is better than going from
	// hardcoded DMX_PES_AUDIO0 to DMX_PES_AUDIO1 to DMX_PES_AUDIO2
	// etc. -- our way automatically supports DMX_PES_AUDIO4 etc.
	// if the API is ever extended to handle more than 4 concurrent
	// streams of each type.
	dmx_pes_filter_params f;
	f.pid = pid;
	f.input = DMX_IN_FRONTEND;
	f.output = DMX_OUT_TS_TAP;
	f.pes_type = DMX_PES_PCR0;
	f.flags = DMX_CHECK_CRC|DMX_IMMEDIATE_START;
	switch(t) {
	case Stream::Audio:
		f.pes_type = DMX_PES_AUDIO0;
		while(f.pes_type<=DMX_PES_OTHER && _pesFd[f.pes_type]>=0)
			f.pes_type = static_cast<dmx_pes_type_t>(f.pes_type+DMX_PES_AUDIO1-DMX_PES_AUDIO0);
		break;
	case Stream::Video:
		f.pes_type = DMX_PES_VIDEO0;
		while(f.pes_type<=DMX_PES_OTHER && _pesFd[f.pes_type]>=0)
			f.pes_type = static_cast<dmx_pes_type_t>(f.pes_type+DMX_PES_VIDEO1-DMX_PES_VIDEO0);
		break;
	case Stream::Teletext:
		f.pes_type = DMX_PES_TELETEXT0;
		while(f.pes_type<=DMX_PES_OTHER && _pesFd[f.pes_type]>=0)
			f.pes_type = static_cast<dmx_pes_type_t>(f.pes_type+DMX_PES_TELETEXT1-DMX_PES_TELETEXT0);
		break;
	case Stream::Subtitle:
		f.pes_type = DMX_PES_SUBTITLE0;
		while(f.pes_type<=DMX_PES_OTHER && _pesFd[f.pes_type]>=0)
			f.pes_type = static_cast<dmx_pes_type_t>(f.pes_type+DMX_PES_SUBTITLE1-DMX_PES_SUBTITLE0);
		break;
	case Stream::PCR:
		f.pes_type = DMX_PES_PCR0;
		while(f.pes_type<=DMX_PES_OTHER && _pesFd[f.pes_type]>=0)
			f.pes_type = static_cast<dmx_pes_type_t>(f.pes_type+DMX_PES_PCR1-DMX_PES_PCR0);
		break;
	case Stream::Other:
		if(_pesFd[DMX_PES_OTHER]>=0)
			f.pes_type = static_cast<dmx_pes_type_t>(DMX_PES_OTHER+1);
		else
			f.pes_type = DMX_PES_OTHER;
		break;
	case Stream::Any:
		break;
	}
	if(t == Stream::Any ||
			(fallbackToAny &&
			 (f.pes_type > DMX_PES_OTHER ||
			  _pesFd[f.pes_type]>=0))) {
		f.pes_type = static_cast<dmx_pes_type_t>(DMX_PES_OTHER+1);
		for(int i=0; i<=DMX_PES_OTHER; i++)
			if(_pesFd[i]<0)
				f.pes_type = static_cast<dmx_pes_type_t>(i);
	}
	if(f.pes_type > DMX_PES_OTHER)
		return false;
	openPES(f.pes_type);
	ioctl(_pesFd[f.pes_type], DMX_SET_PES_FILTER, &f);
	return true;
}

int DVBInterface::openPES(dmx_pes_type_t pes) {
	if(_pesFd[pes]>=0) {
		ioctl(_pesFd[pes], DMX_STOP);
		::close(_pesFd[pes]);
	}
	return _pesFd[pes] = open("demux0", O_RDWR);
}

uint16_t DVBInterface::nitPID() const {
	int dmx = open("demux0", O_RDWR|O_NONBLOCK);
	ProgramAssociationTables *pats = DVBTables<ProgramAssociationTable>::read<ProgramAssociationTables>(dmx);
	::close(dmx);
	if(!pats)
		return NIT;
	std::map<uint16_t,uint16_t> PMTPids = pats->pids();
	if(PMTPids.count(0))
		return PMTPids[0];
	return NIT;
}

std::vector<Transponder*> DVBInterface::scanTransponders() {
	int dmx = open("demux0", O_RDWR|O_NONBLOCK);
	std::cerr << "Reading NIT" << std::endl;
	auto nit = DVBTables<NetworkInformationTable>::read<NetworkInformationTables>(dmx, nitPID());
	::close(dmx);
	if(!nit) {
		std::cerr << "No NIT" << std::endl;
		return std::vector<Transponder*>();
	}
	std::cerr << "Looking at transponders" << std::endl;
	return nit->transponders();
}

/* More Diseqc bits not yet supported (no satellite dish to test with...):
 * https://linuxtv.org/downloads/v4l-dvb-apis-new/uapi/dvb/fe-diseqc-send-master-cmd.html
 * https://linuxtv.org/downloads/v4l-dvb-apis-new/uapi/dvb/fe-diseqc-recv-slave-reply.html
 * https://linuxtv.org/downloads/v4l-dvb-apis-new/uapi/dvb/fe-diseqc-send-burst.html
 * https://linuxtv.org/downloads/v4l-dvb-apis-new/uapi/dvb/fe-set-tone.html
 * https://linuxtv.org/downloads/v4l-dvb-apis-new/uapi/dvb/fe-set-voltage.html
 * https://linuxtv.org/downloads/v4l-dvb-apis-new/uapi/dvb/fe-enable-high-lnb-voltage.html
 * https://linuxtv.org/downloads/v4l-dvb-apis-new/uapi/dvb/fe-set-frontend-tune-mode.html
 *
 * Obscure features not yet supported:
 * https://linuxtv.org/downloads/v4l-dvb-apis-new/uapi/dvb/fe-set-frontend-tune-mode.html
 */

/*
 * ioctl(fd, FE_GET_PROPERTY, struct dtv_properties *arg);
 * ioctl(fd, FE_SET_PROPERTY, struct dtv_properties *arg);
 * dtv_properties:
 * 	uint32_t num
 * 	struct dtv_property *props
 */

DVBInterfaces DVBInterfaces::all() {
	DVBInterfaces cards;
	for(int i=0; ; i++) {
		DVBInterface c = DVBInterface(i);
		if(!c.exists())
			break;
		cards.push_back(c);
	}
	return cards;
}
