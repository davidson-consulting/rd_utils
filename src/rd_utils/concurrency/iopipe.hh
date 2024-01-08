#pragma once

#include <string>

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

        int _pipe;

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
         * Write on the pipe
         */
        void write (const std::string & msg);

        /**
         * Close this end of the pipe
         */
        void close ();

        /**
         * Non block writing
         */
        void setNonBlocking ();

        /**
         * Blocking write (wait for the other end to read)
         */
        void setBlocking ();

        /**
         * @returns: the file descriptor of the pipe
         */
        int getHandle () const;

        /**
         * Close the pipe
         */
        ~OPipe ();
	    
    };

    /**
     * Input pipe (reading end)
     * By default the pipe is blocking
     */
    class IPipe {

        int _pipe;

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
         * Read a string from the pipe
         */
        std::string read ();

        /**
         * Read a single char from the pipe
         */
        char readC ();

        /**
         * Set the pipe in non blocking mode (does not wait for a write to return while reading)
         */
        void setNonBlocking ();

        /**
         * Set the pipe in blocking mode (wait for something written to return from reading)
         */
        void setBlocking ();

        /**
         * Close this end of the pipe
         */
        void close ();

        /**
         * @returns: the file descriptor of the pipe
         */
        int getHandle () const;

        /**
         * Close the pipe
         */
        ~IPipe ();
	    
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
        IOPipe ();

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
         * Close both pipes
         */
        void close ();
	    
        /**
         * close inner pipes
         */
        ~IOPipe ();
	    
    };
	
}   
