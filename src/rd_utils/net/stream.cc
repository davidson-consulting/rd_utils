#define LOG_LEVEL 10

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

	/**
	 * ================================================================================
	 * ================================================================================
	 * ==============================        SEND        ==============================
	 * ================================================================================
	 * ================================================================================
	 */

	bool TcpStream::sendU64 (uint64_t i, bool t) {
		return this-> inner_sendRaw (reinterpret_cast <const uint8_t*> (&i), sizeof (uint64_t), t);
	}

	bool TcpStream::sendU32 (uint32_t i, bool t) {
		return this-> inner_sendRaw (reinterpret_cast <const uint8_t*> (&i), sizeof (uint32_t), t);
	}

	bool TcpStream::sendI64 (int64_t i, bool t) {
		return this-> inner_sendRaw (reinterpret_cast <const uint8_t*> (&i), sizeof (int64_t), t);
	}

	bool TcpStream::sendI32 (int32_t i, bool t) {
		return this-> inner_sendRaw (reinterpret_cast <const uint8_t*> (&i), sizeof (int32_t), t);
	}

	bool TcpStream::sendChar (uint8_t i, bool t) {
		return this-> inner_sendRaw (reinterpret_cast <const uint8_t*> (&i), sizeof (uint8_t), t);
	}

	bool TcpStream::sendF32 (float i, bool t) {
		return this-> inner_sendRaw (reinterpret_cast <const uint8_t*> (&i), sizeof (float), t);
	}

	bool TcpStream::sendF64 (double i, bool t) {
		return this-> inner_sendRaw (reinterpret_cast <const uint8_t*> (&i), sizeof (double), t);
	}

	bool TcpStream::sendStr (const std::string & msg, bool t) {
		return this-> inner_sendRaw (reinterpret_cast <const uint8_t*> (msg.c_str ()), msg.length () * sizeof (char), t);
	}

	bool TcpStream::inner_sendRaw (const uint8_t * buffer, int len, bool t) {
		if (this-> _sockfd != 0 && !this-> _error) {
			uint32_t lenToSend = len;
			const uint8_t * curr = buffer;
			while (lenToSend != 0) {
				auto sent = ::send (this-> _sockfd, curr, lenToSend, 0);
				if (sent < 1) {
					this-> _error = true;
					if (t) throw std::runtime_error ("Stream is closed");
					return false;
				}

				lenToSend -= sent;
				curr += sent;
			}

			return true;
		}

		if (t) throw std::runtime_error ("Stream is closed");
		return false;
	}

	/**
	 * ================================================================================
	 * ================================================================================
	 * ==============================        RECV        ==============================
	 * ================================================================================
	 * ================================================================================
	 */

	bool TcpStream::inner_receiveRaw (uint8_t * buffer, uint32_t len, bool t) {
		if (this-> _sockfd != 0 && !this-> _error) {
			uint8_t * curr = buffer;
			uint32_t lenToRecv = len;
			while (lenToRecv > 0) {
				auto valread = ::recv (this-> _sockfd, curr, lenToRecv, 0);
				if (valread <= 0) {
					this-> _error = true;
					if (t) throw std::runtime_error ("Stream is closed");
					return false;
				}

				curr += valread;
				lenToRecv -= valread;
			}

			return true;
		}

		if (t) throw std::runtime_error ("Stream is closed");
		return false;
	}


	bool TcpStream::receiveStr (std::string & v, uint32_t len) {
		return this-> receiveStr (v, len, false);
	}

	std::string TcpStream::receiveStr (uint32_t len) {
		std::string v;
		this-> receiveStr (v, len, true);

		return v;
	}

	bool TcpStream::receiveStr (std::string & v, uint32_t len, bool t) {
		if (this-> _sockfd != 0 && !this-> _error) {
			std::stringstream result;
			uint32_t bufferSize = 1024;
			char buffer [bufferSize + 1];
			uint32_t lenToRecv = len;

			while (lenToRecv > 0) {
				uint32_t currentRecv = std::min (lenToRecv, bufferSize);
				auto valread = ::recv (this-> _sockfd, &buffer, currentRecv, 0);
				if(valread <= 0) {
					this-> _error = true;
					v = "";

					if (t) throw std::runtime_error ("Stream is closed");
					return false;
				}

				buffer [valread] = 0;
				result << buffer;
				lenToRecv -= valread;
			}

			v = result.str ();
			return true;
		}

		if (t) throw std::runtime_error ("Stream is closed");
		return false;
	}

	bool TcpStream::receiveI64 (int64_t & val) {
		return this-> inner_receiveRaw (reinterpret_cast <uint8_t*> (&val), sizeof (int64_t), false);
	}

	bool TcpStream::receiveI32 (int32_t & val) {
		return this-> inner_receiveRaw (reinterpret_cast <uint8_t*> (&val), sizeof (int32_t), false);
	}

	bool TcpStream::receiveU64 (uint64_t & val) {
		return this-> inner_receiveRaw (reinterpret_cast <uint8_t*> (&val), sizeof (uint64_t), false);
	}

	bool TcpStream::receiveU32 (uint32_t & val) {
		return this-> inner_receiveRaw (reinterpret_cast <uint8_t*> (&val), sizeof (uint32_t), false);
	}

	bool TcpStream::receiveF32 (float & val) {
		return this-> inner_receiveRaw (reinterpret_cast <uint8_t*> (&val), sizeof (float), false);
	}

	bool TcpStream::receiveF64 (double & val) {
		return this-> inner_receiveRaw (reinterpret_cast <uint8_t*> (&val), sizeof (double), false);
	}

	bool TcpStream::receiveChar (uint8_t & val) {
		return this-> inner_receiveRaw (reinterpret_cast <uint8_t*> (&val), sizeof (uint8_t), false);
	}


	int64_t TcpStream::receiveI64 () {
		int64_t val;
		this-> inner_receiveRaw (reinterpret_cast <uint8_t*> (&val), sizeof (int64_t), true);

		return val;
	}

	int32_t TcpStream::receiveI32 () {
		int32_t val;
		this-> inner_receiveRaw (reinterpret_cast <uint8_t*> (&val), sizeof (int32_t), true);

		return val;
	}

	uint64_t TcpStream::receiveU64 () {
		uint64_t val;
		this-> inner_receiveRaw (reinterpret_cast <uint8_t*> (&val), sizeof (uint64_t), true);

		return val;
	}

	uint32_t TcpStream::receiveU32 () {
		uint32_t val;
		this-> inner_receiveRaw (reinterpret_cast <uint8_t*> (&val), sizeof (uint32_t), true);

		return val;
	}

	uint8_t TcpStream::receiveChar () {
		uint8_t val;
		this-> inner_receiveRaw (reinterpret_cast <uint8_t*> (&val), sizeof (uint8_t), true);

		return val;
	}

	float TcpStream::receiveF32 () {
		float val;
		this-> inner_receiveRaw (reinterpret_cast <uint8_t*> (&val), sizeof (float), true);

		return val;
	}

	double TcpStream::receiveF64 () {
		double val;
		this-> inner_receiveRaw (reinterpret_cast <uint8_t*> (&val), sizeof (double), true);

		return val;
	}

	/**
	 * ================================================================================
	 * ================================================================================
	 * =============================        GETTERS        ============================
	 * ================================================================================
	 * ================================================================================
	 */

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
			::close (this-> _sockfd);

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
