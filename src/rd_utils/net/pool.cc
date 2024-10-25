
#ifndef __PROJECT__
#define __PROJECT__ "TCPPOOL"
#endif

#include "pool.hh"
#include <rd_utils/utils/log.hh>
#include <rd_utils/concurrency/timer.hh>
#include "session.hh"

namespace rd_utils::net {

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
        other._sendTimeout = -1;
        other._recvTimeout = -1;
    }

    void TcpPool::operator= (TcpPool && other) {
        this->_addr = std::move(other._addr);
        this->_open = std::move(other._open);
        this->_socketFds = std::move(other._socketFds);
        this->_free = std::move(other._free);
        this->_m = std::move(other._m);
        this->_release = std::move(other._release);
        this->_max = other._max;
        this-> _recvTimeout = other._recvTimeout;
        this-> _sendTimeout = other._sendTimeout;

        other._max = 0;
        other._sendTimeout = -1;
        other._recvTimeout = -1;
    }

    void TcpPool::setRecvTimeout (float timeout) {
        this-> _recvTimeout = timeout;
    }

    void TcpPool::setSendTimeout (float timeout) {
        this-> _sendTimeout = timeout;
    }

    TcpSession TcpPool::get (float timeout) {
        concurrency::timer t;
        WITH_LOCK (this-> _m) {
            if (this-> _open.size () < this-> _max) {
                auto s = std::make_shared <TcpStream> (this-> _addr);
                s-> setSendTimeout (this-> _sendTimeout);
                s-> setRecvTimeout (this-> _recvTimeout);

                try {
                    s-> connect ();
                } catch (utils::Rd_UtilsError & err) { // failed to connect
                    throw std::runtime_error ("Failed to connect");
                }

                // std::cout << "New socket : " << s-> getHandle () << " " << s.get () << std::endl;
                if (s != nullptr) {
                    // WARNING: if a socket is closed by hand, then we might reopen a second socket with the same fd
                    // In that case, socketFds might have the same values multiple times
                    this-> _open.emplace (s-> getHandle (), s);

                    // Socket might be closed manually, we need to store the current handle to clear later on
                    this-> _socketFds.emplace ((uint64_t) s.get (), s-> getHandle ());
                    return TcpSession (s, this);
                }
            }
        }

        float rest = timeout;
        for (;;) {
            if (rest > 0) {
                rest = timeout - t.time_since_start ();
                if (t.time_since_start () > timeout) {
                    throw std::runtime_error ("timeout");
                }
            }

            if (this-> _release.wait (rest)) {
                std::shared_ptr <TcpStream> conn = nullptr;
                if (this-> _free.receive (conn)) {
                    return TcpSession (conn, this);
                }
            } else {
                throw std::runtime_error ("timeout");
            }
        }
    }

    void TcpPool::release (std::shared_ptr <TcpStream> s) {
        if (!s-> isOpen ()) {
            WITH_LOCK (this-> _m) { // closing the socket that is either broken or closed on the other side
                auto handle = this-> _socketFds.find ((uint64_t) s.get ());

                auto it = this-> _open.find (handle-> second);
                if (it != this-> _open.end () && it-> second == s) {
                    // we might have inserted a new socket with the same handle in between if the socket was closed by hand
                    this-> _open.erase (handle-> second);
                }

                // However, the pointer of the stream is OK
                this-> _socketFds.erase ((uint64_t) s.get ());
                s-> close ();
            }
        } else {
            this-> _free.send (s);
        }

        this-> _release.post ();
    }

    void TcpPool::dispose () {
        WITH_LOCK (this-> _m) {
            this-> _free.dispose ();
            this-> _open.clear ();
        }
    }

    TcpPool::~TcpPool () {
        this-> dispose ();
    }


}
