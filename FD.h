#ifndef _FD_H_
#define _FD_H_ 1
#include <string>

extern "C" {
#include <fcntl.h>
#include <unistd.h>
}

/**
 * Simple wrapper around open() that closes the FD when the
 * variable goes out of scope
 */
class FD {
public:
	/**
	 * Open a file
	 */
	FD(std::string const &file, int mode = O_RDONLY):_fd(open(file.c_str(), mode)) {}
	/**
	 * Destructor -- closes the file
	 */
	~FD() { if(_fd >= 0) close(_fd); }
	/**
	 * Get the file descriptor
	 */
	operator int() const { return _fd; }
private:
	int const _fd;
};
#endif
