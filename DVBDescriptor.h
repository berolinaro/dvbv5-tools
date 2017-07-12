#pragma once
#include <iostream>

// Descriptors 0 to 18 are defined in ISO/IEC 13818-1, available at
// http://www.ece.cmu.edu/~ece796/documents/MPEG-2_Systems_IS.doc
// Descriptors 0x40 to 0x7f are defined in ETSI EN 300 468, available at
// http://www.etsi.org/deliver/etsi_en/300400_300499/300468/01.15.01_60/en_300468v011501p.pdf
// (see also http://www.etsi.org/standards-search)

class DVBDescriptor {
public:
	enum DescriptorType {
		VideoStream			= 0x02,
		AudioStream			= 0x03,
		Hierarchy			= 0x04,
		Registration			= 0x05,
		DataStreamAlignment		= 0x06,
		TargetBackgroundGrid		= 0x07,
		VideoWindow			= 0x08,
		CA				= 0x09,	
		Language			= 0x0a,
		SystemClock			= 0x0b,
		MultiplexBufferUtilization	= 0x0c,
		Copyright			= 0x0d,
		MaximumBitrate			= 0x0e,
		PrivateDataIndicator		= 0x0f,
		SmoothingBuffer			= 0x10,
		STD				= 0x11,
		IBP				= 0x12,

		NetworkName			= 0x40,
		ServiceList			= 0x41,
		Stuffing			= 0x42,
		SatelliteDeliverySystem		= 0x43,
		CableDeliverySystem		= 0x44,
		VBIData				= 0x45,
		VBITeletext			= 0x46,
		BouquetName			= 0x47,
		Service				= 0x48,
		CountryAvailability		= 0x49,
		Linkage				= 0x4a,
		NVODReference			= 0x4b,
		TimeShiftedService		= 0x4c,
		ShortEvent			= 0x4d,
		ExtendedEvent			= 0x4e,
		TimeShiftedEvent		= 0x4f,
		Component			= 0x50,
		Mosaic				= 0x51,
		StreamIdentifier		= 0x52,
		CAIdentifier			= 0x53,
		Content				= 0x54,
		ParentalRating			= 0x55,
		Teletext			= 0x56,
		Telephone			= 0x57,
		LocalTimeOffset			= 0x58,
		Subtitling			= 0x59,
		TerrestrialDeliverySystem	= 0x5a,
		MultilingualNetworkName		= 0x5b,
		MultilingualBouquetName		= 0x5c,
		MultilingualServiceName		= 0x5d,
		MultilingualComponent		= 0x5e,
		PrivateDataSpecifier		= 0x5f,
		ServiceMove			= 0x60,
		ShortSmoothingBuffer		= 0x61,
		FrequencyList			= 0x62,
		PartialTransportStream		= 0x63,
		DataBroadcast			= 0x64,
		Scrambling			= 0x65,
		DataBroadcastID			= 0x66,
		TransportStream			= 0x67,
		DSNG				= 0x68,
		PDC				= 0x69,
		AC3				= 0x6a,
		AncillaryData			= 0x6b,
		CellList			= 0x6c,
		CellFrequencyLink		= 0x6d,
		AnnouncementSupport		= 0x6e,
		ApplicationSignaling		= 0x6f,
		AdaptationFieldData		= 0x70,
		ServiceIdentifier		= 0x71,
		ServiceAvailability		= 0x72,
		DefaultAuthority		= 0x73, // ETSI TS 102 323 [13]
		RelatedContent			= 0x74, // ETSI TS 102 323 [13]
		TVAID				= 0x75, // ETSI TS 102 323 [13]
		ContentIdentifier		= 0x76, // ETSI TS 102 323 [13]
		TimeSliceFECIdentifier		= 0x77, // ETSI TS 301 192 [4]
		ECMRepetitionRate		= 0x78, // ETSI TS 301 192 [4]
		S2SatelliteDeliverySystem	= 0x79,
		EnhancedAC3			= 0x7a,
		DTS				= 0x7b,
		AAC				= 0x7c,
		XAITLocation			= 0x7d,
		FTAContentManagement		= 0x7e,
		Extension			= 0x7f
	};
public:
	static DVBDescriptor *get(unsigned char *&where);
	virtual void dump(std::ostream &where=std::cerr, std::string const &indent="") const;
	DescriptorType tag() const { return static_cast<DescriptorType>(_tag); }
	uint8_t length() const { return _length; }
	unsigned char *data() const { return _data; }
protected:
	DVBDescriptor() { }
	DVBDescriptor(unsigned char *&where);
protected:
	uint8_t	_tag;
	uint8_t _length;
	unsigned char *_data;
};
