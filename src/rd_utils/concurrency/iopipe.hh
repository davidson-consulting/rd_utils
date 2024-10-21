#pragma once

#include <string>
#include <cstdint>

namespace rd_utils::concurrency {

    struct iopipe {
        int i;
        int o;
    };

    /**
     * Output pipe (writing end)
     * By default the pipe is blocking
     */
    class OPipe {

        int _pipe = 0;
        bool _error = false;

    private :

        OPipe (const OPipe & other);
        void operator= (const OPipe & other);
	
    public:

        /**
         * @params:
         *    - pipe: the pipe used to write
         */
        OPipe (int pipe);

        /**
         * Move ctor
         */
        OPipe (OPipe && other);

        /**
         * Move affectation
         */
        void operator=(OPipe && other);


        /**
         * ================================================================================
         * ================================================================================
         * =========================        WRITE / READ        =========================
         * ================================================================================
         * ================================================================================
         */

        /**
         * Write 8 bytes to the stream
         * @params:
         *   - throwIfFail: if true instead of returning false, throw an exception in case of failure
         * @returns:
         *    - true: if succeeded
         */
        bool writeI64 (int64_t i, bool throwIfFail = true);

        /**
         * Write 4 bytes to the stream
         * @params:
         *   - throwIfFail: if true instead of returning false, throw an exception in case of failure
         * @returns:
         *    - true: if succeeded
         */
        bool writeI32 (int32_t i, bool throwIfFail = true);

        /**
         * Write 8 bytes to the stream
         * @params:
         *   - throwIfFail: if true instead of returning false, throw an exception in case of failure
         * @returns:
         *    - true: if succeeded
         */
        bool writeU64 (uint64_t i, bool throwIfFail = true);

        /**
         * Write 4 bytes to the stream
         * @params:
         *   - throwIfFail: if true instead of returning false, throw an exception in case of failure
         * @returns:
         *    - true: if succeeded
         */
        bool writeU32 (uint32_t i, bool throwIfFail = true);

        /**
         * Write a 4 byte float to the stream
         * @params:
         *   - throwIfFail: if true instead of returning false, throw an exception in case of failure
         * @returns:
         *    - true: if succeeded
         */
        bool writeF32 (float value, bool throwIfFail = true);

        /**
         * Write a 4 byte float to the stream
         * @params:
         *   - throwIfFail: if true instead of returning false, throw an exception in case of failure
         * @returns:
         *    - true: if succeeded
         */
        bool writeF64 (double value, bool throwIfFail = true);

        /**
         * Write 1 bytes to the stream
         * @params:
         *   - throwIfFail: if true instead of returning false, throw an exception in case of failure
         * @returns:
         *    - true: if succeeded
         */
        bool writeChar (uint8_t c, bool throwIfFail = true);

        /**
         * Write a message through the stream
         * @params:
         *   - throwIfFail: if true instead of returning false, throw an exception in case of failure
         * @returns: true, iif the write was successful
         */
        bool writeStr (const std::string & msg, bool throwIfFail = true);

        /**
         * Write a message of an arbitrary type
         * @params:
         *   - nb: the number of element of type T in the buffer
         *   - throwIfFail: if true instead of returning false, throw an exception in case of failure
         * @warning: pointer types won't be correctly sent, only works with flat data
         * @returns:
         *     - true: if the write worked
         */
        template <typename T>
        bool writeRaw (const T * buffer, uint32_t nb, bool throwIfFail = true) {
            return this-> inner_writeRaw (reinterpret_cast <const uint8_t*> (buffer), nb * sizeof (T), throwIfFail);
        }

        /**
         * Dispose this end of the pipe
         */
        void dispose ();

        /**
         * Non block writing
         */
        void setNonBlocking ();

        /**
         * Blocking write (wait for the other end to read)
         */
        void setBlocking ();

        /**
         * @returns: true if the pipe is open
         */
        bool isOpen () const;

        /**
         * @returns: the file descriptor of the pipe
         */
        int getHandle () const;

        /**
         * Dispose the pipe
         */
        ~OPipe ();

    private:

        /**
         * Write a message through the stream
         * @params:
         *   - throwIfFail: if true instead of returning false, throw an exception in case of failure
         */
        bool inner_writeRaw (const uint8_t * buffer, int len, bool throwIfFail);
	    
    };

    /**
     * Input pipe (reading end)
     * By default the pipe is blocking
     */
    class IPipe {

        int _pipe = 0;
        bool _error = false;

    private :

        IPipe (const IPipe & other);
        void operator= (const IPipe & other);
	
    public: 

        /**
         * Move constructor
         */
        IPipe (IPipe && other);

        /**
         * Move affectation
         */
        void operator=(IPipe && other);

        /**
         * @params:
         *    - pipe: the reading end of the pipe
         */
        IPipe (int pipe);

        /**
         * Set the pipe in non blocking mode (does not wait for a write to return while reading)
         */
        void setNonBlocking ();

        /**
         * Set the pipe in blocking mode (wait for something written to return from reading)
         */
        void setBlocking ();



        /**
         * ================================================================================
         * ================================================================================
         * ==============================        RECV        ==============================
         * ================================================================================
         * ================================================================================
         */

