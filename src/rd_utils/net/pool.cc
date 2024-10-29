
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
    {
        if (this-> _max > 0) {
            for (uint64_t i = 0 ; i < this-> _max ; i++) {
                this-> _release.post ();
            }
        }
    }

    TcpPool::TcpPool (TcpPool && other):
        _addr (std::move(other._addr))
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

        float rest = timeout;
        for (;;) {
            if (rest > 0) {
                rest = timeout - t.time_since_start ();
                if (t.time_since_start () > timeout) {
                    throw std::runtime_error ("timeout");
                }
            }

            if (this-> _release.wait (rest)) {
                return this-> openNew ();
            }

            throw std::runtime_error ("timeout");
        }
    }

    TcpSession TcpPool::openNew () {
        WITH_LOCK (this-> _m) {
            auto s = std::make_shared <TcpStream> (this-> _addr);
            s-> setSendTimeout (this-> _sendTimeout);
            s-> setRecvTimeout (this-> _recvTimeout);

            try {
                s-> connect ();
            } catch (utils::Rd_UtilsError & err) { // failed to connect
                throw std::runtime_error ("Failed to connect");
            }

            // std::cout << "New socket in Pool : " << s-> getHandle () << " " << s.get () << std::endl;
            if (s != nullptr) {
                return TcpSession (s, this);
            }

            throw std::runtime_error ("Failed to connect");
        }
    }

    void TcpPool::release (std::shared_ptr <TcpStream> s) {
        WITH_LOCK (this-> _m) {
            s-> close ();
            this-> _release.post ();
        }
    }

    TcpPool::~TcpPool () {}

}
