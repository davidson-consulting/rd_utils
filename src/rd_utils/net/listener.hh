#pragma once

#include <rd_utils/net/addr.hh>
#include <rd_utils/net/stream.hh>


namespace rd_utils {

    namespace net {

		class TcpServer;
		class TcpListener {

			int _sockfd = 0;

			SockAddrV4 _addr;

			/// The binded port
			short _port;

		private :

			friend TcpServer;

		private:

			TcpListener (const TcpListener &);
			void operator=(const TcpListener&);

		public :

			/**
			 * Empty tcp listener bound to nothing
			 */
			TcpListener ();

			/**
			 * Create a tcp listener
			 * @params:
			 *    - addr: the address bind by the listener
			 */
			TcpListener (SockAddrV4 addr);

			/**
			 * Move ctor
			 */
			TcpListener (TcpListener && other);

			/**
			 * Move affectation
			 */
			void operator=(TcpListener && other);

			/**
			 * Start the tcp listener
			 */
			void start ();

			/**
			 * Accept incoming connexions
			 */
			TcpStream accept ();

			/**
			 * Close the tcp listener
			 */
			void close ();
	    
			/**
			 * @returns: the binded port
			 */
			unsigned short port () const;

			/**
			 * @returns: the fd of the listener
			 */
			int getHandle () const;

			/**
			 * this-> close ()
			 */
			~TcpListener ();

		private :

			void bind ();
	    
		};
	

    }    

}
