#pragma once

#include "DVBTable.h"
#include "DVBDescriptor.h"
#include <iostream>
#include <vector>
#include <map>

class Stream {
public:
	Stream(uint8_t streamType, uint16_t pid, std::vector<DVBDescriptor*> descriptors):_streamType(streamType),_pid(pid),_descriptors(descriptors) { }
	uint8_t streamType() const { return _streamType; }
	uint16_t pid() const { return _pid; }
	std::vector<DVBDescriptor*> descriptors() const { return _descriptors; }
	void dump(std::ostream &where=std::cerr, std::string const &indent="") const;
protected:
	uint8_t _streamType;
	uint16_t _pid;
	std::vector<DVBDescriptor*> _descriptors;
};

class ProgramMapTable:public DVBTable {
	friend class Program;
public:
	ProgramMapTable(DVBTable *t):DVBTable(t) { }
	uint16_t programNumber() const { return _number; }
	uint16_t pcrPid() const { return _pcrPid; }
	virtual void dump(std::ostream &where=std::cerr, std::string const &indent="") const override;
	static constexpr uint8_t tableFilter = ProgramMap;
	static constexpr uint8_t tableMask = 0xff;
protected:
	uint16_t	_pcrPid;
};

class Program;

class ProgramMapTables:public DVBTables<ProgramMapTable> {
public:
	ProgramMapTables():DVBTables<ProgramMapTable>() { }
	ProgramMapTables(ProgramMapTable *t):DVBTables<ProgramMapTable>(t) { }
	std::map<uint16_t,uint16_t> pids() const;
	Program program() const;
};

class Program {
public:
	Program(ProgramMapTables const &pmt);
	std::vector<DVBDescriptor*> const &descriptors() const { return _descriptors; }
	std::vector<Stream> const &streams() const { return _streams; }
	void dump(std::ostream &where=std::cerr, std::string const &indent="") const;
protected:
	uint16_t			_programNumber;
	uint16_t			_pcrPid;
	std::vector<DVBDescriptor*>	_descriptors;
	std::vector<Stream>		_streams;
};

