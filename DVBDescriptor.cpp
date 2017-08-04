#include "DVBDescriptor.h"
#include "LanguageDescriptor.h"
#include "StreamIdentifierDescriptor.h"
#include "AncillaryDataDescriptor.h"
#include "AC3Descriptor.h"
#include "ApplicationSignalingDescriptor.h"
#include "TeletextDescriptor.h"
#include "CopyrightDescriptor.h"
#include "DataBroadcastIDDescriptor.h"
#include "ServiceDescriptor.h"
#include "ServiceListDescriptor.h"
#include "PrivateDataSpecifierDescriptor.h"
#include "CableDeliverySystemDescriptor.h"
#include "TerrestrialDeliverySystemDescriptor.h"
#include "Util.h"
#include <cxxabi.h>

DVBDescriptor *DVBDescriptor::get(unsigned char *&where) {
	DVBDescriptor *d=new DVBDescriptor(where);
	switch(d->tag()) {
	case Language:
		return new LanguageDescriptor(d);
	case Copyright:
		return new CopyrightDescriptor(d);
	case StreamIdentifier:
		return new StreamIdentifierDescriptor(d);
	case Teletext:
		return new TeletextDescriptor(d);
	case AncillaryData:
		return new AncillaryDataDescriptor(d);
	case AC3:
		return new AC3Descriptor(d);
	case ApplicationSignaling:
		return new ApplicationSignalingDescriptor(d);
	case DataBroadcastID:
		return new DataBroadcastIdDescriptor(d);
	case Service:
		return new ServiceDescriptor(d);
	case ServiceList:
		return new ServiceListDescriptor(d);
	case PrivateDataSpecifier:
		return new PrivateDataSpecifierDescriptor(d);
	case CableDeliverySystem:
		return new CableDeliverySystemDescriptor(d);
	case TerrestrialDeliverySystem:
		return new TerrestrialDeliverySystemDescriptor(d);
	default:
		return d;
	}
}

DVBDescriptor::DVBDescriptor(unsigned char *&where) {
	_tag = *where++;
	_length = *where++;
	_data = where;
	where += _length;
}

void DVBDescriptor::dump(std::ostream &where, std::string const &indent) const {
	Util::SaveIOState sis(where);
	where << indent << abi::__cxa_demangle(typeid(this).name(), 0, 0, 0) << " tag " << std::hex << static_cast<int>(_tag) << std::endl;
	Util::hexdump(_data, _length, where, indent);
}
