#include <rd_utils/concurrency/iopipe.hh>
#include <unistd.h>
#include <sstream>
#include <fcntl.h>
#include <iostream>
#include <string.h>

namespace rd_utils::concurrency {

    /**
     * ================================================================================
     * ================================================================================
     * =========================            OPIPE             =========================
     * ================================================================================
     * ================================================================================
     */
	
    OPipe::OPipe (int o) : _pipe (o)
    {
        int err_flags = ::fcntl(this-> _pipe, F_GETFL, 0);
        ::fcntl(this-> _pipe, F_SETFL, err_flags | O_NONBLOCK);
    }

    OPipe::OPipe (OPipe && o) : _pipe (std::move (o._pipe)) {
        o._pipe = 0;
    }

    void OPipe::operator= (OPipe && other) {
        this-> dispose ();
        this-> _pipe = other._pipe;
        other._pipe = 0;
    }

	bool OPipe::writeU64 (uint64_t i, bool t) {
		return this-> inner_writeRaw (reinterpret_cast <const uint8_t*> (&i), sizeof (uint64_t), t);
	}

	bool OPipe::writeU32 (uint32_t i, bool t) {
		return this-> inner_writeRaw (reinterpret_cast <const uint8_t*> (&i), sizeof (uint32_t), t);
	}

	bool OPipe::writeI64 (int64_t i, bool t) {
		return this-> inner_writeRaw (reinterpret_cast <const uint8_t*> (&i), sizeof (int64_t), t);
	}

	bool OPipe::writeI32 (int32_t i, bool t) {
		return this-> inner_writeRaw (reinterpret_cast <const uint8_t*> (&i), sizeof (int32_t), t);
	}

	bool OPipe::writeChar (uint8_t i, bool t) {
		return this-> inner_writeRaw (reinterpret_cast <const uint8_t*> (&i), sizeof (uint8_t), t);
	}

	bool OPipe::writeF32 (float i, bool t) {
		return this-> inner_writeRaw (reinterpret_cast <const uint8_t*> (&i), sizeof (float), t);
	}

	bool OPipe::writeF64 (double i, bool t) {
		return this-> inner_writeRaw (reinterpret_cast <const uint8_t*> (&i), sizeof (double), t);
	}

	bool OPipe::writeStr (const std::string & msg, bool t) {
		return this-> inner_writeRaw (reinterpret_cast <const uint8_t*> (msg.c_str ()), msg.length () * sizeof (char), t);
	}

	bool OPipe::inner_writeRaw (const uint8_t * buffer, int len, bool t) {
		if (this-> _pipe != 0 && !this-> _error) {
			uint32_t lenToSend = len;
			const uint8_t * curr = buffer;
			while (lenToSend != 0) {
				uint32_t sent = ::write (this-> _pipe, curr, lenToSend);
				if (sent < 1) {
					this-> _error = true;
					if (t) throw std::runtime_error ("Stream is closed");
					return false;
				}

				lenToSend -= sent;
				curr += sent;
			}

			return true;
		}

		if (t) throw std::runtime_error ("Stream is closed");
		return false;
	}

    void OPipe::dispose () {
        if (this-> _pipe != 0 && this-> _pipe != STDOUT_FILENO && this-> _pipe != STDERR_FILENO) {
            ::close (this-> _pipe);
            this-> _pipe = 0;
        }
    }

    bool OPipe::isOpen () const {
        return this-> _pipe != 0 && !this-> _error;
    }

    void OPipe::setNonBlocking () {
        auto old_flg = fcntl (this-> _pipe, F_GETFL, 0);
        fcntl (this-> _pipe, F_SETFL, old_flg | O_NONBLOCK);
	
    }
   
    void OPipe::setBlocking () {
        auto old_flg = fcntl (this-> _pipe, F_GETFL, 0);
        fcntl (this-> _pipe, F_SETFL, old_flg & ~O_NONBLOCK);
    }
    
    int OPipe::getHandle () const {
        return this-> _pipe;
    }
	
    OPipe::~OPipe () {
        this-> dispose ();
    }

    /**
     * ================================================================================
     * ================================================================================
     * =========================            IPIPE             =========================
     * ================================================================================
     * ================================================================================
     */
	
    IPipe::IPipe (int i) : _pipe (i)
    {
        int err_flags = ::fcntl(this-> _pipe, F_GETFL, 0);
        ::fcntl(this-> _pipe, F_SETFL, err_flags | O_NONBLOCK);
    }
   
    IPipe::IPipe (IPipe && o) : _pipe (std::move (o._pipe)) {
        o._pipe = 0;
    }


    void IPipe::operator= (IPipe && other) {
        this-> dispose ();
        this-> _pipe = other._pipe;
        other._pipe = 0;
    }
    
	bool IPipe::inner_readRaw (uint8_t * buffer, uint32_t len, bool t) {
		if (this-> _pipe != 0 && !this-> _error) {
			uint8_t * curr = buffer;
			uint32_t lenToRecv = len;
			while (lenToRecv > 0) {
				auto valread = ::read (this-> _pipe, curr, lenToRecv);
				if (valread <= 0) {
					this-> _error = true;
					if (t) throw std::runtime_error ("Stream is closed");
					return false;
				}

				curr += valread;
				lenToRecv -= valread;
			}

			return true;
		}

		if (t) throw std::runtime_error ("Stream is closed");
		return false;
	}


	bool IPipe::readStr (std::string & v, uint32_t len) {
		return this-> readStr (v, len, false);
	}

	std::string IPipe::readStr (uint32_t len) {
		std::string v;
		this-> readStr (v, len, true);

		return v;
	}

