#include "pool.hh"
#include <rd_utils/utils/log.hh>

namespace rd_utils::net {

    TcpSession::TcpSession (TcpStream * stream, TcpPool * context) :
        _stream (stream)
        , _context (context)
    {}

    TcpSession::TcpSession (TcpSession && other) :
        _stream (other._stream)
        , _context (other._context)
    {
        other._stream = nullptr;
        other._context = nullptr;
    }

    void TcpSession::operator= (TcpSession && other) {
        this-> dispose ();
        this-> _stream = other._stream;
        this-> _context = other._context;

        other._stream = nullptr;
        other._context = nullptr;
    }

    void TcpSession::dispose () {
        if (this-> _context != nullptr) {
            // std::cout << "Freeing " << this-> _stream-> getHandle () << std::endl;
            this-> _context-> release (this-> _stream);
            this-> _context = nullptr;
            this-> _stream = nullptr;
        }
    }

    TcpSession::~TcpSession () {
        this-> dispose ();
    }

    TcpStream* TcpSession::operator-> () {
        return this-> _stream;
    }

    TcpPool::TcpPool (SockAddrV4 addr, int max) :
        _addr (addr)
        , _max (max)
    {}

    TcpPool::TcpPool (TcpPool && other):
        _addr (std::move(other._addr))
        , _open (std::move(other._open))
        , _socketFds (std::move(other._socketFds))
        , _free (std::move(other._free))
        , _m (std::move(other._m))
        , _release (std::move(other._release))
        , _max (other._max)    
    {
        other._max = 0;
    }

    void TcpPool::operator= (TcpPool && other) {
        this->_addr = std::move(other._addr);
        this->_open = std::move(other._open);
        this->_socketFds = std::move(other._socketFds);
        this->_free = std::move(other._free);
        this->_m = std::move(other._m);
        this->_release = std::move(other._release);
        this->_max = other._max;

        other._max = 0;
    }

    TcpSession TcpPool::get () {
        bool check = false;
        WITH_LOCK (this-> _m) {
            if (this-> _open.size () < this-> _max) {
                TcpStream * s = new TcpStream (this-> _addr);
                try {
                    s-> connect ();
                } catch (utils::Rd_UtilsError err) { // failed to connect
                    delete s;
                    s = nullptr;
                }

                if (s != nullptr) {
                    // WARNING: if a socket is closed by hand, then we might reopen a second socket with the same fd
                    // In that case, socketFds might have the same values multiple times
                    this-> _open.emplace (s-> getHandle (), s);

                    // Socket might be closed manually, we need to store the current handle to clear later on
                    this-> _socketFds.emplace ((uint64_t) s, s-> getHandle ());
                    return TcpSession (s, this);
                }
            }
        }

        // LOG_INFO ("Waiting ?", pthread_self ());
        this-> _release.wait ();
        // LOG_INFO ("NO ?", pthread_self ());

        TcpStream * conn = nullptr;
        if (this-> _free.receive (conn)) {
            return TcpSession (conn, this);
        } else {
            return this-> get ();
        }
    }

    void TcpPool::release (TcpStream * s) {
        if (!s-> isOpen ()) {
            WITH_LOCK (this-> _m) { // closing the socket that is either broken or closed on the other side
                auto handle = this-> _socketFds.find ((uint64_t) s);

                auto it = this-> _open.find (handle-> second);
                if (it != this-> _open.end () && it-> second == s) {
                    // we might have inserted a new socket with the same handle in between if the socket was closed by hand
                    this-> _open.erase (handle-> second);
                }

                // However, the pointer of the stream is OK
                this-> _socketFds.erase ((uint64_t) s);

                s-> close ();
                delete s;
            }
        } else {
            this-> _free.send (s);
        }

        this-> _release.post ();
    }

    void TcpPool::dispose () {
        WITH_LOCK (this-> _m) {
            this-> _free.clear ();
            for (auto & [it, sock] : this-> _open) {
                delete sock;
            }

            this-> _open.clear ();
        }
    }

    TcpPool::~TcpPool () {
        this-> dispose ();
    }


}
