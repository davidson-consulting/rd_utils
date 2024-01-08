#pragma once

#include <string>
#include <vector>

#include <libssh/libssh.h>

namespace rd_utils::concurrency {

  /**
   * SSH remote command execution
   */
  class SSHProcess {

    // The command to run
    std::string _cmd;

    // The ip of the ssh server
    std::string _hostIp;

    // The user
    std::string _user;

    // THe private key file
    std::string _keyPath;

    ssh_session _session = nullptr;

    ssh_channel _channel = nullptr;

  private:

    SSHProcess (const SSHProcess & other);
    void operator=(const SSHProcess & other);

  public:

    /**
     * @params:
     *    - hostIp: the ip of the remote node
     *    - cmd: the command to launch
     *    - user: the user launching the cmd
     *    - keyPath: the path to the SSH private key
     */
    SSHProcess (const std::string & hostIp, const std::string & cmd, std::string user = "root", std::string keyPath = "");

    /**
     * Move ctor
     */
    SSHProcess (SSHProcess && other);

    /**
     * Move affectation
     */
    void operator=(SSHProcess && other);

    /**
     * Start the remote command
     */
    SSHProcess & start ();

    /**
     * Wait for the command to end
     */
    int wait ();

    /**
     * Read the output of the command
     */
    std::string stdout ();

    /**
     * Close everything
     */
    void dispose ();

    /**
     * this-> dispose ();
     */
    ~SSHProcess ();

  private:

    void launchCmd ();

  };

}
