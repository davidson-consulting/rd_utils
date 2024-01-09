#pragma once

#include <rd_utils/concurrency/tpipe.hh>
#include <rd_utils/concurrency/_.hh>
#include <rd_utils/memory/box.hh>
#include <rd_utils/net/listener.hh>
#include <map>

namespace rd_utils::net {

    enum class TcpSessionKind : int {
        NEW,
        OLD,
        NONE
    };

    class TcpServer {
    private:

        // True if the polling is running
        bool _started = false;

        // The polling thread
        concurrency::Thread _th;

        // The context of the queuing
        TcpListener _context;

        // The list of opened socket
        std::map <int, memory::Box<TcpStream>> _openSockets;

        // The pipe to trigger when a thread close a session
        concurrency::ThreadPipe _trigger;

        // The mutex to lock
        concurrency::mutex _m;

        // The task pool of the queue
        concurrency::TaskPool _pool;

        // The signal emitted on new session
        concurrency::signal<TcpSessionKind, TcpStream&> _onSession;

        // The epoll list
        int _epoll_fd = 0;

        // The number of element in the epoll list
        int _epoll_nb = 0;

        // Mutex locked when waiting for a thread to be ready
        concurrency::mutex _ready;

        // Signal emitted by a thread when ready
        concurrency::condition _readySig;

        // Mutex locked during a epoll update
        concurrency::mutex _updating;

        // Signal emitted when update is finished
        concurrency::condition _updatingSig;

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
        TcpServer (SockAddrV4 addr, int nbThreads);

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
         * Configure the epoll elements
         */
        void configureEpoll ();

        /**
         * The main loop of the poll
         */
        void pollMain (concurrency::Thread);

        /**
         * Submit a new session to the task pool
         */
        void submit (TcpSessionKind, TcpStream * stream);

        /**
         * Main part of a session task
         */
        void onSessionMain (TcpSessionKind, TcpStream * stream);

        /**
         * Close a tcp session
         */
        void close (TcpStream * stream);

    };

}
