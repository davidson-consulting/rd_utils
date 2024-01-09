#pragma once

#include <rd_utils/concurrency/tpipe.hh>
#include <rd_utils/concurrency/_.hh>
#include <rd_utils/net/listener.hh>
#include <map>

namespace rd_utils::net {

    enum class TcpSessionKind : int {
        NEW,
        OLD,
        NONE
    };

    class TcpQueue {
    private:

        // The context of the queuing
        TcpListener * _context;

        // The list of opened socket
        std::map <int, TcpStream*> _openSockets;

        // The pipe to trigger when a thread close a session
        concurrency::ThreadPipe _trigger;

        // The mutex to lock
        concurrency::mutex _m;

        // The task pool of the queue
        concurrency::TaskPool _pool;

        // Task to execute when a session is submitted
        concurrency::signal<TcpSessionKind, TcpStream&> _onSession;

        // The epoll list
        int _epoll_fd = 0;

        // The number of element in the epoll list
        int _epoll_nb = 0;

    private:

        TcpQueue (const TcpQueue & other);
        void operator=(const TcpQueue& other);

    public:

        /**
         * Empty tcp queue
         */
        TcpQueue ();

        /**
         * Create a tcp watcher
         */
        TcpQueue (TcpListener & context, int nbThreads, void (*onSession)(TcpSessionKind, TcpStream&));

        /**
         * Move ctor
         */
        TcpQueue (TcpQueue && other);

        /**
         * Move affectation
         */
        void operator=(TcpQueue && other);

        /**
         * Wait for a stream to be ready
         */
        void poll ();

        /**
         * @returns: the size of the queue
         */
        int len () const;

        /**
         * Close all opened connection
         */
        void dispose ();

        /**
         * this-> dispose ()
         */
        ~TcpQueue ();

    private:

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
