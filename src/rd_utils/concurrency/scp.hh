#pragma once

#include <string>
#include <vector>

#include <libssh/libssh.h>

namespace rd_utils::concurrency {

    /**
     * Scp execution (remote copy of a file)
     */
    class SCPProcess {

        // The command to run
        std::string _in;

        std::string _out;

        // The ip of the ssh server
        std::string _hostIp;

        // The user
        std::string _user;

        // THe private key file
        std::string _keyPath;

        ssh_session _session = nullptr;

        ssh_scp _copy = nullptr;

        bool _fromInput = false;

    private:

        SCPProcess (const SCPProcess & other);
        void operator=(const SCPProcess & other);

    public:

        /**
         * @params:
         *    - hostIp: the ip of the remote node
         *    - in: the name of the input file
         *    - out: the name of the output file
         */
        SCPProcess (const std::string & hostIp, const std::string & in, const std::string & out, bool fromInput = false, std::string user = "root", std::string keyPath = "");

        /**
         * Move ctor
        */
        SCPProcess (SCPProcess && other);

        /**
         * Move affectation
         */
        void operator=(SCPProcess && other);

        /**
         * Copy the file from remote to local (in -> out)
         */
        void download ();

        /**
         * Copy the file from local to remote (in -> out)
         */
        void upload ();

        /**
         * Dispose everything
         */
        void dispose ();

        /**
         * this-> dispose ()
         */
        ~SCPProcess ();

    private:

        void launchUploadCmd ();

        void launchDownloadCmd ();

        void launchUploadCmdFromInput ();

    };

}
