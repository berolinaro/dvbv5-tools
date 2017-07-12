#include "DVBInterface.h"
#include "DVBTable.h"
#include "ProgramAssociationTable.h"
#include "ProgramMapTable.h"
#include "ServiceDescriptionTable.h"
#include "Util.h"
#include <cstring>

extern "C" {
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <unistd.h>
#include <linux/dvb/dmx.h>
}

DVBInterface::DVBInterface(int num, std::string const devPath):_frontendFd(-1) {
	_devPath = devPath;
	if(_devPath[_devPath.length()-1] != '/')
		_devPath += '/';
	_devPath += "adapter" + std::to_string(num) + "/";

	memset(&_feInfo, 0, sizeof(dvb_frontend_info));

	FD fd = open("frontend0");
	if(fd >= 0) {
		ioctl(fd, FE_GET_INFO, &_feInfo);
	}
}

FD DVBInterface::open(std::string const &dev, int mode) const {
	return FD(_devPath + dev, mode);
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

	int fd = (_frontendFd >= 0) ? _frontendFd : open("frontend0");
	if(fd >= 0)
		ioctl(fd, FE_READ_STATUS, &s);

	return s;
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
		_frontendFd = ::open((_devPath + "frontend0").c_str(), O_RDWR);
	if(_frontendFd < 0)
		return false;

	if(ioctl(_frontendFd, FE_SET_PROPERTY, p))
		return false;

	if(timeout == 0)
		return true;

	return Util::waitFor(timeout, [this]() { auto s=status(); return s.test(Signal) && s.test(Lock); });
}

void DVBInterface::close() {
	if(_frontendFd >= 0) {
		::close(_frontendFd);
		_frontendFd = -1;
	}
}

#include <iostream>
using namespace std;
#include <errno.h>
#include <cassert>

void DVBInterface::scan() {
	FD dmxFd = open("demux0", O_RDWR);
	if(dmxFd<0)
		return;
	ProgramAssociationTables *pats = DVBTables<ProgramAssociationTable>::read<ProgramAssociationTables>(dmxFd);
	cerr << "Transport stream ID " << (*pats->begin())->number() << std::endl;

	std::map<uint16_t,uint16_t> PMTPids = pats->pids();
	delete pats;
	std::vector<Program> programs;
	for(auto p: PMTPids) {
		if(p.first == 0) continue;
		ProgramMapTables *pmts = DVBTables<ProgramMapTable>::read<ProgramMapTables>(dmxFd, p.second);
		programs.push_back(Program(*pmts));
		delete pmts;
	}

	for(auto p: programs)
		p.dump(std::cerr);

	ServiceDescriptionTables *sdts = DVBTables<ServiceDescriptionTable>::read<ServiceDescriptionTables>(dmxFd);
	sdts->dump();
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
