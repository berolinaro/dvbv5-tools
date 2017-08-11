#pragma once

#include <map>
#include <vector>
#include <iostream>
#include <iomanip>
#include <type_traits>
#include <cstring>
#include <cstdint>
#include "PIDs.h"

extern "C" {
#include <linux/dvb/frontend.h>
#include <linux/dvb/dmx.h>
#include <sys/ioctl.h>
}

/**
 * Identifiers (table_id) of standard tables that are part of the DVB spec
 * ETSI EN 300 468 v1.15.1
 * see http://www.etsi.org/deliver/etsi_en/300400_300499/300468/01.15.01_60/en_300468v011501p.pdf
 */
enum DVBTableIDs {
	ProgramAssociation = 0x00,
	ConditionalAccess = 0x01,
	ProgramMap = 0x02,
	TransportStreamDescription = 0x03,
	NetworkInformation = 0x40,
	NetworkInformationOther = 0x41,
	ServiceDescription = 0x42,
	ServiceDescriptionOther = 0x46,
	BouquetAssociation = 0x4a,
	/// ETSI TS 102 006 [11]
	UpdateNotification = 0x4b,
	EventInformationPF = 0x4e,
	EventInformationPFOther = 0x4f,
	EventInformationStart = 0x50,
	EventInformationEnd = 0x5f,
	EventInformationOtherStart = 0x60,
	EventInformationOtherEnd = 0x6f,
	TimeDate = 0x70,
	RunningStatus = 0x71,
	Stuffing = 0x72,
	TimeOffset = 0x73,
	/// ETSI TS 102 812 [15]
	ApplicationInformation = 0x74,
	/// ETSI TS 102 323 [13]
	Container = 0x75,
	/// ETSI TS 102 323 [13]
	RelatedContent = 0x76,
	/// ETSI TS 102 323 [13]
	ContentIdentifier = 0x77,
	/// ETSI EN 301 192 [4]
	MPE_FEC = 0x78,
	/// ETSI TS 102 323 [13]
	ResolutionProvider = 0x79,
	Invalid = 0xff // 0xff is "reserved" and not "for future use", so we use it to signal timeouts
};

class DVBTable {
public:
	static DVBTable *read(int fd);
	~DVBTable();
	virtual bool ok() const;
	virtual void dump(std::ostream &where=std::cerr, std::string const &indent="") const;
	uint8_t tableId() const { return _tableId; }
	uint8_t version() const { return _version; }
	uint8_t section() const { return _section; }
	uint8_t lastSection() const { return _lastSection; }
	uint16_t number() const { return _number; }
	void releaseData() { _data = nullptr; }
	bool operator ==(DVBTable const &other) const { return _tableId == other._tableId && _section == other._section && _version == other._version; }
	static constexpr uint8_t tablePid = 0x00;
	static constexpr uint8_t tableFilter = 0x00;
	static constexpr uint8_t tableMask = 0x00;
protected:
	/**
	 * use DVBTable::read
	 */
	DVBTable(int fd);
	DVBTable(DVBTable *t);
protected:
	uint8_t _tableId;
	uint8_t _sectionSyntaxIndicator;
	uint8_t _private;
	uint16_t _dataLength;
	// Always an identifier number, but with slightly different
	// meanings in different tables -- network ID in PAT,
	// program number in PMT, ...
	uint16_t _number;
	uint8_t _version;
	uint8_t _currentNext;
	uint8_t _section;
	uint8_t _lastSection;
	unsigned char *_data;
	uint32_t _crc;
	uint32_t _calculatedCrc;
};

template <typename T> class DVBTables:public std::vector<T*> {
	static_assert(std::is_base_of<DVBTable, T>::value, "DVBTables members need to be derived from DVBTable");
public:
	typedef T TableType;
	template <class DT> static DT* read(int fd, uint16_t pid=DT::TableType::tablePid, uint8_t tableFilter=DT::TableType::tableFilter, uint8_t tableMask=DT::TableType::tableMask) {
		dmx_sct_filter_params f;
		memset(&f, 0, sizeof(f));
		f.pid = pid;
		f.filter.filter[0] = tableFilter;
		f.filter.mask[0] = tableMask;
		f.timeout = 0;
		f.flags = DMX_IMMEDIATE_START | DMX_CHECK_CRC;
		DT *ret = new DT;
		ioctl(fd, DMX_SET_FILTER, &f);
		while(!ret->complete()) {
			DVBTable *t = DVBTable::read(fd);
			if(!t) {
				// timeout
				ioctl(fd, DMX_STOP);
				return nullptr;
			}
			typename DT::TableType *d=dynamic_cast<typename DT::TableType*>(t);
			if(d && d->tableId() == Invalid) { // Timeout -- probably the table just doesn't exist
				break;
			}
			if(d)
				ret->add(d);
			else
				delete d;
		}
		ioctl(fd, DMX_STOP);
		return ret;
	}
	DVBTables() { }
	DVBTables(T *t) {
		add(t);
	}
	bool add(T *t) {
		if(!t)
			return false;

		if(!t->ok()) {
			// Let's not bother adding bogus tables...
			delete t;
			return false;
		}

		if(std::vector<T*>::size() < t->lastSection()+1) {
			std::vector<T*>::resize(t->lastSection()+1);
		} else if(T *old=(*this)[t->section()]) {
			if(old == t) {
				// Dupe
				t->releaseData();
				delete t;
				return false;
			} else if(*old == *t) {
				// Dupe in content, not pointer
				delete t;
				return false;
			}
			// New version -- let's replace what we have
			delete (*this)[t->section()];
		}
		std::vector<T*>::at(t->section()) = t;
		return true;
	}
	bool complete() const {
		if(std::vector<T*>::empty()) {
			return false;
		}
		T* first = std::vector<T*>::front();
		if(!first) {
			// Can't be complete if we don't have the first element
			return false;
		}
		uint8_t version = first->version();
		for(int i=0; i<first->lastSection(); i++) {
			if(!std::vector<T*>::at(i))
				return false;
			if(std::vector<T*>::at(i)->version() != version)
				// Version mismatch between sections...
				// Better wait until all sections have been updated
				return false;
		}
		return true;
	}
	virtual void dump(std::ostream &where=std::cerr, std::string const &indent="") const {
		if(std::vector<T*>::empty())
			where << indent << "Doesn't contain any tables yet." << std::endl;
		int limit=std::vector<T*>::size();
		for(int i=0; i<limit; i++) {
			if((*this)[i]) {
				limit = (*this)[i]->lastSection();
				where << indent << "Table " << i << "/" << limit << ":" << std::endl;
				(*this)[i]->dump(where, indent);
			} else
				where << indent << "Table " << i << " not yet received" << std::endl;
		}
	}
};
