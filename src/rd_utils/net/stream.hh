#pragma once

#include <iostream>
#include <rd_utils/net/addr.hh>
#include <string>
#include <sys/socket.h>
#include <cstdint>
#include <optional>

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

			bool _connect = false;

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
			 * =========================           SETTERS            =========================
			 * ================================================================================
			 * ================================================================================
			 */

			/**
			 * Set the timeout of a the recv methods in seconds
			 * @params:
			 *    - time: the timeout in second, <= 0 means indefinite
			 */
			TcpStream & setRecvTimeout (float time);

			/**
			 * Set the timeout of a the send methods in seconds
			 * @params:
			 *    - time: the timeout in second, <= 0 means indefinite
			 */
			TcpStream & setSendTimeout (float time);

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
			 * Send 8 bytes to the stream
			 * @params:
			 *   - throwIfFail: if true instead of returning false, throw an exception in case of failure
			 * @returns:
			 *    - true: if succeeded
			 */
			bool sendI64 (int64_t i, bool throwIfFail = true);

			/**
			 * Send 4 bytes to the stream
			 * @params:
			 *   - throwIfFail: if true instead of returning false, throw an exception in case of failure
			 * @returns:
			 *    - true: if succeeded
			 */
			bool sendI32 (int32_t i, bool throwIfFail = true);

			/**
			 * Send 8 bytes to the stream
			 * @params:
			 *   - throwIfFail: if true instead of returning false, throw an exception in case of failure
			 * @returns:
			 *    - true: if succeeded
			 */
			bool sendU64 (uint64_t i, bool throwIfFail = true);

			/**
			 * Send 4 bytes to the stream
			 * @params:
			 *   - throwIfFail: if true instead of returning false, throw an exception in case of failure
			 * @returns:
			 *    - true: if succeeded
			 */
			bool sendU32 (uint32_t i, bool throwIfFail = true);

			/**
			 * Send a 4 byte float to the stream
			 * @params:
			 *   - throwIfFail: if true instead of returning false, throw an exception in case of failure
			 * @returns:
			 *    - true: if succeeded
			 */
			bool sendF32 (float value, bool throwIfFail = true);

						/**
			 * Send a 4 byte float to the stream
			 * @params:
			 *   - throwIfFail: if true instead of returning false, throw an exception in case of failure
			 * @returns:
			 *    - true: if succeeded
			 */
			bool sendF64 (double value, bool throwIfFail = true);

			/**
			 * Send 1 bytes to the stream
			 * @params:
			 *   - throwIfFail: if true instead of returning false, throw an exception in case of failure
			 * @returns:
			 *    - true: if succeeded
			 */
			bool sendChar (uint8_t c, bool throwIfFail = true);

			/**
			 * Send a message through the stream
			 * @params:
			 *   - throwIfFail: if true instead of returning false, throw an exception in case of failure
			 * @returns: true, iif the send was successful
			 */
			bool sendStr (const std::string & msg, bool throwIfFail = true);

			/**
			 * Send a message of an arbitrary type
			 * @params:
			 *   - nb: the number of element of type T in the buffer
			 *   - throwIfFail: if true instead of returning false, throw an exception in case of failure
			 * @warning: pointer types won't be correctly sent, only works with flat data
			 * @returns:
			 *     - true: if the send worked
			 */
			template <typename T>
			bool sendRaw (const T * buffer, uint32_t nb, bool throwIfFail = true) {
				return this-> inner_sendRaw (reinterpret_cast <const uint8_t*> (buffer), nb * sizeof (T), throwIfFail);
			}

			/**
			 * ================================================================================
			 * ================================================================================
			 * ==============================        RECV        ==============================
			 * ================================================================================
			 * ================================================================================
			 */

			/**
			 * Receive a message in a pre allocated buffer
			 * @params:
			 *   - throwIfFail: if true instead of returning false, throw an exception in case of failure
			 * @return: the length read
			 */
			template <typename T>
			bool receiveRaw (T * buffer, uint32_t nb, bool throwIfFail = true) {
				return this-> inner_receiveRaw (reinterpret_cast <uint8_t*> (buffer), nb * sizeof (T), throwIfFail);
			}

			/**
			 * Receive a string from the stream
			 * @throws: if fails
			 * @returns:
			 *    - v: the read value
			 *    - true if a value was read false otherwise
			 */
			std::string receiveStr (uint32_t len);

			/**
			 * Receive a string from the stream
			 * @params:
			 *   - throwIfFail: if true instead of returning false, throw an exception in case of failure
			 * @returns:
			 *    - v: the read value
			 *    - true if a value was read false otherwise
			 */
			bool receiveStr (std::string & v, uint32_t len);

			/**
			 * Receive 8 bytes from the stream
			 * @params:
			 *   - throwIfFail: if true instead of returning false, throw an exception in case of failure
			 * @returns:
			 *   - v: the read value
			 *   - true if something was read, false otherwise
			 */
			bool receiveI64 (int64_t & v);

			/**
			 * Receive 8 bytes from the stream
			 * @throws: if fails
			 * @returns:
			 *   - v: the read value
			 *   - true if something was read, false otherwise
			 */
			int64_t receiveI64 ();

			/**
			 * Receive 4 bytes from the stream
			 * @returns:
			 *   - v: the read value
			 *   - true if something was read, false otherwise
			 */
			bool receiveI32 (int32_t & v);

			/**
			 * Receive 8 bytes from the stream
			 * @throws: if fails
			 * @returns:
			 *   - v: the read value
			 *   - true if something was read, false otherwise
			 */
			int32_t receiveI32 ();

			/**
			 * Receive 8 bytes from the stream
			 * @returns:
			 *   - v: the read value
			 *   - true if something was read, false otherwise
			 */
			bool receiveU64 (uint64_t & v);

			/**
			 * Receive 8 bytes from the stream
			 * @throws: if fails
			 * @returns:
			 *   - v: the read value
			 *   - true if something was read, false otherwise
			 */
			uint64_t receiveU64 ();

			/**
			 * Receive 4 bytes from the stream
			 * @returns:
			 *   - v: the read value
			 *   - true if something was read, false otherwise
			 */
			bool receiveU32 (uint32_t & v);

			/**
			 * Receive 4 bytes from the stream
			 * @throws: if fails
			 * @returns:
			 *   - v: the read value
			 *   - true if something was read, false otherwise
			 */
			uint32_t receiveU32 ();

			/**
			 * Receive 4 bytes float from the stream
			 * @params:
			 *   - throwIfFail: if true instead of returning false, throw an exception in case of failure
			 * @returns:
			 *   - v: the read value
			 *   - true if something was read, false otherwise
			 */
			bool receiveF32 (float & v);

			/**
			 * Receive 4 bytes float from the stream
			 * @throws: if fails reading
			 * @returns:
			 *   - v: the read value
			 */
			float receiveF32 ();

			/**
			 * Receive 8 bytes float from the stream
			 * @returns:
			 *   - v: the read value
			 *   - true if something was read, false otherwise
			 */
			bool receiveF64 (double & v);

			/**
			 * Receive 8 bytes float from the stream
			 * @throws: if fails reading
			 * @returns:
			 *   - v: the read value
			 */
			double receiveF64 ();

			/**
			 * Receive 1 byte from the stream
			 * @throws: if fails reading
			 * @returns:
			 *   - v: the read value
			 */
			uint8_t receiveChar ();


			/**
			 * Receive 1 byte from the stream
			 * @throws: if fails reading
			 * @returns:
			 *   - v: the read value
			 */
			bool receiveChar (uint8_t & v);

			/**
			 * this-> close ();
			 */
			~TcpStream ();

		private:

			/**
			 * Receive string data from the stream
			 * @params:
			 *   - throwIfFail: if true instead of returning false, throw an exception in case of failure
			 * @info: blocking function, waits until everything is read of nothing is left
			 */
			bool receiveStr (std::string & v, uint32_t len, bool t);

			/**
			 * Receive raw data from the stream
			 * @params:
			 *   - throwIfFail: if true instead of returning false, throw an exception in case of failure
			 * @info: blocking function, waits until everything is read of nothing is left
			 */
			bool inner_receiveRaw (uint8_t * buffer, uint32_t len, bool throwIfFail);

			/**
			 * Send a message through the stream
			 * @params:
			 *   - throwIfFail: if true instead of returning false, throw an exception in case of failure
			 */
			bool inner_sendRaw (const uint8_t * buffer, int len, bool throwIfFail);

		};


	}

}


std::ostream & operator<< (std::ostream & s, const rd_utils::net::TcpStream & stream);
