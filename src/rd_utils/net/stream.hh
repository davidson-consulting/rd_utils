#pragma once

#include <iostream>
#include <rd_utils/net/addr.hh>
#include <string>


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
	    
	    
			/**
			 * Send a message through the stream
			 * @returns: true, iif the send was successful
			 */
			bool send (const std::string & msg);
	    
			/**
			 * Receive a message from the stream
			 * @params:
			 *   - size: the size of the string to receive
			 */
			std::string receive ();

			/**
			 * Receive an int from the stream
			 */
			unsigned long receiveInt ();

			/**
			 * this-> close ();
			 */
			~TcpStream ();

		};
	
	
	}

}


std::ostream & operator<< (std::ostream & s, const rd_utils::net::TcpStream & stream);