        /**
         * Read a message in a pre allocated buffer
         * @params:
         *   - throwIfFail: if true instead of returning false, throw an exception in case of failure
         * @return: the length read
         */
        template <typename T>
        bool readRaw (T * buffer, uint32_t nb, bool throwIfFail = true) {
            return this-> inner_readRaw (reinterpret_cast <uint8_t*> (buffer), nb * sizeof (T), throwIfFail);
        }

        /**
         * Read a string from the stream
         * @throws: if fails
         * @returns:
         *    - v: the read value
         *    - true if a value was read false otherwise
         */
        std::string readStr (uint32_t len);

        /**
         * Read a string from the stream
         * @params:
         *   - throwIfFail: if true instead of returning false, throw an exception in case of failure
         * @returns:
         *    - v: the read value
         *    - true if a value was read false otherwise
         */
        bool readStr (std::string & v, uint32_t len);

        /**
         * Read 8 bytes from the stream
         * @params:
         *   - throwIfFail: if true instead of returning false, throw an exception in case of failure
         * @returns:
         *   - v: the read value
         *   - true if something was read, false otherwise
         */
        bool readI64 (int64_t & v);

        /**
         * Read 8 bytes from the stream
         * @throws: if fails
         * @returns:
         *   - v: the read value
         *   - true if something was read, false otherwise
         */
        int64_t readI64 ();

        /**
         * Read 4 bytes from the stream
         * @returns:
         *   - v: the read value
         *   - true if something was read, false otherwise
         */
        bool readI32 (int32_t & v);

        /**
         * Read 8 bytes from the stream
         * @throws: if fails
         * @returns:
         *   - v: the read value
         *   - true if something was read, false otherwise
         */
        int32_t readI32 ();

        /**
         * Read 8 bytes from the stream
         * @returns:
         *   - v: the read value
         *   - true if something was read, false otherwise
         */
        bool readU64 (uint64_t & v);

        /**
         * Read 8 bytes from the stream
         * @throws: if fails
         * @returns:
         *   - v: the read value
         *   - true if something was read, false otherwise
         */
        uint64_t readU64 ();

        /**
         * Read 4 bytes from the stream
         * @returns:
         *   - v: the read value
         *   - true if something was read, false otherwise
         */
        bool readU32 (uint32_t & v);

        /**
         * Read 4 bytes from the stream
         * @throws: if fails
         * @returns:
         *   - v: the read value
         *   - true if something was read, false otherwise
         */
        uint32_t readU32 ();

        /**
         * Read 4 bytes float from the stream
         * @params:
         *   - throwIfFail: if true instead of returning false, throw an exception in case of failure
         * @returns:
         *   - v: the read value
         *   - true if something was read, false otherwise
         */
        bool readF32 (float & v);

        /**
         * Read 4 bytes float from the stream
         * @throws: if fails reading
         * @returns:
         *   - v: the read value
         */
        float readF32 ();

        /**
         * Read 8 bytes float from the stream
         * @returns:
         *   - v: the read value
         *   - true if something was read, false otherwise
         */
        bool readF64 (double & v);

        /**
         * Read 8 bytes float from the stream
         * @throws: if fails reading
         * @returns:
         *   - v: the read value
         */
        double readF64 ();

        /**
         * Read 1 byte from the stream
         * @throws: if fails reading
         * @returns:
         *   - v: the read value
         */
        uint8_t readChar ();

        /**
         * Read 1 byte from the stream
         * @throws: if fails reading
         * @returns:
         *   - v: the read value
         */
        bool readChar (uint8_t & v);

        /**
         * @returns: true if the pipe is open
         */
        bool isOpen () const;

        /**
         * Dispose this end of the pipe
         */
        void dispose ();

        /**
         * @returns: the file descriptor of the pipe
         */
        int getHandle () const;

        /**
         * Dispose the pipe
         */
        ~IPipe ();

    private:

        /**
         * Read string data from the stream
         * @params:
         *   - throwIfFail: if true instead of returning false, throw an exception in case of failure
         * @info: blocking function, waits until everything is read of nothing is left
         */
        bool readStr (std::string & v, uint32_t len, bool t);

        /**
         * Read raw data from the stream
         * @params:
         *   - throwIfFail: if true instead of returning false, throw an exception in case of failure
         * @info: blocking function, waits until everything is read of nothing is left
         */
        bool inner_readRaw (uint8_t * buffer, uint32_t len, bool throwIfFail);

    };


	/**
     * Input/Output pipe
     */
    class IOPipe {

        IPipe _ipipe;
        OPipe _opipe;

    private :

        IOPipe (const IOPipe & other);
        void operator= (const IOPipe & other);
	
	    
    public:

        /**
         * Pipe construction
         */
        IOPipe (bool create = true);

        /**
         * Move affectation
         */
        void operator=(IOPipe && other);

        /**
         * Create a pipe from file descriptors
         */
        IOPipe (iopipe pipes);

        /**
         * Move ctor
         */
        IOPipe (IOPipe && pipes);

        /**
         * @returns: the reading pipe
         */
        IPipe & ipipe ();

        /**
         * @returns: the writting pipe
         */
        OPipe & opipe ();

        /**
         * @returns: true if the pipe is open
         */
        bool isOpen () const;

        /**
         * Dispose both pipes
         */
        void dispose ();
	    
        /**
         * dispose inner pipes
         */
        ~IOPipe ();
	    
    };
	
}   
