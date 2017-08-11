#include "DVBInterface.h"
#include "ChannelList.h"
#include "Util.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <thread>
#include <mutex>

extern "C" {
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <poll.h>
}

std::mutex newClientsMutex;
std::vector<int> newClients;
std::mutex newChannelMutex;
auto newChannel = std::make_pair<Transponder const *,Service const *>(nullptr, nullptr);

void httpThread(int fd) {
	pollfd p;
	p.fd = fd;
	p.events = POLLIN|POLLHUP;
	bool isGet = false, isHead = false, isBad = false, complete = false;
	std::string path;
	while(!complete) {
		char buf[1024];
		if(poll(&p, 1, 10000)>0) {
			if(p.revents & POLLHUP) {
				close(fd);
				return;
			}
			ssize_t count = recv(fd, buf, 1024, 0);
			buf[count]=0;
			for(int i=0; i<count; i++) {
				if(buf[i]=='\r') // Get rid of DOS-isms
					memmove(buf+i, buf+i+1, (count--)-i);
			}
			Util::hexdump(reinterpret_cast<unsigned char*>(buf), count);

			char *tmp;
			complete = ((count>=2) && buf[count-2]=='\n' && buf[count-1]=='\n') || ((count == 1) && buf[0] == '\n');
			char *line = strtok_r(buf, "\n", &tmp);
			while(line) {
				Util::hexdump(reinterpret_cast<unsigned char*>(line), strlen(line));
				bool curHead = !strncasecmp(line, "HEAD ", 5);
				bool curGet = !strncasecmp(line, "GET ", 4);
				if(curHead || curGet) {
					isHead = curHead;
					isGet = curGet;
					char *tmp1;
					char *s = strtok_r(line, " ", &tmp1);
					// Skip over "HEAD /" or "GET /" -- we've already seen that
					s = strtok_r(nullptr, " ", &tmp1);
					for(int i=1; i<strlen(s); i++) {
						if(i<strlen(s)-2 && s[i] == '%' &&
								((s[i+1] >= '0' && s[i+1] <= '9') || (s[i+1] >= 'a' && s[i+1] <= 'f') || (s[i+1] >= 'A' && s[i+1] <= 'F')) &&
								((s[i+2] >= '0' && s[i+2] <= '9') || (s[i+2] >= 'a' && s[i+2] <= 'f') || (s[i+2] >= 'A' && s[i+2] <= 'F'))) {
							char code[3] = {s[i+1], s[i+2], 0};
							path.push_back(strtoul(code, nullptr, 16));
							i+=2;
						} else if(s[i] == '+')
							path.push_back(' ');
						else
							path.push_back(s[i]);
					}
					s=strtok_r(nullptr, "", &tmp1);
					if(!s || !strncmp(s, "HTTP/0", 6)) { // HTTP before 1.0 -- no extras expected
						complete = true;
						break;
					}
				} else if(!isHead && !isGet) {
					isBad = true; complete = true;
					break;
				}
				line = strtok_r(nullptr, "\n", &tmp);
			}
		} else {
			close(fd);
			break;
		}
	}
	if(complete) {
		std::cerr << "Got request for " << path << std::endl;
		ChannelList channels("channels.dvb");
		auto channel = channels.find(path);
		if(!channel.first)
			std::cerr << "No such channel!" << std::endl;
		if(isGet || isHead) {
			send(fd, "HTTP/1.1 200 OK\r\n", 17, 0);
			send(fd, "Cache-Control: no-store, no-cache\r\n", 35, 0);
			send(fd, "Content-Type: video/mp2t\r\n", 26, 0);
			send(fd, "\r\n", 2, 0);
			if(isGet) {
				// FIXME don't allow switching the channel if we're already
				// streaming to someone else...
				{
					std::lock_guard<std::mutex> channelGuard(newChannelMutex);
					newChannel = channel;
				}
				// Start sending the stream...
				{
					std::lock_guard<std::mutex> guard(newClientsMutex);
					newClients.push_back(fd);
				}
				std::cerr << "Client added" << std::endl;
			} else {
				close(fd);
			}
		} else {
			send(fd, "HTTP/1.1 400 Bad Request\r\n", 26, 0);
			send(fd, "Content-Type: text/plain\r\n", 26, 0);
			send(fd, "\r\n", 2, 0);
			send(fd, "Send HEAD or GET\r\n", 18, 0);
			close(fd);
		}
	}
}

