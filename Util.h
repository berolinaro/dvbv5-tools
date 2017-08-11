#pragma once

#include <functional>
#include <iostream>

extern "C" {
#include <sys/time.h>
}

namespace Util {
	/**
	 * Wait for a condition to become true
	 * @param timeout Timeout in microseconds
	 * @param condition Condition to wait for (e.g. lambda function)
	 * @param delay Delay between checks for the condition
	 * @return @c true if the condition became true, @c false if the timeout was hit
	 */
	bool waitFor(uint32_t timeout, std::function<bool ()> condition, timespec delay = { 0, 1000000 });
	bool waitForData(int fd, uint32_t timeout=10000);
	/**
	 * Generate a hex dump of data
	 * @param data Data to dump
	 * @param size Size of data
	 * @param where Destination of hex dump
	 * @param bytesPerRow Bytes per row
	 */
	void hexdump(void const * const data, size_t size, std::ostream &where=std::cerr, std::string const &indent="", uint8_t bytesPerRow=16);
	/**
	 * Calculate a DVB-Compatible CRC-32 checksum
	 * @param data Data to checksum
	 * @param size Size of data
	 * @param previous Previous CRC (for adding data to a previously calculated CRC)
	 * @return CRC-32 checksum
	 */
	uint32_t crc32(unsigned char const * const data, size_t size, uint32_t previous = 0xffffffff);

	/**
	 * Helper that saves and restores std::iomanip format flags
	 * When manipulating iostream flags (std::hex, ...), simply construct a
	 * SaveIOState object earlier - flags will be restored as the object
	 * goes out of scope.
	 */
	class SaveIOState {
	public:
		SaveIOState(std::ostream &stream):_stream(stream),_flags(stream.flags()) {
		}
		~SaveIOState() {
			_stream.flags(_flags);
		}
	private:
		std::ostream &		_stream;
		std::ios::fmtflags	_flags;
	};
}
