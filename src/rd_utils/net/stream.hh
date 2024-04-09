#pragma once

#include <iostream>
#include <rd_utils/net/addr.hh>
#include <string>
#include <sys/socket.h>


namespace rd_utils {

	namespace net {

		class TcpListener;
		class TcpStream {

			// True when the socket failed to read or write
			bool _error = false;

			/// The socket of the stream
			int _sockfd = 0;

			/// The address connected or not depending on _sockfd
			SockAddrV4 _addr;

		private :

			friend TcpListener;

			/**
			 * Construction of a stream from an already connected socket
			 * @warning: should be used only by the listener
			 */
			TcpStream (int sock, SockAddrV4 addr);

			TcpStream (const TcpStream & other);
			void operator=(const TcpStream & other);

		public :

			/**
			 * Move ctor
			 */
			TcpStream (TcpStream && other);

			/**
			 * Move affectation
			 */
			void operator=(TcpStream && other);

			/**
			 * Construction of a stream for a given tcp address
			 * @warning: does not connect the stream (cf. this-> connect);
			 */
			TcpStream (SockAddrV4 addr);

			/**
			 * ================================================================================
			 * ================================================================================
			 * =========================           GETTERS            =========================
			 * ================================================================================
			 * ================================================================================
			 */

			/**
			 * @returns: the file descriptor of the stream
			 */
			int getHandle () const;

			/**
			 * @returns: the address of the stream
			 */
			SockAddrV4 addr () const;

			/**
			 * @returns: true if the socket is still open
			 */
			bool isOpen () const;

			/**
			 * ================================================================================
			 * ================================================================================
			 * =========================           CONNECT            =========================
			 * ================================================================================
			 * ================================================================================
			 */

			/**
			 * Connect the stream as a client
			 * @info: use the addr given in the constructor
			 * @warning: close the current stream if connected to something
			 */
			void connect ();

			/**
			 * Close the stream if connected
			 */
			void close ();

			/**
			 * ================================================================================
			 * ================================================================================
			 * =========================        SEND / RECEIVE        =========================
			 * ================================================================================
			 * ================================================================================
			 */

			/**
			 * Send a int into the stream
			 * @params:
			 *   - i: the int to send
			 */
			bool sendInt (unsigned long i);


			bool sendChar (uint8_t c);


			/**
			 * Send a message through the stream
			 * @returns: true, iif the send was successful
			 */
			bool send (const std::string & msg);

			/**
			 * Send a message through the stream
			 */
			bool send (const char * buffer, int len);

			/**
			 * Receive a message in a pre allocated buffer
			 * @return: the length read
			 */
			int receive (char * buffer, int len);

			/**
			 * Receive a message in a pre allocated buffer
			 * @return: the length read
			 */
			template <typename T>
			bool receiveRaw (T * buffer, uint32_t nb) {
				if (this-> _sockfd != 0 && !this-> _error) {
					uint8_t * buf = (uint8_t*) buffer;
					uint32_t full = nb * sizeof (T);
					uint32_t got = 0;
					while (full > got) {
						auto valread = recv (this-> _sockfd, buf + got, full - got, 0);
						if (valread == -1) {
							this-> _error = true;
							return false;
						}
						got += valread;
					}

					return true;
				}

				return false;
			}

			/**
			 * Receive a message from the stream
			 */
			std::string receive (char until = '\0');

			/**
			 * Receive an int from the stream
			 */
			unsigned long receiveInt ();

			/**
			 * Receive 1 byte from the stream
			 */
			uint8_t receiveChar ();

			/**
			 * this-> close ();
			 */
			~TcpStream ();


		private:

			std::string receiveUntil (char until);

		};


	}

}


std::ostream & operator<< (std::ostream & s, const rd_utils::net::TcpStream & stream);
