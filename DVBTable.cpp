#include "DVBTable.h"
#include "ProgramAssociationTable.h"
#include "ProgramMapTable.h"
#include "ServiceDescriptionTable.h"
#include "NetworkInformationTable.h"
#include "Util.h"
#include <iostream>
#include <cstring>
#include <cassert>
#include <cxxabi.h>

extern "C" {
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
}

using namespace std;

DVBTable *DVBTable::read(int fd) {
	DVBTable *t=new DVBTable(fd);
	switch(t->tableId()) {
	case ProgramAssociation:
		return new ProgramAssociationTable(t);
	case ProgramMap:
		return new ProgramMapTable(t);
	case ServiceDescription:
	case ServiceDescriptionOther:
		return new ServiceDescriptionTable(t);
	case NetworkInformation:
	case NetworkInformationOther:
		return new NetworkInformationTable(t);
	case Invalid:
		cerr << "Received Invalid table, sending nullptr" << endl;
		delete t;
		return nullptr;
	default:
		std::cerr << "Received table with unknown id " << static_cast<int>(t->tableId()) << std::endl;
	}
	return t;
}

DVBTable::DVBTable(int fd) {
	ssize_t count;
	unsigned char header[8];
	do {
		if(!Util::waitForData(fd)) {
			_tableId = Invalid;
			return;
		}
		count = ::read(fd, header, sizeof(header));
		if(count != sizeof(header)) {
			std::cerr << "Incomplete packet, got " << count << ", expected at least " << sizeof(header) << " from " << fd << std::endl;
			if(count<0)
				std::cerr << strerror(errno) << std::endl;
			ioctl(fd, DMX_STOP);
			sleep(1);
			ioctl(fd, DMX_START);
			continue;
		}
		_tableId = header[0];
		_sectionSyntaxIndicator = (header[1]&0b10000000)>>7;
		if(!_sectionSyntaxIndicator)
			std::cerr << "Ignoring bogus section syntax indicator" << std::endl;
		_private = (header[1]&0b01000000)>>6;
		// The next 12 bits are the section length (number of
		// following bytes including CRC). We change it to
		// data length by subtracting the CRC and the rest of
		// the header...
		_dataLength = (((header[1]&0xf)<<8)|header[2]);
		if(_dataLength < 9) {
			std::cerr << "Length smaller than header+CRC" << std::endl;
			ioctl(fd, DMX_STOP);
			sleep(1);
			ioctl(fd, DMX_START);
			continue;
		}
		_dataLength -= 9;
		_number = (header[3]<<8)|header[4];
		_version = (header[5]&0b00111110)>>1;
		_currentNext = header[5]&1;
		_section = header[6];
		_lastSection = header[7];
		_data = new unsigned char[_dataLength];
		count = ::read(fd, _data, _dataLength);
		if(count != _dataLength) {
			std::cerr << "Got unexpected data: Expected " << _dataLength << " bytes, got " << count << std::endl;
			ioctl(fd, DMX_STOP);
			sleep(1);
			ioctl(fd, DMX_START);
			continue;
		}
		count = ::read(fd, &_crc, 4);
		if(count != 4) {
			std::cerr << "Incomplete CRC" << std::endl;
			ioctl(fd, DMX_STOP);
			sleep(1);
			ioctl(fd, DMX_START);
			continue;
		}
	} while(count != 4);
	_crc = ntohl(_crc);
	_calculatedCrc = Util::crc32(header, sizeof(header));
	_calculatedCrc = Util::crc32(_data, _dataLength, _calculatedCrc);
	if(_crc != _calculatedCrc) {
		Util::SaveIOState sis(std::cerr);
		std::cerr << "CRC mismatch! Given: " << hex << _crc << ", calculated: " << _calculatedCrc << std::endl;
	}
}

DVBTable::DVBTable(DVBTable *t):
	_tableId(std::move(t->_tableId)),
	_sectionSyntaxIndicator(std::move(t->_sectionSyntaxIndicator)),
	_private(std::move(t->_private)),
	_dataLength(std::move(t->_dataLength)),
	_number(std::move(t->_number)),
	_version(std::move(t->_version)),
	_currentNext(std::move(t->_currentNext)),
	_section(std::move(t->_section)),
	_lastSection(std::move(t->_lastSection)),
	_data(std::move(t->_data)),
	_crc(std::move(t->_crc)),
	_calculatedCrc(std::move(t->_calculatedCrc))
{
	t->_data = 0;
	delete t;
}

bool DVBTable::ok() const {
	return _sectionSyntaxIndicator && (_crc == _calculatedCrc);
}

DVBTable::~DVBTable() {
	if(_data)
		delete[] _data;
}

void DVBTable::dump(std::ostream &where, std::string const &indent) const {
	cerr << indent << abi::__cxa_demangle(typeid(*this).name(), 0, 0, 0) << endl;
	cerr << indent << "Section " << static_cast<int>(_section) << "/" << static_cast<int>(_lastSection) << endl;
	if(_data)
		Util::hexdump(_data, _dataLength, where, indent);
}