int main(int argc, char **argv) {
	bool useIPv6 = argc>1 && !strcmp(argv[1], "-6");
	signal(SIGPIPE, SIG_IGN);

	int socksize;
	int s;
	union {
		sockaddr addr;
		sockaddr_in addr_in;
		sockaddr_in6 addr_in6;
	};
	union {
		sockaddr dest;
		sockaddr_in dest_in;
		sockaddr_in6 dest_in6;
	};
	memset(&addr, 0, sizeof(addr_in));
	if(useIPv6) {
		addr_in6.sin6_family = AF_INET6;
		addr_in6.sin6_addr = in6addr_any;
		addr_in6.sin6_port = htons(8080);
		socksize = sizeof(addr_in6);
		s = socket(AF_INET6, SOCK_STREAM, 0);
	} else {
		addr_in.sin_family = AF_INET;
		addr_in.sin_addr.s_addr = htonl(INADDR_ANY);
		addr_in.sin_port = htons(8080);
		socksize = sizeof(addr_in);
		s = socket(AF_INET, SOCK_STREAM, 0);
	}
	if(s<0)
		std::cerr << "socket: " << strerror(errno) << std::endl;
	int b = bind(s, &addr, socksize);
	while(b<0 && errno == EADDRINUSE) {
		std::cerr << "Please kill any other process using the port" << std::endl;
		sleep(1);
		b = bind(s, &addr, socksize);
	}
	if(b<0)
		std::cerr << "bind: " << strerror(errno) << std::endl;
	int l = listen(s, SOMAXCONN);
	if(l<0)
		std::cerr << "listen: " << strerror(errno) << std::endl;

	DVBInterfaces cards = DVBInterfaces::all();
	if(!cards.size()) {
		std::cerr << "No DVB interfaces found" << std::endl;
		return 1;
	}

	ChannelList channels("channels.dvb");
	if(!channels.size()) {
		std::cerr << "No channel list - run \"scan >channels.dvb\"" << std::endl;
		return 2;
	}

	auto channel = channels.find("Das Erste HD");
	std::cerr << "Tuning..." << std::endl;
	cards[0].tune(channel.first);
	std::cerr << "Setting up channel..." << std::endl;
	cards[0].setup(*channel.second);
	std::cerr << "Accepting connections" << std::endl;

	int dvbFd = open("/dev/dvb/adapter0/dvr0", O_RDONLY|O_NONBLOCK);
	char dvbbuf[188];


	pollfd p[128];
	p[0].fd = s;
	p[0].events = POLLIN;
	p[1].fd = dvbFd;
	p[1].events = POLLIN;
	int nfds = 2;

	while(true) {
		if(poll(p, nfds, 10000) > 0) {
			for(int i=2; i<nfds; i++) {
				if(p[i].revents & POLLHUP) {
					close(p[i].fd);
					memmove(&p[i], &p[i+1], sizeof(pollfd)*(nfds-i));
					nfds--; i--;
					std::cerr << "Client disconnected" << std::endl;
				}
			}
			if(p[0].revents & POLLIN) {
				socklen_t ss;
				int conn = accept(s, &dest, &ss);
				std::thread *http = new std::thread(&httpThread, conn);
			}
			{
				std::lock_guard<std::mutex> channelGuard(newChannelMutex);
				if(newChannel.first) {
					cards[0].close();
					close(dvbFd);
					cards[0].tune(newChannel.first);
					cards[0].setup(*newChannel.second);
					newChannel=std::make_pair<Transponder const *, Service const *>(nullptr, nullptr);
					dvbFd = open("/dev/dvb/adapter0/dvr0", O_RDONLY|O_NONBLOCK);
					p[1].fd = dvbFd;
					continue;
				}
			}
			if(p[1].revents & POLLIN) {
				size_t bytes = read(dvbFd, dvbbuf, 188);
				if(bytes>0)
				{
					if(dvbbuf[0] != 0x47)
						std::cerr << "Invalid sync byte" << std::endl;
					{
						std::lock_guard<std::mutex> guard(newClientsMutex);
						for(int fd: newClients) {
							p[nfds].fd = fd;
							p[nfds++].events = POLLOUT|POLLHUP;
						}
						newClients=std::vector<int>();
					}
					for(int i=2; i<nfds; i++)
						write(p[i].fd, dvbbuf, bytes);
				}
			}
		}
	}
}
