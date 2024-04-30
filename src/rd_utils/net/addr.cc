#include <rd_utils/net/addr.hh>
#include <rd_utils/utils/tokenizer.hh>
#include <rd_utils/utils/error.hh>

#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sstream>

union Packer {
    unsigned char pack [4];
    unsigned int n;
};
       
namespace rd_utils::net {

	Ipv4Address::Ipv4Address (const std::string & addr) {
		std::vector <std::string> tokens = utils::tokenize (addr, {".", " "}, {" "});
		if (tokens.size () != 7) {
			throw utils::AddrError (std::string ("Malformed addr : ") + addr);
		}

		if (tokens [1] != "." || tokens [3] != "." || tokens [5] != ".") {
			throw utils::AddrError (std::string ("Malformed addr : ") + addr);
		}

		this-> _a = std::atoi (tokens [0].c_str ());
		this-> _b = std::atoi (tokens [2].c_str ());
		this-> _c = std::atoi (tokens [4].c_str ());
		this-> _d = std::atoi (tokens [6].c_str ());
	}

	Ipv4Address::Ipv4Address (int full) {
		Packer p;
		p.n = full;
		this-> _a = p.pack [0];
		this-> _b = p.pack [1];
		this-> _c = p.pack [2];
		this-> _d = p.pack [3];
	}
	
	Ipv4Address::Ipv4Address (unsigned char a, unsigned char b, unsigned char c, unsigned char d) :
		_a (a), _b (b), _c (c), _d (d)
	{
	}

	unsigned int Ipv4Address::toN () const {
		Packer p;
		p.pack [0] = this-> _a;
		p.pack [1] = this-> _b;
		p.pack [2] = this-> _c;
		p.pack [3] = this-> _d;

		return p.n;
	}

	unsigned char Ipv4Address::A () const {
		return this-> _a;
	}

	unsigned char Ipv4Address::B () const {
		return this-> _b;
	}

	unsigned char Ipv4Address::C () const {
		return this-> _c;
	}

	unsigned char Ipv4Address::D () const {
		return this-> _d;
	}

	Ipv4Address Ipv4Address::A (unsigned char a) {
		this-> _a = a;
		return *this;
	}

	Ipv4Address Ipv4Address::B (unsigned char b) {
		this-> _b = b;
		return *this;
	}

	Ipv4Address Ipv4Address::C (unsigned char c) {
		this-> _c = c;
		return *this;
	}

	Ipv4Address Ipv4Address::D (unsigned char d) {
		this-> _d = d;
		return *this;
	}


	std::string Ipv4Address::toString () const {
		std::stringstream s;
		s << (int) this-> A () << "." << (int) this-> B () << "." << (int) this-> C () << "." << (int) this-> D ();
		return s.str ();
	}



	SockAddrV4::SockAddrV4 (Ipv4Address addr, unsigned short port) :
		_addr (addr), _port (port)
	{}

	SockAddrV4::SockAddrV4 (const std::string & addr) :
		_addr (Ipv4Address (0, 0, 0, 0)), _port (0)
	{
		std::vector <std::string> tokens = utils::tokenize (addr, {".", ":", " "}, {" "});
		if (tokens.size () != 7 && tokens.size () != 9) {
			throw utils::AddrError (std::string ("Malformed addr : ") + addr);
		}

		if (tokens [1] != "." || tokens [3] != "." || tokens [5] != ".") {
			throw utils::AddrError (std::string ("Malformed addr : ") + addr);
		}

		auto fst = std::atoi (tokens [0].c_str ());
		auto scd = std::atoi (tokens [2].c_str ());
		auto thd = std::atoi (tokens [4].c_str ());
		auto fth = std::atoi (tokens [6].c_str ());
		uint32_t port = 0;
		if (tokens.size () == 9) {
			if (tokens [7] != ":") throw utils::AddrError (std::string ("Malformed addr : ") + addr);
			port = std::atoi (tokens [8].c_str ());
		}

		this-> _addr = Ipv4Address (fst, scd, thd, fth);
		this-> _port = port;
	}

	Ipv4Address SockAddrV4::ip () const {
		return this-> _addr;
	}

	unsigned short SockAddrV4::port () const {
		return this-> _port;
	}

	std::string SockAddrV4::toString () const {
		return this-> _addr.toString () + ":" + std::to_string (this-> _port);
	}

	Ipv4Address Ipv4Address::getIfaceIp (const std::string & iface) {
		struct ifaddrs * ifAddrStruct=NULL;
		struct ifaddrs * ifa=NULL;
		void * tmpAddrPtr=NULL;

		getifaddrs(&ifAddrStruct);
		SockAddrV4 ip ("0.0.0.0:0");

		for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
			if (!ifa->ifa_addr) {
				continue;
			}
			if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
				// is a valid IP4 Address
				tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
				char addressBuffer[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
				if (iface == ifa-> ifa_name) {
					ip = SockAddrV4 (std::string (addressBuffer) + ":0");
					break;
				}
			}
		}

		if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
		return ip.ip ();
	}

	std::vector <Ipv4Address> Ipv4Address::getAllIps (bool onlyABC) {
		struct ifaddrs * ifAddrStruct=NULL;
		struct ifaddrs * ifa=NULL;
		void * tmpAddrPtr=NULL;

		getifaddrs(&ifAddrStruct);
		std::vector <Ipv4Address> res;
		for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
			if (!ifa->ifa_addr) {
				continue;
			}
			if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
				// is a valid IP4 Address
				tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
				char addressBuffer[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
				auto ip = SockAddrV4 (std::string (addressBuffer) + ":0").ip ();
				if (onlyABC) {
					res.push_back (ip.D (0));
				} else {
					res.push_back (ip);
				}
			}
		}

		if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
		return res;

	}

}




std::ostream & operator<< (std::ostream & s, const rd_utils::net::Ipv4Address & addr) {
    s << (int) addr.A () << "." << (int) addr.B () << "." << (int) addr.C () << "." << (int) addr.D ();
    return s;
}

std::ostream & operator<< (std::ostream & s, const rd_utils::net::SockAddrV4 & addr) {
    s << addr.ip () << ":" << (int) addr.port ();
    return s;
}
