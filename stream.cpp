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

void httpThread(int fd) {
	pollfd p;
	p.fd = fd;
	p.events = POLLIN|POLLHUP;
	bool isGet = false, isHead = false, isBad = false, complete = false;
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
				if(!strncasecmp(line, "HEAD ", 5))
					isHead = true;
				else if(!strncasecmp(line, "GET ", 4))
					isGet = true;
				else if(!isHead && !isGet) {
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
		if(isGet || isHead) {
			send(fd, "HTTP/1.1 200 OK\r\n", 17, 0);
			send(fd, "Cache-Control: no-store, no-cache\r\n", 35, 0);
			send(fd, "Content-Type: video/mp2t\r\n", 26, 0);
			send(fd, "\r\n", 2, 0);
			if(isGet) {
				// Start sending the stream...
				std::lock_guard<std::mutex> guard(newClientsMutex);
				newClients.push_back(fd);
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
	int b = bind(s, &addr, socksize);
	int l = listen(s, SOMAXCONN);

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
			if(p[1].revents & POLLIN) {
				size_t bytes = read(dvbFd, dvbbuf, 188);
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