	bool IPipe::readStr (std::string & v, uint32_t len, bool t) {
		if (this-> _pipe != 0 && !this-> _error) {
			std::stringstream result;
			uint32_t bufferSize = 1024;
			char buffer [bufferSize + 1];
			uint32_t lenToRecv = len;

			while (lenToRecv > 0) {
				uint32_t currentRecv = std::min (lenToRecv, bufferSize);
				auto valread = ::read (this-> _pipe, &buffer, currentRecv);
				if(valread <= 0) {
					this-> _error = true;
					v = "";

					if (t) throw std::runtime_error ("Stream is closed");
					return false;
				}

				buffer [valread] = 0;
				result << buffer;
				lenToRecv -= valread;
			}

			v = result.str ();
			return true;
		}

		if (t) throw std::runtime_error ("Stream is closed");
		return false;
	}

	bool IPipe::readI64 (int64_t & val) {
		return this-> inner_readRaw (reinterpret_cast <uint8_t*> (&val), sizeof (int64_t), false);
	}

	bool IPipe::readI32 (int32_t & val) {
		return this-> inner_readRaw (reinterpret_cast <uint8_t*> (&val), sizeof (int32_t), false);
	}

	bool IPipe::readU64 (uint64_t & val) {
		return this-> inner_readRaw (reinterpret_cast <uint8_t*> (&val), sizeof (uint64_t), false);
	}

	bool IPipe::readU32 (uint32_t & val) {
		return this-> inner_readRaw (reinterpret_cast <uint8_t*> (&val), sizeof (uint32_t), false);
	}

	bool IPipe::readF32 (float & val) {
		return this-> inner_readRaw (reinterpret_cast <uint8_t*> (&val), sizeof (float), false);
	}

	bool IPipe::readF64 (double & val) {
		return this-> inner_readRaw (reinterpret_cast <uint8_t*> (&val), sizeof (double), false);
	}

	bool IPipe::readChar (uint8_t & val) {
		return this-> inner_readRaw (reinterpret_cast <uint8_t*> (&val), sizeof (uint8_t), false);
	}


	int64_t IPipe::readI64 () {
		int64_t val;
		this-> inner_readRaw (reinterpret_cast <uint8_t*> (&val), sizeof (int64_t), true);

		return val;
	}

	int32_t IPipe::readI32 () {
		int32_t val;
		this-> inner_readRaw (reinterpret_cast <uint8_t*> (&val), sizeof (int32_t), true);

		return val;
	}

	uint64_t IPipe::readU64 () {
		uint64_t val;
		this-> inner_readRaw (reinterpret_cast <uint8_t*> (&val), sizeof (uint64_t), true);

		return val;
	}

	uint32_t IPipe::readU32 () {
		uint32_t val;
		this-> inner_readRaw (reinterpret_cast <uint8_t*> (&val), sizeof (uint32_t), true);

		return val;
	}

	uint8_t IPipe::readChar () {
		uint8_t val;
		this-> inner_readRaw (reinterpret_cast <uint8_t*> (&val), sizeof (uint8_t), true);

		return val;
	}

	float IPipe::readF32 () {
		float val;
		this-> inner_readRaw (reinterpret_cast <uint8_t*> (&val), sizeof (float), true);

		return val;
	}

	double IPipe::readF64 () {
		double val;
		this-> inner_readRaw (reinterpret_cast <uint8_t*> (&val), sizeof (double), true);

		return val;
	}

    void IPipe::setNonBlocking () {
        auto old_flg = fcntl (this-> _pipe, F_GETFL, 0);
        fcntl (this-> _pipe, F_SETFL, old_flg | O_NONBLOCK);
    }

    void IPipe::setBlocking () {
        auto old_flg = fcntl (this-> _pipe, F_GETFL, 0);
        fcntl (this-> _pipe, F_SETFL, old_flg & ~O_NONBLOCK);
    }

    void IPipe::dispose () {
        if (this-> _pipe != 0 && this-> _pipe != STDIN_FILENO) {
            ::close (this-> _pipe);
            this-> _pipe = 0;
        }
    }

    bool IPipe::isOpen () const {
        return this-> _pipe != 0 && !this-> _error;
    }

    int IPipe::getHandle () const {
        return this-> _pipe;
    }
	
    IPipe::~IPipe () {
        this-> dispose ();
    }

    /**
     * ================================================================================
     * ================================================================================
     * =========================              IO              =========================
     * ================================================================================
     * ================================================================================
     */

	
    iopipe createPipes (bool create) {
        iopipe res;
        int pipes[2] = {0, 0};
        if (create) {
            if (::pipe (pipes) != 0) {
                throw std::runtime_error ("failed creating pipes");
            }
        }

        res.i = pipes [0];
        res.o = pipes [1];
	    
        return res;
    }
	
    IOPipe::IOPipe (bool create) : IOPipe (createPipes (create))
    {}
	
    IOPipe::IOPipe (iopipe io) :
        _ipipe (io.i), _opipe (io.o)
    {}

    IOPipe::IOPipe (IOPipe && other) :
        _ipipe (std::move (other._ipipe)), _opipe (std::move (other._opipe))
    {}

    void IOPipe::operator= (IOPipe && other) {
        this-> dispose ();
        this-> _ipipe = std::move (other._ipipe);
        this-> _opipe = std::move (other._opipe);
    }
    
    IPipe & IOPipe::ipipe () {
        return this-> _ipipe;
    }

    OPipe & IOPipe::opipe () {
        return this-> _opipe;
    }

    bool IOPipe::isOpen () const {
        return this-> _ipipe.isOpen () && this-> _opipe.isOpen ();
    }

    void IOPipe::dispose () {
        this-> _ipipe.dispose ();
        this-> _opipe.dispose ();
    }
	
    IOPipe::~IOPipe () {
        this-> dispose ();
    }

}
  
