#include <rd_utils/net/stream.hh>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <rd_utils/utils/log.hh>
#include <rd_utils/utils/error.hh>

namespace rd_utils::net {

	TcpStream::TcpStream (int sock, SockAddrV4 addr) :
		_sockfd (sock),
		_addr (addr)
	{
	}

	TcpStream::TcpStream (SockAddrV4 addr) :
		_sockfd (0),
		_addr (addr)
	{
	}

	TcpStream::TcpStream (TcpStream && other) :
		_sockfd (other._sockfd),
		_addr (other._addr)
	{
		other._sockfd = 0;
		other._addr = SockAddrV4 (Ipv4Address (0), 0);
	}

	void TcpStream::operator= (TcpStream && other) {
		this-> close ();
		this-> _sockfd = other._sockfd;
		this-> _addr = other._addr;

		other._sockfd = 0;
		other._addr = SockAddrV4 (Ipv4Address (0), 0);
	}

	void TcpStream::connect () {
		this-> close ();
		this-> _sockfd = socket (AF_INET, SOCK_STREAM, 0);
		if (this-> _sockfd == -1) {
			throw utils::Rd_UtilsError ("Error creating socket.");
		}

		sockaddr_in sin = { 0 };
		sin.sin_addr.s_addr = this-> _addr.ip ().toN ();
		sin.sin_port = htons(this-> _addr.port ());
		sin.sin_family = AF_INET;

		if (::connect (this-> _sockfd, (sockaddr*) &sin, sizeof (sockaddr_in)) != 0) {
			throw utils::Rd_UtilsError ("Error creating socket.");
		}
	}
	
	
	bool TcpStream::sendInt (unsigned long i) {
		if (this-> _sockfd != 0 && !this-> _error) {
			if (write (this-> _sockfd, &i, sizeof (unsigned long)) == -1) {
				this-> _error = true;
				return false;
			}
			return true;
		}

		return false;
	}

	bool TcpStream::send (const std::string & msg) {
		if (this-> _sockfd != 0 && !this-> _error) {
			if (write (this-> _sockfd, msg.c_str (), msg.length () * sizeof (char)) == -1) {
				this-> _error = true;
				return false;
			}
			return true;
		}
		return false;
	}

	std::string TcpStream::receive () {
		if (this-> _sockfd != 0 && !this-> _error) {
			std::stringstream res;
			long bufferSize = 100;
			char buffer[bufferSize];
			ssize_t valread = 0;

			do {
				valread = recv(this-> _sockfd, &buffer, bufferSize, 0);
				if(valread == -1) {
					this-> _error = true;
					break;
				}

				buffer [valread] = 0;
				res << buffer;

			} while(valread == bufferSize);

			if (res.str ().length () == 0) { this-> _error = true; }
			return res.str();
		}

		return "";
	}

	unsigned long TcpStream::receiveInt () {
		unsigned long res = 0;
		if (this-> _sockfd != 0 && !this-> _error) {
			auto r = read (this-> _sockfd, &res, sizeof (unsigned long));
			if (r == -1) {
				this-> _error = true;
			}
		}

		return res;
	}

	int TcpStream::getHandle () const {
		return this-> _sockfd;
	}

	bool TcpStream::isOpen () const {
		return !this-> _error && this-> _sockfd != 0;
	}

	SockAddrV4 TcpStream::addr () const {
		return this-> _addr;
	}
	
	void TcpStream::close  () {
		if (this-> _sockfd != 0) {
			// ::shutdown (this-> _sockfd, SHUT_RDWR);
			auto i = ::close (this-> _sockfd);
			std::cout << "Closing socket ?" << this-> _sockfd << " " << i << std::endl;


			this-> _sockfd = 0;
			this-> _addr = SockAddrV4 (0, 0);
			this-> _error = false;
		}
	}

	TcpStream::~TcpStream () {
		this-> close ();
	}

}




std::ostream & operator<< (std::ostream & s, const rd_utils::net::TcpStream & stream) {
	s << "SOCK(" << stream.getHandle () << ")";
	return s;
}
