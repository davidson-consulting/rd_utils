#pragma once

#include <string>
#include <cstdio>
#include <rd_utils/concurrency/timer.hh>
#include <rd_utils/net/_.hh>


namespace rd_utils::memory::cache::remote {

        class BlockPersister {
        protected:

                // The number of time a block had to be loaded
                uint32_t _nbLoaded;

                // The number of times a block was saved to disk
                uint32_t _nbSaved;

                // Time taken by loads
                float _loadElapsed;

                // Time taken by saves
                float _saveElapsed;

                concurrency::timer _t;

        public:

                BlockPersister ();

                /**
                 * @returns: true if the block exists
                 */
                virtual bool exists (uint64_t addr) = 0;

                /**
                 * Load a block from disk
                 * @warning: delete the block from disk
                 */
                virtual void load (uint64_t addr, uint8_t* memory, uint64_t size) = 0;

                /**
                 * Save a block to disk
                 */
                virtual void save (uint64_t addr, uint8_t* memory, uint64_t size) = 0;

                /**
                 * Delete a block from disk
                 */
                virtual void erase (uint64_t addr) = 0;


                /**
                 * Print persister informations to stdout
                 */
                void printInfo () const;

                /**
                 * Retreive persiter statistics
                 * @returns:
                 *    - nbWrites: the number of block writes to disk
                 *    - writeTime: the elapsed time taken by block writing
                 *    - nbReads: the number of block read from disk
                 *    - readTime: the elapsed time taken by block reading
                 */
                void getInfo (uint64_t & nbWrites, double & writeTime, uint64_t & nbReads, double & readTime) const;

                virtual ~BlockPersister () = 0;

        };


        /**
         * Class to read/write blocks from memory to disk
         */
        class LocalPersister : public BlockPersister {
        private:

                // The root path of the persister
                std::string _path;

                // The number of time a block had to be loaded
                uint32_t _nbLoaded;

                // The number of times a block was saved to disk
                uint32_t _nbSaved;

                // Time taken by loads
                float _loadElapsed;

                // Time taken by saves
                float _saveElapsed;

                concurrency::timer _t;

                char * _buffer;

        public:

                /**
                 * @params:
                 *    - path: the directory in which block will be persisted
                 */
                LocalPersister (const std::string & path = "./");

                /**
                 * @returns: true if the block exist
                 */
                 bool exists (uint64_t addr) override;

                /**
                 * Load a block from disk
                 * @warning: delete the block from disk
                 */
                 void load (uint64_t addr, uint8_t* memory, uint64_t size) override;

                /**
                 * Save a block to disk
                 */
                 void save (uint64_t addr, uint8_t* memory, uint64_t size) override;

                /**
                 * Delete a block from disk
                 */
                 void erase (uint64_t addr) override;

                /**
                 * Clear the persiter
                 */
                ~LocalPersister ();

        };


        class RemotePersister : public BlockPersister {
        private:

                // Connection to the remote repository
                net::SockAddrV4 _addr;

                // The id of the persiter
                uint32_t _clientId;

        public:

                /**
                 * @params:
                 *    - addr: the address of the remote repository
                 */
                RemotePersister (net::SockAddrV4 addr);


                 bool exists (uint64_t addr) override;

                /**
                 * Load a block from disk
                 * @warning: delete the block from disk
                 */
                 void load (uint64_t addr, uint8_t* memory, uint64_t size) override;

                /**
                 * Save a block to disk
                 */
                 void save (uint64_t addr, uint8_t* memory, uint64_t size) override;

                /**
                 * Delete a block from disk
                 */
                void erase (uint64_t addr) override;

                /**
                 * Close the remote connection
                 */
                ~RemotePersister ();


        private:

                void read (net::TcpStream & str, uint8_t * mem, uint64_t size);

                void write (net::TcpStream & str, uint8_t * mem, uint64_t size);

        };

}
