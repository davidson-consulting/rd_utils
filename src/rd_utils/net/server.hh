#pragma once

#include <rd_utils/concurrency/tpipe.hh>
#include <rd_utils/concurrency/mailbox.hh>
#include <rd_utils/concurrency/thread.hh>
#include <rd_utils/concurrency/signal.hh>
#include <rd_utils/net/listener.hh>
#include <map>
#include <set>
#include <tuple>
#include <memory>

namespace rd_utils::net {

        class TcpSession;
        enum class TcpSessionKind : int {
                NEW,
                OLD,
                NONE
        };

        struct MailElement {
                TcpSessionKind kind;
                std::shared_ptr <TcpStream> stream;
        };


        /**
         * A Tcp server is used to managed arriving clients from a Tcp connection pool.
         *
         * It manages a list of threads used to answer client
         * requests while ensure there are never more than a fixed amount of
         * threads, and more than a fixed amount of new clients
         *
         * @info: A session is triggered when a client is sending data to the server
         * @example:
         * ========
         * void foo (TcpSession& session) {
         *     auto msg = session-> receiveStr ();
         *     session-> sendStr ("ACK");
         *
         *     std::cout << "Message from client : " << msg << std::endl;
         * }
         *
         *
         * // Tcp server accepting connection on port 8080, coming for any addr
         * // 4 threads to manage client session, and never more than 100 connected clients
         * TcpServer server (SockAddrV4 ("0.0.0.0", 8080), 4, 100);
         * server.start (&foo); // start the server, and call 'foo' when client sends data to the server
         *
         * server.join (); // infinite join, cf: server.stop ();
         * ========
         */
        class TcpServer {
        private: // Socket management

                // The epoll list
                int _epoll_fd = 0;

                // The context of the queuing
                TcpListener _context;

                // The maximum number of clients that are allowed to connect at the same time
                int _maxConn;

                // The list of opened socket
                std::map <int, std::shared_ptr <TcpStream> > _openSockets;

                // The fds of the open sockets (a socket can be closed manually during a session)
                std::map <std::shared_ptr <TcpStream>, int> _socketFds;

                // Timeout of the recv methods of the session created by this server
                float _recvTimeout = -1;

                // Timeout of the send methods of the session created by this server
                float _sendTimeout = -1;

        private: // Task pool

                // True if the polling is running
                bool _started = false;

                // The number of threads in the pool
                int _nbThreads;

                // The polling thread
                concurrency::Thread _th;

                // The worker threads
                std::map <int, concurrency::Thread> _runningThreads;

                // The signal emitted when a session is started
                concurrency::signal<TcpSessionKind, TcpSession& > _onSession;

                // The signal emitted when a session is started using a ptr
                concurrency::signal<TcpSessionKind, std::shared_ptr <TcpSession> > _onSessionPtr;

        private: // Task jobs/completed

                // The number of session submitted
                int _nbSubmitted = 0;

                // The list of socket whose session has to be launched
                concurrency::Mailbox<MailElement> _jobs;

                // The number of session completed
                int _nbCompleted = 0;

                // The list of socket whose session is finished
                concurrency::Mailbox<std::shared_ptr <TcpStream> > _completed;

                // Closed threads
                concurrency::Mailbox<int> _closed;

        private: // Synchronization

                // The pipe to trigger when a thread close a session
                concurrency::ThreadPipe _trigger;

                // Mutex to lock when writing inside the trigger
                concurrency::mutex _triggerM;

                // Mutex to lock when waiting for a thread to be ready
                concurrency::semaphore _ready;

                // Signal emitted when a task is ready
                concurrency::semaphore _waitTask;

        private:

                TcpServer (const TcpServer & other);
                void operator=(const TcpServer& other);

        public:

                /**
                 * Empty tcp queue
                 */
                TcpServer ();

                /**
                 * Create a tcp watcher
                 */
                TcpServer (SockAddrV4 addr, int nbThreads, int maxCon = -1);

                /**
                 * Move ctor
                 */
                TcpServer (TcpServer && other);

                /**
                 * Move affectation
                 */
                void operator=(TcpServer && other);

