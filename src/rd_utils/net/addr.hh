#pragma once

#include <string>
#include <iostream>
#include <vector>

namespace rd_utils {

    namespace net {

		class Ipv4Address {

			unsigned char _a = 0;

			unsigned char _b = 0;

			unsigned char _c = 0;

			unsigned char _d = 0;

		public :

			Ipv4Address (int full);

			Ipv4Address (unsigned char a, unsigned char b, unsigned char c, unsigned char d);


			/**
			 * Store the four part of the ip A.B.C.D in a single u64 (A << 24 | B << 16 | C << 8 | D)
			 */
			unsigned int toN () const;

			unsigned char A () const;
			unsigned char B () const;
			unsigned char C () const;
			unsigned char D () const;

			Ipv4Address A (unsigned char a);
			Ipv4Address B (unsigned char b);
			Ipv4Address C (unsigned char c);
			Ipv4Address D (unsigned char d);


			static Ipv4Address getIfaceIp (const std::string & iface);

			/**
			 * @returns: the list of ips of the machine
			 */
			static std::vector <Ipv4Address> getAllIps ();

			/**
			 * @returns: the list of vlan ids
			 */
			static std::vector <int> getAllVlanIds ();

			std::string toString () const;

		};
      	
		class SockAddrV4 {

			Ipv4Address _addr;

			unsigned short _port;

		public :

			SockAddrV4 (Ipv4Address addr, unsigned short port);

			SockAddrV4 (const std::string & addr);
	    
			Ipv4Address ip () const;

			unsigned short port () const;
	    	    
		};


    }

}




std::ostream & operator<< (std::ostream & s, const rd_utils::net::Ipv4Address & addr);
std::ostream & operator<< (std::ostream & s, const rd_utils::net::SockAddrV4 & addr);
