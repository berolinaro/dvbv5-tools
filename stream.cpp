#include "DVBInterface.h"
#include "ChannelList.h"
#include "ProgramAssociationTable.h"
#include "ProgramMapTable.h"
#include "Util.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <thread>
#include <set>
#include <mutex>

extern "C" {
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <poll.h>
#include <unistd.h>
}

std::mutex newClientsMutex;
std::map<int,std::string> newClients;
Transponder const *currentTransponder = nullptr;
std::map<int,std::set<int>> watchers;
int threadNotify[2];

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
		if(isGet || isHead) {
			send(fd, "HTTP/1.1 200 OK\r\n", 17, 0);
			send(fd, "Cache-Control: no-store, no-cache\r\n", 35, 0);
			send(fd, "Content-Type: video/MP2T\r\n", 26, 0);
			send(fd, "\r\n", 2, 0);
			if(isGet) {
				// Start sending the stream...
				{
					std::lock_guard<std::mutex> guard(newClientsMutex);
					newClients.insert_or_assign(fd, path);
				}
				write(threadNotify[1], "x", 1);
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

	int dvbFd = open("/dev/dvb/adapter0/dvr0", O_RDONLY|O_NONBLOCK);
	char dvbbuf[188];

	pipe(threadNotify);

	pollfd p[128];
	p[0].fd = s;
	p[0].events = POLLIN;
	p[1].fd = dvbFd;
	p[1].events = POLLIN;
	p[2].fd = threadNotify[0];
	p[2].events = POLLIN;
	int nfds = 3;

	ProgramAssociationTables *pats;

	while(true) {
		if(poll(p, nfds, 10000) > 0) {
			// Close connections to disconnected clients
			for(int i=3; i<nfds; i++) {
				if(p[i].revents & POLLHUP) {
					close(p[i].fd);
					for(auto w: watchers) {
						auto entry = w.second.find(p[i].fd);
						if(entry != w.second.end())
							w.second.erase(entry);
					}
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
			if(p[1].revents & POLLIN) {
				size_t const bytes = read(dvbFd, dvbbuf, 188);
				if(bytes>0)
				{
					if(dvbbuf[0] != 0x47)
						std::cerr << "Invalid sync byte" << std::endl;
					uint16_t const pid = ((dvbbuf[1]&0b00011111)<<8)|dvbbuf[2];
					auto w = watchers.find(pid);
					if(w != watchers.end())
						for(int fd: w->second)
							write(fd, dvbbuf, bytes);
				}
			}
		}
		// Check for new connections
		{
			std::lock_guard<std::mutex> guard(newClientsMutex);
			for(auto c: newClients) {
				std::cerr << "Looking at new requests" << std::endl;
				int const newFd = c.first;
				auto newChannel = channels.find(c.second);
				if(newChannel.first)
					std::cerr << "Valid channel " << c.second << std::endl;
				else {
					std::cerr << "Invalid channel " << c.second << std::endl;
					close(newFd);
					continue;
				}

				if(!currentTransponder || (*currentTransponder != *newChannel.first)) {
					std::cerr << "Switching transponder" << std::endl;
					// FIXME no way to switch transponders gracefully... Probably shouldn't
					// allow this to everyone!
					for(int i=3; i<nfds; i++)
						close(p[i].fd);
					nfds = 3;
					cards[0].tune(newChannel.first);
					currentTransponder = newChannel.first;

					int patDmx = cards[0].open("demux0", O_RDWR|O_NONBLOCK);
					pats = DVBTables<ProgramAssociationTable>::read<ProgramAssociationTables>(patDmx);
					close(patDmx);
				} else
					std::cerr << "Staying on current transponder" << std::endl;
				if(pats) {
					std::map<uint16_t,uint16_t> PMTPids = pats->pids();
					if(PMTPids.find(newChannel.second->serviceId()) == PMTPids.end())
						std::cerr << "new channel invalid" << std::endl;
					else {
						int pmtDmx=cards[0].open("demux0", O_RDWR|O_NONBLOCK);
						ProgramMapTables *pmts = DVBTables<ProgramMapTable>::read<ProgramMapTables>(pmtDmx, PMTPids[newChannel.second->serviceId()]);
						close(pmtDmx);
						if(!pmts)
							std::cerr << "No PMT for " << newChannel.second->serviceId() << std::endl;
						else {
							Program p(*pmts);
							for(Stream s: p.streams()) {
								if(watchers.find(s.pid()) == watchers.end()) {
									if(s.pid() == 1)
										continue;
									if(cards[0].addPES(s.type(), s.pid(), true)) {
										std::set<int> w;
										w.insert(w.end(), newFd);
										watchers.insert_or_assign(s.pid(), w);
									}
								} else
									watchers[s.pid()].insert(watchers[s.pid()].end(), newFd);
							}
						}
					}
				} else
					std::cerr << "No PAT" << std::endl;
				p[nfds].fd = newFd;
				p[nfds++].events = POLLOUT|POLLHUP;
			}
			newClients.clear();
		}
	}
}
