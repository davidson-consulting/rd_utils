#pragma once

#include <rd_utils/concurrency/tpipe.hh>
#include <rd_utils/concurrency/_.hh>
#include <rd_utils/memory/box.hh>
#include <rd_utils/net/listener.hh>
#include <map>
#include <tuple>

namespace rd_utils::net {

        enum class TcpSessionKind : int {
                NEW,
                OLD,
                NONE
        };

        class TcpServer {
        private: // Socket management

                // The epoll list
                int _epoll_fd = 0;

                // The context of the queuing
                TcpListener _context;

                // The maximum number of clients that are allowed to connect at the same time
                int _maxConn;

                // The list of opened socket
                std::map <int, TcpStream*> _openSockets;

                // The fds of the open sockets (a socket can be closed manually during a session)
                std::map <TcpStream*, int> _socketFds;

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
                concurrency::signal<TcpSessionKind, TcpStream&> _onSession;

        private: // Task jobs/completed

                // The number of session submitted
                int _nbSubmitted = 0;

                // The list of socket whose session has to be launched
                concurrency::Mailbox<std::tuple<TcpSessionKind, TcpStream*> > _jobs;

                // The number of session completed
                int _nbCompleted = 0;

                // The list of socket whose session is finished
                concurrency::Mailbox<TcpStream*> _completed;

                // Closed threads
                concurrency::Mailbox<int> _closed;

        private: // Synchronization

                // The pipe to trigger when a thread close a session
                concurrency::ThreadPipe _trigger;

                // Mutex to lock when writing inside the trigger
                concurrency::mutex _triggerM;

                // Mutex to lock when waiting for a thread to be ready
                concurrency::mutex _ready;

                // Signal emitted when a mutex is ready
                concurrency::condition _readySig;

                // Mutex to lock when waiting a task
                concurrency::mutex _waitTask;

                // Condition emitted when a task is submitted
                concurrency::condition _waitTaskSig;

                // Mutex to lock when waiting for a task to be completed
                concurrency::mutex _completeM;

                // The signal emitted when a task is completed
                concurrency::condition _completeSig;


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
                 * Wait for a stream to be ready
                 */
                void start (void (*onSession)(TcpSessionKind, TcpStream&));

                /**
                 * Wait for a stream to be ready
                 */
                template <class X>
                void start (X* x, void (X::*onSession)(TcpSessionKind, TcpStream&)) {
                        if (this-> _started) throw utils::Rd_UtilsError ("Already running");

                        this-> _onSession.dispose ();
                        this-> _onSession.connect (x, onSession);

                        // need to to that in the main thread to catch the exception on binding failure
                        this-> configureEpoll ();

                        // Then spawning the thread with working tcplistener already configured
                        this-> _th = concurrency::spawn (this, &TcpServer::pollMain);
                        WITH_LOCK (this-> _ready) {
                                this-> _readySig.wait (this-> _ready);
                        }
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
                 * Stop the server in the future
                 */
                void stop ();

                /**
                 * Wait for the polling thread to stop
                 */
                void join ();

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
                void addEpoll (int fd);

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
                void submit (TcpSessionKind, TcpStream * stream);

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

        };

}
