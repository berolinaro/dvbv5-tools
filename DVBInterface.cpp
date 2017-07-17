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

DVBInterface::DVBInterface(int num, std::string const devPath):_frontendFd(-1),_dmxFd(-1),_currentTransponder(nullptr) {
	_devPath = devPath;
	if(_devPath[_devPath.length()-1] != '/')
		_devPath += '/';
	_devPath += "adapter" + std::to_string(num) + "/";

	for(int i=0; i<DMX_PES_OTHER+1; i++)
		_pesFd[i] = -1;

	memset(&_feInfo, 0, sizeof(dvb_frontend_info));

	FD fd = open("frontend0", O_RDWR);
	if(fd >= 0) {
		ioctl(fd, FE_GET_INFO, &_feInfo);
	}
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
	FD fd = open("frontend0", O_RDWR);
	if(fd >= 0)
		return !ioctl(fd, FE_DISEQC_RESET_OVERLOAD);
	return false;
}

bool DVBInterface::tune(Transponder const &t, uint32_t timeout) {
	dtv_properties const *p = t;
	if(_frontendFd < 0)
		_frontendFd = open("frontend0", O_RDWR);
	if(_frontendFd < 0)
		return false;

	if(ioctl(_frontendFd, FE_SET_PROPERTY, p))
		return false;

	if(timeout == 0)
		return true;

	if(Util::waitFor(timeout, [this]() { auto s=status(); return s.test(Signal) && s.test(Lock); })) {
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
	if(_dmxFd >= 0) {
		::close(_dmxFd);
		_dmxFd = -1;
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
	if(_dmxFd < 0)
		_dmxFd = open("demux0", O_RDWR|O_NONBLOCK);
	if(_dmxFd < 0)
		return std::vector<Service>();
	ProgramAssociationTables *pats = DVBTables<ProgramAssociationTable>::read<ProgramAssociationTables>(_dmxFd);
	if(!pats) // Bogus transponder didn't even send a PAT on time
		return std::vector<Service>();

	cerr << "Transport stream ID " << (*pats->begin())->number() << std::endl;

	std::map<uint16_t,uint16_t> PMTPids = pats->pids();
	delete pats;
	std::vector<Program> programs;
	for(auto const &p: PMTPids) {
		if(p.first == 0) continue;
		ProgramMapTables *pmts = DVBTables<ProgramMapTable>::read<ProgramMapTables>(_dmxFd, p.second);
		if(!pmts) // Dropped w/ timeout or other error
			continue;
		programs.push_back(Program(*pmts));
		delete pmts;
	}

	for(auto const &p: programs)
		p.dump(std::cerr);

	ServiceDescriptionTables *sdts = DVBTables<ServiceDescriptionTable>::read<ServiceDescriptionTables>(_dmxFd);
	std::vector<Service> services=sdts->services();
	delete sdts;
	return services;
}

void DVBInterface::scan() {
	if(_dmxFd < 0)
		_dmxFd = open("demux0", O_RDWR|O_NONBLOCK);
	if(_dmxFd < 0)
		return;
	NetworkInformationTables *nits = DVBTables<NetworkInformationTable>::read<NetworkInformationTables>(_dmxFd);
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
	if(_dmxFd < 0)
		_dmxFd = open("demux0", O_RDWR|O_NONBLOCK);
	ProgramAssociationTables *pats = DVBTables<ProgramAssociationTable>::read<ProgramAssociationTables>(_dmxFd);
	std::map<uint16_t,uint16_t> PMTPids = pats->pids();
	auto pmtpid = PMTPids.find(s.serviceId());
	if(pmtpid == PMTPids.end()) {
		std::cerr << "Service ID not found in PAT" << std::endl;
		return false;
	}
	ProgramMapTables *pmts = DVBTables<ProgramMapTable>::read<ProgramMapTables>(_dmxFd, (*pmtpid).second);
	if(!pmts) {
		delete pats;
		std::cerr << "No PMT" << std::endl;
		return false;
	}

	Program p(*pmts);

	dmx_pes_filter_params f;
	f.pid = p.pcrPid();
	f.input = DMX_IN_FRONTEND;
	f.output = DMX_OUT_TS_TAP;
	f.pes_type = DMX_PES_PCR0;
	f.flags = DMX_CHECK_CRC|DMX_IMMEDIATE_START;
	openPES(f.pes_type);
	ioctl(_pesFd[f.pes_type], DMX_SET_PES_FILTER, &f);

	dmx_pes_type_t audioFilter = DMX_PES_AUDIO0,
		videoFilter = DMX_PES_VIDEO0,
		teletextFilter = DMX_PES_TELETEXT0,
		subtitleFilter = DMX_PES_SUBTITLE0,
		pcrFilter = DMX_PES_PCR1;

	for(auto s: p.streams()) {
		// The code below assumes that dmx_pes_type_t is a list of
		// audio, video, teletext, subtitle, pcr, audio, video, ...
		// If the layout of dmx_pes_type_t ever changes in the kernel,
		// we need to make some adjustments here.
		// In the mean time, this calculation is better than going from
		// hardcoded DMX_PES_AUDIO0 to DMX_PES_AUDIO1 to DMX_PES_AUDIO2
		// etc. -- our way automatically supports DMX_PES_AUDIO4 etc.
		// if the API is ever extended to handle more than 4 concurrent
		// streams of each type.
		if(s.isAudio()) {
			std::cerr << "Audio PID " << s.pid() << std::endl;
			if(audioFilter > DMX_PES_OTHER)
				continue;
			f.pes_type = audioFilter;
			audioFilter = static_cast<dmx_pes_type_t>(audioFilter+DMX_PES_AUDIO1-DMX_PES_AUDIO0);
		} else if(s.isVideo()) {
			std::cerr << "Video PID " << s.pid() << std::endl;
			if(videoFilter > DMX_PES_OTHER)
				continue;
			f.pes_type = videoFilter;
			videoFilter = static_cast<dmx_pes_type_t>(videoFilter+DMX_PES_VIDEO1-DMX_PES_VIDEO0);
		} else if(s.isTeletext()) {
			std::cerr << "Teletext PID " << s.pid() << std::endl;
			if(teletextFilter > DMX_PES_OTHER)
				continue;
			f.pes_type = teletextFilter;
			teletextFilter = static_cast<dmx_pes_type_t>(teletextFilter+DMX_PES_TELETEXT1-DMX_PES_TELETEXT0);
		} else if(s.isSubtitle()) {
			std::cerr << "Subtitle PID " << s.pid() << std::endl;
			if(subtitleFilter > DMX_PES_OTHER)
				continue;
			f.pes_type = subtitleFilter;
			subtitleFilter = static_cast<dmx_pes_type_t>(subtitleFilter+DMX_PES_SUBTITLE1-DMX_PES_SUBTITLE0);
		} else if(s.isPcr()) {
			std::cerr << "PCR PID " << s.pid() << std::endl;
			if(pcrFilter > DMX_PES_OTHER)
				continue;
			f.pes_type = pcrFilter;
			pcrFilter = static_cast<dmx_pes_type_t>(pcrFilter+DMX_PES_PCR1-DMX_PES_PCR0);
		} else {
			std::cerr << "Other PID (type " << static_cast<int>(s.streamType()) << ") " << s.pid() << std::endl;
			f.pes_type = DMX_PES_OTHER;
		}
		f.pid = s.pid();
		if(_pesFd[f.pes_type]>=0)
			continue;
		openPES(f.pes_type);
		if(_pesFd[f.pes_type] < 0)
			std::cerr << strerror(errno) << std::endl;
		ioctl(_pesFd[f.pes_type], DMX_SET_PES_FILTER, &f);
	}
	return true;
}

int DVBInterface::openPES(dmx_pes_type_t pes) {
	if(_pesFd[pes]>=0)
		::close(_pesFd[pes]);
	return _pesFd[pes] = open("demux0", O_RDWR);
}

std::vector<Transponder*> DVBInterface::scanTransponders() {
	return DVBTables<NetworkInformationTable>::read<NetworkInformationTables>(FD(open("demux0", O_RDWR|O_NONBLOCK)))->transponders();
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
