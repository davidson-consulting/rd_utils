#pragma once

#include <rd_utils/net/stream.hh>
#include <vector>
#include <map>
#include <rd_utils/concurrency/taskpool.hh>
#include <memory>

#include <sys/epoll.h>

namespace rd_utils::net {

        class TcpSession;

        /**
         * Connection pool
         */
        class TcpPool {

                // The server address
                SockAddrV4 _addr;

                // mutex locked when updating connections
                concurrency::mutex _m;

                // signal emitted when a connection is released
                concurrency::semaphore _release;

                // The maximum number of opened connection
                uint32_t _max = 0;

                friend TcpSession;

                float _recvTimeout = -1;
                float _sendTimeout = -1;

        private :

                TcpPool (const TcpPool& );
                void operator=(const TcpPool&);

        public:

                /**
                 * @params:
                 *    - addr: the address of the server
                 *    - max: the maximum number of connection in the pool
                 */
                TcpPool (SockAddrV4 addr, int max);

                /**
                 * Move ctor
                 */
                TcpPool (TcpPool &&);

                /**
                 * Move affectation
                 */
                void operator=(TcpPool &&);

                /**
                 * @params:
                 *    - timeout: -1 means wait until found, otherwise throw if not found before timeout
                 * @returns: an avalaible stream
                 */
                TcpSession get (float timeout = 5);


                /**
                 * Set the timeout of the recv methods of all the session that can be created by this tcp pool
                 * @params:
                 *    - timeout: the timeout in second, <= 0 means indefinite
                 */
                void setRecvTimeout (float timeout);


                /**
                 * Set the timeout of the send methods of all the session that can be created by this tcp pool
                 * @params:
                 *    - timeout: the timeout in second, <= 0 means indefinite
                 */
                void setSendTimeout (float timeout);

                /**
                 * this-> dispose ();
                 */
                ~TcpPool ();

        private:

                /**
                 * Release a connection from the pool
                 */
                void release (std::shared_ptr <TcpStream> stream);

                TcpSession openNew ();

        };



}