                /**
                 * Start the server
                 * @params:
                 *    - onSession: the function to call when a stream is ready to read/write
                 */
                void start (void (*onSession)(TcpSessionKind, TcpSession&));

                /**
                 * Start the server
                 * @params:
                 *    - onSession: the function to call when a stream is ready to read/write
                 */
                void start (void (*onSession)(TcpSessionKind, std::shared_ptr<TcpSession>));

                /**
                 * Start the server
                 * @params:
                 *    - onSession: the function to call when a stream is ready to read/write
                 */
                template <class X>
                void start (X* x, void (X::*onSession)(TcpSessionKind, TcpSession&)) {
                        if (this-> _started) throw utils::Rd_UtilsError ("Already running");

                        this-> _onSession.dispose ();
                        this-> _onSessionPtr.dispose ();
                        this-> _onSession.connect (x, onSession);

                        // need to to that in the main thread to catch the exception on binding failure
                        this-> configureEpoll ();

                        // Then spawning the thread with working tcplistener already configured
                        this-> _th = concurrency::spawn (this, &TcpServer::pollMain);
                        this-> _ready.wait ();
                }

                /**
                 * Start the server
                 * @params:
                 *    - onSession: the function to call when a stream is ready to read/write
                 */
                template <class X>
                void start (X* x, void (X::*onSession)(TcpSessionKind, std::shared_ptr <TcpSession> )) {
                        if (this-> _started) throw utils::Rd_UtilsError ("Already running");

                        this-> _onSession.dispose ();
                        this-> _onSessionPtr.dispose ();
                        this-> _onSessionPtr.connect (x, onSession);

                        // need to to that in the main thread to catch the exception on binding failure
                        this-> configureEpoll ();

                        // Then spawning the thread with working tcplistener already configured
                        this-> _th = concurrency::spawn (this, &TcpServer::pollMain);
                        this-> _ready.wait ();
                }

                /**
                 * @return: the port number of the listener
                 */
                int port () const;

                /**
                 * @returns: the current number of connected socket
                 */
                int nbConnected () const;

                /**
                 * Remember a socket that was forgotten
                 */
                void release (std::shared_ptr <net::TcpStream> stream);

                /**
                 * Set the timeout of the recv methods created by this server
                 * @params:
                 *    - timeout: the time in seconds, <= 0 means indefinite
                 */
                void setRecvTimeout (float timeout);

                /**
                 * Set the timeout of the send methods created by this server
                 * @params:
                 *    - timeout: the time in seconds, <= 0 means indefinite
                 */
                void setSendTimeout (float timeout);

                /**
                 * Stop the server in the future
                 */
                void stop ();

                /**
                 * Wait for the polling thread to stop
                 * @returns: true iif the server was effectively joined
                 * @info: the join can fail if it is called from a worker thread of the server
                 */
                bool join ();

                /**
                 * Close all opened connection
                 */
                void dispose ();

                /**
                 * this-> dispose ()
                 */
                ~TcpServer ();

        private:

                /**
                 * Close all the socket that are finished
                 */
                void closeAllError ();

                /**
                 * Configure the epoll elements
                 */
                void configureEpoll ();

                /**
                 * Add file descriptor to epoll list
                 */
                void addEpoll (TcpSessionKind, int fd);

                /**
                 * delete file descriptor from epoll list
                 */
                void delEpoll (int fd);

                /**
                 * The main loop of the poll
                 */
                void pollMain (concurrency::Thread);

                /**
                 * The thread managing sessions
                 */
                void workerThread (concurrency::Thread);

                /**
                 * Submit a new session to the task pool
                 */
                void submit (TcpSessionKind, std::shared_ptr <TcpStream> stream);

                /**
                 * Reload all the socket whose session is finished
                 */
                void reloadAllFinished ();

                /**
                 * Spawn the worker threads
                 */
                void spawnThreads ();

                /**
                 * Wait for all submitted tasks to complete
                 */
                void waitAllCompletes ();

                /**
                 * @returns: true if the current thread is a worker thread instead of a main thread
                 */
                bool isWorkerThread () const;

        };



}
