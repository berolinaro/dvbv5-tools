#pragma once

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

